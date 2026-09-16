// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DrumTimingEngine.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace
{
void require (bool condition, const char* message)
{
    if (! condition) { std::cerr << message << '\n'; std::exit (1); }
}
}

int main()
{
    using namespace lr608;
    require (DrumTimingEngine::hitsPerBar (0) == 0, "Roll Off mapping failed");
    require (DrumTimingEngine::hitsPerBar (16) == 768, "Fast roll mapping failed");
    require (DrumTimingEngine::pitchBendDivision (5, 0) == 5, "Pitch bend centre failed");
    require (DrumTimingEngine::pitchBendDivision (5, 8191) == 16, "Pitch bend upper bound failed");
    require (DrumTimingEngine::pitchBendDivision (5, -8192) == 1, "Pitch bend lower bound failed");
    require (DrumTimingEngine::pitchBendDivision (0, 8191) == 0, "Pitch bend must not enable Roll");
    require (DrumTimingEngine::isSupportedNote (0) && DrumTimingEngine::isSupportedNote (127), "Slot MIDI range is not fully supported");

    DrumTimingEngine timing;
    TimingSettings settings;
    settings.sampleRate = 48000.0; settings.tempo = 120.0; settings.rollDivision = 5;
    settings.secondHitReductionPercent = 25.0;
    timing.setSettings (settings); timing.reset();
    require (! timing.needsSampleClock(), "Configured Roll prevents idle sleep without a held note");

    std::vector<std::pair<int, DrumTrigger>> events;
    int sample = 0;
    auto collect = [&] (DrumTrigger trigger) { events.emplace_back (sample, trigger); };
    std::vector<std::pair<int,GeneratedDrumMidi>> midiEvents;
    auto collectMidi=[&](GeneratedDrumMidi event){midiEvents.emplace_back(sample,event);};
    require(timing.handleMidi (0x92, 36, 100, collect,collectMidi),"Roll did not consume its physical Note On");
    require (timing.needsSampleClock(), "Held Roll note did not wake the sample clock");
    for (sample = 0; sample < 7000; ++sample) timing.tick (collect,collectMidi);
    require (events.size() >= 2, "Roll did not repeat");
    require (events[0].first == 0 && events[0].second.velocity == 100, "First roll hit is not immediate");
    // JSFX increments roll_clock at the start of @sample, so the grid point
    // whose clock value is 6000 is rendered at zero-based sample 5999.
    require (events[1].first == 5999 && events[1].second.velocity == 75, "Second roll hit timing/velocity failed");
    require(midiEvents.size()>=3&&midiEvents[0].first==0&&midiEvents[0].second.noteOn&&midiEvents[0].second.channel==2&&midiEvents[0].second.velocity==100,"Generated Roll MIDI Note On failed");
    require(!midiEvents[1].second.noteOn&&midiEvents[1].first==960,"Generated Roll MIDI gate failed");
    require(timing.handleMidi(0x82,36,0,collect,collectMidi),"Roll did not consume its matching physical Note Off");
    require (! timing.needsSampleClock(), "Released Roll note did not return the timing engine to idle");

    timing.reset();events.clear();midiEvents.clear();
    settings.hostTimelineValid=true;settings.hostPlaying=true;settings.hostPpqPosition=.1;settings.hostBarStartPpq=0;
    timing.setSettings(settings);
    timing.handleMidi(0x90,36,100,collect,collectMidi);
    for(sample=0;sample<4000;++sample)timing.tick(collect,collectMidi);
    require(!events.empty()&&events.front().first==3600,"Host-synchronised Roll did not wait for the DAW sixteenth-note grid");

    timing.reset(); events.clear(); settings.rollDivision = 0;settings.hostTimelineValid=false;settings.hostPlaying=false; timing.setSettings (settings);
    timing.handleMidi (0xb0, 1, 127, collect);
    timing.handleMidi (0x90, 38, 100, collect);
    for (sample = 0; sample < 400; ++sample) timing.tick (collect);
    require (events.size() == 3, "Drag must emit exactly three hits");
    require (events[0].first == 0 && events[0].second.velocity == 35, "First drag grace hit failed");
    require (events[1].first == 192 && events[1].second.velocity == 55, "Second drag grace hit failed");
    require (events[2].first == 384 && events[2].second.velocity == 100, "Drag main hit failed");

    std::cout << "LR-608 timing audit passed\n";
}
