// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SnareVoice.h"

#include <algorithm>
#include <cmath>

namespace lr608
{
namespace { constexpr double pi = 3.14159265358979323846; }

void SnareVoice::Svf::init (double frequency, double invq, double sampleRate)
{
    frequency=std::clamp(frequency,20.0,sampleRate*.45); const auto g=std::tan(pi*frequency/sampleRate);
    k=std::max(.015,invq); a1=1/(1+g*(g+k)); a2=g*a1; a3=g*a2; ic1=ic2=0;
}
double SnareVoice::Svf::bp(double x){const auto v3=x-ic2,v1=a1*ic1+a2*v3,v2=ic2+a2*ic1+a3*v3;ic1=2*v1-ic1;ic2=2*v2-ic2;return v1;}
double SnareVoice::Svf::lp(double x){const auto v3=x-ic2,v1=a1*ic1+a2*v3,v2=ic2+a2*ic1+a3*v3;ic1=2*v1-ic1;ic2=2*v2-ic2;return v2;}
double SnareVoice::Svf::hp(double x){const auto v3=x-ic2,v1=a1*ic1+a2*v3,v2=ic2+a2*ic1+a3*v3;ic1=2*v1-ic1;ic2=2*v2-ic2;return x-k*v1-v2;}
double SnareVoice::Envelope::tick(){if(t<attackSamples){val+=rise*(1-val);t+=1;}else val-=decay*val;if(val<1e-7)val=0;return val;}

void SnareVoice::prepare (double sampleRate) { sr = std::max (1.0, sampleRate); reset(); }

void SnareVoice::reset()
{
    active = false; bodyEnv = noiseEnv = clickEnv = pitchEnv = attack = accent = 0.0;
    phase1 = phase2 = phase3 = ringPhase = ringSmooth = 0.0;
    noiseLp = noiseBp = noiseHpMemory = colourMemory = dcX = dcY = rms = runningDb = 0.0;
    simLp1 = simLp2 = simLp3 = simLp4 = simNoiseDc = 0.0;
    modPhase = pinkState = brownState = nx1 = nx2 = ny1 = ny2 = 0.0;
    ringEnv = ringPrevious = ringSampleHold = 0.0; noiseAttackStage = false;
    linnPhase=linnBodyEnv=linnBodyAttack=linnPitchEnv=linnCrackEnv=linnImpactEnv=0;
    linnWireEnv=linnWireAge=linnDacPhase=linnDacHold=linnAge=0;
    linnRingPhase=linnRingHold=linnRingSmoothed=0;
    holdCounter = 0; heldNoise = 0.0; rng = 0x6085a11u;
}

double SnareVoice::randomBipolar()
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (rng / double (0xffffffffu)) * 2.0 - 1.0;
}

void SnareVoice::trigger (int engine, int midiVelocity, const SnareParameters& p, std::uint32_t randomSeed)
{
    if (randomSeed != 0) rng = randomSeed;
    activeEngine = std::clamp (engine, 0, 8);
    velocity = std::clamp (midiVelocity / 127.0, 0.0, 1.0);
    const auto threshold = p.accentThreshold / 127.0;
    accent = velocity > threshold
        ? std::pow ((velocity-threshold) / std::max (.001, 1.0-threshold), 1.6) * p.accentCharacter : 0.0;
    bodyEnv = clickEnv = pitchEnv = 1.0;
    noiseEnv = p.v[13] > 0.0 ? 0.0 : 1.0;
    noiseAttackStage = p.v[13] > 0.0;
    attack = 0.0; phase1 = phase2 = phase3 = 0.0;
    ringPhase = ringEnv = ringPrevious = ringSampleHold = ringSmooth = 0.0;
    holdCounter = 0; heldNoise = randomBipolar();
    // Each trigger starts a voice using only the selected engine's bank.
    // Filter memories are deliberately retained, as in the continuously running JSFX filters.
    active = p.v[0] > 0.0;
    constexpr double db2log = 0.11512925464970229;
    compMix = p.v[30] / 100.0;
    compRmsCoefficient = std::exp(-1.0/(.01*sr));
    compThresholdLinear = std::exp((p.v[25]-3.0)*db2log);
    compThresholdLog = std::log(compThresholdLinear);
    compAttackCoefficient = std::exp(-1.0/(std::max(.0001,p.v[28]*.001)*sr));
    compReleaseCoefficient = std::exp(-1.0/(std::max(.001,p.v[29]*.001)*sr));
    static constexpr double ratios[] {4,8,12,20,20};
    compRatioReduction = 1.0-1.0/ratios[std::clamp(int(std::round(p.v[26])),0,4)];
    compMakeupGain = std::exp(p.v[27]*db2log);
    if (activeEngine == 2) resetAcoustic (p);
    else if (activeEngine >= 3 && activeEngine <= 5) resetSaike (p);
    else if (activeEngine == 7) resetSaike (p);
    else if (activeEngine == 6) resetPlaits (p);
    else if (activeEngine == 8) resetLinn (p);
}

