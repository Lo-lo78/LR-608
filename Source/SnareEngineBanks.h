// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace lr608
{
class SnareEngineBanks
{
public:
    explicit SnareEngineBanks (juce::AudioProcessorValueTreeState&);
    void resetFromCurrentState();
    bool switchTo (int slot, int engine);
    void captureCurrent (int slot);
    static bool ownsParameter (int slot, juce::StringRef id);
    int current (int slot) const noexcept { return currentEngine[slot]; }

private:
    static constexpr int slotCount = 2, engineCount = 9, parameterCount = 31;
    void fillFactory();
    void load (int slot, int engine);
    juce::AudioProcessorValueTreeState& state;
    std::array<std::array<std::array<float, parameterCount>, engineCount>, slotCount> banks {};
    std::array<int, slotCount> currentEngine {};
    bool applying = false;
};
}
