// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>
namespace lr608 {
struct MetalSvf{double i1=0,i2=0,k=1,a1=0,a2=0,a3=0;void init(double,double,double);double bp(double);double hp(double);double lp(double);};
double metalPoly(double,double);double metalSaw(double,double);double metalSquare(double,double,double);
struct QuickEllip{std::array<double,16>s{};double tick(double);};
struct FrequencyShift{double c1=0,c2=0,s1=0,s2=0,cc1=0,cc2=0,ss1=0,ss2=0,oc1=0,oc2=0;QuickEllip l1,l2;void init(double,double);double tick(double);};
class SaikeCymbal{
public:void reset(int,double,double,double,double,double);double tick(std::uint32_t&);bool isAlive()const{return alive;}
private:double rnd(std::uint32_t&);int type=0;double sr=44100,tone=.5,duty=.5,age=0,maxAge=0,t=0;bool alive=false;
 std::array<double,13>ph{},dt{};std::array<MetalSvf,19>f{};FrequencyShift shift1,shift2;
 double s12=0,n12=0,k12a=0,k12r=0,a12=0,s13=0,n13=0,k13a=0,k13r=0,a13=0,s2=0,n2=0,k2a=0,k2r=0,a2=0,s3=0,n3=0,k3a=0,k3r=0,a3=0;
 double e1s=0,e1amp=0,e1n=0,e1atk=0,e1rel=0,e2s=0,e2k=0,e3s=0,e3amp=0,e3n=0,e3atk=0,e3rel=0,e4s=0,e4amp=0,e4n=0,e4atk=0,e4rel=0;
};
class SaikeHat{
public:void reset(int,bool,const std::array<double,10>&,double,std::uint32_t&);double tick(std::uint32_t&);bool isAlive()const{return alive;}
private:double rnd(std::uint32_t&);int type=0;double sr=44100,age=0,maxAge=0,t=0,body=0,metal1=0,metal2=0,resAmount=0,resSpeed=80,resPhase=0;bool alive=false;
 std::array<double,13>ph{},dt{};std::array<MetalSvf,10>f{};
 double s12=0,n12=0,k12a=0,k12r=0,a12=0,s13=0,n13=0,k13a=0,k13r=0,a13=0,s2=0,n2=0,k2a=0,k2r=0,a2=0,s3=0,n3=0,k3a=0,k3r=0,a3=0;
};
}
