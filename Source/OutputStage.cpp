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
    reset();
}

void OutputStage::reset()
{
    glueEnvelope = 0.0; stereoDc = {}; stemDc.fill ({});
    warmState.fill ({});
    smoothedWarmth = 0.0;
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

void OutputStage::process (std::array<StereoSample, stemCount>& stems, bool multichannel,
                           double masterDb, int zapEngine, double warmthPercent)
{
    const auto targetWarmth = std::clamp (std::isfinite (warmthPercent) ? warmthPercent : 0.0,
                                          0.0, 100.0) * 0.01;
    smoothedWarmth += warmthSmoothingCoefficient * (targetWarmth - smoothedWarmth);
    const auto wetHarmonics = 0.55 * smoothedWarmth;
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
            && colour.left == 0.0 && colour.right == 0.0) continue;
        // Match the same gain staging as the untouched dry path. Do not let
        // harmonic generation change the old compressor/limiter sidechain.
        stem.left = (stem.left + colour.left * stagedGain * gainL) * master;
        stem.right = (stem.right + colour.right * stagedGain * gainR) * master;
        // The JSFX equations are preserved above. This final native boundary
        // additionally prevents a corrupt preset/state from poisoning a DAW.
        if (! std::isfinite (stem.left)) stem.left = 0.0;
        if (! std::isfinite (stem.right)) stem.right = 0.0;
        stem.left = std::clamp (stem.left, -8.0, 8.0);
        stem.right = std::clamp (stem.right, -8.0, 8.0);
    }
}
}
