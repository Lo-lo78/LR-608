// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace lr608
{
class KickEngineBanks
{
public:
    explicit KickEngineBanks (juce::AudioProcessorValueTreeState&);
    void resetFromCurrentState();
    bool switchTo (int engine);
    void captureCurrent();
    static bool ownsParameter (juce::StringRef id);
    int current() const noexcept { return currentEngine; }

private:
    static constexpr int engineCount = 8;
    static constexpr int parameterCount = 25;
    void fillFactory();
    void load (int engine);
    juce::AudioProcessorValueTreeState& state;
    std::array<std::array<float, parameterCount>, engineCount> banks {};
    int currentEngine = 0;
    bool applying = false;
};
}
