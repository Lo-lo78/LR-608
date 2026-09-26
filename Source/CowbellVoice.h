// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "SaikeMetal.h"
#include <array>
#include <cstdint>
namespace lr608 {
struct CowbellParameters {
  std::array<double, 11> v{};
  std::array<double, 57> captured{};
  double accentThreshold = 112, accentCharacter = 1;
};
class CowbellVoice {
public:
  void prepare(double);
  void reset();
  void trigger(int, int, const CowbellParameters &, std::uint32_t randomSeed = 0, int midiNote = 60);
  double render(const CowbellParameters &);
  bool isActive() const { return active; }

private:
  struct Mode {
    double c1 = 0, c2 = 0, y1 = 0, y2 = 0, baseTheta = 0, rad = 0, gain = 1;
    void init(double, double, double, double, bool = false);
    void pitch(double);
    double tick(double input = 0);
  };
  struct SaikeCow {
    void reset(int, const CowbellParameters &, double, double);
    double tick(std::uint32_t &);
    bool alive = false;
    int type = 0;
    double sr = 44100, state = 1, state2 = 1, state3 = 1, atk = 1, k = 0,
           k2 = 0, k3 = 0, kAtk = 0, phase1 = 0, phase2 = 0, dt1 = 0, dt2 = 0,
           pulse = 0, pulse2 = 0, clickEnv = 1, snapEnv = 1, kClick = 0,
           kSnap = 0, click = 0, metal = 0, ring = 0, bite = 0, snap = 0,
           accent = 0;
    std::array<MetalSvf, 7> f{};
  };
  struct Timbale {
    void reset(const CowbellParameters &, double, double, double);
    double tick(std::uint32_t &);
    bool alive = false;
    double sr = 44100, tune = 0, metal = 0, ring = 0, snap = 0, accent = 0,
           sat = 0, velTune = 1, pitchEnv = 1, pitchSamples = 1,
           pitchElapsed = 0, pitchSemi = 0, pitchCounter = 0, stickEnv = 1,
           stickK = 0, stickPhase = 0, stickFreq = 0, stickNoisePrev = 0,
           snapEnv = 1, snapK = 0, snapPhase = 0, tailEnv = 1, tailK = 0,
           vca = 1, vcaFloor = .68, vcaStep = 0, slowCounter = 0, slowSemi = 0,
           slowMult = 1, lp1 = 0, lp2 = 0, lpA = 0, lpMix = 0,
           stickStep = 0, snapBaseStep = 0, snapModDepth = 0, snapGain = 0,
           shellGain = 0, saturationMix = 0, saturationDrive = 1,
           inverseVcaRange = 1, lastBodyPitch = 1, lastShellPitch = 1;
    bool pitchDone = false;
    std::array<Mode, 12> m{};
    std::array<Mode, 3> shell{};
  };
  struct CapturedTimbale {
    void reset(const CowbellParameters &, double, int, double);
    double tick(std::uint32_t &);
    bool alive = false;
    double sr = 44100, velocity = 1, age = 0, ratio = 1;
    double bodyLevel=1, decayScale=1, tone=0, attackLevel=1;
    double noiseDecay=.0065, slapDecay=.0038, metalLevel=1, metalFreq=3675, metalDecay=.018;
    double saturation=.55, outGain=1, preDrive=1.35, finalLevel=.72;
    double noiseMix=.24, slapMix=.22, metalMix=.075, velMin=.30, velSens=.70;
    double velTone=.35, velAttack=.30, velDecay=.22, velPitch=.18, velMetal=.20;
    double postVelAmount=.65, postVelCurve=1.20;
    double noiseEnv=1, noisePrev=0, slapPhase=0, metalPhase=0;
    std::array<double,10> phase{}, freq{}, level{}, decay{};
  };
  struct Mesh {
    void reset(const CowbellParameters &, double, double, double);
    double tick();
    bool alive = false;
    double sr = 44100, metal = 0, ring = 0, accent = 0, sat = 0, snap = 0,
           lambdaBase = 0,
           anis = 0, damp = 0, excPhase = 0, excStep = 0, excLevel = 0,
           excBite = 0, pitchEnv = 1, pitchSamples = 1, pitchElapsed = 0,
           pitchSemi = 0, pitchCounter = 0, snapEnv = 1, snapK = 0, vca = 1,
           vcaFloor = .68, vcaStep = 0, slowCounter = 0, slowSemi = 0,
           slowMult = 1, lp1 = 0, lp2 = 0, lpA = 0, dcX = 0, dcY = 0,
           tailEnv = 1, tailK = 0, snapPitchAmount = 0,
           anisX = 1, anisY = 1, shellGain = 0, saturationMix = 0,
           saturationDrive = 1, lpMix = 0, waveCurrent = 2,
           wavePrevious = 1, meshInputGain = 1;
    bool pitchDone = false;
    std::array<double, 384> mem{};
    std::array<int, 81> nodes{};
    int prev = 0, cur = 128, next = 256, count = 0;
    std::array<Mode, 3> shell{};
  };
  double unitRandom(), random();
  double sr = 44100, velocity = 1, accent = 0, env = 0, pitchEnv = 0,
         clickEnv = 0, phase1 = 0, phase2 = 0, satMem = 0;
  int engine = 0, silenceCount = 0;
  bool active = false;
  std::uint32_t rng = 0x608c0b1u;
  SaikeCow saike;
  Timbale timbale;
  Mesh mesh;
  CapturedTimbale captured;
};
} // namespace lr608
