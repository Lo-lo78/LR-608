// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "SlotArchitecture.h"
#include "Kick808Voice.h"
#include "KickOtherVoices.h"
#include "SnareVoice.h"
#include "ClapRimVoices.h"
#include "TomVoice.h"
#include "HatCymbalVoices.h"
#include "MaracasVoice.h"
#include "CowbellVoice.h"
#include "ZapVoice.h"
#include "OutputStage.h"
#include <array>
#include <atomic>
#include <cstdint>
namespace lr608 {
class PolyphonicVoice {
public:
 void prepare(double);void reset();
 void start(int engine,int velocity,int output,int midiNote,int sourceSlotIndex,const std::array<std::atomic<float>,slotParameterValueCount>&,double tempo,std::uint64_t age);
 void choke();StereoSample render();bool isActive()const{return active;}std::uint64_t getAge()const{return voiceAge;}int getRoute()const{return route;}int getMidiNote()const{return sourceMidiNote;}int getSlot()const{return sourceSlot;}
private:
 struct Biquad
 {
  double b0=1,b1=0,b2=0,a1=0,a2=0,z1L=0,z2L=0,z1R=0,z2R=0;bool bypass=true;
  void reset();void configure(bool highPass,double cutoff,double resonance,double sampleRate,bool neutral);double process(double input,bool right);bool hasTail()const;
 };
 Kick808Voice kick808;KickOtherVoices kickOther;SnareVoice snare;ClapVoice clap;RimVoice rim;TomVoice low{2},mid{1},high{0};HatVoice hat;CymbalVoice crash{true},ride{false};MaracasVoice maracas;CowbellVoice cowbell;ZapVoice zap;
 Kick808Parameters kp{};SnareParameters sp{};ClapParameters cp{};RimParameters rp{};TomParameters tp{};HatParameters hp{};CymbalParameters yp{};MaracasParameters mp{};CowbellParameters wp{};ZapParameters zp{};
 SlotFamily family=SlotFamily::kick;int sub=0,route=0,sourceMidiNote=-1,sourceSlot=-1,chokeRemaining=0,chokeLength=96;bool active=false;std::uint64_t voiceAge=0;double tempo=120,pan=0,filterSampleRate=44100;Biquad highPassFilter,lowPassFilter;
 double lpBaseCutoff=18000,lpResonance=.707,hpBaseCutoff=20,hpResonance=.707,filterEnvelopeDepth=0,filterEnvelopeValue=0;
 std::int64_t filterEnvelopeAgeSamples=0,filterEnvelopeDecaySamples=0,filterEnvelopeAttackSamples=1;int filterEnvelopeCounter=0;
 double degradeAmount=0,degradeQScale=128.0,degradeHoldL=0,degradeHoldR=0,degradeJitter=0;int degradeHoldSamples=1,degradeCount=0;std::uint32_t degradeRng=0x608d4a7u;
};
}
