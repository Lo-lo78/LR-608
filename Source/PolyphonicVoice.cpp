// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PolyphonicVoice.h"
#include <algorithm>
#include <cmath>
namespace lr608 {
namespace {
std::uint32_t voiceRandomSeed(std::uint64_t age,int slot) noexcept
{
    auto x=age^(std::uint64_t(slot+1)*0x9e3779b97f4a7c15ULL);
    x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;x^=x>>31;
    const auto seed=std::uint32_t(x)^std::uint32_t(x>>32);
    return seed!=0?seed:0x608d4a7u;
}
}
void PolyphonicVoice::Biquad::reset(){b0=1;b1=b2=a1=a2=z1L=z2L=z1R=z2R=0;bypass=true;}
void PolyphonicVoice::Biquad::configure(bool highPass,double cutoff,double resonance,double sampleRate,bool neutral){if(neutral){bypass=true;b0=1;b1=b2=a1=a2=0;return;}bypass=false;const auto w=2*3.14159265358979323846*std::clamp(cutoff,20.0,sampleRate*.45)/sampleRate,c=std::cos(w),s=std::sin(w),alpha=s/(2*std::clamp(resonance,.5,10.0)),a0=1+alpha;if(highPass){b0=(1+c)*.5/a0;b1=-(1+c)/a0;b2=b0;}else{b0=(1-c)*.5/a0;b1=(1-c)/a0;b2=b0;}a1=-2*c/a0;a2=(1-alpha)/a0;}
double PolyphonicVoice::Biquad::process(double input,bool right){if(bypass)return input;auto&z1=right?z1R:z1L;auto&z2=right?z2R:z2L;const auto output=b0*input+z1;z1=b1*input-a1*output+z2;z2=b2*input-a2*output;return output;}
bool PolyphonicVoice::Biquad::hasTail()const{return !bypass&&(std::abs(z1L)+std::abs(z2L)+std::abs(z1R)+std::abs(z2R)>1.0e-9);}
void PolyphonicVoice::prepare(double sr){filterSampleRate=std::max(1.0,sr);kick808.prepare(sr);kickOther.prepare(sr);snare.prepare(sr);clap.prepare(sr);rim.prepare(sr);low.prepare(sr);mid.prepare(sr);high.prepare(sr);hat.prepare(sr);crash.prepare(sr);ride.prepare(sr);maracas.prepare(sr);cowbell.prepare(sr);zap.prepare(sr);chokeLength=std::max(1,int(std::lround(sr*.002)));active=false;highPassFilter.reset();lowPassFilter.reset();filterEnvelopeValue=0;filterEnvelopeAgeSamples=0;filterEnvelopeDecaySamples=0;filterEnvelopeAttackSamples=std::max<std::int64_t>(1,std::llround(filterSampleRate*.004));filterEnvelopeCounter=0;}
void PolyphonicVoice::reset(){kick808.reset();kickOther.reset();snare.reset();clap.reset();rim.reset();low.reset();mid.reset();high.reset();hat.reset();crash.reset();ride.reset();maracas.reset();cowbell.reset();zap.reset();highPassFilter.reset();lowPassFilter.reset();active=false;chokeRemaining=0;sourceMidiNote=-1;sourceSlot=-1;degradeAmount=0;degradeQScale=128.0;degradeHoldL=degradeHoldR=0;degradeJitter=0;degradeHoldSamples=1;degradeCount=0;degradeRng=0x608d4a7u;filterEnvelopeDepth=0;filterEnvelopeValue=0;filterEnvelopeAgeSamples=0;filterEnvelopeDecaySamples=0;filterEnvelopeAttackSamples=std::max<std::int64_t>(1,std::llround(filterSampleRate*.004));filterEnvelopeCounter=0;}
void PolyphonicVoice::start(int engine,int velocity,int output,int midiNote,int sourceSlotIndex,const std::array<std::atomic<float>,slotParameterValueCount>&v,double bpm,std::uint64_t age){engine=std::clamp(engine,0,slotEngineCount-1);if(isOffEngine(engine)){reset();return;}const auto&info=slotEngines[engine];family=info.family;sub=info.subEngine;route=std::clamp(output,0,OutputStage::stemCount-1);sourceMidiNote=std::clamp(midiNote,0,127);sourceSlot=std::clamp(sourceSlotIndex,0,slotCount-1);chokeRemaining=0;voiceAge=age;tempo=bpm;const auto randomSeed=voiceRandomSeed(age,sourceSlot);const auto s=[&](int n){return double(v[n-1].load(std::memory_order_relaxed));};const auto at=s(250),ac=s(251);pan=s(271);lpBaseCutoff=v[slotLowPassCutoffParameterIndex].load(std::memory_order_relaxed);lpResonance=v[slotLowPassResonanceParameterIndex].load(std::memory_order_relaxed);hpBaseCutoff=v[slotHighPassCutoffParameterIndex].load(std::memory_order_relaxed);hpResonance=v[slotHighPassResonanceParameterIndex].load(std::memory_order_relaxed);
 const auto envDepthPercent=double(v[slotFilterEnvelopeDepthParameterIndex].load(std::memory_order_relaxed));
 const auto envDecaySeconds=double(v[slotFilterEnvelopeDecayParameterIndex].load(std::memory_order_relaxed));
 filterEnvelopeDepth=std::clamp(envDepthPercent*.01,0.0,1.0);
 filterEnvelopeValue=(filterEnvelopeDepth>1.0e-9&&envDecaySeconds>=0.01)?1.0:0.0;
 filterEnvelopeAttackSamples=std::max<std::int64_t>(1,std::llround(filterSampleRate*.004));
 filterEnvelopeDecaySamples=filterEnvelopeValue>0?std::max<std::int64_t>(1,std::llround(std::max(0.01,envDecaySeconds)*filterSampleRate)):0;
 filterEnvelopeAgeSamples=0;filterEnvelopeCounter=0;
 highPassFilter.reset();lowPassFilter.reset();
 highPassFilter.configure(true,hpBaseCutoff,hpResonance,filterSampleRate,hpBaseCutoff<=20.0);lowPassFilter.configure(false,lpBaseCutoff,lpResonance,filterSampleRate,lpBaseCutoff>=18000.0);
 degradeAmount=std::clamp(double(v[slotDegradeAmountParameterIndex].load(std::memory_order_relaxed))*.01,0.0,1.0);
 const auto targetBits=std::clamp(int(std::lround(v[slotDegradeBitsParameterIndex].load(std::memory_order_relaxed))),1,16);
 const auto effectiveBits=std::clamp(int(std::lround(16.0-(16.0-targetBits)*degradeAmount)),1,16);
 const auto targetHold=std::clamp(int(std::lround(v[slotDegradeHoldParameterIndex].load(std::memory_order_relaxed))),1,64);
 degradeHoldSamples=std::max(1,int(std::lround(1.0+(targetHold-1)*degradeAmount)));
 degradeJitter=std::clamp(double(v[slotDegradeJitterParameterIndex].load(std::memory_order_relaxed))*.01,0.0,1.0);
 degradeQScale=std::max(1.0,std::pow(2.0,effectiveBits-1));degradeCount=0;degradeHoldL=degradeHoldR=0;degradeRng=randomSeed;
 if(family==SlotFamily::kick){kp={s(11),s(12),s(13),s(14),s(15),s(16),s(17),s(18),s(67),s(78),s(79),s(100),s(101),s(102),s(103),int(std::lround(s(104))),s(105),s(106),s(107),s(138),s(140),s(141),s(142),s(148),s(149),s(150),s(152),s(153),s(154),s(155),int(std::lround(s(151))),at,ac,s(256)};if(sub==0){kickOther.reset();kick808.trigger(velocity,kp,randomSeed);}else{kick808.reset();kickOther.trigger(sub,velocity,kp,randomSeed);}}
 else if(family==SlotFamily::snare1||family==SlotFamily::snare2){const int a[][31]={{20,21,22,23,24,29,116,117,26,25,27,28,19,92,94,96,98,108,139,132,133,134,135,136,137,156,157,158,159,160,161},{221,222,223,224,225,230,232,233,227,226,228,229,220,93,95,97,99,231,240,234,235,236,237,238,239,241,242,243,244,245,246}};const auto slot=family==SlotFamily::snare1?0:1;for(int i=0;i<31;++i)sp.v[i]=s(a[slot][i]);sp.accentThreshold=at;sp.accentCharacter=ac;snare.trigger(sub,velocity,sp,randomSeed);}
 else if(family==SlotFamily::clap){const int a[]{30,31,32,33,34,208,35,36,37,38,39};for(int i=0;i<11;++i)cp.v[i]=s(a[i]);cp.accentThreshold=at;cp.accentCharacter=ac;clap.trigger(sub,velocity,cp,randomSeed);}
 else if(family==SlotFamily::rim){const int a[]{71,70,69,68,124,125,143,144,145,146,147,252,162,163,164,165,166,167};for(int i=0;i<18;++i)rp.v[i]=s(a[i]);rp.accentThreshold=at;rp.accentCharacter=ac;rp.tempo=bpm;rim.trigger(sub,velocity,rp,randomSeed);}
 else if(family==SlotFamily::lowTom||family==SlotFamily::midTom||family==SlotFamily::highTom){const int a[][18]={{64,40,41,42,43,44,45,109,118,46,47,121,174,175,176,177,178,179},{65,48,49,50,51,52,53,110,119,54,55,122,180,181,182,183,184,185},{66,56,57,58,59,60,61,111,120,62,63,123,186,187,188,189,190,191}};const auto t=family==SlotFamily::lowTom?0:family==SlotFamily::midTom?1:2;for(int i=0;i<18;++i)tp.v[i]=s(a[t][i]);tp.accentThreshold=at;tp.accentCharacter=ac;if(t==0)low.trigger(sub,velocity,tp,randomSeed);else if(t==1)mid.trigger(sub,velocity,tp,randomSeed);else high.trigger(sub,velocity,tp,randomSeed);}
 else if(family==SlotFamily::hatClosed||family==SlotFamily::hatOpen){const int a[]{5,1,2,3,4,114,218,115,90,91};for(int i=0;i<10;++i)hp.v[i]=s(a[i]);hat.trigger(sub,family==SlotFamily::hatOpen,velocity,hp,randomSeed);}
 else if(family==SlotFamily::crash||family==SlotFamily::ride){const int a[]{9,10,6,7,8,112,113};for(int i=0;i<7;++i)yp.v[i]=s(a[i]);if(family==SlotFamily::crash)crash.trigger(sub,velocity,yp,randomSeed);else ride.trigger(sub,velocity,yp,randomSeed);}
 else if(family==SlotFamily::maracas){const int a[]{168,169,170,171,172,173,205,206};for(int i=0;i<8;++i)mp.v[i]=s(a[i]);mp.accentThreshold=at;mp.accentCharacter=ac;maracas.trigger(sub,velocity,mp,randomSeed);}
 else if(family==SlotFamily::cowbell){const int a[]{80,81,82,83,84,85,86,87,88,89,204};for(int i=0;i<11;++i)wp.v[i]=s(a[i]);for(int i=0;i<57;++i)wp.captured[i]=double(v[291+i].load(std::memory_order_relaxed));wp.accentThreshold=at;wp.accentCharacter=ac;cowbell.trigger(sub,velocity,wp,randomSeed,midiNote);}
 else {const int a[]{72,73,74,75,76,77,126,127,128,129,130,131,210,211,212,213,192,193,194,195,196,197};for(int i=0;i<22;++i)zp.v[i]=s(a[i]);zp.tempo=bpm;zap.trigger(sub,velocity,zp,randomSeed);}active=true;}
void PolyphonicVoice::choke(){if(active&&chokeRemaining==0)chokeRemaining=chokeLength;}
StereoSample PolyphonicVoice::render(){if(!active)return{};StereoSample out;switch(family){case SlotFamily::kick:{auto x=kick808.render(kp,tempo)+kickOther.render(kp,tempo);out={x,x};active=kick808.isActive()||kickOther.isActive();break;}case SlotFamily::snare1:case SlotFamily::snare2:{auto x=snare.render(sp);out={x,x};active=snare.isActive();break;}case SlotFamily::clap:{auto x=clap.render(cp);out={x.left,x.right};active=clap.isActive();break;}case SlotFamily::rim:{auto x=rim.render(rp);out={x.left,x.right};active=rim.isActive();break;}case SlotFamily::lowTom:{auto x=low.render(tp);out={x.left,x.right};active=low.isActive();break;}case SlotFamily::midTom:{auto x=mid.render(tp);out={x.left,x.right};active=mid.isActive();break;}case SlotFamily::highTom:{auto x=high.render(tp);out={x.left,x.right};active=high.isActive();break;}case SlotFamily::hatClosed:case SlotFamily::hatOpen:{auto x=hat.render(hp);out={x,x};active=hat.isActive();break;}case SlotFamily::crash:{auto x=crash.render(yp);out={x,x};active=crash.isActive();break;}case SlotFamily::ride:{auto x=ride.render(yp);out={x,x};active=ride.isActive();break;}case SlotFamily::maracas:{auto x=maracas.render(mp);out={x,x};active=maracas.isActive();break;}case SlotFamily::cowbell:{auto x=cowbell.render(wp);out={x,x};active=cowbell.isActive();break;}case SlotFamily::zap:{auto x=zap.render(zp);static constexpr double g[]{1,5.011872336,1,1.995262315,5.623413252,4.466835922,3.981071706,4.466835922,15.848931925,5.011872336,5.623413252};x*=g[std::clamp(sub,0,10)];out={x,x};active=zap.isActive();break;}}if(degradeAmount>1.0e-9){if(degradeCount<=0){degradeHoldL=out.left;degradeHoldR=out.right;degradeRng^=degradeRng<<13;degradeRng^=degradeRng>>17;degradeRng^=degradeRng<<5;const auto jitterSpan=int(std::floor(degradeHoldSamples*degradeJitter));const auto jitterAdd=jitterSpan>0?int(degradeRng%std::uint32_t(jitterSpan*2+1))-jitterSpan:0;degradeCount=std::max(1,degradeHoldSamples+jitterAdd);}--degradeCount;const auto quantize=[this](double x){const auto offset=x>=0?0.5:-0.5;return std::floor(x*degradeQScale+offset)/degradeQScale;};const auto crushedL=quantize(degradeHoldL),crushedR=quantize(degradeHoldR);out.left=out.left*(1.0-degradeAmount)+crushedL*degradeAmount;out.right=out.right*(1.0-degradeAmount)+crushedR*degradeAmount;}const auto isTom=family==SlotFamily::lowTom||family==SlotFamily::midTom||family==SlotFamily::highTom;if(!isTom&&std::abs(pan)>1.0e-12){constexpr double q=.78539816339744830962,r=1.4142135623730950488;const auto angle=(std::clamp(pan,-1.0,1.0)+1.0)*q;out.left*=r*std::cos(angle);out.right*=r*std::sin(angle);}if(filterEnvelopeValue>1.0e-6){
 if(filterEnvelopeCounter<=0){
  double shape=0.0;
  if(filterEnvelopeAgeSamples<filterEnvelopeAttackSamples){
   const auto x=std::clamp(double(filterEnvelopeAgeSamples)/double(filterEnvelopeAttackSamples),0.0,1.0);
   shape=x*x*(3.0-2.0*x);
  }else{
   const auto decayAge=filterEnvelopeAgeSamples-filterEnvelopeAttackSamples;
   const auto x=std::clamp(double(decayAge)/double(std::max<std::int64_t>(1,filterEnvelopeDecaySamples)),0.0,1.0);
   shape=1.0-x*x*(3.0-2.0*x);
  }
  const auto amount=shape*filterEnvelopeDepth;
  const auto lpScale=std::pow(2.0,amount*2.0);
  const auto hpScale=std::pow(2.0,amount);
  const auto lp=std::clamp(lpBaseCutoff*lpScale,80.0,18000.0);
  const auto hp=std::clamp(hpBaseCutoff*hpScale,20.0,6000.0);
  highPassFilter.configure(true,hp,hpResonance,filterSampleRate,hp<=20.0);
  lowPassFilter.configure(false,lp,lpResonance,filterSampleRate,lp>=18000.0);
  filterEnvelopeCounter=15;
 }else --filterEnvelopeCounter;
 ++filterEnvelopeAgeSamples;
 if(filterEnvelopeAgeSamples>=filterEnvelopeAttackSamples+filterEnvelopeDecaySamples){filterEnvelopeValue=0;highPassFilter.configure(true,hpBaseCutoff,hpResonance,filterSampleRate,hpBaseCutoff<=20.0);lowPassFilter.configure(false,lpBaseCutoff,lpResonance,filterSampleRate,lpBaseCutoff>=18000.0);}
}out.left=lowPassFilter.process(highPassFilter.process(out.left,false),false);out.right=lowPassFilter.process(highPassFilter.process(out.right,true),true);if(!active)active=highPassFilter.hasTail()||lowPassFilter.hasTail();if(chokeRemaining>0){const auto gain=double(chokeRemaining)/double(chokeLength);out.left*=gain;out.right*=gain;if(--chokeRemaining==0)reset();}return out;}
}
