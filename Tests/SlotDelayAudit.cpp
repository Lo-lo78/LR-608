// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SlotDelay.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace
{
void require (bool condition, const char* message)
{
    if (! condition)
    {
        std::cerr << "SlotDelay audit failed: " << message << '\n';
        std::exit (1);
    }
}
}

int main()
{
    constexpr double sampleRate = 48000.0;

    // The delay is a pure send return. At its default Wet=0 it contributes
    // absolute zero; the dry signal lives only in LR-608's original renderer.
    {
        lr608::SlotDelay delay;
        delay.prepare (sampleRate);
        lr608::SlotDelay::Settings settings;
        delay.setSettings (settings);
        for (int i = 0; i < 128; ++i)
        {
            const lr608::StereoSample input { std::sin (double (i) * 0.17) * 0.37,
                                              std::cos (double (i) * 0.11) * 0.23 };
            const auto output = delay.process (input);
            require (output.left == 0.0 && output.right == 0.0,
                     "default Wet=0 send return is not silent");
        }
    }

    // Equal L/R time preserves an existing stereo image rather than collapsing it.
    {
        lr608::SlotDelay delay;
        delay.prepare (sampleRate);
        lr608::SlotDelay::Settings settings;
        settings.wetPercent = 100.0;
        settings.timeIndex = 0; // 1/1024: short enough for a fast audit.
        settings.feedbackPercent = 0.0;
        delay.setSettings (settings);
        delay.process ({ 0.25, 0.75 });
        double bestL = 0.0, bestR = 0.0;
        for (int i = 0; i < 512; ++i)
        {
            const auto out = delay.process ({ 0.0, 0.0 });
            if (std::abs (out.left) > std::abs (bestL)) { bestL = out.left; bestR = out.right; }
        }
        require (std::abs (bestL) > 1.0e-4, "short stereo echo was not produced");
        require (std::abs ((bestR / bestL) - 3.0) < 0.05,
                 "stereo image was not preserved by equal delay times");
    }

    // A hard-panned source must remain hard-panned: the delay never invents
    // crossfeed unless the source itself contains both channels.
    {
        lr608::SlotDelay delay;
        delay.prepare (sampleRate);
        lr608::SlotDelay::Settings settings;
        settings.wetPercent = 100.0;
        settings.timeIndex = 0;
        settings.feedbackPercent = 45.0;
        delay.setSettings (settings);
        delay.process ({ 0.8, 0.0 });
        double rightPeak = 0.0, leftPeak = 0.0;
        for (int i = 0; i < 2048; ++i)
        {
            const auto out = delay.process ({ 0.0, 0.0 });
            leftPeak = std::max (leftPeak, std::abs (out.left));
            rightPeak = std::max (rightPeak, std::abs (out.right));
        }
        require (leftPeak > 1.0e-4, "hard-left source did not produce a delayed repeat");
        require (rightPeak < 1.0e-12, "delay crossfed a hard-left source into the right channel");
    }

    // Independent right offset must postpone that channel, enabling user-made ping-pong spacing.
    {
        lr608::SlotDelay delay;
        delay.prepare (sampleRate);
        lr608::SlotDelay::Settings settings;
        settings.wetPercent = 100.0;
        settings.timeIndex = 0;
        settings.feedbackPercent = 0.0;
        settings.rightOffsetMs = 1.0;
        delay.setSettings (settings);
        delay.process ({ 1.0, 1.0 });
        int firstL = -1, firstR = -1;
        for (int i = 0; i < 512; ++i)
        {
            const auto out = delay.process ({ 0.0, 0.0 });
            if (firstL < 0 && std::abs (out.left) > 1.0e-3) firstL = i;
            if (firstR < 0 && std::abs (out.right) > 1.0e-3) firstR = i;
        }
        require (firstL >= 0 && firstR > firstL + 35,
                 "right-channel millisecond offset did not postpone the echo");
    }

    // Maximum feedback is intentionally endless but must remain numerically bounded.
    {
        lr608::SlotDelay delay;
        delay.prepare (sampleRate);
        lr608::SlotDelay::Settings settings;
        settings.wetPercent = 100.0;
        settings.timeIndex = 0;
        settings.feedbackPercent = 100.0;
        delay.setSettings (settings);
        delay.process ({ 0.4, -0.3 });
        double peak = 0.0;
        for (int i = 0; i < int (sampleRate * 2.0); ++i)
        {
            const auto out = delay.process ({ 0.0, 0.0 });
            require (std::isfinite (out.left) && std::isfinite (out.right),
                     "100% feedback produced NaN/Inf");
            peak = std::max ({ peak, std::abs (out.left), std::abs (out.right) });
        }
        require (delay.isActive(), "100% feedback stopped instead of sustaining");
        require (peak < 2.0, "100% feedback escaped the tape limiter");
    }


    // The filter must not start together with the first repeat. The first wet
    // echo is bit-identical to the neutral-filter delay; filtering begins only
    // when that echo is written back for the following generation.
    {
        lr608::SlotDelay neutral, coloured;
        neutral.prepare (sampleRate);
        coloured.prepare (sampleRate);
        lr608::SlotDelay::Settings a, b;
        a.wetPercent = b.wetPercent = 100.0;
        a.timeIndex = b.timeIndex = 0;
        a.feedbackPercent = b.feedbackPercent = 100.0;
        b.filter = 0.0;
        neutral.setSettings (a);
        coloured.setSettings (b);
        double firstRepeatDifference = 0.0, secondRepeatDifference = 0.0;
        for (int n = 0; n < 300; ++n)
        {
            const auto input = n < 48 ? 0.3 * std::sin (double (n) * 0.51) : 0.0;
            const auto outA = neutral.process ({ input, input });
            const auto outB = coloured.process ({ input, input });
            if (n < 180)
                firstRepeatDifference = std::max (firstRepeatDifference, std::abs (outA.left - outB.left));
            else
                secondRepeatDifference = std::max (secondRepeatDifference, std::abs (outA.left - outB.left));
        }
        require (firstRepeatDifference < 1.0e-12,
                 "feedback filter starts on the first repeat instead of the following one");
        require (secondRepeatDifference > 1.0e-4,
                 "feedback filter did not advance on the following repeat");
    }

    // The feedback filter must colour the tail progressively, not jump close
    // to its final LP/HP colour on the first filtered repeat. At maximum
    // strength a tone should still retain most of its energy on the next trip,
    // then continue moving in the same direction over successive repeats.
    for (const auto mode : { 0, 1 })
    {
        lr608::SlotDelay delay;
        delay.prepare (sampleRate);
        lr608::SlotDelay::Settings settings;
        settings.wetPercent = 100.0;
        settings.timeIndex = 0;
        settings.feedbackPercent = 100.0;
        settings.filter = mode == 0 ? 0.0 : 1.0;
        settings.filterResonance = 0.707;
        delay.setSettings (settings);

        constexpr int repeatSamples = 94; // 1/1024 at 120 BPM / 48 kHz = 93.75 samples.
        constexpr int toneSamples = 64;
        const auto toneHz = mode == 0 ? 10000.0 : 1000.0;
        double rms[8] {};
        for (int n = 0; n < 1200; ++n)
        {
            const auto input = n < toneSamples
                             ? 0.35 * std::sin (2.0 * 3.14159265358979323846 * toneHz * double (n) / sampleRate)
                             : 0.0;
            const auto out = delay.process ({ input, input });
            for (int repeat = 0; repeat < 8; ++repeat)
                if (n >= repeatSamples * (repeat + 1)
                    && n < repeatSamples * (repeat + 1) + toneSamples)
                    rms[repeat] += out.left * out.left;
        }
        for (auto& value : rms)
            value = std::sqrt (value / double (toneSamples));

        require (rms[1] > rms[0] * 0.60,
                 "feedback filter jumps too close to its final colour on the next repeat");
        for (int repeat = 1; repeat < 8; ++repeat)
            require (rms[repeat] <= rms[repeat - 1] * 1.03,
                     "feedback filter does not evolve progressively from repeat to repeat");
        require (rms[7] < rms[0] * 0.40,
                 "feedback filter no longer reaches a clearly coloured tail over time");
    }


    // Pitch is applied by moving read heads in the feedback path. +12 semitones
    // must transpose the first repeat by one octave, then feed that shifted
    // result back so the second repeat is another octave higher. A DFT probe is
    // used rather than zero crossings because the dual-head crossfade can add
    // harmless amplitude modulation around grain boundaries.
    {
        lr608::SlotDelay delay;
        delay.prepare (sampleRate);
        lr608::SlotDelay::Settings settings;
        settings.wetPercent = 100.0;
        settings.timeIndex = 960; // 1/64 = 1500 samples at 120 BPM / 48 kHz.
        settings.feedbackPercent = 100.0;
        settings.pitchSemitones = 12.0;
        delay.setSettings (settings);

        constexpr double sourceHz = 440.0;
        constexpr int totalSamples = 5200;
        std::vector<double> output (totalSamples, 0.0);
        for (int n = 0; n < totalSamples; ++n)
        {
            const auto input = n < 1200
                             ? 0.2 * std::sin (2.0 * 3.14159265358979323846 * sourceHz * double (n) / sampleRate)
                             : 0.0;
            output[std::size_t (n)] = delay.process ({ input, input }).left;
        }
        const auto spectralMagnitude = [&] (int start, int count, double frequency)
        {
            double real = 0.0, imaginary = 0.0;
            for (int n = 0; n < count; ++n)
            {
                const auto window = 0.5 - 0.5 * std::cos (2.0 * 3.14159265358979323846 * double (n) / double (count - 1));
                const auto phase = 2.0 * 3.14159265358979323846 * frequency * double (n) / sampleRate;
                const auto sample = output[std::size_t (start + n)] * window;
                real += sample * std::cos (phase);
                imaginary -= sample * std::sin (phase);
            }
            return std::sqrt (real * real + imaginary * imaginary);
        };
        const auto first440 = spectralMagnitude (1600, 900, 440.0);
        const auto first880 = spectralMagnitude (1600, 900, 880.0);
        const auto second880 = spectralMagnitude (3100, 900, 880.0);
        const auto second1760 = spectralMagnitude (3100, 900, 1760.0);
        require (first880 > first440 * 10.0,
                 "+12 pitch did not transpose the first repeat by an octave");
        require (second1760 > second880 * 10.0,
                 "+12 pitch did not accumulate on the second repeat");
    }

    // Both pitch extremes must remain finite and bounded. Pitch=0 is tested
    // throughout the earlier cases and follows the exact legacy read path.
    for (double pitch : { -48.0, 48.0 })
    {
        lr608::SlotDelay delay;
        delay.prepare (sampleRate);
        lr608::SlotDelay::Settings settings;
        settings.wetPercent = 100.0;
        settings.timeIndex = 0;
        settings.feedbackPercent = 100.0;
        settings.pitchSemitones = pitch;
        delay.setSettings (settings);
        delay.process ({ 0.3, -0.2 });
        for (int n = 0; n < int (sampleRate); ++n)
        {
            const auto out = delay.process ({ 0.0, 0.0 });
            require (std::isfinite (out.left) && std::isfinite (out.right),
                     "extreme delay pitch produced NaN/Inf");
            require (std::abs (out.left) < 2.0 && std::abs (out.right) < 2.0,
                     "extreme delay pitch escaped the tape limiter");
        }
    }

    // Resonant LP/HP feedback is capped at a +12 dB peak and gain-compensated
    // inside the loop. Even at 100% feedback it may sustain existing material,
    // but it must never grow into a resonant self-oscillation.
    for (double filter : { 0.0, 1.0 })
    {
        lr608::SlotDelay delay;
        delay.prepare (sampleRate);
        lr608::SlotDelay::Settings settings;
        settings.wetPercent = 100.0;
        settings.timeIndex = 0;
        settings.feedbackPercent = 100.0;
        settings.filter = filter;
        settings.filterResonance = 10.0;
        delay.setSettings (settings);
        delay.process ({ 0.35, -0.2 });
        double earlyPeak = 0.0, latePeak = 0.0;
        const auto totalSamples = int (sampleRate * 4.0);
        for (int i = 0; i < totalSamples; ++i)
        {
            const auto out = delay.process ({ 0.0, 0.0 });
            require (std::isfinite (out.left) && std::isfinite (out.right),
                     "resonant feedback filter produced NaN/Inf");
            const auto peak = std::max (std::abs (out.left), std::abs (out.right));
            require (peak < 1.0, "12 dB resonant feedback grew into self-oscillation");
            if (i < int (sampleRate)) earlyPeak = std::max (earlyPeak, peak);
            if (i >= totalSamples - int (sampleRate)) latePeak = std::max (latePeak, peak);
        }
        require (latePeak <= earlyPeak * 1.01 + 1.0e-9,
                 "resonant feedback accumulated energy instead of remaining bounded");
    }

    std::cout << "SlotDelay audit passed: pure send return, continuous musical time, stereo/pan preservation, L/R offsets, repeat-progressive filters, cumulative pitch, resonant filters, infinite bounded feedback.\n";
    return 0;
}
