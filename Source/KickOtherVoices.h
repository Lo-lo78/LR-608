// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include "Kick808Voice.h"
#include <array>
#include <cstdint>

namespace lr608
{
class KickOtherVoices
{
public:
    void prepare (double sampleRate);
    void reset();
    void trigger (int engine, int velocity, const Kick808Parameters&, std::uint32_t randomSeed = 0);
    float render (const Kick808Parameters&, double tempo = 120.0);
    bool isActive() const noexcept { return active; }

public:
    struct Svf
    {
        double ic1 {}, ic2 {}, a1 {}, a2 {}, a3 {};
        void init (double frequency, double inverseQ, double sr);
        double bandPass (double input);
    };
    struct Bell
    {
        double ic1 {}, ic2 {}, a1 {}, a2 {}, a3 {}, m1 {};
        void init (double frequency, double q, double gainDb, double sr);
        double tick (double input);
    };
    struct Elliptic
    {
        std::array<double, 16> s {};
        double tick (double input);
    };
    struct Shifter
    {
        double coefficient1 {}, coefficient2 {}, cos11 {}, cos12 {}, sin11 {}, sin12 {};
        double cos21 {}, cos22 {}, sin21 {}, sin22 {}, t1 {}, t2 {}, dt1 {}, dt2 {};
        Elliptic l1, l2;
        void init (double shift, double sr);
        double tick (double input);
    };

private:

    double randomBipolar();
    double finish (double signal, const Kick808Parameters&);
    float renderSimmons (const Kick808Parameters&);
    float render909 (const Kick808Parameters&);
    float renderLinn (const Kick808Parameters&, double tempo);
    float renderSaike (const Kick808Parameters&);
    void resetSaike (const Kick808Parameters&);

    double sr = 44100.0;
    bool active = false;
    int activeEngine = 1;
    double velocity = 1.0, accent = 0.0;
    double env {}, pitchEnv {}, clickEnv {}, impulseEnv {}, noiseEnv {};
    double phase {}, impulsePhase {}, lfoPhase {}, lfoEnv {}, lfoSmooth {};
    double noiseHold {}, bodyLp {}, noiseLp1 {}, noiseLp2 {}, noiseLp3 {}, noiseLp4 {}, noiseDc {};
    double noiseCount {}, dcX {}, dcY {}, rms {}, runningDb {};
    std::array<double,4> linnPhase{},linnLp{};
    double linnFilterEnv{},linnBeaterLp1{},linnBeaterLp2{},linnAirLp1{},linnAirLp2{};
    double linnDigitalPhase{},linnDigitalHold{},linnHitDetune{1},linnHitColor{},linnSampleHold{};
    double dcCoefficient {}, rmsCoefficient {}, compThresholdLinear {1.0};
    double compAttackCoefficient {}, compReleaseCoefficient {};
    double compRatioReduction {}, compMakeupGain {1.0};

    // Saike shared state.
    double pitchValue {}, pitchTime {}, pitchRise {}, pitchDecay {}, pitchAttackSamples {};
    double ampValue {}, ampTime {}, ampRise {}, ampDecay {}, ampAttackSamples {};
    double saikeNoiseLevel {}, saikeNoiseDecay {}, phase2 {}, lastY {}, postLp {};
    int smoothCount {};
    Svf saikeNoiseFilter;
    Bell mudDip, clickBoost;
    Shifter shifter;
    std::uint32_t rng = 0x608u;
};
}
