// SPDX-License-Identifier: AGPL-3.0-or-later
// Standalone: g++ -std=c++17 -O2 -ISource Tests/OutputStageTransientTest.cpp Source/OutputStage.cpp -o transient-test
#include "OutputStage.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

using lr608::OutputStage;
using lr608::StereoSample;
using lr608::TransientControls;
using Buses = std::array<StereoSample, OutputStage::stemCount>;
constexpr double pi = 3.14159265358979323846;

struct Measurements { double earlyRms {}, tailRms {}, lateRms {}, peak {}, otherBusDelta {}; };

static Measurements kick (TransientControls controls, bool highFrequency = false)
{
    OutputStage stage, reference;
    stage.prepare (48000.0);
    reference.prepare (48000.0);
    Measurements m;
    double early = 0.0, tail = 0.0, late = 0.0;
    for (int i = 0; i < 24000; ++i)
    {
        const double seconds = i / 48000.0;
        const double tone = (highFrequency ? 4200.0 : 60.0);
        const double input = 0.7 * std::exp (-seconds / 0.11)
                           * std::sin (2.0 * pi * tone * seconds);
        const double hat = 0.3 * std::sin (2.0 * pi * 5100.0 * seconds);
        Buses stems {}, original {};
        stems[0] = {input, input};
        stems[13] = {hat, hat};
        original = stems;
        stage.process (stems, true, 0.0, 0, 30.0, controls);
        reference.process (original, true, 0.0, 0, 30.0);
        const auto out = stems[0].left;
        assert (std::isfinite (out) && std::isfinite (stems[13].left));
        assert (stems[0].left == stems[0].right);
        m.otherBusDelta = std::max (m.otherBusDelta, std::abs (stems[13].left - original[13].left));
        m.peak = std::max (m.peak, std::abs (out));
        if (seconds < 0.035) early += out * out;
        if (seconds >= 0.055 && seconds < 0.130) tail += out * out;
        if (seconds >= 0.200 && seconds < 0.290) late += out * out;
        for (int bus = 1; bus < OutputStage::stemCount; ++bus)
            if (bus != 13) assert (stems[bus].left == 0.0 && stems[bus].right == 0.0);
    }
    m.earlyRms = std::sqrt (early / (0.035 * 48000.0));
    m.tailRms = std::sqrt (tail / (0.075 * 48000.0));
    m.lateRms = std::sqrt (late / (0.090 * 48000.0));
    return m;
}

int main()
{
    const auto dry = kick ({});
    const auto attack = kick ({60, 0, 40, 0});
    const auto reduced = kick ({-60, 0, 40, 0});
    const auto sustainDown = kick ({0, -60, 40, 0});
    const auto sustainUp = kick ({0, 60, 40, 0});
    std::printf ("60 Hz early RMS: dry %.7f attack+60 %.7f attack-60 %.7f\n",
                 dry.earlyRms, attack.earlyRms, reduced.earlyRms);
    std::printf ("60 Hz late RMS: dry %.7f sustain-60 %.7f sustain+60 %.7f\n",
                 dry.lateRms, sustainDown.lateRms, sustainUp.lateRms);
    assert (attack.earlyRms > dry.earlyRms * 1.12);
    assert (reduced.earlyRms < dry.earlyRms * 0.94);
    assert (sustainDown.lateRms < dry.lateRms * 0.88);
    assert (sustainUp.lateRms > dry.lateRms * 1.12);
    assert (attack.otherBusDelta < 0.0002); // High-frequency hat on a different pair stays stable.
    const auto highDry = kick ({}, true);
    const auto highLow = kick ({70, 0, 40, 0}, true);
    const auto highFull = kick ({70, 0, 40, 1}, true);
    assert (std::abs (highDry.earlyRms - highLow.earlyRms) < highDry.earlyRms * 0.004);
    assert (highFull.earlyRms > highDry.earlyRms * 1.08);

    // Sanitise extreme/invalid controls and verify that a silent pair cannot
    // create new audio; changes to global controls cannot leak between stems.
    OutputStage stage;
    stage.prepare (96000.0);
    for (int i = 0; i < 96000; ++i)
    {
        Buses stems {};
        stems[31].left = 0.8 * std::sin (2.0 * pi * 68.0 * i / 96000.0);
        stage.process (stems, true, 0.0, 0, 45,
                       {std::numeric_limits<double>::quiet_NaN(), 10000, -1, 5});
        assert (std::isfinite (stems[31].left));
        assert (stems[31].right == 0.0);
        for (int bus = 0; bus < 31; ++bus)
            assert (stems[bus].left == 0.0 && stems[bus].right == 0.0);
    }
    std::puts ("Output Stage Transient: PASS");
}
