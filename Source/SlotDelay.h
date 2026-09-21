// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "OutputStage.h"
#include <cstddef>
#include <vector>

namespace lr608
{
class SlotDelay
{
public:
    struct Settings
    {
        double dryPercent = 100.0;
        double wetPercent = 0.0;
        double outputDb = 0.0;
        int divisionIndex = 4; // 1/8 note.
        double feedbackPercent = 50.0;
        double glideMs = 0.0;
        double filter = 0.5;
        double leftOffsetMs = 0.0;
        double rightOffsetMs = 0.0;
        double tempo = 120.0;
    };

    void prepare (double sampleRate);
    void reset();
    void setSettings (const Settings&);
    StereoSample process (StereoSample input);
    bool isActive() const noexcept { return enabled && tailActive; }

private:
    static double quarterNotesForDivision (int index) noexcept;
    static double tapeLimit (double x) noexcept;
    void ensureCapacity (std::size_t samples);
    double readFractional (const std::vector<float>& buffer, double delaySamples) const noexcept;
    double filterFeedback (double input, bool right) noexcept;
    void updateDelayGlide() noexcept;

    double sr = 44100.0;
    std::vector<float> leftBuffer, rightBuffer;
    std::size_t writeIndex = 0;

    double dryGain = 1.0, wetGain = 0.0, outputGain = 1.0;
    double feedbackGain = 0.5;
    double filterPosition = 0.5;
    double filterLpCoefficient = 1.0;
    double filterHpCoefficient = 1.0;
    double filterLpL = 0.0, filterLpR = 0.0;
    double filterHpLpL = 0.0, filterHpLpR = 0.0;

    double currentDelayL = 1.0, currentDelayR = 1.0;
    double targetDelayL = 1.0, targetDelayR = 1.0;
    double delayStepL = 0.0, delayStepR = 0.0;
    int glideSamplesRemaining = 0;
    double glideMs = 0.0;

    Settings lastSettings {};
    bool hasSettings = false;
    bool enabled = false;
    bool tailActive = false;
    int silentSamples = 0;
};
}