float SnareVoice::render (const SnareParameters& p)
{
    if (! active) return 0.0f;
    if (activeEngine == 0) return render808 (p);
    if (activeEngine == 1) return renderSimmons (p);
    if (activeEngine == 2) return renderAcoustic (p);
    if ((activeEngine >= 3 && activeEngine <= 5) || activeEngine == 7) return renderSaike (p);
    if (activeEngine == 6) return renderPlaits (p);
    if (activeEngine == 8) return renderLinn (p);
    const auto bodyDecay = std::max (0.003, p.v[2]);
    const auto noiseDecay = std::max (0.003, p.v[9]);
    bodyEnv *= std::exp (-1.0 / (bodyDecay * sr));
    noiseEnv *= std::exp (-1.0 / (noiseDecay * sr));
    clickEnv *= std::exp (-1.0 / (0.0025 * sr));
    pitchEnv *= std::exp (-1.0 / (std::max (0.003, p.v[5]) * sr));
    const auto attackSeconds = std::max (0.00002, p.v[13] * 0.04);
    attack += (1.0 - attack) * (1.0 - std::exp (-1.0 / (attackSeconds * sr)));

    const auto base = std::clamp (178.0 * std::pow (2.0, p.v[3]), 35.0, sr * 0.2);
    const auto sweep = std::pow (2.0, p.v[5] * 3.0 * pitchEnv);
    auto f1 = base * sweep, f2 = f1 * 1.47, f3 = f1 * 2.03;
    if (activeEngine == 1) { f1 *= 0.68; f2 = f1 * 1.996; f3 = f1 * 3.01; }
    else if (activeEngine == 2) { f1 *= 1.13; f2 = f1 * 1.59; f3 = f1 * 2.15; }
    else if (activeEngine >= 3 && activeEngine <= 5) { f1 *= 0.82 + .13 * (activeEngine - 3); f2 = f1 * 1.62; f3 = f1 * 2.32; }
    else if (activeEngine == 6) { f2 = f1 * (1.82 + p.v[6]); f3 = f1 * (2.7 + p.v[7]); }
    else if (activeEngine == 7) { f1 *= 1.22; f2 = f1 * 1.51; f3 = f1 * 2.09; }
    phase1 += f1 / sr; phase2 += f2 / sr; phase3 += f3 / sr;
    phase1 -= std::floor (phase1); phase2 -= std::floor (phase2); phase3 -= std::floor (phase3);

    double body;
    if (activeEngine == 1)
        body = (phase1 < 0.5 ? 1.0 : -1.0) * .55 + std::sin (2*pi*phase2) * .35;
    else if (activeEngine >= 3 && activeEngine <= 5)
    {
        const auto tri = 1.0 - 4.0 * std::abs (phase1 - .5);
        body = activeEngine == 3 ? std::tanh (2.2 * tri) : activeEngine == 4 ? tri : std::tanh (3.2 * tri) * .8;
        body += std::sin (2*pi*phase2) * (.18 + .14 * p.v[6]);
    }
    else if (activeEngine == 6)
        body = std::sin (2*pi*phase1 + std::sin (2*pi*phase2) * (1.0 + 5.0*p.v[6]))
             + .28 * std::sin (2*pi*phase3);
    else
        body = std::sin (2*pi*phase1) + .42 * std::sin (2*pi*phase2) + .18 * std::sin (2*pi*phase3);
    body *= std::pow (std::max (0.0, bodyEnv), .62) * p.v[4];

    auto white = randomBipolar();
    const auto degrade = std::clamp (p.v[15], 0.0, 1.0);
    if (--holdCounter <= 0) { heldNoise = white; holdCounter = 1 + int (degrade * degrade * 80.0); }
    white += (heldNoise - white) * degrade;
    const auto tone = std::clamp ((p.v[10] + 1.0) * .5, 0.0, 1.0);
    const auto cutoff = std::clamp (650.0 + std::pow (tone, 1.4) * 15500.0, 80.0, sr*.44);
    const auto a = (2*pi*cutoff) / (sr + 2*pi*cutoff);
    noiseLp += (white - noiseLp) * a;
    const auto hp = noiseLp - noiseHpMemory; noiseHpMemory += (noiseLp - noiseHpMemory) * .015;
    colourMemory += (hp - colourMemory) * (.01 + p.v[14] * .14);
    auto noise = (hp * (1.0-p.v[14]) + colourMemory*p.v[14]);
    noise *= noiseEnv * attack * p.v[8];

    ringPhase += std::clamp (p.v[19], 1.0, sr*.4) / sr; ringPhase -= std::floor (ringPhase);
    const auto ringWave = std::sin (2*pi*ringPhase);
    ringSmooth += (ringWave-ringSmooth) * (.002 + (1.0-std::clamp(p.v[22],0.0,1.0))*.2);
    noise *= 1.0 + ringSmooth * p.v[20] * noiseEnv;
    auto y = body + noise + clickEnv * p.v[1] * (white*.72 + std::sin (2*pi*phase3)*.28);
    y *= 1.0 + p.v[12] * noiseEnv * .08;

    const auto drive = 1.0 + std::pow (std::max (0.0, p.v[17]) * .001, 1.2) * 18.0;
    const auto driven = std::tanh (y * drive);
    y += (driven-y) * std::clamp (p.v[18], 0.0, 1.0);
    if (p.v[30] > .0001)
    {
        const auto db2log = std::log(10.0)/20.0;
        rms = y*y + std::exp(-1.0/(.01*sr))*(rms-y*y);
        const auto over = std::max(0.0, 20.0/std::log(10.0)*std::log(std::max(std::sqrt(std::max(0.0,rms)),1e-7)
                              / std::exp(p.v[25]*db2log)));
        const auto coeff = std::exp(-1.0/(std::max(.0001, (over>runningDb?p.v[28]:p.v[29])*.001)*sr));
        runningDb += (over-runningDb)*(1.0-coeff);
        static constexpr int ratios[] {4,8,12,20,40};
        const auto gain = std::exp((-runningDb*(1.0-1.0/ratios[std::clamp(int(std::round(p.v[26])),0,4)])+p.v[27])*db2log);
        y += (y*gain-y) * std::clamp(p.v[30]/100.0,0.0,1.0);
    }
    // Non-808 families are still behind the conservative compatibility gain
    // until their dedicated JSFX functions replace this provisional branch.
    y *= p.v[0] * velocity * .055;
    const auto previous = dcX; dcX = y; dcY = dcX-previous+std::exp(-2*pi*5.0/sr)*dcY;
    if (! std::isfinite (dcY)) { reset(); return 0.0f; }
    active = bodyEnv > 1e-6 || noiseEnv > 1e-6 || clickEnv > 1e-6;
    return static_cast<float> (active ? dcY : 0.0);
}

double SnareVoice::compress (double signal, const SnareParameters&)
{
    if (compMix <= .000001) { rms = runningDb = 0.0; return signal; }
    constexpr double db2log = 0.11512925464970229;
    constexpr double log2db = 8.6858896380650366;
    const auto square = signal * signal;
    rms = square + compRmsCoefficient * (rms-square);
    const auto overDb = std::max(0.0, log2db*(0.5*std::log(std::max(rms,1e-14))-compThresholdLog));
    runningDb += (overDb-runningDb) * (1.0-(overDb>runningDb?compAttackCoefficient:compReleaseCoefficient));
    const auto gain = std::exp(-runningDb*compRatioReduction*db2log) * compMakeupGain;
    return signal + (signal*gain-signal)*compMix;
}

