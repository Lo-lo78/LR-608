// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
namespace lr608 {
class HatCymbalEngineBanks {
public:
 explicit HatCymbalEngineBanks(juce::AudioProcessorValueTreeState&);
 void resetFromCurrentState(); void resetToFactory();
 bool switchHat(int); bool switchCymbal(int); void captureHat(); void captureCymbal();
 static bool ownsHat(juce::StringRef); static bool ownsCymbal(juce::StringRef);
private:
 void loadHat(int);void loadCymbal(int);juce::AudioProcessorValueTreeState& state;
 std::array<std::array<float,10>,7> hats{};std::array<std::array<float,7>,4> cymbals{};
 int hatEngine=0,cymbalEngine=0;bool applying=false;
}; }
