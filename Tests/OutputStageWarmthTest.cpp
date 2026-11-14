// SPDX-License-Identifier: AGPL-3.0-or-later
// Build standalone: g++ -std=c++17 -O2 -I Source Tests/OutputStageWarmthTest.cpp Source/OutputStage.cpp -o /tmp/lr608-output-test
#include "OutputStage.h"
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

using lr608::OutputStage;
using lr608::StereoSample;
using Buses = std::array<StereoSample, OutputStage::stemCount>;

struct Result { double fundamental{}, second{}, peak{}, dc{}; };
static constexpr double pi = 3.14159265358979323846;

static Result measure (double frequency, double warmth, int route = 0, double amplitude = 0.75)
{
    OutputStage stage;
    stage.prepare (48000.0);
    constexpr int n = 48000;
    // Integer-period sine tones, allowing direct Fourier projection.
    double fundamentalCos = 0, fundamentalSin = 0, secondCos = 0, secondSin = 0;
    double peak = 0, sum = 0;
    for (int i = 0; i < n; ++i)
    {
        Buses stems {};
        const double phase = 2.0 * pi * frequency * i / 48000.0;
        stems[route].left = amplitude * std::sin (phase);
        stems[route].right = amplitude * std::sin (phase);
        stage.process (stems, true, 0.0, 0, warmth);
        const double x = stems[route].left;
        if (i >= n / 2)
        {
            fundamentalCos += x * std::cos (phase);
            fundamentalSin += x * std::sin (phase);
            secondCos += x * std::cos (2.0 * phase);
            secondSin += x * std::sin (2.0 * phase);
            peak = std::max (peak, std::abs (x));
            sum += x;
        }
        for (int bus = 0; bus < OutputStage::stemCount; ++bus)
        {
            if (bus != route)
                assert (stems[bus].left == 0.0 && stems[bus].right == 0.0);
        }
        assert (stems[route].left == stems[route].right);
        assert (std::isfinite (x));
    }
    const double scale = 2.0 / (n / 2);
    return {scale * std::hypot (fundamentalSin, fundamentalCos),
            scale * std::hypot (secondSin, secondCos), peak, sum / (n / 2)};
}

int main()
{
    for (const int bus : {0, 1, 15, 31})
    {
        const auto clean = measure (60.0, 0.0, bus);
        const auto warm = measure (60.0, 35.0, bus);
        const auto stronger = measure (60.0, 80.0, bus);
        std::printf ("Bus %d clean: fundamental %.8f second %.8f peak %.8f DC %.8f\n", bus, clean.fundamental, clean.second, clean.peak, clean.dc);
        std::printf ("Bus %d warm 35: fundamental %.8f second %.8f peak %.8f DC %.8f\n", bus, warm.fundamental, warm.second, warm.peak, warm.dc);
        std::printf ("Bus %d warm 80: fundamental %.8f second %.8f peak %.8f DC %.8f\n", bus, stronger.fundamental, stronger.second, stronger.peak, stronger.dc);
        assert (warm.second > clean.second + 1.0e-5);
        assert (stronger.second > warm.second);
        // The 60 Hz fundamental must not be changed by bass EQ or a level boost.
        assert (std::abs (warm.fundamental / clean.fundamental - 1.0) < 0.012);
        assert (std::abs (stronger.fundamental / clean.fundamental - 1.0) < 0.020);
        assert (std::abs (warm.dc) < 0.001);
        assert (warm.peak < 1.0);
    }
    // The generator for one pair must not change the dry dynamics of another.
    // This specifically checks that the new harmonic branch bypasses the
    // historical shared compressor and limiter detectors.
    OutputStage original, coloured;
    original.prepare(48000.0); coloured.prepare(48000.0);
    double maxBrightDifference = 0.0;
    for (int i = 0; i < 24000; ++i)
    {
        Buses dry {}, wet {};
        dry[0] = {1.9 * std::sin (2.0 * pi * 60.0 * i / 48000.0), 1.9 * std::sin (2.0 * pi * 60.0 * i / 48000.0)};
        dry[11] = {0.7 * std::sin (2.0 * pi * 4000.0 * i / 48000.0), 0.7 * std::sin (2.0 * pi * 4000.0 * i / 48000.0)};
        wet = dry;
        original.process(dry, true, 0.0, 0, 0.0);
        coloured.process(wet, true, 0.0, 0, 80.0);
        maxBrightDifference = std::max(maxBrightDifference, std::abs(wet[11].left - dry[11].left));
    }
    std::printf("Separate buses, high-frequency output delta: %.10f\n", maxBrightDifference);
    assert(maxBrightDifference < 1.0e-4);

    const auto highClean = measure (4000.0, 0.0);
    const auto highWarm = measure (4000.0, 80.0);
    std::printf ("High tone 4 kHz clean %.8f warm %.8f\n", highClean.fundamental, highWarm.fundamental);
    assert (std::abs (highWarm.fundamental / highClean.fundamental - 1.0) < 0.005);
    std::puts ("Output Stage Warmth: PASS");
}