float SnareVoice::render808 (const SnareParameters& p)
{
    const auto clickDecay = 1.0-std::exp(-1.0/(.0009*sr));
    clickEnv -= clickEnv*clickDecay;
    if (clickEnv < .00001) clickEnv = 0.0;
    const auto clickFrequency = 4200.0 + accent*1800.0;
    clickEnv = std::max(0.0, clickEnv);
    phase3 += clickFrequency/sr; phase3 -= std::floor(phase3);
    const auto click = std::sin(phase3*2*pi)*clickEnv*p.v[1];

    const auto bodyDecay = 1.0-std::exp(-1.0/(std::max(p.v[2],.001)*sr));
    bodyEnv -= bodyEnv*bodyDecay;
    if (bodyEnv < .00001) bodyEnv = 0.0;
    const auto endFrequency = 170.0+p.v[3]*230.0;
    const auto startFrequency = endFrequency*(1.0+p.v[5]*3.5);
    auto frequency = endFrequency+(startFrequency-endFrequency)*std::pow(bodyEnv,2.2+accent*4.6);
    frequency += randomBipolar()*3.0*bodyEnv;
    phase1 += frequency/sr; phase1 -= std::floor(phase1);
    auto body1 = std::sin(phase1*2*pi);
    phase2 += frequency*2.1/sr; phase2 -= std::floor(phase2);
    auto body2 = std::sin(phase2*2*pi)*p.v[6];
    ringPhase += frequency*4.8/sr; ringPhase -= std::floor(ringPhase);
    auto body3 = std::sin(ringPhase*2*pi)*p.v[7];
    const auto bodyBus = body1+body2*.6+body3*.35;
    const auto coupling = bodyBus*.12*bodyEnv;
    body1 += coupling*.6; body2 += coupling*.4; body3 += coupling*.25;
    auto bodySignal = bodyBus*bodyEnv*p.v[4];
    bodySignal *= 1.0-accent*.05;

    if (noiseAttackStage)
    {
        const auto coefficient = 1.0-std::exp(-1.0/((.0005+p.v[13]*.02)*sr));
        noiseEnv += (1.0-noiseEnv)*coefficient;
        if (noiseEnv >= .999) { noiseEnv=1.0; noiseAttackStage=false; }
    }
    else
    {
        const auto decay = 1.0-std::exp(-1.0/(std::max(p.v[9],.001)*sr));
        noiseEnv -= noiseEnv*decay;
        if (noiseEnv < .00001) noiseEnv=0.0;
    }
    modPhase += (110.0+(randomBipolar()+1.0)*.5*120.0)/sr; modPhase -= std::floor(modPhase);
    const auto whiteSource = randomBipolar()*std::sin(modPhase*2*pi);
    const auto degradeAmount = std::pow(p.v[15],2.6);
    const auto holdSamples = std::max(1,int(std::floor(1.0+degradeAmount*(sr*.012))));
    if (holdCounter <= 0) { heldNoise=whiteSource; holdCounter=holdSamples; }
    --holdCounter;
    const auto white = whiteSource*(1.0-p.v[15])+heldNoise*p.v[15];
    pinkState += (white-pinkState)*.075;
    const auto pink = pinkState*2.4;
    brownState += pink*.02; brownState *= .9992; brownState=std::clamp(brownState,-1.0,1.0);
    const auto brown = brownState*3.8;
    double coloured;
    if (p.v[14] <= .5) { const auto t=p.v[14]*2.0; coloured=white*(1.0-t)+pink*t; }
    else { const auto t=(p.v[14]-.5)*2.0; coloured=pink*(1.0-t)+brown*t; }
    auto noise = coloured + body1*bodyEnv*.35 + body2*bodyEnv*.25 + body3*bodyEnv*.18;

    const auto filterEnvelopeAmount = p.v[12]*p.v[16];
    const auto baseFrequency = 1200.0+p.v[10]*3800.0;
    const auto envelopeFrequency = baseFrequency*(1.0+noiseEnv*filterEnvelopeAmount*1.8);
    auto filterFrequency = baseFrequency*(1.0-filterEnvelopeAmount)+envelopeFrequency*filterEnvelopeAmount;
    filterFrequency=std::clamp(filterFrequency,1.0,sr*.49);
    const auto q=.7+p.v[11]*3.0, w0=2*pi*filterFrequency/sr;
    const auto alpha=std::sin(w0)/(2*q), a0=1+alpha;
    const auto b0=alpha/a0, b1=0.0, b2=-alpha/a0;
    const auto a1=-2*std::cos(w0)/a0, a2=(1-alpha)/a0;
    auto band = b0*noise+b1*nx1+b2*nx2-a1*ny1-a2*ny2;
    nx2=nx1; nx1=noise; ny2=ny1; ny1=band;

    if (std::abs(p.v[20]) > .000001)
    {
        if (p.v[23] > 0.0) ringEnv += (1.0-ringEnv)*(1.0-std::exp(-1.0/(p.v[23]*sr)));
        else ringEnv=1.0;
        // phase3 is the click oscillator; keep the ring carrier independent in dcX.
        dcX += p.v[19]/sr; dcX -= std::floor(dcX);
        const auto carrierPhase=dcX, shape=p.v[24];
        auto sine=std::sin((carrierPhase+(shape-.5)*.4)*2*pi);
        auto triangle=carrierPhase<.5?carrierPhase*4.0-1.0:3.0-carrierPhase*4.0;
        const auto curve=1.0+(shape-.5)*6.0;
        triangle=std::copysign(std::pow(std::abs(triangle),curve),triangle);
        const auto up=(carrierPhase*(1.0+shape))*2.0-1.0;
        const auto down=1.0-(carrierPhase*(1.0+shape))*2.0;
        const auto pwm=carrierPhase<shape?1.0:-1.0;
        if (carrierPhase<ringPrevious) ringSampleHold=randomBipolar();
        ringPrevious=carrierPhase;
        const double waves[] {sine,triangle,up,down,pwm,ringSampleHold};
        const auto raw=waves[std::clamp(int(std::round(p.v[21])),0,5)];
        const auto smoothing=1.0-std::exp(-1.0/((.00005+p.v[22]*.02)*sr));
        ringSmooth += (raw-ringSmooth)*smoothing;
        const auto modulation=ringSmooth*ringEnv;
        band *= (1.0-p.v[20])+modulation*p.v[20];
    }
    else { ringEnv=ringSmooth=0.0; }
    auto noiseSignal=band*noiseEnv*p.v[8]*1.35*(1.0+accent*2.0);
    bodySignal *= 1.0+accent*bodyEnv*.12;
    const auto driveCurve=std::pow(p.v[17]*.01,1.3), driveMix=p.v[18];
    const auto drive=1.0+driveCurve*(2.0+bodyEnv*2.0+accent*2.5);
    const auto bodyDriven=bodySignal*drive;
    const auto bodyFinal=bodySignal*(1.0-driveMix)+(bodyDriven/(1.0+std::abs(bodyDriven)))*driveMix;
    const auto noiseDriven=noiseSignal*(1.0+driveCurve*1.5);
    const auto noiseFinal=noiseSignal*(1.0-driveMix)+(noiseDriven/(1.0+std::abs(noiseDriven)))*driveMix;
    auto signal=(click+bodyFinal+noiseFinal)*p.v[0]*velocity;
    signal=compress(signal,p);
    active=clickEnv>0.0||bodyEnv>0.0||noiseEnv>0.0||noiseAttackStage;
    if (!std::isfinite(signal)) { reset(); return 0.0f; }
    return static_cast<float>(active?signal:0.0);
}

