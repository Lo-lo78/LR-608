// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_core/juce_core.h>
#include <array>
namespace lr608 {
inline constexpr int slotCount=128;
inline constexpr int maxMidiNoteLayers=8;
inline constexpr int slotEngineCount=90;
inline constexpr int offEngineIndex=89;
inline constexpr int slotParameterValueCount=276;
inline constexpr int slotPanParameterIndex=270;
inline constexpr int slotVoiceOverlapParameterIndex=271;
inline constexpr int slotLowPassCutoffParameterIndex=272;
inline constexpr int slotLowPassResonanceParameterIndex=273;
inline constexpr int slotHighPassCutoffParameterIndex=274;
inline constexpr int slotHighPassResonanceParameterIndex=275;
enum class SlotFamily{kick,snare1,snare2,clap,rim,lowTom,midTom,highTom,hatClosed,hatOpen,crash,ride,maracas,cowbell,zap};
struct SlotEngineInfo{const char*name;SlotFamily family;int subEngine;int page;int routeParameterIndex;};
#define LR_ENG(n,f,s,p,r) {n,SlotFamily::f,s,p,r}
inline constexpr SlotEngineInfo slotEngines[]{
 LR_ENG("Kick 808",kick,0,0,256),LR_ENG("Kick Simmons",kick,1,0,256),LR_ENG("Kick 909",kick,2,0,256),LR_ENG("Kick Saike Type 0",kick,3,0,256),LR_ENG("Kick Saike Type 1",kick,4,0,256),LR_ENG("Kick Saike Type 2",kick,5,0,256),LR_ENG("Kick Saike Type 3",kick,6,0,256),LR_ENG("Kick Linn Acoustic",kick,7,0,256),
 LR_ENG("Snare 1 808",snare1,0,1,257),LR_ENG("Snare 1 Simmons",snare1,1,1,257),LR_ENG("Snare 1 Acoustic",snare1,2,1,257),LR_ENG("Snare 1 Saike Type 1",snare1,3,1,257),LR_ENG("Snare 1 Saike Type 2",snare1,4,1,257),LR_ENG("Snare 1 Saike Type 3",snare1,5,1,257),LR_ENG("Snare 1 Hybrid FM",snare1,6,1,257),LR_ENG("Snare 1 Saike 909 Variant",snare1,7,1,257),LR_ENG("Snare Linn",snare1,8,1,257),
 LR_ENG("Clap 808 Circuit",clap,0,3,259),LR_ENG("Clap 909 Crunch",clap,1,3,259),LR_ENG("Clap Roland 808 909",clap,2,3,259),LR_ENG("Clap Saike Type 0",clap,3,3,259),LR_ENG("Clap Saike Type 1",clap,4,3,259),LR_ENG("Clap Saike Type 2",clap,5,3,259),LR_ENG("Clap Linn",clap,6,3,259),
 LR_ENG("Rim LR-608",rim,0,4,260),LR_ENG("Rim Saike Type 0",rim,1,4,260),LR_ENG("Rim Saike Type 1",rim,2,4,260),LR_ENG("Rim Saike Type 2",rim,3,4,260),LR_ENG("Rim Acoustic Cross-Stick",rim,4,4,260),LR_ENG("Rim Acoustic Pop Rimshot",rim,5,4,260),
 LR_ENG("Low Tom 808",lowTom,0,5,261),LR_ENG("Low Tom Simmons",lowTom,1,5,261),LR_ENG("Low Tom Saike Type 0",lowTom,2,5,261),LR_ENG("Low Tom Saike Type 1",lowTom,3,5,261),LR_ENG("Low Tom Saike Type 2",lowTom,4,5,261),
 LR_ENG("Mid Tom 808",midTom,0,5,262),LR_ENG("Mid Tom Simmons",midTom,1,5,262),LR_ENG("Mid Tom Saike Type 0",midTom,2,5,262),LR_ENG("Mid Tom Saike Type 1",midTom,3,5,262),LR_ENG("Mid Tom Saike Type 2",midTom,4,5,262),
 LR_ENG("High Tom 808",highTom,0,5,263),LR_ENG("High Tom Simmons",highTom,1,5,263),LR_ENG("High Tom Saike Type 0",highTom,2,5,263),LR_ENG("High Tom Saike Type 1",highTom,3,5,263),LR_ENG("High Tom Saike Type 2",highTom,4,5,263),
 LR_ENG("HiHat Closed LR-608",hatClosed,0,6,264),LR_ENG("HiHat Closed Ring Alloy",hatClosed,1,6,264),LR_ENG("HiHat Closed Noise PM",hatClosed,2,6,264),LR_ENG("HiHat Closed Modal Shell",hatClosed,3,6,264),LR_ENG("HiHat Closed Saike Type 0",hatClosed,4,6,264),LR_ENG("HiHat Closed Saike Type 1",hatClosed,5,6,264),LR_ENG("HiHat Closed Saike Type 2",hatClosed,6,6,264),
 LR_ENG("HiHat Open LR-608",hatOpen,0,6,264),LR_ENG("HiHat Open Ring Alloy",hatOpen,1,6,264),LR_ENG("HiHat Open Noise PM",hatOpen,2,6,264),LR_ENG("HiHat Open Modal Shell",hatOpen,3,6,264),LR_ENG("HiHat Open Saike Type 0",hatOpen,4,6,264),LR_ENG("HiHat Open Saike Type 1",hatOpen,5,6,264),LR_ENG("HiHat Open Saike Type 2",hatOpen,6,6,264),
 LR_ENG("Crash LR-608",crash,0,7,265),LR_ENG("Crash Saike Type 0",crash,1,7,265),LR_ENG("Crash Saike Type 1",crash,2,7,265),LR_ENG("Crash Saike Type 2",crash,3,7,265),
 LR_ENG("Ride LR-608",ride,0,7,266),LR_ENG("Ride Saike Type 0",ride,1,7,266),LR_ENG("Ride Saike Type 1",ride,2,7,266),LR_ENG("Ride Saike Type 2",ride,3,7,266),
 LR_ENG("Maracas LR-608",maracas,0,8,267),LR_ENG("Maracas Saike Type 0",maracas,1,8,267),LR_ENG("Maracas Saike Type 1",maracas,2,8,267),LR_ENG("Maracas Saike Type 2",maracas,3,8,267),
 LR_ENG("Clave Cowbell LR-608",cowbell,0,9,268),LR_ENG("Clave Cowbell Saike Type 0",cowbell,1,9,268),LR_ENG("Clave Cowbell Saike Type 1",cowbell,2,9,268),LR_ENG("Clave Cowbell Saike Type 2",cowbell,3,9,268),LR_ENG("Clave Cowbell Saike Type 3",cowbell,4,9,268),LR_ENG("Timbales Physical",cowbell,5,9,268),LR_ENG("Timbales Wave Mesh",cowbell,6,9,268),
 LR_ENG("Zap LR-608",zap,0,10,269),LR_ENG("Zap Clocked Alarm",zap,1,10,269),LR_ENG("Zap Particle Beacon",zap,2,10,269),LR_ENG("Zap Modal UFO",zap,3,10,269),LR_ENG("Zap Karplus Wire",zap,4,10,269),LR_ENG("Zap FM Siren",zap,5,10,269),LR_ENG("Zap Grain Laser",zap,6,10,269),LR_ENG("Zap Chaos Relay",zap,7,10,269),LR_ENG("Zap Hyper Spring",zap,8,10,269),LR_ENG("Zap Bouncing Coin",zap,9,10,269),LR_ENG("Zap Karplus Wire Tune -",zap,10,10,269),
 LR_ENG("Off - No sound",kick,0,0,256)
};
#undef LR_ENG
static_assert(std::size(slotEngines)==slotEngineCount);
inline juce::String slotEngineId(int slot){return "slot"+juce::String(slot+1).paddedLeft('0',3)+"Engine";}
inline juce::String slotNoteId(int slot){return "slot"+juce::String(slot+1).paddedLeft('0',3)+"Note";}
inline juce::String slotChokeTriggerId(int slot){return "slot"+juce::String(slot+1).paddedLeft('0',3)+"ChokeTrigger";}
inline juce::String slotChokeTargetId(int slot){return "slot"+juce::String(slot+1).paddedLeft('0',3)+"ChokeTarget";}
inline constexpr int primarySlotEngines[]{0,8,8,17,24,30,35,40,45,52,59,63,67,71,78};
inline constexpr int primarySlotNotes[]{36,38,40,39,37,41,45,48,42,46,49,51,58,56,52};
// Slots beyond the original fixed kit retain the exact pre-expansion cycle.
// The historical bank contained 84 sound engines. New Kick Linn, Snare Linn,
// Clap Linn and the two acoustic Rim engines must not shift existing sounds.
inline int historicalEngineToCurrent(int engine)
{
 if(engine<=6)return engine;
 if(engine<=14)return engine+1;
 if(engine<=20)return engine+2;
 if(engine<=24)return engine+3;
 return engine+5;
}
inline int defaultSlotEngine(int slot){return slot<15?primarySlotEngines[slot]:historicalEngineToCurrent((slot-15)%84);}
inline bool isOffEngine(int engine){return engine==offEngineIndex;}
inline int familyDefaultNote(SlotFamily f){switch(f){case SlotFamily::kick:return 36;case SlotFamily::snare1:return 38;case SlotFamily::snare2:return 40;case SlotFamily::clap:return 39;case SlotFamily::rim:return 37;case SlotFamily::lowTom:return 41;case SlotFamily::midTom:return 45;case SlotFamily::highTom:return 48;case SlotFamily::hatClosed:return 42;case SlotFamily::hatOpen:return 46;case SlotFamily::crash:return 49;case SlotFamily::ride:return 51;case SlotFamily::maracas:return 58;case SlotFamily::cowbell:return 56;case SlotFamily::zap:return 52;}return 36;}
inline int defaultSlotNote(int slot){return slot<15?primarySlotNotes[slot]:60+(slot-15)%68;}
inline int slotFamilyGroup(SlotFamily family)
{
 switch(family){case SlotFamily::kick:return 0;case SlotFamily::snare1:case SlotFamily::snare2:return 1;case SlotFamily::clap:return 2;case SlotFamily::rim:return 3;case SlotFamily::lowTom:case SlotFamily::midTom:case SlotFamily::highTom:return 4;case SlotFamily::hatClosed:case SlotFamily::hatOpen:return 5;case SlotFamily::crash:case SlotFamily::ride:return 6;case SlotFamily::maracas:return 7;case SlotFamily::cowbell:return 8;case SlotFamily::zap:return 9;}return-1;
}
inline const char* slotFamilyGroupName(SlotFamily family)
{
 constexpr const char*names[]{"Kick","Snare","Clap","Rim","Toms","HiHat","Cymbal","Maracas","Clave Cowbell","Zap"};return names[juce::jlimit(0,9,slotFamilyGroup(family))];
}
inline bool isEngineSelectorId(juce::StringRef id){const juce::String s(id.text);return s=="slider247"||s=="slider248"||s=="slider198"||s=="slider207"||s=="slider209"||s=="slider219"||s=="slider217"||s=="slider214"||s=="slider216"||s=="slider215"||s=="slider199";}
}
