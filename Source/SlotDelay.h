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
        double wetPercent = 0.0;
        int timeIndex = 1016; // 1/8 note; see quarterNotesForTimeIndex().
        double feedbackPercent = 50.0;
        double glideMs = 0.0;
        double filter = 0.5;
        double filterResonance = 0.707;
        double pitchSemitones = 0.0;
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
    static double quarterNotesForTimeIndex (int index) noexcept;
    static double tapeLimit (double x) noexcept;
    void ensureCapacity (std::size_t samples);
    double readFractional (const std::vector<float>& buffer, double delaySamples) const noexcept;
    double readPitchShifted (const std::vector<float>& buffer, double baseDelaySamples,
                             double phase, double windowSamples) const noexcept;
    double filterFeedback (double input, bool right) noexcept;
    void updateDelayGlide() noexcept;

    double sr = 44100.0;
    std::vector<float> leftBuffer, rightBuffer;
    std::size_t writeIndex = 0;

    double wetGain = 0.0;
    double feedbackGain = 0.5;
    double filterPosition = 0.5;
    double filterResonance = 0.707;
    double filterFeedbackCompensation = 1.0;
    double filterA1 = 1.0, filterA2 = 0.0, filterA3 = 0.0, filterK = 1.0 / 0.707;
    double filterIc1L = 0.0, filterIc2L = 0.0;
    double filterIc1R = 0.0, filterIc2R = 0.0;

    double currentDelayL = 1.0, currentDelayR = 1.0;
    double targetDelayL = 1.0, targetDelayR = 1.0;
    double delayStepL = 0.0, delayStepR = 0.0;
    int glideSamplesRemaining = 0;
    double glideMs = 0.0;

    double pitchSemitones = 0.0;
    double pitchRatio = 1.0;
    double pitchPhase = 0.0;

    Settings lastSettings {};
    bool hasSettings = false;
    bool enabled = false;
    bool tailActive = false;
    int silentSamples = 0;
};
}