float SnareVoice::renderSimmons (const SnareParameters& p)
{
    pitchEnv *= std::exp(-1.0/(std::max(.003,.010+p.v[6]*.44)*sr));
    if (pitchEnv<.00001) pitchEnv=0.0;
    bodyEnv -= bodyEnv*(1.0-std::exp(-1.0/(std::max(.008,p.v[2])*sr)));
    if (bodyEnv<.00001) bodyEnv=0.0;
    modPhase += std::max(.1,p.v[19])/sr; modPhase-=std::floor(modPhase);
    const auto shape=p.v[24], triBase=1.0-4.0*std::abs(modPhase-.5);
    const double waves[] {
        std::sin((modPhase+(shape-.5)*.22)*2*pi),
        std::copysign(std::pow(std::abs(triBase),.35+shape*1.7),triBase),
        modPhase*2.0-1.0, 1.0-modPhase*2.0,
        modPhase<std::clamp(shape,.05,.95)?1.0:-1.0, heldNoise };
    const auto modRaw=waves[std::clamp(int(std::round(p.v[21])),0,5)];
    ringSmooth += (modRaw-ringSmooth)*(.001+(1.0-p.v[22])*.075);
    ringEnv += (1.0-ringEnv)*(1.0-std::exp(-1.0/(std::max(.001,p.v[23])*sr)));
    const auto modulation=ringSmooth*ringEnv;
    const auto base=100.0*std::pow(8.8,(p.v[3]+1.0)*.5);
    auto frequency=base*std::pow(2.0,p.v[5]*(2.4+accent*.38)*std::pow(pitchEnv,1.42)+modulation*p.v[20]*.13);
    frequency=std::clamp(frequency,55.0,sr*.38);
    phase1+=frequency/sr; phase1-=std::floor(phase1);
    const auto triangle=1.0-4.0*std::abs(phase1-.5);
    const auto bodyCut=std::min(sr*.42,390.0+base*.95);
    const auto bodyCoef=(2*pi*bodyCut)/(sr+2*pi*bodyCut);
    colourMemory+=(triangle-colourMemory)*bodyCoef;
    const auto shapeMix=p.v[7];
    auto body=colourMemory*(.88-shapeMix*.20)+triangle*(.12+shapeMix*.20);
    body/=1.0+std::abs(body)*(.08+shapeMix*.16);
    const auto bodySignal=body*std::pow(bodyEnv,.68)*(1.0+accent*bodyEnv*.26)*p.v[4];
    if (noiseAttackStage)
    {
        noiseEnv+=(1.0-noiseEnv)*(1.0-std::exp(-1.0/((.0004+p.v[13]*.024)*sr)));
        if(noiseEnv>=.999){noiseEnv=1.0;noiseAttackStage=false;}
    }
    else
    {
        noiseEnv-=noiseEnv*(1.0-std::exp(-1.0/(std::max(.006,p.v[9])*sr)));
        if(noiseEnv<.00001)noiseEnv=0.0;
    }
    const auto texture=std::clamp(p.v[14],0.0,1.0), grain=std::clamp(p.v[15],0.0,1.0);
    if(--holdCounter<=0){heldNoise=randomBipolar();holdCounter=std::max(1,int(std::floor(sr/(13.0+std::pow(texture,1.6)*125.0))));}
    simLp4+=(heldNoise-simLp4)*(.00055+texture*.0038);
    const auto white=randomBipolar();
    auto highCut=1800.0+std::pow(p.v[10],1.28)*14500.0;
    highCut*=.54+.46*std::sqrt(std::max(0.0,noiseEnv))+p.v[12]*p.v[16]*noiseEnv*1.35+accent*p.v[16]*.18;
    highCut*=1.0+simLp4*texture*.10; highCut=std::clamp(highCut,900.0,sr*.42);
    const auto midCut=700.0+texture*3100.0, lowCut=105.0+grain*720.0;
    const auto coef=[](double f,double sampleRate){return 2*pi*f/(sampleRate+2*pi*f);};
    simLp1+=(white-simLp1)*coef(highCut,sr); simLp2+=(white-simLp2)*coef(midCut,sr); simLp3+=(white-simLp3)*coef(lowCut,sr);
    const auto air=simLp1-simLp2, flour=simLp2-simLp3;
    const auto flourMix=.30+texture*.42+grain*.18;
    auto noiseColour=air*(1.0-flourMix*.58)+flour*flourMix;
    noiseColour*=.86+simLp4*(.08+texture*.12); noiseColour/=1.0+std::abs(noiseColour)*.16;
    simNoiseDc+=(noiseColour-simNoiseDc)*.00125;
    const auto input=noiseColour-simNoiseDc;
    auto filterCut=850.0+std::pow(p.v[10],1.25)*9300.0;
    filterCut*=.58+.42*std::sqrt(std::max(0.0,noiseEnv))+p.v[12]*p.v[16]*noiseEnv*1.45+modulation*p.v[7]*.10;
    filterCut=std::clamp(filterCut,350.0,sr*.30);
    const auto f=std::min(.72,coef(filterCut,sr)*1.65), damp=1.82-p.v[11]*1.52;
    const auto hp=input-noiseLp-damp*noiseBp; noiseBp+=f*hp; noiseLp+=f*noiseBp;
    const auto filtered=noiseBp*(.80+p.v[11]*1.20)+hp*(.20+texture*.18);
    const auto noiseSignal=filtered*noiseEnv*p.v[8]*.82*(1.0+accent*.62);
    clickEnv*=std::exp(-1.0/(.0035*sr)); if(clickEnv<.00001)clickEnv=0.0;
    phase3+=(2500.0+base*1.8)/sr;phase3-=std::floor(phase3);
    const auto click=(std::sin(phase3*2*pi)*.42+air*.58)*std::pow(clickEnv,2.15)*p.v[1]*.58*(1.0+accent*.66);
    auto raw=bodySignal+noiseSignal+click;
    const auto drive=1.0+std::pow(p.v[17]*.001,1.25)*7.0;
    const auto saturated=raw*drive/(1.0+std::abs(raw*drive));
    raw=raw*(1.0-p.v[18]*.70)+saturated*(p.v[18]*.70);
    auto signal=compress(raw*p.v[0]*velocity,p);
    active=clickEnv>0||bodyEnv>0||pitchEnv>0||noiseEnv>0||noiseAttackStage;
    if(!std::isfinite(signal)){reset();return 0.0f;}
    return static_cast<float>(active?signal:0.0);
}

void SnareVoice::resetAcoustic(const SnareParameters& p)
{
    const auto decay=std::clamp(p.v[2],0.0,1.0), damping=std::clamp(p.v[22],0.0,1.0);
    const auto ts=.015*(.2+1.3*decay)*(1.28-.56*damping);
    acPunch=acPunch2=acPunch3=acAttackFast=acAtk=1; acTwack=66.6666*std::min(.5,std::max(0.0,p.v[1])*.5)*(1+accent*.16);
    acK=std::pow(10.0,-24.0/(20*std::max(.001,ts)*sr)); acK2=std::pow(10.0,-24.0/(30*std::max(.001,ts)*sr)); acK3=std::pow(10.0,-24.0/(150*std::max(.001,ts)*sr));
    const auto crack=.006+std::clamp(p.v[23],0.0,3.0)*.018, attackTime=.0015+std::clamp(p.v[13],0.0,1.0)*.018;
    acKAtk=std::pow(10.0,-24.0/(20*std::max(.001,crack)*sr)); acKAtkFast=std::pow(10.0,-24.0/(20*std::max(.0005,attackTime)*sr));
    acNoiseVal=acNoiseTime=0; acNoiseAttackSamples=(.001+std::clamp(p.v[13],0.0,1.0)*.065)*sr; acNoiseRise=1-std::exp(-1/(std::max(.0005,attackTime)*sr));
    acNoiseDecay=1-std::exp(-1/((.003+std::pow(std::clamp(p.v[9],0.0,1.0),1.35)*.852)*sr)); acNoiseVol=(.40+.60*decay)*(.30+std::max(0.0,p.v[8])*.34);
    acBodyGain=.42+std::max(0.0,p.v[4])*1.35; acMidGain=.35+std::clamp(p.v[6],0.0,1.0)*1.15; acHighGain=.28+std::clamp(p.v[7],0.0,1.0)*1.25; acAirGain=std::max(0.0,p.v[12])*.72; acWireGain=.46+std::clamp(p.v[8],0.0,6.0)*.10; acClickGain=std::max(0.0,p.v[1])*.8; acCoupling=std::clamp(p.v[16],0.0,1.0); acColor=std::clamp(p.v[14],0.0,1.0); acSparseProbability=.72-std::clamp(p.v[15],0.0,1.0)*.60; acDriveGain=1+std::pow(std::max(0.0,p.v[17])*.001,1.18)*8; acDriveMix=std::clamp(p.v[18],0.0,1.0);
    auto base=170.0*std::pow(2.0,std::clamp(p.v[3],-1.0,1.0)*1.2); base*=std::pow(2.0,randomBipolar()*std::min(1.0,std::abs(p.v[20]))*.030);
    static constexpr double ratios[12][5]={{1.05,1.25,1.40,1.575,2.15},{1.035,1.18,1.32,1.49,1.95},{1.09,1.31,1.52,1.78,2.35},{1.12,1.44,1.67,1.98,2.72},{1.025,1.17,1.29,1.43,1.82},{1.075,1.36,1.61,1.89,2.48},{1.020,1.12,1.24,1.38,1.70},{1.080,1.34,1.62,2.02,2.55},{1.170,1.29,1.73,2.11,2.93},{1.040,1.22,1.38,1.58,2.04},{1.140,1.48,1.92,2.41,3.10},{1.030,1.14,1.27,1.41,1.66}};
    const auto mode=std::clamp(int(std::round(p.v[21])),0,11);
    const auto spread=.58+std::clamp(p.v[5],0.0,1.0)*.84, balance=.72+std::clamp(p.v[24],.01,.99)*.56, res=(1.15-std::clamp(p.v[11],0.0,1.0)*.72)+damping*.20;
    double rr[5]; for(int i=0;i<5;++i) rr[i]=1+(ratios[mode][i]-1)*spread*balance;
    acousticFilters[0].init(base,(.03+(1-decay))*res,sr); acousticFilters[1].init(base*rr[0],(.03+damping*.08)*res,sr); acousticFilters[2].init(base*rr[1],(.05+.02*(1-decay)+damping*.08)*res,sr); acousticFilters[3].init(base*rr[2],(.04+damping*.08)*res,sr); acousticFilters[4].init(base*rr[3],(.04+.02*(1-decay)+damping*.08)*res,sr); acousticFilters[5].init(base*rr[4],(.03+.02*(1-decay)+damping*.08)*res,sr);
    const auto toneScale=std::pow(2.0,(std::clamp(p.v[10],0.0,1.0)-.5)*1.10); acDarkAmount=std::clamp(-p.v[10],0.0,1.0);
    acousticFilters[6].init((5600+acColor*3100)*toneScale,1.55-std::clamp(p.v[11],0.0,1.0)*.55,sr); acousticFilters[7].init((3000+acColor*2100)*toneScale,.95-std::clamp(p.v[11],0.0,1.0)*.42,sr); acousticFilters[8].init(std::clamp(p.v[19],120.0,1200.0)*toneScale,1.05-std::clamp(p.v[11],0.0,1.0)*.42,sr); acousticFilters[9].init(10000*std::pow(.025,acDarkAmount),1,sr);
}

