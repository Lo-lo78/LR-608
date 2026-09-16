// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "ClapRimVoices.h"
#include <array>
#include <cstdint>

namespace lr608 {
// Per voice: level, pan, click, tune, pitch decay/amount, decay, wave,
// saturation, noise level/tone/decay, then the six compressor controls.
struct TomParameters { std::array<double,18> v{}; double accentThreshold=112,accentCharacter=1; };
class TomVoice {
public:
    explicit TomVoice(int whichTom=2):which(whichTom){}
    void prepare(double); void reset(); void trigger(int,int,const TomParameters&,std::uint32_t randomSeed=0);
    VoiceStereo render(const TomParameters&); bool isActive()const{return active&&(engine<2||state>2e-5||state2>2e-5||state3>2e-5||clickEnv>2e-5);}
private:
    struct Svf { double i1=0,i2=0,k=1,a1=0,a2=0,a3=0; void init(double,double,double); double lp(double); double bp(double); };
    double random(), render808(const TomParameters&), renderSimmons(const TomParameters&), renderSaike(const TomParameters&);
    VoiceStereo compressAndPan(double,const TomParameters&);
    void resetSaike(const TomParameters&);
    int which=2,engine=0,noiseCount=0; bool active=false; std::uint32_t rng=0x60870u;
    double sr=44100,velocity=1,accent=0,env=0,noiseEnv=0,clickEnv=0,pitchEnv=0,phase=0,phase3=0,clickPhase=0;
    double noiseSmooth=0,noiseLp=0,satMem=0,bodyLp=0,noiseHold=0,nlp1=0,nlp2=0,nlp3=0,nlp4=0,noiseDc=0,lfoPhase=0;
    double state=0,state2=0,state3=0,atk=0,k=0,k2=0,k3=0,kAtk=0,pitchK=0,dt=0,dt2=0,dt3=0,lowScale=1,gain=1;
    double clickK=0,clickDt=0,waveMorph=0,driveMix=0,noiseGain=0;
    double panLeft=1,panRight=1,compMix=0,compRmsK=0,compThreshold=1,compAttackK=0,compReleaseK=0,compRatioReduction=0,compMakeup=1;
    double voicePitchK=0,voiceEnvK=0,voiceNoiseK=0,voiceClickK=0,voiceBodyLpK=0,voiceBase=0,voiceEnd=0,voiceStart=0,voicePitchStep=0;
    Svf bp1,bp2,lp1,lp2; double rms=0,runningDb=0;
};
}
