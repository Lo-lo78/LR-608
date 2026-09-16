// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace lr608 {
class TomEngineBanks {
public:
    explicit TomEngineBanks (juce::AudioProcessorValueTreeState&);
    void resetFromCurrentState();
    void resetToFactory();
    bool switchTo (int engine);
    void captureCurrent();
    static bool ownsParameter (juce::StringRef id);
private:
    void load (int engine);
    juce::AudioProcessorValueTreeState& state;
    std::array<std::array<float, 54>, 5> banks {};
    int currentEngine = 0;
    bool applying = false;
};
}
