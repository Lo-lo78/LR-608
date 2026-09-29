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
float lfoRandomSigned(std::uint32_t& state) noexcept
{
    state^=state<<13;state^=state>>17;state^=state<<5;
    return float(double(state)/double(0xffffffffu)*2.0-1.0);
}
}
void PolyphonicVoice::Biquad::reset(){b0=1;b1=b2=a1=a2=z1L=z2L=z1R=z2R=0;bypass=true;}
void PolyphonicVoice::Biquad::configure(bool highPass,double cutoff,double resonance,double sampleRate,bool neutral){if(neutral){bypass=true;b0=1;b1=b2=a1=a2=0;return;}bypass=false;const auto w=2*3.14159265358979323846*std::clamp(cutoff,20.0,sampleRate*.45)/sampleRate,c=std::cos(w),s=std::sin(w),alpha=s/(2*std::clamp(resonance,.5,10.0)),a0=1+alpha;if(highPass){b0=(1+c)*.5/a0;b1=-(1+c)/a0;b2=b0;}else{b0=(1-c)*.5/a0;b1=(1-c)/a0;b2=b0;}a1=-2*c/a0;a2=(1-alpha)/a0;}
double PolyphonicVoice::Biquad::process(double input,bool right){if(bypass)return input;auto&z1=right?z1R:z1L;auto&z2=right?z2R:z2L;const auto output=b0*input+z1;z1=b1*input-a1*output+z2;z2=b2*input-a2*output;return output;}
bool PolyphonicVoice::Biquad::hasTail()const{return !bypass&&(std::abs(z1L)+std::abs(z2L)+std::abs(z1R)+std::abs(z2R)>1.0e-9);}
void PolyphonicVoice::prepare(double sr){filterSampleRate=std::max(1.0,sr);kick808.prepare(sr);kickOther.prepare(sr);snare.prepare(sr);clap.prepare(sr);rim.prepare(sr);low.prepare(sr);mid.prepare(sr);high.prepare(sr);hat.prepare(sr);crash.prepare(sr);ride.prepare(sr);maracas.prepare(sr);cowbell.prepare(sr);zap.prepare(sr);chokeLength=std::max(1,int(std::lround(sr*.002)));active=false;highPassFilter.reset();lowPassFilter.reset();filterEnvelopeValue=0;filterEnvelopeAgeSamples=0;filterEnvelopeDecaySamples=0;filterEnvelopeAttackSamples=std::max<std::int64_t>(1,std::llround(filterSampleRate*.004));filterEnvelopeCounter=0;lfoDestinationMask=0;sourceActive=false;lfoOneShot=false;lfoOneShotDone=false;lfoHasEvaluated=false;lfoPhase=0;lfoIncrement=0;lfoOneShotPosition=0;lfoDelayEnvelope=0;lfoRateEnvelope=1;lfoPreviousPhase=1.0f;lfoSmoothState=lfoSampleHold=lfoCurrent=0.0f;}
void PolyphonicVoice::reset(){kick808.reset();kickOther.reset();snare.reset();clap.reset();rim.reset();low.reset();mid.reset();high.reset();hat.reset();crash.reset();ride.reset();maracas.reset();cowbell.reset();zap.reset();highPassFilter.reset();lowPassFilter.reset();active=false;chokeRemaining=0;sourceMidiNote=-1;sourceSlot=-1;degradeAmount=0;degradeQScale=128.0;degradeHoldL=degradeHoldR=0;degradeJitter=0;degradeHoldSamples=1;degradeCount=0;degradeRng=0x608d4a7u;filterEnvelopeDepth=0;filterEnvelopeValue=0;filterEnvelopeAgeSamples=0;filterEnvelopeDecaySamples=0;filterEnvelopeAttackSamples=std::max<std::int64_t>(1,std::llround(filterSampleRate*.004));filterEnvelopeCounter=0;lfoDestinationMask=0;sourceActive=false;lfoOneShot=false;lfoOneShotDone=false;lfoHasEvaluated=false;lfoPhase=0;lfoIncrement=0;lfoOneShotPosition=0;lfoDelayEnvelope=0;lfoRateEnvelope=1;lfoPreviousPhase=1.0f;lfoSmoothState=lfoSampleHold=lfoCurrent=0.0f;lfoVolumeDepth=lfoLowPassDepth=lfoHighPassDepth=lfoPanDepth=0;}
void PolyphonicVoice::start(int engine,int velocity,int output,int midiNote,int sourceSlotIndex,const std::array<std::atomic<float>,slotParameterValueCount>&v,double bpm,std::uint64_t age,bool pingPongRight){engine=std::clamp(engine,0,slotEngineCount-1);if(isOffEngine(engine)){reset();return;}const auto&info=slotEngines[engine];family=info.family;sub=info.subEngine;route=std::clamp(output,0,OutputStage::stemCount-1);sourceMidiNote=std::clamp(midiNote,0,127);sourceSlot=std::clamp(sourceSlotIndex,0,slotCount-1);chokeRemaining=0;voiceAge=age;tempo=bpm;const auto randomSeed=voiceRandomSeed(age,sourceSlot);const auto s=[&](int n){return double(v[n-1].load(std::memory_order_relaxed));};const auto at=s(250),ac=s(251);const auto pingPongDepth=std::clamp(double(v[slotPingPongPanDepthParameterIndex].load(std::memory_order_relaxed)),-1.0,1.0);const auto pingPongPan=[](double base,double depth,bool right){base=std::clamp(base,-1.0,1.0);const auto magnitude=std::abs(depth);if(magnitude<=1.0e-12)return base;const auto excursion=(1.0-std::abs(base))*magnitude;const auto effectiveRight=depth<0.0?!right:right;return std::clamp(base+(effectiveRight?excursion:-excursion),-1.0,1.0);};pan=pingPongPan(s(271),pingPongDepth,pingPongRight);lpBaseCutoff=v[slotLowPassCutoffParameterIndex].load(std::memory_order_relaxed);lpResonance=v[slotLowPassResonanceParameterIndex].load(std::memory_order_relaxed);hpBaseCutoff=v[slotHighPassCutoffParameterIndex].load(std::memory_order_relaxed);hpResonance=v[slotHighPassResonanceParameterIndex].load(std::memory_order_relaxed);
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
 // Read only the four destinations first. If they are all zero, the rest of the
 // LFO parameter block is deliberately untouched: a disabled LFO has zero DSP cost.
 lfoVolumeDepth=std::clamp(double(v[slotLfoVolumeDepthParameterIndex].load(std::memory_order_relaxed)),-1.0,1.0);
 lfoLowPassDepth=std::clamp(double(v[slotLfoLowPassDepthParameterIndex].load(std::memory_order_relaxed)),-8.0,8.0);
 lfoHighPassDepth=std::clamp(double(v[slotLfoHighPassDepthParameterIndex].load(std::memory_order_relaxed)),-8.0,8.0);
 lfoPanDepth=std::clamp(double(v[slotLfoPanDepthParameterIndex].load(std::memory_order_relaxed)),-1.0,1.0);
 lfoDestinationMask=0;
 if(std::abs(lfoVolumeDepth)>1.0e-12)lfoDestinationMask|=1;
 if(std::abs(lfoLowPassDepth)>1.0e-12)lfoDestinationMask|=2;
 if(std::abs(lfoHighPassDepth)>1.0e-12)lfoDestinationMask|=4;
 if(std::abs(lfoPanDepth)>1.0e-12)lfoDestinationMask|=8;
 sourceActive=true;
 if(lfoDestinationMask!=0)
 {
  lfoRate=std::clamp(double(v[slotLfoRateParameterIndex].load(std::memory_order_relaxed)),0.125,128.0);
  lfoWave=std::clamp(int(std::lround(v[slotLfoWaveParameterIndex].load(std::memory_order_relaxed))),0,5);
  lfoSmooth=std::clamp(double(v[slotLfoSmoothParameterIndex].load(std::memory_order_relaxed)),0.0,100.0);
  lfoOneShot=v[slotLfoOneShotParameterIndex].load(std::memory_order_relaxed)>=0.5f;
  lfoOneShotPercent=std::clamp(double(v[slotLfoOneShotPercentParameterIndex].load(std::memory_order_relaxed)),0.0,100.0);
  lfoPhaseOffset=std::clamp(double(v[slotLfoPhaseParameterIndex].load(std::memory_order_relaxed)),0.0,1.0);
  lfoEnvRate=std::clamp(double(v[slotLfoEnvRateParameterIndex].load(std::memory_order_relaxed)),-10.0,10.0);
  lfoDelay=std::clamp(double(v[slotLfoDelayParameterIndex].load(std::memory_order_relaxed)),0.0,20.0);
  lfoSquarePwm=std::clamp(double(v[slotLfoSquarePwmParameterIndex].load(std::memory_order_relaxed)),0.05,0.95);
  lfoUpperSquash=std::clamp(double(v[slotLfoUpperSquashParameterIndex].load(std::memory_order_relaxed)),0.0,1.0);
  lfoLowerSquash=std::clamp(double(v[slotLfoLowerSquashParameterIndex].load(std::memory_order_relaxed)),0.0,1.0);
  lfoPhase=0.0;lfoOneShotPosition=0.0;lfoOneShotDone=false;lfoHasEvaluated=false;lfoDelayEnvelope=0.0;lfoRateEnvelope=1.0;lfoPreviousPhase=1.0f;lfoSmoothState=0.0f;lfoCurrent=0.0f;lfoRng=randomSeed^0x1f012bb5u;lfoSampleHold=lfoRandomSigned(lfoRng);
  lfoIncrement=std::max(0.0,lfoRate*std::max(1.0,bpm)/240.0/filterSampleRate);
  lfoRateEnvelopeDecay=lfoEnvRate!=0.0?std::exp(-1.0/filterSampleRate):1.0;
  lfoDelayStep=lfoDelay>1.0e-12?1.0/std::max(1.0,lfoDelay*filterSampleRate):1.0;
  const auto smoothSquared=lfoSmooth*lfoSmooth;
  lfoSmoothCoefficient=smoothSquared>0.0?1.0/(1.0+smoothSquared*(lfoWave==5?2000.0:400.0)):0.0;
  lfoSquashPositive=1.0-lfoUpperSquash;lfoSquashNegative=1.0-lfoLowerSquash;
 }
 else
 {
  lfoIncrement=0.0;lfoCurrent=0.0f;
 }
 if(family==SlotFamily::kick){kp={s(11),s(12),s(13),s(14),s(15),s(16),s(17),s(18),s(67),s(78),s(79),s(100),s(101),s(102),s(103),int(std::lround(s(104))),s(105),s(106),s(107),s(138),s(140),s(141),s(142),s(148),s(149),s(150),s(152),s(153),s(154),s(155),int(std::lround(s(151))),double(v[kickNoiseResonanceParameterIndex].load(std::memory_order_relaxed)),double(v[kickNoiseGranulationParameterIndex].load(std::memory_order_relaxed)),double(v[kickClickBodyCouplingParameterIndex].load(std::memory_order_relaxed)),at,ac,s(256)};if(sub==0){kickOther.reset();kick808.trigger(velocity,kp,randomSeed);}else{kick808.reset();kickOther.trigger(sub,velocity,kp,randomSeed);}}
 else if(family==SlotFamily::snare1||family==SlotFamily::snare2){const int a[][31]={{20,21,22,23,24,29,116,117,26,25,27,28,19,92,94,96,98,108,139,132,133,134,135,136,137,156,157,158,159,160,161},{221,222,223,224,225,230,232,233,227,226,228,229,220,93,95,97,99,231,240,234,235,236,237,238,239,241,242,243,244,245,246}};const auto slot=family==SlotFamily::snare1?0:1;for(int i=0;i<31;++i)sp.v[i]=s(a[slot][i]);const int x[][6]={{snare1BodyMidTuneParameterIndex,snare1BodyHighTuneParameterIndex,snare1NoiseGranulationParameterIndex,snare1BodyMidDecayParameterIndex,snare1BodyHighDecayParameterIndex,snare1ClickBodyCouplingParameterIndex},{snare2BodyMidTuneParameterIndex,snare2BodyHighTuneParameterIndex,snare2NoiseGranulationParameterIndex,snare2BodyMidDecayParameterIndex,snare2BodyHighDecayParameterIndex,snare2ClickBodyCouplingParameterIndex}};for(int i=0;i<6;++i)sp.v[31+i]=double(v[x[slot][i]].load(std::memory_order_relaxed));sp.v[37]=family==SlotFamily::snare1?double(v[snare1SaikeBodyAmpDecayParameterIndex].load(std::memory_order_relaxed)):10.0;sp.accentThreshold=at;sp.accentCharacter=ac;snare.trigger(sub,velocity,sp,randomSeed);}
 else if(family==SlotFamily::clap){const int a[]{30,31,32,33,34,208,35,36,37,38,39};for(int i=0;i<11;++i)cp.v[i]=s(a[i]);cp.accentThreshold=at;cp.accentCharacter=ac;clap.trigger(sub,velocity,cp,randomSeed);}
 else if(family==SlotFamily::rim){const int a[]{71,70,69,68,124,125,143,144,145,146,147,252,162,163,164,165,166,167};for(int i=0;i<18;++i)rp.v[i]=s(a[i]);rp.accentThreshold=at;rp.accentCharacter=ac;rp.tempo=bpm;rim.trigger(sub,velocity,rp,randomSeed);}
 else if(family==SlotFamily::lowTom||family==SlotFamily::midTom||family==SlotFamily::highTom){const int a[][18]={{64,40,41,42,43,44,45,109,118,46,47,121,174,175,176,177,178,179},{65,48,49,50,51,52,53,110,119,54,55,122,180,181,182,183,184,185},{66,56,57,58,59,60,61,111,120,62,63,123,186,187,188,189,190,191}};const auto t=family==SlotFamily::lowTom?0:family==SlotFamily::midTom?1:2;for(int i=0;i<18;++i)tp.v[i]=s(a[t][i]);tp.v[1]=pingPongPan(tp.v[1],pingPongDepth,pingPongRight);pan=tp.v[1];const int x[][4]={{lowTomNoiseResonanceParameterIndex,lowTomNoiseGranulationParameterIndex,lowTomClickBodyCouplingParameterIndex,lowTomPitchDecayCurveParameterIndex},{midTomNoiseResonanceParameterIndex,midTomNoiseGranulationParameterIndex,midTomClickBodyCouplingParameterIndex,midTomPitchDecayCurveParameterIndex},{highTomNoiseResonanceParameterIndex,highTomNoiseGranulationParameterIndex,highTomClickBodyCouplingParameterIndex,highTomPitchDecayCurveParameterIndex}};for(int i=0;i<4;++i)tp.v[18+i]=double(v[x[t][i]].load(std::memory_order_relaxed));tp.accentThreshold=at;tp.accentCharacter=ac;if(t==0)low.trigger(sub,velocity,tp,randomSeed);else if(t==1)mid.trigger(sub,velocity,tp,randomSeed);else high.trigger(sub,velocity,tp,randomSeed);}
 else if(family==SlotFamily::hatClosed||family==SlotFamily::hatOpen){const int a[]{5,1,2,3,4,114,218,115,90,91};for(int i=0;i<10;++i)hp.v[i]=s(a[i]);hp.v[10]=double(v[hiHatNoiseGranulationParameterIndex].load(std::memory_order_relaxed));hat.trigger(sub,family==SlotFamily::hatOpen,velocity,hp,randomSeed);}
 else if(family==SlotFamily::crash||family==SlotFamily::ride){const int a[]{9,10,6,7,8,112,113};for(int i=0;i<7;++i)yp.v[i]=s(a[i]);yp.v[7]=double(v[crashNoiseGranulationParameterIndex].load(std::memory_order_relaxed));yp.v[8]=double(v[rideNoiseGranulationParameterIndex].load(std::memory_order_relaxed));if(family==SlotFamily::crash)crash.trigger(sub,velocity,yp,randomSeed);else ride.trigger(sub,velocity,yp,randomSeed);}
 else if(family==SlotFamily::maracas){const int a[]{168,169,170,171,172,173,205,206};for(int i=0;i<8;++i)mp.v[i]=s(a[i]);mp.accentThreshold=at;mp.accentCharacter=ac;maracas.trigger(sub,velocity,mp,randomSeed);}
 else if(family==SlotFamily::cowbell){const int a[]{80,81,82,83,84,85,86,87,88,89,204};for(int i=0;i<11;++i)wp.v[i]=s(a[i]);for(int i=0;i<57;++i)wp.captured[i]=double(v[291+i].load(std::memory_order_relaxed));wp.accentThreshold=at;wp.accentCharacter=ac;cowbell.trigger(sub,velocity,wp,randomSeed,midiNote);}
 else {const int a[]{72,73,74,75,76,77,126,127,128,129,130,131,210,211,212,213,192,193,194,195,196,197};for(int i=0;i<22;++i)zp.v[i]=s(a[i]);zp.tempo=bpm;zap.trigger(sub,velocity,zp,randomSeed);}active=true;}
