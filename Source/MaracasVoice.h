// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "SaikeMetal.h"
#include <array>
#include <cstdint>
namespace lr608 {
struct MaracasParameters { std::array<double,8> v{}; double accentThreshold=112,accentCharacter=1; };
class MaracasVoice {
public:void prepare(double);void reset();void trigger(int,int,const MaracasParameters&);void trigger(int,int,const MaracasParameters&,std::uint32_t);double render(const MaracasParameters&);bool isActive()const{return active;}
private:
 struct OnePole{double coeff=0,state=0;void init(double,double);double hp(double);};
 struct GainBell{double i1=0,i2=0,a1=0,a2=0,a3=0,m1=0;void init(double,double,double,double);double tick(double);};
 struct SaikeShaker{void reset(int,double,double,double);double tick(std::uint32_t&);bool isAlive()const{return alive;}int type=0;double sr=44100,state=0,state2=0,state3=0,attack=1,k=0,kFast=0,kSlow=0,kAtk=0,nAttack=0,t=0,dt=0,age=0,maxAge=0;bool alive=false;MetalSvf lp1,lp2,hp2;OnePole hp1;GainBell bell1,bell2,bell3;};
 double unitRandom(),random(); double sr=44100,velocity=1,accent=0,env=0,attTime=0,decTime=0,ghostEnv=0,ghostCount=0,ghostLevel=0,ghostAtt=0,ghostDec=0,shCount=0,shTarget=0,shSmooth=0,shHold=1,shCoef=0,pink=0,lp=0,hpState=0,ghostLp=0,ghostHp=0,attackCoef=1,decayCoef=1,ghostAttackCoef=1,ghostDecayCoef=1,lpCoef=0,hpCoef=0,ghostLpCoef=0,ghostHpCoef=0;int engine=0;bool active=false,attackStage=false,ghostPending=false,ghostStage=false;std::uint32_t rng=0x6085a17u;SaikeShaker saike;
};
}