float SnareVoice::renderAcoustic(const SnareParameters& p)
{
    auto& f=acousticFilters;
    const auto r1=randomBipolar(),r2=randomBipolar();
    auto body=.7*(1-acAttackFast)*acBodyGain*(.27*f[0].bp(acPunch3)+.60*f[1].bp(acPunch3+r1*.21)*acPunch3+.11*acMidGain*f[2].bp(acPunch3)+.15*acMidGain*f[3].bp(acPunch2+r2*.5)*acPunch3+.14*acHighGain*f[4].bp(acPunch3)+.12*acHighGain*f[5].bp(acPunch3));
    acTwack*=acKAtk; acAtk*=acKAtk; acAttackFast*=acKAtkFast; acPunch*=acK; acPunch2*=acK2; acPunch3*=acK3;
    if(acNoiseTime<acNoiseAttackSamples){acNoiseVal+=(1-acNoiseVal)*acNoiseRise;acNoiseTime+=1;}else acNoiseVal-=acNoiseVal*acNoiseDecay; if(acNoiseVal<1e-6)acNoiseVal=0;
    // Do not keep exciting the resonators with inaudible random input after
    // their owning envelopes have ended; let the existing filter tail drain.
    const auto noiseExcitation = acNoiseVal > 1e-6;
    const auto crackExcitation = acTwack > 2e-5 || acAtk > 2e-5;
    const auto sparse1=noiseExcitation&&(randomBipolar()+1)*.5<acSparseProbability?randomBipolar()*.5:0.0;
    const auto sparse2=noiseExcitation&&(randomBipolar()+1)*.5<acSparseProbability?randomBipolar()*.5:0.0;
    const auto crackNoise=crackExcitation?randomBipolar():0.0,crackDirect=crackExcitation?randomBipolar():0.0;
    const auto air=.5*acNoiseVol*acNoiseVal*acAirGain*f[6].bp(sparse1), low=1.4*acNoiseVal*acNoiseVol*acWireGain*acCoupling*f[8].bp(sparse2)*acPunch3*acPunch3, crack=2*std::min(1.0,acTwack)*acNoiseVol*acClickGain*(1-acAttackFast)*(f[7].lp(crackNoise)+.3*crackDirect)*acAtk;
    auto noise=air*(.72+acColor*.55)+low*(1.25-acColor*.55)+crack; if(acDarkAmount>1e-6) noise+=(f[9].lp(noise)-noise)*acDarkAmount;
    auto y=body+noise; y=4*y/(1+std::abs(y+y)); if(acDriveMix>1e-6){auto d=y*acDriveGain;d/=1+std::abs(d);y+=(d-y)*acDriveMix;}
    active=acPunch3>.00012||acNoiseVal>.00001||acTwack>.00002;
    if(!active){double energy=0;for(auto& sv:f)energy+=std::abs(sv.ic1);active=energy>.00012;}
    if(!active)return 0;
    auto signal=compress(y*.12*p.v[0]*velocity*12.589254118,p); if(!std::isfinite(signal)){reset();return 0;} return static_cast<float>(signal);
}