void PolyphonicVoice::choke(){if(active&&chokeRemaining==0)chokeRemaining=chokeLength;}
StereoSample PolyphonicVoice::render()
{
    if(!active)return{};

    // LFO fast path: zero destination mask means zero LFO work.  Filter-only
    // modulation is evaluated only when filter coefficients are refreshed (1/16 rate).
    const bool lfoConfigured=lfoDestinationMask!=0;
    const bool lfoSourceThisSample=lfoConfigured&&sourceActive;
    const bool lfoAudioRate=lfoSourceThisSample&&((lfoDestinationMask&0x09)!=0); // volume or pan
    double lfo=double(lfoCurrent);
    const auto evaluateLfo=[this](int elapsedSamples)->double
    {
        if(elapsedSamples<=0)return double(lfoCurrent);
        const auto previous=lfoPhase;
        if(lfoEnvRate!=0.0)
        {
            const auto midpointEnvelope=elapsedSamples==1?lfoRateEnvelope:lfoRateEnvelope*std::pow(lfoRateEnvelopeDecay,0.5*double(elapsedSamples-1));
            const auto rateMultiplier=std::exp2(midpointEnvelope*lfoEnvRate);
            auto advance=lfoIncrement*double(elapsedSamples)*rateMultiplier;
            const auto oneShot=lfoOneShot&&lfoOneShotPercent>0.0;
            if(!(oneShot&&lfoOneShotDone))
            {
                if(oneShot)
                {
                    const auto limit=std::clamp(lfoOneShotPercent*.01,0.0,1.0);
                    advance=std::min(advance,std::max(0.0,limit-lfoOneShotPosition));
                    lfoOneShotPosition+=advance;if(lfoOneShotPosition>=limit)lfoOneShotDone=true;
                }
                lfoPhase+=advance;lfoPhase-=std::floor(lfoPhase);
            }
            lfoRateEnvelope*=elapsedSamples==1?lfoRateEnvelopeDecay:std::pow(lfoRateEnvelopeDecay,double(elapsedSamples));
        }
        else
        {
            auto advance=lfoIncrement*double(elapsedSamples);
            const auto oneShot=lfoOneShot&&lfoOneShotPercent>0.0;
            if(!(oneShot&&lfoOneShotDone))
            {
                if(oneShot)
                {
                    const auto limit=std::clamp(lfoOneShotPercent*.01,0.0,1.0);
                    advance=std::min(advance,std::max(0.0,limit-lfoOneShotPosition));
                    lfoOneShotPosition+=advance;if(lfoOneShotPosition>=limit)lfoOneShotDone=true;
                }
                lfoPhase+=advance;lfoPhase-=std::floor(lfoPhase);
            }
        }
        auto phase=lfoPhase+lfoPhaseOffset;phase-=std::floor(phase);
        float raw=0.0f;
        switch(lfoWave)
        {
            case 0:raw=float(std::sin(2.0*3.14159265358979323846*phase));break;
            case 1:raw=phase<0.5?float(phase*4.0-1.0):float(3.0-phase*4.0);break;
            case 2:raw=float(phase*2.0-1.0);break;
            case 3:raw=float(1.0-phase*2.0);break;
            case 4:raw=phase<lfoSquarePwm?1.0f:-1.0f;break;
            default:
                if(phase<lfoPreviousPhase||(lfoPhase<previous))lfoSampleHold=lfoRandomSigned(lfoRng);
                raw=lfoSampleHold;break;
        }
        if(lfoSmoothCoefficient>0.0)
        {
            const auto effective=elapsedSamples==1?lfoSmoothCoefficient:1.0-std::pow(1.0-lfoSmoothCoefficient,double(elapsedSamples));
            lfoSmoothState+=float((double(raw)-double(lfoSmoothState))*effective);lfoCurrent=lfoSmoothState;
        }
        else {lfoSmoothState=raw;lfoCurrent=raw;}
        lfoPreviousPhase=float(phase);
        if(lfoDelay>1.0e-12)
        {
            const auto effective=elapsedSamples==1?lfoDelayStep:1.0-std::pow(1.0-lfoDelayStep,double(elapsedSamples));
            lfoDelayEnvelope+=(1.0-lfoDelayEnvelope)*effective;lfoCurrent*=float(lfoDelayEnvelope);
        }
        else lfoDelayEnvelope=1.0;
        lfoCurrent*=lfoCurrent>=0.0f?float(lfoSquashPositive):float(lfoSquashNegative);
        lfoHasEvaluated=true;return double(lfoCurrent);
    };
    if(lfoAudioRate)lfo=evaluateLfo(1);

    const bool tomFamily=family==SlotFamily::lowTom||family==SlotFamily::midTom||family==SlotFamily::highTom;
    if(tomFamily)
        tp.v[1]=std::clamp(pan+(lfoAudioRate?lfo*lfoPanDepth:0.0),-1.0,1.0);

    StereoSample out;
    switch(family)
    {
        case SlotFamily::kick:{auto x=kick808.render(kp,tempo)+kickOther.render(kp,tempo);out={x,x};sourceActive=kick808.isActive()||kickOther.isActive();break;}
        case SlotFamily::snare1:case SlotFamily::snare2:{auto x=snare.render(sp);out={x,x};sourceActive=snare.isActive();break;}
        case SlotFamily::clap:{auto x=clap.render(cp);out={x.left,x.right};sourceActive=clap.isActive();break;}
        case SlotFamily::rim:{auto x=rim.render(rp);out={x.left,x.right};sourceActive=rim.isActive();break;}
        case SlotFamily::lowTom:{auto x=low.render(tp);out={x.left,x.right};sourceActive=low.isActive();break;}
        case SlotFamily::midTom:{auto x=mid.render(tp);out={x.left,x.right};sourceActive=mid.isActive();break;}
        case SlotFamily::highTom:{auto x=high.render(tp);out={x.left,x.right};sourceActive=high.isActive();break;}
        case SlotFamily::hatClosed:case SlotFamily::hatOpen:{auto x=hat.render(hp);out={x,x};sourceActive=hat.isActive();break;}
        case SlotFamily::crash:{auto x=crash.render(yp);out={x,x};sourceActive=crash.isActive();break;}
        case SlotFamily::ride:{auto x=ride.render(yp);out={x,x};sourceActive=ride.isActive();break;}
        case SlotFamily::maracas:{auto x=maracas.render(mp);out={x,x};sourceActive=maracas.isActive();break;}
        case SlotFamily::cowbell:{auto x=cowbell.render(wp);out={x,x};sourceActive=cowbell.isActive();break;}
        case SlotFamily::zap:{auto x=zap.render(zp);static constexpr double g[]{1,5.011872336,1,1.995262315,5.623413252,4.466835922,3.981071706,4.466835922,15.848931925,5.011872336,5.623413252};x*=g[std::clamp(sub,0,10)];out={x,x};sourceActive=zap.isActive();break;}
    }
    active=sourceActive;

    if(degradeAmount>1.0e-9)
    {
        if(degradeCount<=0)
        {
            degradeHoldL=out.left;degradeHoldR=out.right;
            degradeRng^=degradeRng<<13;degradeRng^=degradeRng>>17;degradeRng^=degradeRng<<5;
            const auto jitterSpan=int(std::floor(degradeHoldSamples*degradeJitter));
            const auto jitterAdd=jitterSpan>0?int(degradeRng%std::uint32_t(jitterSpan*2+1))-jitterSpan:0;
            degradeCount=std::max(1,degradeHoldSamples+jitterAdd);
        }
        --degradeCount;
        const auto quantize=[this](double x){const auto offset=x>=0?0.5:-0.5;return std::floor(x*degradeQScale+offset)/degradeQScale;};
        const auto crushedL=quantize(degradeHoldL),crushedR=quantize(degradeHoldR);
        out.left=out.left*(1.0-degradeAmount)+crushedL*degradeAmount;
        out.right=out.right*(1.0-degradeAmount)+crushedR*degradeAmount;
    }

    if(lfoSourceThisSample&&(lfoDestinationMask&0x01)!=0)
    {
        const auto gain=std::max(0.0,1.0+lfo*lfoVolumeDepth);
        out.left*=gain;out.right*=gain;
    }

    if(!tomFamily)
    {
        const auto effectivePan=std::clamp(pan+(lfoAudioRate?lfo*lfoPanDepth:0.0),-1.0,1.0);
        if(std::abs(effectivePan)>1.0e-12)
        {
            constexpr double q=.78539816339744830962,r=1.4142135623730950488;
            const auto angle=(effectivePan+1.0)*q;
            out.left*=r*std::cos(angle);out.right*=r*std::sin(angle);
        }
    }

    const bool lfoFilterRunning=lfoSourceThisSample&&((lfoDestinationMask&0x06)!=0);
    if(filterEnvelopeValue>1.0e-6||lfoFilterRunning)
    {
        if(filterEnvelopeCounter<=0)
        {
            if(lfoFilterRunning&&!lfoAudioRate)lfo=evaluateLfo(lfoHasEvaluated?16:1);
            double envAmount=0.0;
            if(filterEnvelopeValue>1.0e-6)
            {
                double shape=0.0;
                if(filterEnvelopeAgeSamples<filterEnvelopeAttackSamples)
                {
                    const auto x=std::clamp(double(filterEnvelopeAgeSamples)/double(filterEnvelopeAttackSamples),0.0,1.0);
                    shape=x*x*(3.0-2.0*x);
                }
                else
                {
                    const auto decayAge=filterEnvelopeAgeSamples-filterEnvelopeAttackSamples;
                    const auto x=std::clamp(double(decayAge)/double(std::max<std::int64_t>(1,filterEnvelopeDecaySamples)),0.0,1.0);
                    shape=1.0-x*x*(3.0-2.0*x);
                }
                envAmount=shape*filterEnvelopeDepth;
            }
            const auto lpOctaves=envAmount*2.0+(lfoFilterRunning?lfo*lfoLowPassDepth:0.0);
            const auto hpOctaves=envAmount+(lfoFilterRunning?lfo*lfoHighPassDepth:0.0);
            const auto lp=std::clamp(lpBaseCutoff*std::pow(2.0,lpOctaves),80.0,18000.0);
            const auto hpCut=std::clamp(hpBaseCutoff*std::pow(2.0,hpOctaves),20.0,6000.0);
            highPassFilter.configure(true,hpCut,hpResonance,filterSampleRate,hpCut<=20.0);
            lowPassFilter.configure(false,lp,lpResonance,filterSampleRate,lp>=18000.0);
            filterEnvelopeCounter=15;
        }
        else --filterEnvelopeCounter;

        if(filterEnvelopeValue>1.0e-6)
        {
            ++filterEnvelopeAgeSamples;
            if(filterEnvelopeAgeSamples>=filterEnvelopeAttackSamples+filterEnvelopeDecaySamples)
            {
                filterEnvelopeValue=0;
                filterEnvelopeCounter=0;
                if(!lfoFilterRunning)
                {
                    highPassFilter.configure(true,hpBaseCutoff,hpResonance,filterSampleRate,hpBaseCutoff<=20.0);
                    lowPassFilter.configure(false,lpBaseCutoff,lpResonance,filterSampleRate,lpBaseCutoff>=18000.0);
                }
            }
        }
    }

    out.left=lowPassFilter.process(highPassFilter.process(out.left,false),false);
    out.right=lowPassFilter.process(highPassFilter.process(out.right,true),true);

    // Once the source engine has finished, the LFO disappears completely.
    // Filter tails may continue, but with static coefficients only.
    if(!sourceActive&&lfoDestinationMask!=0)
    {
        const auto hadFilterLfo=std::abs(lfoLowPassDepth)>1.0e-12||std::abs(lfoHighPassDepth)>1.0e-12;
        lfoDestinationMask=0;lfoCurrent=0.0f;
        if(hadFilterLfo)
        {
            filterEnvelopeCounter=0;
            if(filterEnvelopeValue<=1.0e-6)
            {
                highPassFilter.configure(true,hpBaseCutoff,hpResonance,filterSampleRate,hpBaseCutoff<=20.0);
                lowPassFilter.configure(false,lpBaseCutoff,lpResonance,filterSampleRate,lpBaseCutoff>=18000.0);
            }
        }
    }

    if(!active)active=highPassFilter.hasTail()||lowPassFilter.hasTail();
    if(chokeRemaining>0)
    {
        const auto gain=double(chokeRemaining)/double(chokeLength);
        out.left*=gain;out.right*=gain;
        if(--chokeRemaining==0)reset();
    }
    return out;
}
}
