// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <cstdint>

namespace lr608
{
struct Kick808Parameters
{
    double level {}, tune {}, punchAmount {}, punchTime {}, decay {}, curve {};
    double clickLevel {}, noiseTone {}, clickPhase {}, curvePitch {}, curvePitchDecay {};
    double noiseDecay {}, noiseLevel {}, lfoVolumeDepth {}, lfoFrequency {};
    int lfoWave {};
    double lfoPitchDepth {}, lfoDelay {}, bodyDrive {}, bodyDriveMix {};
    double bodyPhase {}, lfoSmooth {}, lfoShape {}, clickTone {}, clickResonance {};
    double compThreshold {}, compMakeup {}, compAttack {}, compRelease {}, compMix {};
    int compRatio {};
    // Kick 808 LAB TEST11 additions. Defaults are backward-compatible:
    // Noise Resonance=0, Noise Granulation=0, Click/Body Coupling=0.11.
    double noiseResonance {}, noiseGranulation {}, clickBodyCoupling { 0.11 };
    double accentThreshold {}, accentCharacter {}, masterDb {};
};

// Direct native transcription of kick_engine_voice == 0 (the LR-608 808
// branch). Random noise uses a fixed local PRNG so offline renders repeat.
class Kick808Voice
{
public:
    void prepare (double sampleRate);
    void reset();
    void trigger (int velocity, const Kick808Parameters&, std::uint32_t randomSeed = 0);
    float render (const Kick808Parameters&, double tempo);
    bool isActive() const noexcept { return active; }

private:
    double randomBipolar();
    double sr = 44100.0;
    bool active = false;
    double velocity = 1.0, accent = 0.0;
    double env = 0.0, pitchEnv = 0.0, clickEnv = 0.0, impulseEnv = 0.0, noiseEnv = 0.0;
    double phase = 0.0, impulsePhase = 0.0;
    double lfoPhase = 0.0, lfoPreviousPhase = 0.0, lfoEnv = 0.0, lfoSmoothState = 0.0, lfoSampleHold = 0.0;
    double clickX1 = 0.0, clickX2 = 0.0, clickY1 = 0.0, clickY2 = 0.0;
    double impulseX1 = 0.0, impulseX2 = 0.0, impulseY1 = 0.0, impulseY2 = 0.0;
    double dcX = 0.0, dcY = 0.0, detectorEnvelope = 0.0, runningDb = 0.0;
    double impulseTailEnvelope = 0.0, impulseTailStep = 0.0;
    double noiseContactE1 = 0.0, noiseContactE2 = 0.0, noiseContactE3 = 0.0, noiseContactE4 = 0.0, noiseContactE5 = 0.0;
    double noiseContactC1 = 1.0, noiseContactC2 = 1.0, noiseContactC3 = 1.0, noiseContactC4 = 1.0, noiseContactC5 = 1.0;
    double noiseContactMean = 0.0;
    double noiseContactStep1 = 1.0, noiseContactStep2 = 1.0, noiseContactStep3 = 1.0, noiseContactStep4 = 1.0, noiseContactStep5 = 1.0;
    double noiseContactD1Fast = 0.0, noiseContactD2Fast = 0.0, noiseContactD3Fast = 0.0, noiseContactD4Fast = 0.0, noiseContactD5Fast = 0.0;
    double noiseContactD1Slow = 0.0, noiseContactD2Slow = 0.0, noiseContactD3Slow = 0.0, noiseContactD4Slow = 0.0, noiseContactD5Slow = 0.0;
    double noiseContactMeanCoefficient = 0.0, noiseGranulationAmount = 0.0;
    double pitchEnvStep = 0.0, lfoEnvRate = 0.0, lfoSmoothRate = 0.0;
    double endFrequency = 0.0, startFrequency = 0.0, envLoss = 0.0;
    double amplitudeExponent = 1.0, pitchCurveReciprocal = 1.0;
    double clickEnvStep = 0.0, impulseEnvStep = 0.0, impulsePhaseStep = 0.0;
    double impulseB0 = 0.0, impulseA1 = 0.0, impulseA2 = 0.0;
    double clickB0 = 0.0, clickA1 = 0.0, clickA2 = 0.0, noiseEnvLoss = 0.0;
    double bodyDriveCurve = 0.0, dcCoefficient = 0.0;
    // Refined local 1176-style compressor from Kick 808 LAB TEST7.
    double detectorAttackCoefficient = 0.0, detectorReleaseCoefficient = 0.0;
    double compThresholdDb = -21.0, compKneeDb = 6.0;
    double compAttackCoefficient = 0.0, compReleaseSeconds = 0.08, compRatioReduction = 0.0;
    double compMakeupGain = 1.0;
    std::uint32_t rng = 0x608u;
};
}
