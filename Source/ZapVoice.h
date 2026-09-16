// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "HeapArray.h"
#include <array>
#include <cstdint>
namespace lr608 {
struct ZapParameters { std::array<double,22> v{}; double tempo=120; };
class ZapVoice {
public:
    void prepare(double); void reset(); void trigger(int,int,const ZapParameters&); void trigger(int,int,const ZapParameters&,std::uint32_t);
    double render(const ZapParameters&); bool isActive()const{return active;}
private:
    double unitRandom(),random(),wave(int,double,double);
    double sr=44100,velocity=1,envFast=0,envSlow=0,clickEnv=0,tailEnergy=0,phase=0,satMem=0;
    double lfoPhase=0,lfoEnv=0,lfoPrev=0,lfoSmooth=0,lfoSH=0,lfoVal=0;
    double rmPhase=0,rmNoise=0,rmPrev=0,compRunAve=0,compRunDb=0;
    double fastCoef=0,slowCoef=0,lfoEnvCoef=0,lfoSmoothCoef=0;
    double clockPhase=0,clockHold=0,clockLow=0,clockBand=0;
    std::array<double,4> partPhase{},partEnv{},modalPhase{},modalEnv{};
    double partAP=0,fmC=0,fmM=0,fmPrev=0;
    std::array<double,4> grainPhase{},grainLife{},grainStep{},grainDecay{};
    double grainTimer=0; int grainSlot=0;
    double chaosPhase=0,chaosX=.371,chaosY=.619,chaosOsc1=0,chaosOsc2=0,chaosLP=0;
    HeapArray<double,4096> ks; int ksLen=64,ksPos=0,ksVarMax=64;
    HeapArray<double,32768> spring; std::array<double,512> springAP{};
    int springW=0; std::array<int,4> springAPPos{}; double springLP=0,springDC=0,springExc=0,springChirpPhase=0,springChirpEnv=0,springDelay2=96; int springSilence=0;
    double bounceAge=0,bounceTimer=0,bounceProgress=0,bounceHit=0,bounceClick=0,bounceNoise=0,bounceLP=0;
    std::array<double,3> bouncePhase{}; bool bounceDone=true;
    int engine=0; bool active=false; std::uint32_t rng=0x6082a9u;
};
}