void SnareVoice::resetLinn(const SnareParameters& p)
{
    const auto tune=std::clamp(p.v[3],-3.0,1.0),decay=std::clamp(p.v[2],0.0,1.0);
    linnBaseFrequency=std::clamp(156.0*std::pow(2.0,tune*.50),45.0,420.0);
    const auto bodyTime=.060+decay*.150;
    linnBodyK=std::pow(10.0,-24.0/(20*bodyTime*sr));
    linnBodyAttackK=1-std::exp(-1/(.00075*sr));
    linnPitchAmount=std::clamp(p.v[5],0.0,1.0)*3.5;
    const auto pitchTime=.0015+std::pow(std::clamp(p.v[24],0.0,1.0),2.5)*.120;
    linnPitchK=std::pow(10.0,-40.0/(20*pitchTime*sr));
    const auto crackTime=.0018+std::clamp(p.v[1],0.0,2.0)*.0012;
    linnCrackK=std::pow(10.0,-30.0/(20*crackTime*sr));
    linnImpactK=std::pow(10.0,-36.0/(20*.0045*sr));
    const auto wireAttack=.001+std::clamp(p.v[13],0.0,1.0)*.049;
    linnWireAttackSamples=wireAttack*sr;
    linnWireRise=1-std::exp(-1/(std::max(.0005,wireAttack*.22)*sr));
    const auto wireTime=.070+std::clamp(p.v[9],0.0,1.0)*.200;
    linnWireDecay=std::pow(10.0,-30.0/(20*wireTime*sr));
    const auto toneScale=std::pow(2.0,std::clamp(p.v[10],-1.0,1.0)*.75);
    linnColor=std::clamp(p.v[14],0.0,1.0);
    const auto resonance=std::clamp(p.v[11],0.0,1.0),modeQ=.19,wireQ=1.65-resonance*1.47;
    linnFilters[0].init(linnBaseFrequency*2.21,modeQ*.90,sr);
    linnFilters[1].init(linnBaseFrequency*2.92,modeQ*1.08,sr);
    linnFilters[2].init(linnBaseFrequency*4.58,modeQ*1.32,sr);
    linnFilters[3].init(linnBaseFrequency*5.68,modeQ*1.48,sr);
    linnFilters[4].init((1550+linnColor*1100)*toneScale,1.30-resonance*1.02,sr);
    linnFilters[5].init((780+linnColor*650)*toneScale,1.20-resonance*.92,sr);
    linnFilters[6].init((2800+linnColor*1700)*toneScale,wireQ,sr);
    linnFilters[7].init((4300+linnColor*2300)*toneScale,std::max(.18,wireQ*1.08),sr);
    linnFilters[8].init(6500+linnColor*5000,.92,sr);
    linnPhase=0;linnBodyEnv=1;linnBodyAttack=0;linnPitchEnv=1;linnCrackEnv=1;linnImpactEnv=1;linnWireEnv=0;linnWireAge=0;
    linnBodyGain=std::max(0.0,p.v[4])*1.25;linnMidGain=std::clamp(p.v[6],0.0,1.0)*.80;linnHighGain=std::clamp(p.v[7],0.0,1.0)*.65;
    linnNoiseGain=std::clamp(p.v[8],0.0,6.0)*.32;linnClickGain=std::clamp(p.v[1],0.0,2.0)*1.75;linnCoupling=std::clamp(p.v[16],0.0,1.0);
    linnDegrade=std::clamp(p.v[15],0.0,1.0);linnDriveGain=1+std::pow(std::max(0.0,p.v[17])*.001,1.15)*7.5;linnDriveMix=std::clamp(p.v[18],0.0,1.0);
    linnDacStep=std::min(1.0,(28600+linnColor*3600)/sr);linnDacPhase=0;linnDacHold=0;
    linnRingPhase=linnRingHold=linnRingSmoothed=0;
    linnRingSmoothK=1-std::exp(-1/((.00005+std::clamp(p.v[22],0.0,1.0)*.030)*sr));
    linnRingDelaySamples=std::clamp(p.v[23],0.0,3.0)*sr;linnRingShape=.50;
    linnAge=0;linnMaxAge=(.190+decay*.090+std::clamp(p.v[9],0.0,1.0)*.150)*sr;
}

float SnareVoice::renderLinn(const SnareParameters& p)
{
    ++linnAge;linnBodyAttack+=(1-linnBodyAttack)*linnBodyAttackK;linnBodyEnv*=linnBodyK;linnPitchEnv*=linnPitchK;linnCrackEnv*=linnCrackK;linnImpactEnv*=linnImpactK;
    const auto frequency=linnBaseFrequency+linnBaseFrequency*linnPitchAmount*std::pow(std::max(0.0,linnPitchEnv),.85+accent*1.15);
    linnPhase+=frequency/sr;linnPhase-=std::floor(linnPhase);
    const auto body=std::sin(linnPhase*2*pi)*linnBodyEnv*linnBodyAttack;
    const auto impact=linnImpactEnv*(.32+linnClickGain*.45)*(.70+randomBipolar()*.48);
    const auto m2=linnFilters[0].bp(impact),m3=linnFilters[1].bp(impact),m4=linnFilters[2].bp(impact),m5=linnFilters[3].bp(impact);
    const auto membrane=body*linnBodyGain+(m2*.42+m3*.26)*linnMidGain+(m4*.20+m5*.13)*linnHighGain;
    const auto rawNoise=randomBipolar();
    auto crack=linnFilters[4].bp(rawNoise)*linnCrackEnv*linnClickGain;
    if(linnWireAge<linnWireAttackSamples){linnWireEnv+=(1-linnWireEnv)*linnWireRise;++linnWireAge;}else linnWireEnv*=linnWireDecay;
    const auto hpWire=linnFilters[5].hp(rawNoise);
    auto wireMid=linnFilters[6].bp(rawNoise),wireAir=linnFilters[7].bp(rawNoise);
    const auto resonance=std::clamp(p.v[11],0.0,1.0);wireMid*=.72+resonance*1.38;wireAir*=.68+resonance*1.52;
    const auto toneEnvelope=std::clamp(p.v[12]*.1,0.0,1.0),lateBright=std::min(1.0,linnWireEnv*(.25+toneEnvelope*1.40));
    auto wire=(hpWire*(.42+linnColor*.16)+wireMid*(.38+lateBright*.30)+wireAir*(.12+lateBright*.34))*linnWireEnv*linnNoiseGain;
    linnRingPhase+=std::clamp(p.v[19],1.0,1000.0)/sr;
    if(linnRingPhase>=1){linnRingPhase-=1;if(std::clamp(int(std::floor(p.v[21]+.5)),0,11)==5)linnRingHold=randomBipolar();}
    const auto wave=std::clamp(int(std::floor(p.v[21]+.5)),0,11);double ringRaw;
    if(wave==0)ringRaw=std::sin(2*pi*linnRingPhase);else if(wave==1)ringRaw=4*std::abs(linnRingPhase-.5)-1;else if(wave==2)ringRaw=2*linnRingPhase-1;else if(wave==3)ringRaw=1-2*linnRingPhase;else if(wave==4)ringRaw=linnRingPhase<linnRingShape?1:-1;else if(wave==5)ringRaw=linnRingHold;else if(wave==6)ringRaw=std::sin(2*pi*linnRingPhase)*.72+std::sin(4*pi*linnRingPhase)*.28;else if(wave==7)ringRaw=std::sin(2*pi*linnRingPhase)*std::sin(6*pi*linnRingPhase);else if(wave==8)ringRaw=std::sin(2*pi*linnRingPhase)*.52+std::sin(7*pi*linnRingPhase)*.30+linnRingHold*.18;else if(wave==9)ringRaw=std::sin(2*pi*linnRingPhase)+std::sin(2*pi*linnRingPhase*2.17)*.28;else if(wave==10)ringRaw=std::sin(2*pi*linnRingPhase)*.60+std::sin(2*pi*linnRingPhase*2.71)*.40;else ringRaw=std::sin(2*pi*linnRingPhase)-std::sin(4*pi*linnRingPhase)*.45;
    linnRingSmoothed+=(ringRaw-linnRingSmoothed)*linnRingSmoothK;
    const auto ringDepth=std::clamp(p.v[20],-1.0,1.0);if(linnAge>=linnRingDelaySamples&&std::abs(ringDepth)>1e-6){const auto ringMod=linnRingSmoothed*ringDepth;wire*=std::max(.10,1+ringMod*.62);wire+=ringMod*linnWireEnv*linnNoiseGain*.32;}
    const auto bodyEnergy=std::min(1.0,std::abs(membrane)*1.55);wire*=std::max(.12,1-bodyEnergy*(.18+linnCoupling*.67));
    crack+=rawNoise*linnCrackEnv*linnClickGain*.55;const auto stick=rawNoise*linnImpactEnv*linnClickGain*.90;
    auto y=membrane+crack+stick+wire;
    linnDacPhase+=linnDacStep;if(linnDacPhase>=1){linnDacPhase-=1;const auto qmag=std::floor(std::min(1.0,std::abs(y))*127+.5)/127;linnDacHold=y<0?-qmag:qmag;}
    const auto digital=linnFilters[8].lp(linnDacHold);y+=(digital-y)*linnDegrade*.72;
    y=(y*1.48)/(1+std::abs(y*1.48)*.58);if(linnDriveMix>1e-6){auto driven=y*linnDriveGain;driven/=1+std::abs(driven);y+=(driven-y)*linnDriveMix;}y*=1+std::clamp(accent,0.0,1.0)*.55;
    double filterEnergy=0;for(int i=0;i<8;++i)filterEnergy+=std::abs(linnFilters[std::size_t(i)].ic1);
    active=linnAge<linnMaxAge&&(linnBodyEnv>4e-6||linnWireEnv>4e-6||linnCrackEnv>4e-6||std::abs(y)>4e-6||filterEnergy>2e-5);
    auto signal=compress(y*.6*p.v[0]*velocity*1.5,p);if(!std::isfinite(signal)){reset();return 0;}return static_cast<float>(active?signal:0);
}

