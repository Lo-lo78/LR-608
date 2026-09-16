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
    reset();
}

void OutputStage::reset()
{
    glueEnvelope = 0.0; stereoDc = {}; stemDc.fill ({});
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

void OutputStage::process (std::array<StereoSample, stemCount>& stems, bool multichannel,
                           double masterDb, int zapEngine)
{
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
    for (auto& stem : stems)
    {
        if (stem.left == 0.0 && stem.right == 0.0) continue;
        stem.left *= master; stem.right *= master;
        // The JSFX equations are preserved above. This final native boundary
        // additionally prevents a corrupt preset/state from poisoning a DAW.
        if (! std::isfinite (stem.left)) stem.left = 0.0;
        if (! std::isfinite (stem.right)) stem.right = 0.0;
        stem.left = std::clamp (stem.left, -8.0, 8.0);
        stem.right = std::clamp (stem.right, -8.0, 8.0);
    }
}
}
