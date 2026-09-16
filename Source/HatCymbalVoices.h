// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "SaikeMetal.h"
#include <array>
#include <cstdint>
namespace lr608 {
struct HatParameters{std::array<double,10>v{};};
struct CymbalParameters{std::array<double,7>v{};};
struct MetalBiquad{double x1=0,x2=0,y1=0,y2=0;double tickHp(double,double,double,double,double,double);void clear(){x1=x2=y1=y2=0;}};
class HatVoice{
public:void prepare(double);void reset();void trigger(int,bool,int,const HatParameters&);void trigger(int,bool,int,const HatParameters&,std::uint32_t);double render(const HatParameters&);bool isActive()const{return active;}
private:double random(),unitRandom();double sr=44100,velocity=1,env=0,phase[13]{},clock=0,clockSample=0,resPhase=0,resTarget=0,resSmooth=0,modalExc=0,modalContact=0,modalExcK=0,modalContactK=0,p1Value=0,p1From=0,p1To=0,p1Phase=0,p1Rate=1,p1Hold=0,p1Curve=0,p2Value=0,p2From=0,p2To=0,p2Phase=0,p2Rate=1,p2Hold=0,p2Curve=0;double decayStep=0,tune=1,structure=0,cutBase=0,qBase=1,phaseStep[6]{},clockStep=0,hpB0=0,hpB1=0,hpB2=0,hpA1=0,hpA2=0;int engine=0;bool open=false,active=false,staticHp=false;std::uint32_t rng=0x608a7u;std::array<MetalSvf,6>modes{};MetalBiquad hp;SaikeHat saike;
};
class CymbalVoice{
public:explicit CymbalVoice(bool crashVoice):crash(crashVoice){}void prepare(double);void reset();void trigger(int,int,const CymbalParameters&);void trigger(int,int,const CymbalParameters&,std::uint32_t);double render(const CymbalParameters&);bool isActive()const{return active;}
private:double random(),unitRandom();double sr=44100,velocity=1,env=0,phase[13]{},shimmer=0,decayLoss=0,hpB0=0,hpB1=0,hpB2=0,hpA1=0,hpA2=0;int engine=0;bool crash=false,active=false;std::uint32_t rng=0x608c7u;MetalBiquad hp;SaikeCymbal saike;
};
}