void SnareVoice::resetSaike(const SnareParameters& p)
{
    skType=activeEngine==7?4:activeEngine-2;
    const auto tf=2302.58509299/sr;
    skPitch.rise=tf*.5; skPitch.decay=tf*.933*std::exp(-4.605170185988092*std::clamp(p.v[2],0.0,1.0)); skPitch.attackSamples=.01*sr; skPitch.val=skPitch.t=0;
    skNoise.rise=tf; skNoise.decay=tf*.4*std::exp(-4.605170185988092*(.6+std::clamp(p.v[9],0.0,1.0)*.3)); skNoise.attackSamples=std::clamp(p.v[13]*.08,0.0,.12)*sr; skNoise.val=skNoise.t=0;
    skAmp.rise=tf; skAmp.decay=tf*.33*std::exp(-4.605170185988092*std::clamp(p.v[6],0.0,1.0)); skAmp.attackSamples=(.001+std::clamp(p.v[23],0.0,1.0)*.039)*sr; skAmp.val=skAmp.t=0;
    phase1=0; skSmoothCount=std::max(0,int(std::floor(std::clamp(p.v[22],0.0,1.0)*12+.5))); skLast=0;
    const auto tune01=std::clamp((p.v[3]+1)*.5,0.0,1.0);
    skBaseLog=p.v[3]<-1?std::log((170*std::pow(2.0,p.v[3]+1))/22050.0):std::log((170+230*tune01)/22050.0);
    double noiseFrequency,noiseInvq,sharedFrequency,sharedInvq;
    if(skType==4){noiseFrequency=720+std::pow(std::clamp(p.v[10],0.0,1.0),1.30)*9000;noiseInvq=1.42-std::clamp(p.v[11],0.0,1.0)*1.12;sharedFrequency=1850+std::clamp(p.v[12],0.0,10.0)*420;sharedInvq=1.18-std::clamp(p.v[16],0.0,1.0)*.78;skBody.init(470+tune01*1180,.58,sr);}
    else{noiseFrequency=900+std::pow(std::clamp(p.v[10],0.0,1.0),1.35)*10500;noiseInvq=1.55-std::clamp(p.v[11],0.0,1.0)*1.25;sharedFrequency=1100+std::clamp(p.v[12],0.0,10.0)*500;sharedInvq=1.35-std::clamp(p.v[16],0.0,1.0);skBody.init(650+tune01*1500,.75,sr);}
    skNoiseFilter.init(noiseFrequency,noiseInvq,sr);skShared.init(sharedFrequency,sharedInvq,sr);skMudDip.init(480,12,-50,sr);skShift.init(std::clamp(p.v[19],.5,80.0),sr);
    skAge=0;skMaxAge=(.18+std::clamp(p.v[9],0.0,1.0)*1.45+std::clamp(p.v[6],0.0,1.0)*.75)*sr;
}

float SnareVoice::renderSaike(const SnareParameters& p)
{
    skAge+=1;const auto pitch=skPitch.tick(),amp=skAmp.tick();
    if(skType==1&&amp>.002&&skNoise.val<(.12+std::clamp(p.v[15],0.0,1.0)*.42)*((randomBipolar()+1)*.5))skNoise.t=0;
    const auto saikeNoiseEnv=skNoise.tick();
    const auto step=.5*std::exp((1-.4*std::clamp(p.v[5]*.5,0.0,.5)*pitch)*skBaseLog)*(48000/sr);
    phase1+=step;phase1-=std::floor(phase1);const auto tri=phase1<=.5?4*phase1-1:3-4*phase1;
    const auto asym=.02+std::clamp(p.v[24],0.0,1.0)*.16,color=std::clamp(p.v[14],0.0,1.0);
    const auto bodyGain=std::max(0.0,p.v[4])*(.72+color*.35),noiseGain=std::max(0.0,p.v[8])*(1.18-color*.38);
    const auto shaped=.91*(2/(1+std::exp(-2*(2*tri+asym)))-1-asym)+.1*tri;
    double y;
    if(skType==1){const auto n=2*saikeNoiseEnv*randomBipolar()*.5*noiseGain;y=(shaped*bodyGain+n)*amp;y=skShared.bp(y);y=skMudDip.tick(y);}
    else if(skType==2){const auto body=shaped*.5*amp*saikeNoiseEnv*bodyGain;const auto interaction=1-std::min(.98,amp*body*body*(.5+std::clamp(p.v[16],0.0,1.0)));const auto n=skNoiseFilter.bp(saikeNoiseEnv*saikeNoiseEnv*randomBipolar()*.5);y=body+n*interaction*noiseGain;y=skMudDip.tick(y);if(std::abs(p.v[20])>1e-6){const auto shifted=skShift.tick(y);y+=(shifted-y)*std::clamp(std::abs(p.v[20]),0.0,1.0);}}
    else if(skType==4){const auto resonance=skBody.bp(shaped*amp);const auto body=(shaped*.44+resonance*.92)*amp*(.42+saikeNoiseEnv*.58)*bodyGain;const auto interaction=1-std::min(.96,amp*body*body*(.42+std::clamp(p.v[16],0.0,1.0)*.88));const auto n=skNoiseFilter.bp(saikeNoiseEnv*saikeNoiseEnv*randomBipolar()*.5),crack=skShared.bp(std::pow(std::max(0.0,saikeNoiseEnv),3.4)*randomBipolar()*.5);y=body+(n*.82+crack*.34)*interaction*noiseGain;y=skMudDip.tick(y);if(std::abs(p.v[20])>1e-6){const auto shifted=skShift.tick(y);y+=(shifted-y)*std::clamp(std::abs(p.v[20]),0.0,1.0);}}
    else{const auto body=tri*.3*amp*bodyGain,n=skNoiseFilter.bp(saikeNoiseEnv*randomBipolar()*.5);y=skMudDip.tick(body+n*noiseGain)*2;}
    if(p.v[1]>1e-6)y+=randomBipolar()*std::pow(std::max(0.0,amp),5)*p.v[1]*.18;
    if(skSmoothCount>0){--skSmoothCount;y=.9*skLast+.1*y;}
    if(p.v[18]>1e-6){auto driven=y*(1+std::pow(std::max(0.0,p.v[17])*.001,1.2)*10);driven/=1+std::abs(driven);y+=(driven-y)*std::clamp(p.v[18],0.0,1.0);}
    y*=1+std::max(0.0,accent)*.85;skLast=y;const auto energy=std::abs(skNoiseFilter.ic1)+std::abs(skShared.ic1)+std::abs(skBody.ic1);active=skAge<skMaxAge&&(amp>3e-6||saikeNoiseEnv>3e-6||std::abs(y)>3e-6||energy>1e-5);
    auto signal=compress(y*.12*p.v[0]*velocity*9.929102934,p);if(!std::isfinite(signal)){reset();return 0;}return static_cast<float>(active?signal:0);
}

