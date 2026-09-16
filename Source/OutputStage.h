// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <array>

namespace lr608
{
struct StereoSample { double left = 0.0, right = 0.0; };

class OutputStage
{
public:
    // 32 stereo buses are exposed to the host: 64 output channels.
    static constexpr int stemCount = 32;
    void prepare (double sampleRate);
    void reset();
    void process (std::array<StereoSample, stemCount>& stems, bool multichannel,
                  double masterDb, int zapEngine);

private:
    struct DcState { double xL {}, yL {}, xR {}, yR {}; };
    StereoSample blockDc (StereoSample input, DcState&);
    double sr = 44100.0;
    double dcCoefficient = 0.999;
    double glueEnvelope = 0.0;
    double glueAttack = 132.0, glueRelease = 882.0;
    double cachedMasterDb = 1.0e30, cachedMasterGain = 1.2;
    DcState stereoDc;
    std::array<DcState, stemCount> stemDc {};
};
}
