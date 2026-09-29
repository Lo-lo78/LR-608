// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
namespace lr608 {
class OrganicGranulator {
public:
 void prepare(double s){sr=std::max(1.0,s);reset();}
 void reset(){e.fill(0);c.fill(1);mean=0;}
 template<class Random01> double process(double x,double env,double percent,Random01 rnd){
  const auto amount=std::clamp(percent*.01,0.0,10.0); if(amount<=1e-9){reset();return x;}
  // 0..100 preserves the original granulator exactly.  Above 100 the
  // control enters an extended texture range up to 1000, increasing event
  // density and modulation depth without changing old presets.
  const auto a=std::min(1.0,amount);
  const auto over=std::clamp((amount-1.0)/9.0,0.0,1.0);
  const auto density=1.0+over*7.0;
  const double rates[]{(1050+a*1250)*density,(1320+a*1580)*density,(1680+a*1900)*density,(2110+a*2220)*density,(2640+a*2550)*density};
  const double wf[]{.26,.23,.20,.17,.14}, base[]{.46,.43,.40,.37,.34}, spread[]{.54,.57,.60,.63,.66};
  const double tailAdd[]{.18,.16,.14,.12,.10}, cmul[]{.52,.50,.48,.46,.44}, cspan[]{1.18,1.22,1.26,1.30,1.34};
  const double fastSec[]{.00022,.00029,.00037,.00046,.00057}, slowSec[]{.00105,.00122,.00142,.00165,.00192};
  const auto tail=1-std::sqrt(std::clamp(env,0.0,1.0)), speed=.10+.90*std::sqrt(std::clamp(env,0.0,1.0));
  double sum=0; for(int i=0;i<5;++i){c[i]-=1;if(c[i]<=0){e[i]+=(base[i]+rnd()*spread[i])*(1+tailAdd[i]*tail);c[i]=(std::max(1.0,sr/rates[i])/speed)*(cmul[i]+rnd()*cspan[i]);}const auto df=std::exp(-1/std::max(1.0,fastSec[i]*sr)),ds=std::exp(-1/std::max(1.0,slowSec[i]*sr));e[i]*=df+(ds-df)*tail;sum+=e[i]*wf[i];}
  mean+=(sum-mean)*(1-std::exp(-1/std::max(1.0,.028*sr))); const auto norm=std::clamp(sum/std::max(.08,mean),.08,2.65);
  const auto strength=a*(1+2*a)*(1.0+over*4.5),depth=strength*(.30+.68*tail);
  return x*std::clamp(1+depth*(norm-1),.02,7.5);
 }
private: double sr=44100,mean=0;std::array<double,5>e{},c{};
}; }