void SnareVoice::resetPlaits(const SnareParameters& p)
{
    const auto decay=std::clamp(p.v[2],0.0,1.0),snappy=std::clamp(p.v[8],0.0,1.0),fmAmount=std::pow(std::clamp(p.v[5],0.0,1.0),2);
    const auto base=std::clamp(175*std::pow(2.0,std::clamp(p.v[3],-3.0,1.0)*.5),36.0,sr*.11),scale=48000/sr;plF0=base/sr;
    auto k48=1-1/(.014*48000)*std::pow(2.0,(-decay*72-fmAmount*13+snappy*6)/12);plDrumDecay=std::pow(std::clamp(k48,.5,.9999999),scale);
    k48=1-1/(.010*48000)*std::pow(2.0,(-decay*62-snappy*8)/12);plSnareDecay=std::pow(std::clamp(k48,.5,.9999999),scale/(.30+std::clamp(p.v[9],0.0,1.0)*1.8));plFmDecay=std::pow(1-1/(.0065*48000),scale);
    const auto tuneScale=std::pow(2.0,(std::clamp(p.v[10],0.0,1.0)-.5)*(1+std::clamp(p.v[12],0.0,1.0)*1.7));plDrumLpCoeff=1-std::exp(-2*pi*std::min(sr*.43,base*(2.7+std::clamp(p.v[14],0.0,1.0)*1.8))/sr);plSnareHpCoeff=1-std::exp(-2*pi*std::min(sr*.40,std::max(80.0,base*9*tuneScale))/sr);plSnareLpCoeff=1-std::exp(-2*pi*std::min(sr*.44,std::max(900.0,base*38*tuneScale))/sr);
    plPhase0=plPhase1=0;plDrumAmp=plSnareAmp=.55+std::clamp(accent,0.0,1.0)*.75;plFm=1;plHoldCounter=int(std::floor((.006+std::clamp(p.v[13],0.0,1.0)*.055)*sr));plDrumLp=plSnareLp=plSnareHpLp=plNoiseHold=0;plNoiseHoldCount=0;plNoiseHoldSamples=std::max(1,int(std::floor(1+std::pow(std::clamp(p.v[15],0.0,1.0),2)*sr*.0025)));plDriveGain=1+std::pow(std::max(0.0,p.v[17])*.001,1.18)*10;plDriveMix=std::clamp(p.v[18],0.0,1.0);plSmoothCount=std::max(0,int(std::floor(std::clamp(p.v[22],0.0,1.0)*10+.5)));plLast=0;skMudDip.init(480,12,-50,sr);skShift.init(std::clamp(p.v[19],.5,80.0),sr);plAge=0;plMaxAge=(.30+decay*2.6+std::clamp(p.v[9],0.0,1.0)*1.7)*sr;
}

float SnareVoice::renderPlaits(const SnareParameters& p)
{
    const auto snappy=std::clamp(p.v[8],0.0,1.0),fmAmount=std::pow(std::clamp(p.v[5],0.0,1.0),2);plDrumAmp*=plDrumDecay;if(plHoldCounter>0)--plHoldCounter;else plSnareAmp*=plSnareDecay;plFm*=plFmDecay;
    auto resetAmount=std::clamp((.125-plF0)*8,0.0,1.0);resetAmount*=resetAmount*fmAmount*(.40+std::clamp(p.v[16],0.0,1.0)*.95);const auto resetNoise=((plPhase0>.5?-1:1)+(plPhase1>.5?-1:1))*resetAmount*.025;
    const auto frequency=plF0*(1+fmAmount*(4.8*plFm));plPhase0+=frequency;plPhase1+=frequency*(1.47+(std::clamp(p.v[7],0.0,1.0)-.5)*.20);if(resetAmount>.1){if(plPhase0>=1+resetNoise)plPhase0=1-plPhase0;if(plPhase1>=1+resetNoise)plPhase1=1-plPhase1;}else{if(plPhase0>=1)--plPhase0;if(plPhase1>=1)--plPhase1;}
    const auto distorted=[](double phase){const auto tri=(phase<.5?phase:1-phase)*4-1.3;return 2*tri/(1+std::abs(tri));};auto body=distorted(plPhase0)*.72+distorted(plPhase1)*(.15+std::clamp(p.v[6],0.0,1.0)*.55);const auto asym=.025+std::clamp(p.v[24],0.0,1.0)*.17;body=.91*(2/(1+std::exp(-2*(2*body+asym)))-1-asym)+.1*body;body*=plDrumAmp*(.55+std::max(0.0,p.v[4])*1.35)*std::sqrt(std::max(0.0,1-snappy*.55));plDrumLp+=(body-plDrumLp)*plDrumLpCoeff;body=plDrumLp;
    const auto raw=randomBipolar();if(plNoiseHoldCount<=0){plNoiseHold=raw;plNoiseHoldCount=plNoiseHoldSamples;}--plNoiseHoldCount;const auto degraded=raw*(1-std::clamp(p.v[15],0.0,1.0))+plNoiseHold*std::clamp(p.v[15],0.0,1.0);plSnareLp+=(degraded-plSnareLp)*plSnareLpCoeff;plSnareHpLp+=(plSnareLp-plSnareHpLp)*plSnareHpCoeff;auto wire=(plSnareLp-plSnareHpLp)*(.75+std::clamp(p.v[11],0.0,1.0)*.75);wire*=(plSnareAmp+plFm*.55)*(snappy*2.10)*(1-std::min(1.0,std::abs(body)*1.8)*std::min(.90,.25+std::clamp(p.v[16],0.0,1.0)*.65));
    auto y=body+wire+randomBipolar()*plFm*std::clamp(p.v[1],0.0,2.0)*.070;y=skMudDip.tick(y);if(std::abs(p.v[20])>1e-6){const auto shifted=skShift.tick(y);y+=(shifted-y)*std::clamp(std::abs(p.v[20]),0.0,1.0);}if(plDriveMix>1e-6){auto driven=y*plDriveGain;driven/=1+std::abs(driven);y+=(driven-y)*plDriveMix;}if(plSmoothCount>0){--plSmoothCount;y=.86*plLast+.14*y;}y*=1+std::clamp(accent,0.0,1.0)*.60;plLast=y;++plAge;active=plAge<plMaxAge&&(plDrumAmp>4e-6||plSnareAmp>4e-6||plFm>4e-6||std::abs(y)>4e-6);
    auto signal=compress(y*.12*p.v[0]*velocity*19.811164906,p);if(!std::isfinite(signal)){reset();return 0;}return static_cast<float>(active?signal:0);
}
}
