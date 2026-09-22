// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SlotDelay.h"
#include <algorithm>
#include <cmath>
#include <iterator>

namespace lr608
{
namespace
{
constexpr double pi = 3.1415926535897932384626433832795;
constexpr double silenceThreshold = 1.0e-8;
}

void SlotDelay::prepare (double sampleRate)
{
    sr = std::max (1.0, sampleRate);
    hasSettings = false;
    reset();
}

void SlotDelay::reset()
{
    std::fill (leftBuffer.begin(), leftBuffer.end(), 0.0f);
    std::fill (rightBuffer.begin(), rightBuffer.end(), 0.0f);
    writeIndex = 0;
    filterIc1L = filterIc2L = filterIc1R = filterIc2R = 0.0;
    currentDelayL = targetDelayL = std::max (1.0, currentDelayL);
    currentDelayR = targetDelayR = std::max (1.0, currentDelayR);
    delayStepL = delayStepR = 0.0;
    glideSamplesRemaining = 0;
    tailActive = false;
    silentSamples = 0;
}

double SlotDelay::quarterNotesForTimeIndex (int index) noexcept
{
    // 0..1020 is every denominator from 1/1024 through 1/4, with no gaps.
    // Above that the time advances one quarter-note beat at a time through
    // four 4/4 bars. The UI renders the long values as 1, 1.1..1.4,
    // 2.1..2.4, 3.1..3.4 as requested.
    index = std::clamp (index, 0, 1035);
    if (index <= 1020)
        return 4.0 / double (1024 - index);
    return double (index - 1019);
}

double SlotDelay::tapeLimit (double x) noexcept
{
    // Keep unity-feedback material untouched below nominal full scale, then
    // bend excess energy smoothly instead of allowing a runaway feedback loop.
    const auto magnitude = std::abs (x);
    if (magnitude <= 1.0)
        return x;
    const auto limited = 1.0 + 0.5 * std::tanh ((magnitude - 1.0) * 1.6);
    return std::copysign (limited, x);
}

void SlotDelay::ensureCapacity (std::size_t samples)
{
    samples = std::max<std::size_t> (samples, 8);
    if (leftBuffer.size() >= samples)
        return;

    // Grow geometrically. Buffers are empty at default settings and are only
    // allocated for Slots whose delay is actually enabled.
    const auto oldSize = leftBuffer.size();
    auto newSize = std::max (samples, oldSize == 0 ? std::size_t (4096) : oldSize * 2);
    std::vector<float> newLeft (newSize, 0.0f), newRight (newSize, 0.0f);
    if (oldSize > 0)
    {
        // Preserve the complete delay history relative to the next write point.
        for (std::size_t age = 1; age < oldSize; ++age)
        {
            const auto oldIndex = (writeIndex + oldSize - age) % oldSize;
            const auto newIndex = (newSize - age) % newSize;
            newLeft[newIndex] = leftBuffer[oldIndex];
            newRight[newIndex] = rightBuffer[oldIndex];
        }
    }
    leftBuffer.swap (newLeft);
    rightBuffer.swap (newRight);
    writeIndex = 0;
}

void SlotDelay::setSettings (const Settings& settings)
{
    if (hasSettings
        && settings.wetPercent == lastSettings.wetPercent
        && settings.timeIndex == lastSettings.timeIndex
        && settings.feedbackPercent == lastSettings.feedbackPercent
        && settings.glideMs == lastSettings.glideMs
        && settings.filter == lastSettings.filter
        && settings.filterResonance == lastSettings.filterResonance
        && settings.leftOffsetMs == lastSettings.leftOffsetMs
        && settings.rightOffsetMs == lastSettings.rightOffsetMs
        && settings.tempo == lastSettings.tempo)
        return;
    lastSettings = settings;
    hasSettings = true;

    const auto wasEnabled = enabled;
    wetGain = std::clamp (settings.wetPercent, 0.0, 100.0) * 0.01;
    enabled = wetGain > 1.0e-9;
    if (! enabled)
    {
        if (wasEnabled)
            reset();
        return;
    }

    feedbackGain = std::clamp (settings.feedbackPercent, 0.0, 100.0) * 0.01;
    // Exactly 100% is deliberately self-sustaining. The slight over-unity
    // compensation offsets fractional-read losses; tapeLimit prevents runaway.
    if (feedbackGain >= 0.999999)
        feedbackGain = 1.0015;
    else
        feedbackGain *= 0.998;

    glideMs = std::clamp (settings.glideMs, 0.0, 10000.0);
    filterPosition = std::clamp (settings.filter, 0.0, 1.0);
    filterResonance = std::clamp (settings.filterResonance, 0.5, 10.0);
    const auto bpm = std::clamp (settings.tempo, 20.0, 999.0);
    const auto baseSeconds = (60.0 / bpm) * quarterNotesForTimeIndex (settings.timeIndex);
    const auto baseSamples = std::max (1.0, baseSeconds * sr);
    // Offsets are expressed in milliseconds, but never allowed to cross the
    // following repeat. Full offset therefore approaches the next echo hit.
    const auto maximumOffsetSamples = std::max (0.0, baseSamples * 0.98);
    const auto offsetL = std::min (std::clamp (settings.leftOffsetMs, 0.0, 60000.0) * 0.001 * sr,
                                   maximumOffsetSamples);
    const auto offsetR = std::min (std::clamp (settings.rightOffsetMs, 0.0, 60000.0) * 0.001 * sr,
                                   maximumOffsetSamples);
    const auto newTargetL = baseSamples + offsetL;
    const auto newTargetR = baseSamples + offsetR;
    ensureCapacity (std::size_t (std::ceil (std::max (newTargetL, newTargetR))) + 8);

    if (! wasEnabled || currentDelayL <= 1.0 || currentDelayR <= 1.0)
    {
        currentDelayL = targetDelayL = newTargetL;
        currentDelayR = targetDelayR = newTargetR;
        delayStepL = delayStepR = 0.0;
        glideSamplesRemaining = 0;
    }
    else if (std::abs (newTargetL - targetDelayL) > 1.0e-6
          || std::abs (newTargetR - targetDelayR) > 1.0e-6)
    {
        targetDelayL = newTargetL;
        targetDelayR = newTargetR;
        const auto samples = std::max (0, int (std::lround (glideMs * 0.001 * sr)));
        if (samples == 0)
        {
            currentDelayL = targetDelayL;
            currentDelayR = targetDelayR;
            delayStepL = delayStepR = 0.0;
            glideSamplesRemaining = 0;
        }
        else
        {
            glideSamplesRemaining = samples;
            delayStepL = (targetDelayL - currentDelayL) / double (samples);
            delayStepR = (targetDelayR - currentDelayR) / double (samples);
        }
    }

    filterMorph = 0.0;
    filterFeedbackCompensation = 1.0;
    if (std::abs (filterPosition - 0.5) > 1.0e-9)
    {
        double cutoff = 1000.0;
        const auto strength = std::abs (filterPosition - 0.5) * 2.0;
        if (filterPosition < 0.5)
            cutoff = 20000.0 * std::pow (80.0 / 20000.0, std::pow (strength, 1.25));
        else
            cutoff = 20.0 * std::pow (8000.0 / 20.0, std::pow (strength, 1.25));

        // The cutoff above is the eventual colour of the echo tail, not a
        // filter that should be imposed almost completely on the very next
        // repeat. Only a controlled fraction of the filtered signal is folded
        // back on each trip through the loop. Repeated trips therefore trace a
        // smooth descending (LP) or ascending (HP) spectral curve and settle
        // progressively instead of jumping close to the final colour at once.
        // At maximum strength roughly 18% of the destination filter is applied
        // per repeat; gentler settings move even more slowly.
        filterMorph = 0.035 + 0.145 * std::pow (strength, 0.85);

        // Topology-preserving state-variable filter. The user control still
        // keeps its historical Q-style range, but the actual resonant peak is
        // hard-capped at +12 dB. Because the filter is now blended per repeat,
        // compensate the maximum possible peak of that blend rather than the
        // full filter. This keeps 100% feedback bounded without flattening the
        // gradual tonal evolution.
        const auto safeCutoff = std::clamp (cutoff, 20.0, sr * 0.45);
        const auto g = std::tan (pi * safeCutoff / sr);
        constexpr auto butterworthQ = 0.7071067811865476;
        constexpr auto maxResonanceDb = 12.0;
        const auto maxPeakGain = std::pow (10.0, maxResonanceDb / 20.0);
        const auto maxQ = std::sqrt ((maxPeakGain * maxPeakGain
                                   + maxPeakGain * std::sqrt (maxPeakGain * maxPeakGain - 1.0)) * 0.5);
        const auto effectiveQ = std::clamp (filterResonance, 0.5, maxQ);
        filterK = 1.0 / effectiveQ;
        filterA1 = 1.0 / (1.0 + g * (g + filterK));
        filterA2 = g * filterA1;
        filterA3 = g * filterA2;

        auto resonantPeakGain = 1.0;
        if (effectiveQ > butterworthQ)
        {
            const auto q2 = effectiveQ * effectiveQ;
            resonantPeakGain = (2.0 * q2) / std::sqrt (4.0 * q2 - 1.0);
        }
        const auto blendedPeakGain = (1.0 - filterMorph) + filterMorph * resonantPeakGain;
        filterFeedbackCompensation = 1.0 / std::max (1.0, blendedPeakGain);
    }
}

double SlotDelay::readFractional (const std::vector<float>& buffer, double delaySamples) const noexcept
{
    if (buffer.empty())
        return 0.0;
    const auto size = double (buffer.size());
    auto position = double (writeIndex) - std::clamp (delaySamples, 1.0, size - 3.0);
    while (position < 0.0)
        position += size;
    while (position >= size)
        position -= size;

    const auto i1 = std::size_t (std::floor (position));
    const auto frac = position - std::floor (position);
    const auto i0 = (i1 + buffer.size() - 1) % buffer.size();
    const auto i2 = (i1 + 1) % buffer.size();
    const auto i3 = (i1 + 2) % buffer.size();
    const auto y0 = double (buffer[i0]), y1 = double (buffer[i1]);
    const auto y2 = double (buffer[i2]), y3 = double (buffer[i3]);
    // Catmull-Rom interpolation gives a much more tape-like moving read head
    // than stepping integer taps while keeping high frequencies alive.
    const auto a0 = -0.5 * y0 + 1.5 * y1 - 1.5 * y2 + 0.5 * y3;
    const auto a1 = y0 - 2.5 * y1 + 2.0 * y2 - 0.5 * y3;
    const auto a2 = -0.5 * y0 + 0.5 * y2;
    return ((a0 * frac + a1) * frac + a2) * frac + y1;
}

double SlotDelay::filterFeedback (double input, bool right) noexcept
{
    if (std::abs (filterPosition - 0.5) <= 1.0e-9)
        return input;

    auto& ic1 = right ? filterIc1R : filterIc1L;
    auto& ic2 = right ? filterIc2R : filterIc2L;
    const auto v3 = input - ic2;
    const auto v1 = filterA1 * ic1 + filterA2 * v3;
    const auto v2 = ic2 + filterA2 * ic1 + filterA3 * v3;
    ic1 = 2.0 * v1 - ic1;
    ic2 = 2.0 * v2 - ic2;

    const auto filtered = filterPosition < 0.5 ? v2 : input - filterK * v1 - v2;
    return input + (filtered - input) * filterMorph;
}

void SlotDelay::updateDelayGlide() noexcept
{
    if (glideSamplesRemaining <= 0)
        return;
    currentDelayL += delayStepL;
    currentDelayR += delayStepR;
    if (--glideSamplesRemaining == 0)
    {
        currentDelayL = targetDelayL;
        currentDelayR = targetDelayR;
        delayStepL = delayStepR = 0.0;
    }
}

StereoSample SlotDelay::process (StereoSample input)
{
    // This processor is a pure send return. The original Slot signal never
    // enters this output path; it stays on LR-608's legacy voice->bus route.
    if (! enabled || leftBuffer.empty())
        return {};

    updateDelayGlide();
    const auto delayedL = readFractional (leftBuffer, currentDelayL);
    const auto delayedR = readFractional (rightBuffer, currentDelayR);
    auto compensatedFeedbackGain = feedbackGain * filterFeedbackCompensation;
    // At 100% feedback the unfiltered delay uses a tiny over-unity correction
    // for fractional-read losses. Do not carry that correction through a
    // resonant filter: its loudest frequency is held exactly at unity instead.
    if (filterFeedbackCompensation < 1.0 && feedbackGain > 1.0)
        compensatedFeedbackGain = filterFeedbackCompensation;
    const auto feedbackL = filterFeedback (delayedL, false) * compensatedFeedbackGain;
    const auto feedbackR = filterFeedback (delayedR, true) * compensatedFeedbackGain;
    const auto writeL = tapeLimit (input.left + feedbackL);
    const auto writeR = tapeLimit (input.right + feedbackR);
    leftBuffer[writeIndex] = float (writeL);
    rightBuffer[writeIndex] = float (writeR);
    if (++writeIndex >= leftBuffer.size())
        writeIndex = 0;

    const auto energy = std::max ({ std::abs (input.left), std::abs (input.right),
                                    std::abs (delayedL), std::abs (delayedR),
                                    std::abs (writeL), std::abs (writeR) });
    if (energy > silenceThreshold)
    {
        tailActive = true;
        silentSamples = 0;
    }
    else if (tailActive && ++silentSamples > 4096)
    {
        tailActive = false;
        silentSamples = 0;
    }

    return { delayedL * wetGain, delayedR * wetGain };
}
}
