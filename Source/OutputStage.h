// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <array>

namespace lr608
{
struct StereoSample { double left = 0.0, right = 0.0; };

// Global transient controls; zero Attack/Sustain reproduces TEST34.1 exactly.
// Focus: 0 = Low (kick/tom body), 1 = Full (entire spectrum).
struct TransientControls
{
    double attackPercent = 0.0;
    double sustainPercent = 0.0;
    double speedMs = 40.0;
    int focus = 0;
};

class OutputStage
{
public:
    // 32 stereo buses are exposed to the host: 64 output channels.
    static constexpr int stemCount = 32;
    void prepare (double sampleRate);
    void reset();
    // 0% is the historical, uncoloured output. Warmth applies separately to
    // every output pair; it does not change the per-Slot routing.
    void process (std::array<StereoSample, stemCount>& stems, bool multichannel,
                  double masterDb, int zapEngine, double warmthPercent = 0.0,
                  TransientControls transient = {});

private:
    struct DcState { double xL {}, yL {}, xR {}, yR {}; };
    StereoSample blockDc (StereoSample input, DcState&);
    double sr = 44100.0;
    double dcCoefficient = 0.999;
    double glueEnvelope = 0.0;
    double glueAttack = 132.0, glueRelease = 882.0;
    double cachedMasterDb = 1.0e30, cachedMasterGain = 1.2;
    // Low-band-only harmonic generator: no boost of the bass fundamental.
    // State is per output *and* per stereo channel to avoid crosstalk.
    struct WarmChannel { double lp1 {}, lp2 {}, dcMean {}; };
    struct WarmPair { WarmChannel left, right; };
    double warmthLpCoefficient = 0.03;
    double warmthDcCoefficient = 0.998;
    double warmthSmoothingCoefficient = 0.001;
    double smoothedWarmth = 0.0;
    std::array<WarmPair, stemCount> warmState {};
    static double warmHarmonics (double input, WarmChannel& state,
                                 double lowPassCoefficient, double dcCoefficient);
    // Differential-envelope transient shaper, per stereo bus with linked L/R
    // detection; Low focus modulates only a complementary low-band wet delta.
    struct TransientPair
    {
        double lowL1 {}, lowL2 {}, lowR1 {}, lowR2 {};
        double fast {}, slow {}, gain = 1.0;
    };
    std::array<TransientPair, stemCount> transientState {};
    double transientLowCoefficient = 0.03;
    double transientControlCoefficient = 0.001;
    double transientGainAttackCoefficient = 0.08;
    double transientGainReleaseCoefficient = 0.015;
    double transientAttack = 0.0, transientSustain = 0.0;
    double transientFocus = 0.0;
    double transientFastAttack = 0.0, transientFastRelease = 0.0;
    double transientSlowAttack = 0.0, transientSlowRelease = 0.0;
    double cachedTransientSpeed = -1.0;
    bool transientWasActive = false;
    void updateTransientSpeed (double speedMs);
    void shapeTransient (StereoSample& signal, TransientPair& state);
    DcState stereoDc;
    std::array<DcState, stemCount> stemDc {};
};
}
