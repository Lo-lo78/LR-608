// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
namespace lr608 {
class ClapRimEngineBanks {
public:
 explicit ClapRimEngineBanks(juce::AudioProcessorValueTreeState&);
 void resetFromCurrentState(); bool switchClap(int); bool switchRim(int);
 void captureClap(); void captureRim(); static bool ownsClap(juce::StringRef); static bool ownsRim(juce::StringRef);
private:
 void loadClap(int); void loadRim(int); juce::AudioProcessorValueTreeState& state;
 std::array<std::array<float,11>,7> clap{}; std::array<std::array<float,18>,6> rim{};
 int clapEngine=0,rimEngine=0; bool applying=false;
}; }
