// SPDX-License-Identifier: AGPL-3.0-or-later
#include "OutputStage.h"

#include <algorithm>
#include <cmath>

namespace lr608
{
namespace { constexpr double pi = 3.14159265358979323846; }

void OutputStage::prepare (double sampleRate)
{
    sr = std::max (1.0, sampleRate);
    dcCoefficient = std::exp (-2.0 * pi * 5.0 / sr);
    glueAttack = std::max (1.0, 0.003 * sr);
    glueRelease = std::max (1.0, 0.020 * sr);
    // Two gentle low-pass poles suppress cymbal/hat components before the
    // nonlinearity. Harmonics therefore originate primarily from kick/tom body,
    // and nonlinear aliasing remains naturally limited without extra latency.
    warmthLpCoefficient = 1.0 - std::exp (-2.0 * pi * 240.0 / sr);
    warmthDcCoefficient = std::exp (-2.0 * pi * 15.0 / sr);
    warmthSmoothingCoefficient = 1.0 - std::exp (-1.0 / (0.015 * sr));
    transientLowCoefficient = 1.0 - std::exp (-2.0 * pi * 360.0 / sr);
    transientControlCoefficient = 1.0 - std::exp (-1.0 / (0.020 * sr));
    transientGainAttackCoefficient = 1.0 - std::exp (-1.0 / (0.001 * sr));
    transientGainReleaseCoefficient = 1.0 - std::exp (-1.0 / (0.004 * sr));
    cachedTransientSpeed = -1.0;
    updateTransientSpeed (40.0);
    reset();
}

void OutputStage::reset()
{
    glueEnvelope = 0.0; stereoDc = {}; stemDc.fill ({});
    warmState.fill ({});
    smoothedWarmth = 0.0;
    transientState.fill ({});
    transientAttack = transientSustain = transientFocus = 0.0;
    transientWasActive = false;
}

StereoSample OutputStage::blockDc (StereoSample input, DcState& state)
{
    StereoSample output;
    output.left = input.left - state.xL + dcCoefficient * state.yL;
    output.right = input.right - state.xR + dcCoefficient * state.yR;
    state.xL = input.left; state.yL = output.left;
    state.xR = input.right; state.yR = output.right;
    return output;
}

double OutputStage::warmHarmonics (double input, WarmChannel& state,
                                    double lpCoefficient, double dcCoefficient)
{
    state.lp1 += lpCoefficient * (input - state.lp1);
    state.lp2 += lpCoefficient * (state.lp1 - state.lp2);
    // Soft-bounded even-order nonlinearity. The signal is generated in a
    // parallel path: unlike an EQ boost, it preserves the dry 40-100 Hz wave.
    const auto square = state.lp2 * state.lp2;
    const auto shaped = square / (1.0 + 0.5 * square);
    state.dcMean = dcCoefficient * state.dcMean + (1.0 - dcCoefficient) * shaped;
    return shaped - state.dcMean;
}

void OutputStage::updateTransientSpeed (double speedMs)
{
    speedMs = std::clamp (speedMs, 8.0, 120.0);
    if (speedMs == cachedTransientSpeed) return;
    cachedTransientSpeed = speedMs;
    // Short peak envelope identifies the initial hit; the slow envelope
    // estimates the recent body. Both have independent attack/release times.
    const auto seconds = speedMs * 0.001;
    const auto coefficient = [this] (double time)
    {
        return 1.0 - std::exp (-1.0 / (std::max (time, 0.0001) * sr));
    };
    transientFastAttack = coefficient (std::max (0.0005, seconds * 0.035));
    transientFastRelease = coefficient (std::max (0.003, seconds * 0.30));
    transientSlowAttack = coefficient (std::max (0.005, seconds * 0.75));
    transientSlowRelease = coefficient (std::max (0.030, seconds * 2.8));
}

void OutputStage::shapeTransient (StereoSample& signal, TransientPair& state)
{
    // Two-pole low-band monitoring and processing: no EQ is applied when the
    // shaper is inactive. 360 Hz cutoff captures kick/tom attack and body;
    // the "Full" setting smoothly extends the gain change to the whole band.
    state.lowL1 += transientLowCoefficient * (signal.left - state.lowL1);
    state.lowL2 += transientLowCoefficient * (state.lowL1 - state.lowL2);
    state.lowR1 += transientLowCoefficient * (signal.right - state.lowR1);
    state.lowR2 += transientLowCoefficient * (state.lowR1 - state.lowR2);
    const auto lowLevel = std::max (std::abs (state.lowL2), std::abs (state.lowR2));
    const auto fullLevel = std::max (std::abs (signal.left), std::abs (signal.right));
    const auto detected = lowLevel + transientFocus * (fullLevel - lowLevel);
    state.fast += (detected - state.fast) * (detected > state.fast ? transientFastAttack : transientFastRelease);
    state.slow += (detected - state.slow) * (detected > state.slow ? transientSlowAttack : transientSlowRelease);

    // Level-relative differential detection, with a quiet floor to avoid
    // amplifying digital silence and noise. Bipolar controls are independent.
    const auto attackShape = std::clamp ((state.fast - state.slow) / (state.fast + 0.004), 0.0, 1.0);
    const auto sustainShape = std::clamp ((state.slow - state.fast) / (state.slow + 0.004), 0.0, 1.0);
    const auto targetGain = std::clamp (1.0 + 1.25 * transientAttack * attackShape
                                                + 0.90 * transientSustain * sustainShape,
                                        0.22, 2.25);
    state.gain += (targetGain - state.gain)
                  * (targetGain > state.gain ? transientGainAttackCoefficient : transientGainReleaseCoefficient);
    const auto delta = state.gain - 1.0;
    // Linked L/R detector/gain keeps the sound centered. Low focus only adds
    // the reshaped bass component to dry, leaving cymbal frequencies nearly
    // unchanged; Full focus shapes the entire pair.
    signal.left += delta * (state.lowL2 + transientFocus * (signal.left - state.lowL2));
    signal.right += delta * (state.lowR2 + transientFocus * (signal.right - state.lowR2));
}

void OutputStage::process (std::array<StereoSample, stemCount>& stems, bool multichannel,
                           double masterDb, int zapEngine, double warmthPercent,
                           TransientControls transient)
{
    const auto targetWarmth = std::clamp (std::isfinite (warmthPercent) ? warmthPercent : 0.0,
                                          0.0, 100.0) * 0.01;
    smoothedWarmth += warmthSmoothingCoefficient * (targetWarmth - smoothedWarmth);
    const auto wetHarmonics = 0.55 * smoothedWarmth;
    const auto safeAttack = std::clamp (std::isfinite (transient.attackPercent) ? transient.attackPercent : 0.0, -100.0, 100.0) * 0.01;
    const auto safeSustain = std::clamp (std::isfinite (transient.sustainPercent) ? transient.sustainPercent : 0.0, -100.0, 100.0) * 0.01;
    transientAttack += transientControlCoefficient * (safeAttack - transientAttack);
    transientSustain += transientControlCoefficient * (safeSustain - transientSustain);
    transientFocus += transientControlCoefficient * (double (transient.focus == 1) - transientFocus);
    const auto safeSpeed = std::clamp (std::isfinite (transient.speedMs) ? transient.speedMs : 40.0, 8.0, 120.0);
    updateTransientSpeed (safeSpeed);
    const auto transientActive = std::abs (safeAttack) > 1.0e-9 || std::abs (safeSustain) > 1.0e-9
                                 || std::abs (transientAttack) > 1.0e-6 || std::abs (transientSustain) > 1.0e-6;
    if (! transientActive && transientWasActive) transientState.fill ({});
    transientWasActive = transientActive;

    for (int index = 0; index < stemCount; ++index)
    {
        auto& stem = stems[index];
        const auto& dc = stemDc[index];
        if (stem.left == 0.0 && stem.right == 0.0
            && dc.xL == 0.0 && dc.yL == 0.0 && dc.xR == 0.0 && dc.yR == 0.0)
            continue;
        if (! std::isfinite (stem.left)) stem.left = 0.0;
        if (! std::isfinite (stem.right)) stem.right = 0.0;
        stem.left = std::clamp (stem.left, -1.0e6, 1.0e6);
        stem.right = std::clamp (stem.right, -1.0e6, 1.0e6);
    }
    // Generate in parallel from the pre-compressor stems. Inject only AFTER
    // the legacy glue detector/peak-control gain calculations: the new colour
    // on bus N must not alter the level of an unrelated output pair.
    std::array<StereoSample, stemCount> warmAdd {};
    if (wetHarmonics > 1.0e-9)
    {
        for (int index = 0; index < stemCount; ++index)
        {
            const auto& stem = stems[index];
            auto& state = warmState[index];
            warmAdd[index].left = wetHarmonics * warmHarmonics (stem.left, state.left,
                                                                 warmthLpCoefficient, warmthDcCoefficient);
            warmAdd[index].right = wetHarmonics * warmHarmonics (stem.right, state.right,
                                                                  warmthLpCoefficient, warmthDcCoefficient);
        }
    }
    StereoSample stereo;
    for (const auto& stem : stems) { stereo.left += stem.left; stereo.right += stem.right; }
    stereo = blockDc (stereo, stereoDc);
    StereoSample multiBus;
    for (int index = 0; index < stemCount; ++index)
    {
        const auto& dc = stemDc[index];
        if (stems[index].left == 0.0 && stems[index].right == 0.0
            && dc.xL == 0.0 && dc.yL == 0.0 && dc.xR == 0.0 && dc.yR == 0.0)
            continue;
        stems[index] = blockDc (stems[index], stemDc[index]);
        multiBus.left += stems[index].left; multiBus.right += stems[index].right;
    }
    const auto detector = multichannel ? std::max (std::abs (multiBus.left), std::abs (multiBus.right))
                                       : std::max (std::abs (stereo.left), std::abs (stereo.right));
    glueEnvelope += (detector - glueEnvelope) /
                    (glueEnvelope < detector ? glueAttack : glueRelease);
    auto glueGain = 1.0;
    if (glueEnvelope > 0.6)
        glueGain = (0.6 + (glueEnvelope - 0.6) / 2.0) / glueEnvelope;
    const auto stagedGain = glueGain * 0.5 * 0.35;
    stereo.left *= stagedGain; stereo.right *= stagedGain;
    for (auto& stem : stems)
        if (stem.left != 0.0 || stem.right != 0.0)
        { stem.left *= stagedGain; stem.right *= stagedGain; }

    const auto limiter = [] (double sample)
    {
        const auto magnitude = std::abs (sample);
        return magnitude > 0.98 ? std::copysign (0.98 + (magnitude - 0.98) * 0.15, sample) : sample;
    };
    const auto limitedL = limiter (stereo.left), limitedR = limiter (stereo.right);
    const auto gainL = std::abs (stereo.left) > 0.0 ? limitedL / stereo.left : 1.0;
    const auto gainR = std::abs (stereo.right) > 0.0 ? limitedR / stereo.right : 1.0;
    for (auto& stem : stems)
        if (stem.left != 0.0 || stem.right != 0.0)
        { stem.left *= gainL; stem.right *= gainR; }

    (void) zapEngine; // Zap engine compensation is now applied per polyphonic voice.

    if(masterDb!=cachedMasterDb){cachedMasterDb=masterDb;cachedMasterGain=std::pow(10.0,masterDb/20.0)*1.2;}
    const auto master = cachedMasterGain;
    for (int index = 0; index < stemCount; ++index)
    {
        auto& stem = stems[index];
        const auto& colour = warmAdd[index];
        if (stem.left == 0.0 && stem.right == 0.0
            && colour.left == 0.0 && colour.right == 0.0)
        {
            // Continue releasing an already-active envelope on silent stems,
            // without emitting filter residue as an artificial audio tail.
            if (transientActive)
            {
                auto& state = transientState[std::size_t (index)];
                if (state.fast > 1.0e-9 || state.slow > 1.0e-9
                    || std::abs (state.lowL2) > 1.0e-9 || std::abs (state.lowR2) > 1.0e-9
                    || std::abs (state.gain - 1.0) > 1.0e-7)
                {
                    StereoSample silence;
                    shapeTransient (silence, state);
                }
            }
            continue;
        }
        // Match the same gain staging as the untouched dry path. Do not let
        // harmonic generation change the old compressor/limiter sidechain.
        stem.left = (stem.left + colour.left * stagedGain * gainL) * master;
        stem.right = (stem.right + colour.right * stagedGain * gainR) * master;
        // The JSFX equations are preserved above. This final native boundary
        // additionally prevents a corrupt preset/state from poisoning a DAW.
        if (! std::isfinite (stem.left)) stem.left = 0.0;
        if (! std::isfinite (stem.right)) stem.right = 0.0;
        // Separate post-colour transient design. Off: the historical final
        // samples and all TEST34.1 harmonic characteristics are unchanged.
        if (transientActive)
            shapeTransient (stem, transientState[std::size_t (index)]);
        stem.left = std::clamp (stem.left, -8.0, 8.0);
        stem.right = std::clamp (stem.right, -8.0, 8.0);
    }
}
}
