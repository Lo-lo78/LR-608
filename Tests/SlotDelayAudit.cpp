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

    std::cout << "SlotDelay audit passed: pure send return, continuous musical time, stereo/pan preservation, L/R offsets, resonant filters, infinite bounded feedback.\n";
    return 0;
}
