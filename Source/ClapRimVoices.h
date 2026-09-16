// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "HeapArray.h"
#include <array>
#include <cstdint>
namespace lr608 {
struct VoiceStereo { float left=0,right=0; };
struct ClapParameters { std::array<double,11> v{}; double accentThreshold=112,accentCharacter=1; };
struct RimParameters { std::array<double,18> v{}; double accentThreshold=112,accentCharacter=1,tempo=120; };
class ClapVoice {
public: void prepare(double);void reset();void trigger(int,int,const ClapParameters&);void trigger(int,int,const ClapParameters&,std::uint32_t);VoiceStereo render(const ClapParameters&);bool isActive()const{return active;}
private:
 struct Svf{double i1=0,i2=0,k=1,a1=0,a2=0,a3=0;void init(double,double,double);double bp(double);double hp(double);};
 double random(),poly(double,double);void resetSaike(const ClapParameters&);double tickSaike(const ClapParameters&);void resetLinn(const ClapParameters&);double tickLinn(const ClapParameters&);VoiceStereo tickRoland(const ClapParameters&);
 double sr=44100,velocity=1,accent=0,time=0,tail=0,hold=0;
 int engine=0,aliveCount=0,snap=0,holdCount=0;bool active=false;
 std::uint32_t rng=0x608c1a9u;std::array<Svf,5> f{};
 double attackState=0,attackThreshold=0,attackK=0,k1=0,k2=0,k3=0,wash=0,washRate=0,atkLevel=0,washLevel=0,hfLevel=0,attackMix=0,washMix=0,phase1=0,phase2=0,phaseRate=0,last=0;
 std::array<Svf,10> linnF{};int linnClock=0,linnTrigger1=0,linnWashHold=0,linnMaxSamples=0;bool linnTriggerDone=false;
 double linnBurst0=0,linnBurst1=0,linnBurst0Rate=0,linnBurst1Rate=0,linnWash=0,linnWashRate=0,linnBright=0,linnBrightRate=0,linnClusterLevel=0,linnWashLevel=0,linnBodyLevel=0,linnUpperLevel=0,linnDigitalPhase=0,linnDigitalHold=0,linnDac1=0,linnDac2=0,linnDac3=0;
 double mainLowL=0,mainBandL=0,mainLowR=0,mainBandR=0,popLowL=0,popBandL=0,popLowR=0,popBandR=0,noiseLpL=0,noiseLpR=0,vcaL=0,vcaR=0;
};
class RimVoice {
public:void prepare(double);void reset();void trigger(int,int,const RimParameters&);void trigger(int,int,const RimParameters&,std::uint32_t);VoiceStereo render(const RimParameters&);bool isActive()const{return voiceOn||delayOn;}
private:
 struct Svf{double i1=0,i2=0,k=1,a1=0,a2=0,a3=0;void init(double,double,double);double bp(double);};double random();double compress(double,const RimParameters&);
 double sr=44100,velocity=1,accent=0,env=0,clickEnv=0,phase1=0,phase2=0,clickPhase=0,sat=0;
 int engine=0;bool voiceOn=false,delayOn=false;std::uint32_t rng=0x60871u;
 double state=0,state2=0,state3=0,atk=0,k1=0,k2=0,k3=0,kAtk=0,secondLevel=0;std::array<Svf,5> filters{};
 std::array<double,6> acousticEnv{},acousticK{};
 std::array<double,5> acousticPhase{},acousticFreq{};
 double acousticCharacter=0,acousticSecond=0,acousticGain=1,acousticHitColor=0;
 double acousticNoisePrev=0,acousticWireLp=0,acousticWireHpMem=0,acousticDc=0;
 HeapArray<double,65536> delay;int writePos=0;double readPos=0,writeMem=0,dc=0,x1=0,x2=0,y1=0,y2=0,rms=0,runningDb=0;
}; }
