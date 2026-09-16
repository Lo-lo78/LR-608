// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
namespace lr608{class CowbellEngineBanks{public:explicit CowbellEngineBanks(juce::AudioProcessorValueTreeState&);void resetFromCurrentState();void resetToFactory();bool switchTo(int);void captureCurrent();static bool ownsParameter(juce::StringRef);private:void load(int);juce::AudioProcessorValueTreeState&state;std::array<std::array<float,11>,7>banks{};int currentEngine=0;bool applying=false;};}
