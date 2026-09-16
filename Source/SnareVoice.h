// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <array>
#include <cstdint>
#include "KickOtherVoices.h"

namespace lr608
{
struct SnareParameters
{
    std::array<double, 31> v {};
    double accentThreshold = 112.0;
    double accentCharacter = 1.0;
};

class SnareVoice
{
public:
    void prepare (double sampleRate);
    void reset();
    void trigger (int engine, int velocity, const SnareParameters&, std::uint32_t randomSeed = 0);
    float render (const SnareParameters&);
    bool isActive() const noexcept { return active; }

private:
    double randomBipolar();
    float render808 (const SnareParameters&);
    float renderSimmons (const SnareParameters&);
    float renderAcoustic (const SnareParameters&);
    void resetAcoustic (const SnareParameters&);
    float renderLinn (const SnareParameters&);
    void resetLinn (const SnareParameters&);
    float renderSaike (const SnareParameters&);
    void resetSaike (const SnareParameters&);
    float renderPlaits (const SnareParameters&);
    void resetPlaits (const SnareParameters&);
    double compress (double, const SnareParameters&);
    double sr = 44100.0;
    int activeEngine = 0;
    bool active = false;
    double velocity = 1.0, accent = 0.0, bodyEnv = 0.0, noiseEnv = 0.0, clickEnv = 0.0;
    double pitchEnv = 0.0, phase1 = 0.0, phase2 = 0.0, phase3 = 0.0;
    double noiseLp = 0.0, noiseBp = 0.0, noiseHpMemory = 0.0, colourMemory = 0.0;
    double simLp1 = 0.0, simLp2 = 0.0, simLp3 = 0.0, simLp4 = 0.0, simNoiseDc = 0.0;
    double ringPhase = 0.0, ringSmooth = 0.0, attack = 0.0, dcX = 0.0, dcY = 0.0;
    double modPhase = 0.0, pinkState = 0.0, brownState = 0.0;
    double nx1 = 0.0, nx2 = 0.0, ny1 = 0.0, ny2 = 0.0;
    double ringEnv = 0.0, ringPrevious = 0.0, ringSampleHold = 0.0;
    bool noiseAttackStage = false;
    double rms = 0.0, runningDb = 0.0;
    double compMix=0.0,compRmsCoefficient=0.0,compThresholdLinear=1.0;
    double compThresholdLog=0.0;
    double compAttackCoefficient=0.0,compReleaseCoefficient=0.0;
    double compRatioReduction=0.0,compMakeupGain=1.0;
    int holdCounter = 0;
    double heldNoise = 0.0;
    std::uint32_t rng = 0x6085a11u;
    struct Svf
    {
        double ic1=0, ic2=0, k=1, a1=0, a2=0, a3=0;
        void init (double, double, double); double bp (double); double lp (double); double hp (double);
    };
    std::array<Svf, 10> acousticFilters {};
    double acPunch=0,acPunch2=0,acPunch3=0,acAttackFast=0,acTwack=0,acAtk=0;
    double acK=0,acK2=0,acK3=0,acKAtk=0,acKAtkFast=0,acNoiseVal=0,acNoiseTime=0;
    double acNoiseRise=0,acNoiseDecay=0,acNoiseAttackSamples=0,acNoiseVol=0;
    double acBodyGain=0,acMidGain=0,acHighGain=0,acAirGain=0,acWireGain=0,acClickGain=0;
    double acCoupling=0,acColor=0,acSparseProbability=0,acDriveGain=1,acDriveMix=0,acDarkAmount=0;
    std::array<Svf,9> linnFilters {};
    double linnPhase=0,linnBodyEnv=0,linnBodyAttack=0,linnPitchEnv=0,linnCrackEnv=0,linnImpactEnv=0;
    double linnWireEnv=0,linnWireAge=0,linnWireAttackSamples=0,linnBodyK=0,linnBodyAttackK=0,linnPitchK=0,linnCrackK=0,linnImpactK=0,linnWireRise=0,linnWireDecay=0;
    double linnBaseFrequency=0,linnPitchAmount=0,linnBodyGain=0,linnMidGain=0,linnHighGain=0,linnNoiseGain=0,linnClickGain=0,linnCoupling=0,linnColor=0,linnDegrade=0,linnDriveGain=1,linnDriveMix=0;
    double linnDacPhase=0,linnDacStep=0,linnDacHold=0,linnAge=0,linnMaxAge=0,linnRingPhase=0,linnRingHold=0,linnRingSmoothed=0,linnRingSmoothK=0,linnRingDelaySamples=0,linnRingShape=.5;
    struct Envelope { double t=0,rise=0,decay=0,attackSamples=0,val=0; double tick(); };
    Envelope skPitch,skNoise,skAmp;
    Svf skBody,skNoiseFilter,skShared;
    KickOtherVoices::Bell skMudDip;
    KickOtherVoices::Shifter skShift;
    double skBaseLog=0,skLast=0,skAge=0,skMaxAge=0;
    int skType=1,skSmoothCount=0;

    double plPhase0=0,plPhase1=0,plDrumAmp=0,plSnareAmp=0,plFm=0;
    double plDrumDecay=0,plSnareDecay=0,plFmDecay=0,plF0=0;
    double plDrumLpCoeff=0,plSnareLpCoeff=0,plSnareHpCoeff=0;
    double plDrumLp=0,plSnareLp=0,plSnareHpLp=0,plNoiseHold=0,plLast=0,plAge=0,plMaxAge=0;
    double plDriveGain=1,plDriveMix=0;
    int plHoldCounter=0,plNoiseHoldCount=0,plNoiseHoldSamples=1,plSmoothCount=0;
};
}
