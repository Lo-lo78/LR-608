// SPDX-License-Identifier: AGPL-3.0-or-later
#include "CowbellVoice.h"
#include <algorithm>
#include <cmath>
namespace lr608 {
namespace {
constexpr double pi = 3.14159265358979323846;
double rnd(std::uint32_t &r) {
  r ^= r << 13;
  r ^= r >> 17;
  r ^= r << 5;
  return r / double(0xffffffffu);
}
double sat(double x, double a) { return x / (1 + std::abs(x) * a); }
} // namespace
void CowbellVoice::Mode::init(double hz, double decay, double amp, double sr,
                              bool driven) {
  hz = std::clamp(hz, 20.0, sr * .45);
  decay = std::max(.004, decay);
  baseTheta = 2 * pi * hz / sr;
  rad = std::pow(.001, 1 / (decay * sr));
  c1 = 2 * rad * std::cos(baseTheta);
  c2 = -(rad * rad);
  gain = amp;
  y2 = 0;
  y1 = driven ? 0 : amp * std::sin(baseTheta);
}
void CowbellVoice::Mode::pitch(double mult) {
  c1 = 2 * rad * std::cos(std::min(pi * .90, baseTheta * std::max(.25, mult)));
}
double CowbellVoice::Mode::tick(double input) {
  const auto y = input * gain + c1 * y1 + c2 * y2;
  y2 = y1;
  y1 = y;
  return y;
}
void CowbellVoice::SaikeCow::reset(int ty, const CowbellParameters &p,
                                   double ac, double sampleRate) {
  type = std::clamp(ty, 0, 3);
  sr = sampleRate;
  const auto tune = std::clamp(p.v[2] / 4, 0.0, 1.0), decay = p.v[1],
             ratio = p.v[3], sharp = p.v[5];
  click = p.v[4];
  metal = p.v[6];
  ring = p.v[7];
  bite = p.v[8];
  snap = p.v[9];
  accent = ac;
  const auto tuning = std::pow(2.0, tune - .5),
             ratioMul = std::pow(2.0, (std::clamp(ratio, 0.0, 1.0) - .5) * 2);
  phase1 = phase2 = 0;
  state = state2 = state3 = atk = clickEnv = snapEnv = 1;
  pulse = .005 * sr;
  pulse2 = .02 * sr * (.25 + std::clamp(decay, 0.0, 1.0));
  const auto clickS = .00015 + (1 - std::clamp(sharp / 5, 0.0, 1.0)) * .004;
  kClick = std::pow(10.0, -60 / (20 * std::max(.00005, clickS) * sr));
  kSnap = std::pow(10.0, -60 / (20 * .006 * sr));
  for (auto &x : f)
    x = {};
  double ts;
  if (type == 0) {
    ts = .15 * (.5 + decay);
    k = std::pow(10.0, -24 / (20 * ts * sr));
    k2 = std::pow(10.0, -24 / (30 * ts * sr));
    k3 = std::pow(10.0, -24 / (50 * ts * sr));
    kAtk = std::pow(10.0, -24 / (20 * .003 * sr));
    dt1 = 487 * tuning / sr;
    dt2 = 1033 * tuning * ratioMul / sr;
    const double hz[]{487 * tuning,  1033 * tuning * ratioMul,
                      1496,          2000 * tuning,
                      2300 * tuning, 3000 * tuning,
                      1500 * tuning},
        q[]{.002, .01, .06, 1, .001, .001, 1};
    for (int i = 0; i < 7; ++i)
      f[i].init(hz[i], q[i], sr);
  } else if (type == 1) {
    ts = .036 * (.25 + 1.25 * decay);
    pulse = .002 * sr * (.5 + decay);
    dt1 = 848.3 * tuning / sr;
    dt2 = 563.2 * tuning * ratioMul / sr;
    state = 1;
    state2 = .15;
    state3 = 0;
    k = std::pow(10.0, -24 / (20 * ts * sr));
    k2 = k3 = std::pow(10.0, -24 / (20 * 10 * ts * sr));
    kAtk = std::pow(10.0, -24 / (20 * .0002 * sr));
    f[0].init(709.3 * tuning, .4, sr);
  } else if (type == 2) {
    ts = .15 * (.5 + decay);
    k = std::pow(10.0, -24 / (20 * ts * sr));
    k2 = std::pow(10.0, -24 / (30 * ts * sr));
    k3 = std::pow(10.0, -24 / (50 * ts * sr));
    kAtk = std::pow(10.0, -24 / (20 * .003 * sr));
    dt1 = 387 * tuning / sr;
    dt2 = 430 * tuning * ratioMul / sr;
    const double hz[]{387 * tuning,  430 * tuning * ratioMul,
                      802,           1281 * tuning * ratioMul,
                      2596 * tuning, 3172 * tuning,
                      4400 * tuning},
        q[]{.002, .001, .006, .05, .001, .001, .1};
    for (int i = 0; i < 7; ++i)
      f[i].init(hz[i], q[i], sr);
  } else {
    ts = .15 * (.5 + decay);
    dt1 = (600 + 4200 * tune) / sr;
    dt2 = .62 * dt1 * ratioMul;
    state = 1;
    state2 = 2 * pi * 3600 / sr;
    state3 = 0;
    k = std::pow(10.0, -24 / (20 * ts * sr));
    k2 = k3 = std::pow(10.0, -24 / (std::max(.001, .05 * ts) * sr));
    kAtk = std::pow(10.0, -24 / (20 * .003 * sr));
    f[2].init(802 + 400 * tune, .0006, sr);
  }
  alive = true;
}
double CowbellVoice::SaikeCow::tick(std::uint32_t &r) {
  if (!alive)
    return 0;
  atk *= kAtk;
  clickEnv *= kClick;
  snapEnv *= kSnap;
  --pulse;
  --pulse2;
  const auto b = 1 + std::clamp(accent, 0.0, 1.0) *
                         std::clamp(bite, 0.0, 10.0) * .12,
             sm = 1 + std::max(0.0, snap) * snapEnv *
                          (.5 + .5 * std::clamp(accent, 0.0, 1.0)),
             md = std::clamp(metal, 0.0, 1.0) - .5;
  phase1 += dt1 * sm;
  phase2 += dt2 * sm;
  phase1 -= std::floor(phase1);
  phase2 -= std::floor(phase2);
  const auto clk =
                 click > 0 ? (2 * rnd(r) - 1) * clickEnv * click * .025 * b : 0,
             chirp = snap > 0
                         ? .04 * snap * snapEnv * std::sin(2 * pi * phase1) * b
                         : 0,
             rm = ring > 0
                      ? std::sin(2 * pi * phase1) * std::sin(2 * pi * phase2) *
                            ring * .18 * state * b
                      : 0;
  double y = 0;
  if (type == 1) {
    if (pulse < 0) {
      state *= k;
      state2 *= k2;
    } else
      state = 1 - atk;
    const auto saw1 = 2 * phase1 - 1, saw2 = 2 * phase2 - 1,
               sum = (state + state2) / (1 + state + state2),
               body = .5 * saw1 * (1 - md * .8) +
                      3 * f[0].bp(saw1) * (1 + md * 1.2) + saw2 * (1 + md * .4);
    y = .35 * body * sum;
  } else if (type == 3) {
    if (pulse2 < 0) {
      state *= k;
      state2 *= k2;
    }
    const auto body = .1 * std::sin(2 * pi * phase1) *
                      (.5 + std::sin(2 * pi * phase2)) * state *
                      (1 + .2 * rnd(r)),
               noise = .04 * (1 - atk) * f[2].bp(rnd(r) - .5) * state;
    y = body * (1 - md * .7) + noise * (1 + md * 1.1);
  } else {
    if (pulse2 < 0) {
      state *= k;
      state2 *= k2;
      state3 *= k3;
    }
    const auto n = 2 * rnd(r) - 1;
    double low, high;
    if (type == 0) {
      low = 1.4 * state3 * f[0].bp(pulse > 0 ? 1 : 0) +
            3 * f[2].bp(state3) * state + f[6].lp(n) * state;
      high = .3 * f[1].bp(n) * state2 + .4 * f[3].bp(n) * state3 * state3 +
             .02 * f[4].bp(n) * state3 + .01 * f[5].bp(n) * state2;
      y = .15 * (1 - atk) * (low * (1 - md * .7) + high * (1 + md * 1.1));
    } else {
      low = 1.4 * state3 * f[0].bp(pulse > 0 ? 1 : 0) +
            .3 * f[2].bp(state) * state + 6 * f[6].bp(state) * state;
      high = .9 * f[1].bp(state3) * state2 +
             2.8 * f[3].bp(state + .1 * n) * state3 * state3 +
             .2 * f[4].bp(state) * state3 + .2 * f[5].bp(state) * state2;
      y = .24 * (1 - atk) * (low * (1 - md * .7) + high * (1 + md * 1.1));
    }
  }
  y += clk + chirp + rm;
  alive = (state > .00002 || state2 > .00002 ||
           ((type != 1 && type != 3) && state3 > .00002) ||
           (click > 0 && clickEnv > .00002) || (snap > 0 && snapEnv > .00002));
  return alive ? y : 0;
}

void CowbellVoice::Timbale::reset(const CowbellParameters &p, double ac,
                                  double velocity, double sampleRate) {
  sr = sampleRate;
  tune = p.v[2];
  metal = std::clamp(p.v[6], 0.0, 1.0);
  ring = std::clamp(p.v[7], 0.0, 1.0);
  snap = p.v[9];
  accent = ac;
  sat = p.v[10];
  velTune = std::pow(
      2.0, (-.5 * std::pow(1 - std::clamp(velocity, 0.0, 1.0), 1.55)) / 12);
  const auto f0 =
                 220 * std::pow(2.0, std::clamp(tune, 0.0, 4.0) * .5) * velTune,
             stretch = (std::clamp(p.v[3], 0.0, 1.0) - .5) * .055,
             base = .045 + .52 * std::pow(std::clamp(p.v[1], 0.0, 1.0), 1.25) +
                    .85 * std::pow(std::clamp(p.v[1], 0.0, 1.0), 3),
             bt = std::clamp(p.v[8], 0.0, 10.0) * std::clamp(ac, 0.0, 1.0);
  double ratios[]{1,
                  1.5933405057,
                  2.1355487866,
                  2.2954172674,
                  2.6530664045,
                  2.9172954551,
                  3.1554648154,
                  3.5001474903,
                  3.5984846740,
                  3.6474511791,
                  4.0589318833,
                  4.1317381597};
  for (int i = 1; i < 12; ++i)
    ratios[i] *= 1 + stretch * (ratios[i] - 1);
  for (int i = 0; i < 12; ++i) {
    const auto amp = .040 * std::abs(std::sin(pi * (i + 1) * .275)) /
                     std::pow(ratios[i], .72) * (1 + bt * (i * .01)),
               dec = base / std::pow(ratios[i], .52);
    m[i].init(f0 * ratios[i], dec, amp, sr);
  }
  const auto sd = .018 + base * (.18 + .30 * metal);
  shell[0].init(f0 * 5.43, sd, .020 * metal * (.35 + .65 * ring), sr);
  shell[1].init(f0 * 7.17, sd * .78, .014 * metal * (.25 + .75 * ring), sr);
  shell[2].init(f0 * 9.31, sd * .62, .010 * metal * (.20 + .80 * ring), sr);
  pitchSemi = std::clamp(p.v[4], 0.0, 10.0) * 2.4;
  pitchEnv = 1;
  auto pt = .00022 + (std::clamp(p.v[5], 0.0, 5.0) / 5) * .00155;
  pt *= .30 + 4.70 * std::pow(std::clamp(velocity, 0.0, 1.0), 2.15);
  pt = std::clamp(pt, .00012, .009);
  pitchSamples = std::max(1.0, pt * sr);
  pitchElapsed = pitchCounter = 0;
  pitchDone = pitchSemi <= .0001;
  lastBodyPitch = lastShellPitch = 1;
  if (!pitchDone) {
    const auto pm = std::pow(2.0, pitchSemi / 12);
    for (auto &x : m)
      x.pitch(pm);
    lastBodyPitch = pm;
  }
  stickEnv = 1;
  stickK = std::pow(.001, 1 / (.001 * sr));
  stickPhase = 0;
  stickFreq = std::min(sr * .22, 3300 + f0 * 1.35);
  stickStep = stickFreq / sr;
  stickNoisePrev = 0;
  snapEnv = 1;
  snapK = std::pow(
      .001,
      1 / (std::max(.0015, .004 + .006 * std::clamp(snap, 0.0, 3.0)) * sr));
  snapPhase = 0;
  const auto clampedSnap = std::clamp(snap, 0.0, 3.0);
  snapBaseStep = f0 / sr;
  snapModDepth = clampedSnap * .45 *
                 (.4 + .6 * std::clamp(accent, 0.0, 1.0));
  snapGain = .020 * clampedSnap;
  shellGain = .55 + .85 * ring;
  saturationMix = std::clamp(sat, 0.0, 1.0);
  saturationDrive = 1.25 + .8 * metal +
                    .4 * std::clamp(accent, 0.0, 1.0);
  tailEnv = 1;
  tailK = std::pow(.001, 1 / (std::max(.06, base * 1.18) * sr));
  vca = 1;
  vcaFloor = .68;
  vcaStep = (1 - vcaFloor) / std::max(1.0, std::max(.090, base * 1.35) * sr);
  inverseVcaRange = 1 / std::max(.0001, 1 - vcaFloor);
  slowCounter = 0;
  slowSemi = -(.08 + .72 * std::pow(std::clamp(velocity, 0.0, 1.0), 1.45));
  slowMult = 1;
  lp1 = lp2 = 0;
  lpA = std::exp(
      -2 * pi *
      std::min(sr * .45,
               450 + 17000 * std::pow(std::clamp(velocity, 0.0, 1.0), 2.15)) /
      sr);
  lpMix = 1 - lpA;
  alive = true;
}
double CowbellVoice::Timbale::tick(std::uint32_t &r) {
  if (!alive)
    return 0;
  if (!pitchDone) {
    ++pitchElapsed;
    --pitchCounter;
    if (pitchCounter <= 0) {
      if (pitchElapsed < pitchSamples) {
        pitchEnv =
            1 - std::pow(std::min(1.0, pitchElapsed / pitchSamples), 1.70);
        const auto pm = std::pow(2.0, pitchSemi * pitchEnv / 12) * slowMult;
        if (pm != lastBodyPitch) {
          for (auto &x : m)
            x.pitch(pm);
          lastBodyPitch = pm;
        }
        pitchCounter = 8;
      } else {
        pitchEnv = 0;
        if (slowMult != lastBodyPitch) {
          for (auto &x : m)
            x.pitch(slowMult);
          lastBodyPitch = slowMult;
        }
        pitchDone = true;
      }
    }
  }
  double head = 0, shellOut = 0;
  for (auto &x : m)
    head += x.tick();
  for (auto &x : shell)
    shellOut += x.tick();
  stickEnv *= stickK;
  stickPhase += stickStep;
  stickPhase -= std::floor(stickPhase);
  const auto noise = 2 * rnd(r) - 1,
             stick = (std::sin(2 * pi * stickPhase) * .45 +
                      (noise - stickNoisePrev) * .55) *
                     stickEnv;
  stickNoisePrev = noise;
  snapEnv *= snapK;
  snapPhase += snapBaseStep * (1 + snapModDepth * snapEnv);
  snapPhase -= std::floor(snapPhase);
  const auto sn = std::sin(2 * pi * snapPhase) * snapEnv * snapGain;
  auto y = head + shellOut * shellGain + stick + sn;
  if (saturationMix > 0) {
    y = y * (1 - saturationMix) +
        y * saturationDrive / (1 + std::abs(y * saturationDrive)) * saturationMix;
  }
  lp1 += (y - lp1) * lpMix;
  lp2 += (lp1 - lp2) * lpMix;
  y = lp2;
  vca = std::max(vcaFloor, vca - vcaStep);
  y *= vca;
  if (--slowCounter <= 0) {
    const auto prog = std::clamp((1 - vca) * inverseVcaRange, 0.0, 1.0);
    slowMult = std::pow(2.0, slowSemi * prog / 12);
    const auto pm = pitchDone
                        ? slowMult
                        : std::pow(2.0, pitchSemi * pitchEnv / 12) * slowMult;
    if (pm != lastBodyPitch) {
      for (auto &x : m)
        x.pitch(pm);
      lastBodyPitch = pm;
    }
    const auto sp = std::pow(2.0, slowSemi * .5 * prog / 12);
    if (sp != lastShellPitch) {
      for (auto &x : shell)
        x.pitch(sp);
      lastShellPitch = sp;
    }
    slowCounter = 64;
  }
  tailEnv *= tailK;
  alive = tailEnv > .00003 || stickEnv > .00003 || snapEnv > .00003;
  return alive ? y : 0;
}

void CowbellVoice::CapturedTimbale::reset(const CowbellParameters &p, double vel,
                                              int midiNote, double sampleRate) {
  sr = sampleRate;
  velocity = std::clamp(vel, 0.0, 1.0);
  age = 0;
  const auto &q = p.captured;
  const auto semis = (midiNote - q[12]) + q[0];
  ratio = std::pow(2.0, semis / 12.0);
  bodyLevel = q[1] / 100.0;
  decayScale = std::max(.05, q[2] / 100.0);
  tone = q[3];
  attackLevel = q[4] / 100.0;
  noiseDecay = std::max(.0001, q[5] / 1000.0);
  slapDecay = std::max(.0001, q[6] / 1000.0);
  metalLevel = q[7] / 100.0;
  metalFreq = q[8];
  metalDecay = std::max(.0001, q[9] / 1000.0);
  saturation = q[10] / 100.0;
  outGain = std::pow(10.0, q[11] / 20.0);
  noiseMix = q[43] / 100.0;
  slapMix = q[44] / 100.0;
  metalMix = q[45] / 100.0;
  velMin = q[46] / 100.0;
  velSens = q[47] / 100.0;
  preDrive = q[48] / 100.0;
  finalLevel = q[49] / 100.0;
  velTone = q[50] / 100.0;
  velAttack = q[51] / 100.0;
  velDecay = q[52] / 100.0;
  velPitch = q[53] / 100.0;
  velMetal = q[54] / 100.0;
  postVelAmount = q[55] / 100.0;
  postVelCurve = q[56];
  for (int i=0;i<10;++i) {
    freq[i] = q[13+i];
    level[i] = q[23+i] / 100.0;
    decay[i] = q[33+i] / 1000.0;
    phase[i] = std::fmod(i*.071 + (velocity*127.0)*.00091 + midiNote*.0017, 1.0);
  }
  noiseEnv = 1; noisePrev = 0; slapPhase = 0; metalPhase = 0;
  alive = true;
}

double CowbellVoice::CapturedTimbale::tick(std::uint32_t &r) {
  if (!alive) return 0;
  const auto v2 = velocity*velocity;
  const auto highBoost = 1.0 + v2*velTone*.75;
  const auto lowSoften = 1.0 - v2*velTone*.18;
  const auto attackVel = 1.0 + v2*velAttack*1.25;
  const auto metalVel = 1.0 + v2*velMetal;
  const auto velDecayScale = std::max(.60, 1.0-v2*velDecay*.22);
  const auto pitchFlex = 1.0 + v2*velPitch*.018;
  const auto toneLo = 1.0 - std::max(0.0,tone)*.35;
  const auto toneHi = 1.0 + std::max(0.0,tone)*.85;
  const auto toneDarkHi = 1.0 - std::max(0.0,-tone)*.65;
  double low=0, high=0;
  for(int i=0;i<10;++i){
    const auto hz=freq[i]*ratio*pitchFlex;
    phase[i]+=hz/sr; phase[i]-=std::floor(phase[i]);
    const auto env=std::exp(-age/std::max(1.0e-6,decay[i]*decayScale*velDecayScale));
    const auto y=std::sin(2*pi*phase[i])*level[i]*env;
    if(i<4)low+=y;else high+=y;
  }
  auto body=(low*toneLo*lowSoften + high*toneHi*toneDarkHi*highBoost)*bodyLevel;
  const auto n=2.0*rnd(r)-1.0;
  const auto hp=n-noisePrev*.92; noisePrev=n;
  const auto transient=hp*noiseEnv*attackLevel*noiseMix*attackVel;
  noiseEnv*=std::exp(-1.0/(noiseDecay*sr));
  const auto slapEnv=std::exp(-age/slapDecay);
  const auto slap=std::sin(2*pi*phase[3])*slapEnv*slapMix*attackLevel*attackVel;
  metalPhase+=(metalFreq*ratio)/sr; metalPhase-=std::floor(metalPhase);
  const auto pingEnv=std::exp(-age/metalDecay);
  const auto ping=std::sin(2*pi*metalPhase)*pingEnv*metalMix*metalLevel*metalVel;
  auto y=body+transient+ping+slap;
  const auto amp=velMin+velSens*velocity;
  y*=amp*preDrive;
  y=y/(1.0+std::abs(y)*saturation);
  const auto postVel=(1.0-postVelAmount)+postVelAmount*std::pow(std::max(velocity,.001),postVelCurve);
  y*=postVel*outGain*finalLevel;
  age+=1.0/sr;
  double maxDecay=0; for(auto d:decay)maxDecay=std::max(maxDecay,d);
  const auto stopTime=std::max({maxDecay*decayScale*7.0,metalDecay*7.0,.10});
  alive=age<=stopTime || noiseEnv>=.00001;
  return alive?y:0;
}

void CowbellVoice::Mesh::reset(const CowbellParameters &p, double ac,
                               double velocity, double sampleRate) {
  sr = sampleRate;
  metal = std::clamp(p.v[6], 0.0, 1.0);
  ring = std::clamp(p.v[7], 0.0, 1.0);
  accent = ac;
  sat = p.v[10];
  snap = p.v[9];
  mem.fill(0);
  count = 0;
  for (int y = 0; y < 9; ++y)
    for (int x = 0; x < 9; ++x) {
      const auto dx = x - 4, dy = y - 4;
      if (dx * dx + dy * dy <= 12.96)
        nodes[count++] = y * 9 + x;
    }
  prev = 0;
  cur = 128;
  next = 256;
  const auto vel = std::clamp(velocity, 0.0, 1.0),
             velTune = std::pow(2.0, (-1.5 * std::pow(1 - vel, 1.55)) / 12),
             f0 = 220 * std::pow(2.0, std::clamp(p.v[2], 0.0, 4.0) * .5) *
                  velTune,
             base = .045 + .52 * std::pow(std::clamp(p.v[1], 0.0, 1.0), 1.25) +
                    .85 * std::pow(std::clamp(p.v[1], 0.0, 1.0), 3);
  lambdaBase = std::clamp(10 * f0 / sr, .018, .60);
  anis = (1 - std::clamp(p.v[3], 0.0, 1.0)) * .14;
  anisX = 1 + anis;
  anisY = 1 - anis;
  damp = std::clamp(13.815510558 / std::max(1.0, base * sr), .000002, .018);
  waveCurrent = 2 - damp;
  wavePrevious = 1 - damp;
  meshInputGain = .10 + .90 * ring;
  pitchSemi = std::clamp(p.v[4], 0.0, 10.0) * 2.4;
  auto pt = .00022 + (std::clamp(p.v[5], 0.0, 5.0) / 5) * .00155;
  pt *= .30 + 4.70 * std::pow(vel, 2.15);
  pitchSamples = std::max(1.0, std::clamp(pt, .00012, .009) * sr);
  pitchElapsed = pitchCounter = 0;
  pitchEnv = 1;
  pitchDone = pitchSemi <= .0001;
  snapEnv = 1;
  snapPitchAmount = std::clamp(p.v[9], 0.0, 3.0) * .018;
  snapK = std::pow(
      .001,
      1 / (std::max(.0012, .0025 + .0014 * std::clamp(p.v[9], 0.0, 3.0)) * sr));
  excPhase = 0;
  excStep = 1 / std::max(4.0, sr * (.00110 - .00065 * std::pow(vel, .70)));
  excLevel = .36 + .64 * vel;
  excBite = std::clamp(p.v[8] / 10, 0.0, 1.0) * std::clamp(ac, 0.0, 1.0);
  vca = 1;
  vcaFloor = .68;
  vcaStep = (1 - vcaFloor) / std::max(1.0, std::max(.090, base * 1.35) * sr);
  slowCounter = 0;
  slowSemi = -(.08 + .72 * std::pow(vel, 4.5));
  slowMult = 1;
  const auto sd = .016 + base * (.16 + .28 * metal);
  shell[0].init(f0 * 5.18, sd, .072, sr, true);
  shell[1].init(f0 * 7.06, sd * .76, .050, sr, true);
  shell[2].init(f0 * 9.42, sd * .58, .034, sr, true);
  shellGain = .35 + 1.65 * metal;
  saturationMix = std::clamp(sat, 0.0, 1.0);
  saturationDrive = 1.18 + .72 * metal +
                    .35 * std::clamp(accent, 0.0, 1.0);
  lpA = std::exp(-2 * pi *
                 std::min(sr * .45, 450 + 17000 * std::pow(vel, 2.15)) / sr);
  lpMix = 1 - lpA;
  lp1 = lp2 = dcX = dcY = 0;
  tailEnv = 1;
  tailK = std::pow(.001, 1 / (std::max(.08, base * 1.45) * sr));
  alive = true;
}
double CowbellVoice::Mesh::tick() {
  if (!alive)
    return 0;
  if (!pitchDone) {
    ++pitchElapsed;
    --pitchCounter;
    if (pitchCounter <= 0) {
      if (pitchElapsed < pitchSamples) {
        pitchEnv =
            1 - std::pow(std::min(1.0, pitchElapsed / pitchSamples), 1.70);
        pitchCounter = 4;
      } else {
        pitchEnv = 0;
        pitchDone = true;
      }
    }
  }
  vca = std::max(vcaFloor, vca - vcaStep);
  if (--slowCounter <= 0) {
    const auto prog =
        std::clamp((1 - vca) / std::max(.0001, 1 - vcaFloor), 0.0, 1.0);
    slowMult = std::pow(2.0, slowSemi * prog / 12);
    const auto sm = std::pow(2.0, slowSemi * .5 * prog / 12);
    for (auto &x : shell)
      x.pitch(sm);
    slowCounter = 64;
  }
  snapEnv *= snapK;
  const auto pm = (pitchDone ? 1.0 : std::pow(2.0, pitchSemi * pitchEnv / 12)) * slowMult,
             total = pm * (1 + snapPitchAmount * snapEnv),
             lam = std::clamp(lambdaBase * total, .010, .68),
             lam2 = lam * lam, lx = lam2 * anisX, ly = lam2 * anisY;
  double exc = 0;
  if (excPhase < 1) {
    const auto t = excPhase, sine = std::sin(pi * t), w = sine * sine;
    exc = (.72 * sine +
           (.20 + .28 * excBite) * std::sin(3 * pi * t)) *
          w * excLevel;
    excPhase += excStep;
  }
  for (int i = 0; i < count; ++i) {
    const auto idx = nodes[i], x = idx % 9, y = idx / 9;
    const auto cv = mem[cur + idx], pv = mem[prev + idx],
               left = x > 0 ? mem[cur + idx - 1] : 0,
               right = x < 8 ? mem[cur + idx + 1] : 0,
               up = y > 0 ? mem[cur + idx - 9] : 0,
               down = y < 8 ? mem[cur + idx + 9] : 0;
    mem[next + idx] = waveCurrent * cv - wavePrevious * pv +
                      lx * (left + right - 2 * cv) + ly * (up + down - 2 * cv);
  }
  mem[next + 39] += exc * .115;
  mem[next + 40] += exc * .044;
  mem[next + 30] += exc * .021;
  const auto raw =
      mem[next + 41] * .90 - mem[next + 49] * .25 + mem[next + 32] * .20;
  const auto old = prev;
  prev = cur;
  cur = next;
  next = old;
  const auto head = raw - dcX + .9975 * dcY;
  dcX = raw;
  dcY = head;
  const auto input = head * meshInputGain,
             shellOut = shell[0].tick(input) + shell[1].tick(input) +
                        shell[2].tick(input);
  auto out = head * 3 + shellOut * shellGain + exc * .045;
  if (saturationMix > 0) {
    out = out * (1 - saturationMix) +
          out * saturationDrive / (1 + std::abs(out * saturationDrive)) * saturationMix;
  }
  lp1 += (out - lp1) * lpMix;
  lp2 += (lp1 - lp2) * lpMix;
  out = lp2 * vca;
  tailEnv *= tailK;
  alive = tailEnv > .00003 || excPhase < 1 || std::abs(head) > .00002 ||
          std::abs(shellOut) > .00002;
  return alive ? out : 0;
}

double CowbellVoice::unitRandom() { return rnd(rng); }
double CowbellVoice::random() { return 2 * unitRandom() - 1; }
void CowbellVoice::prepare(double s) {
  sr = std::max(1.0, s);
  reset();
}
void CowbellVoice::reset() {
  active = false;
  silenceCount = 0;
  env = pitchEnv = clickEnv = phase1 = phase2 = satMem = 0;
  saike.alive = timbale.alive = mesh.alive = captured.alive = false;
}
void CowbellVoice::trigger(int e, int vel, const CowbellParameters &p, std::uint32_t randomSeed, int midiNote) {
  if (randomSeed != 0) rng = randomSeed;
  engine = std::clamp(e, 0, 7);
  velocity = vel / 127.0;
  const auto th = p.accentThreshold / 127.0;
  accent = velocity > th
               ? std::pow((velocity - th) / std::max(.001, 1 - th), 1.6) *
                     p.accentCharacter
               : 0;
  if (engine >= 1 && engine <= 4)
    saike.reset(engine - 1, p, accent, sr);
  else if (engine == 5)
    timbale.reset(p, accent, velocity, sr);
  else if (engine == 6)
    mesh.reset(p, accent, velocity, sr);
  else if (engine == 7)
    captured.reset(p, velocity, midiNote, sr);
  env = pitchEnv = clickEnv = 1;
  phase1 = phase2 = satMem = 0;
  active = true;
  silenceCount = 0;
}
double CowbellVoice::render(const CowbellParameters &p) {
  if (!active)
    return 0;
  if (engine >= 1 && engine <= 4) {
    auto y = saike.tick(rng) * p.v[0] * velocity * 4;
    y = sat(y, .35);
    if (!saike.alive)
      active = false;
    return y;
  }
  if (engine == 5) {
    auto y = timbale.tick(rng) * p.v[0] * velocity * 4.8;
    y = sat(y, .28) * 1.995262315;
    if (!timbale.alive)
      active = false;
    return y;
  }
  if (engine == 6) {
    auto y = mesh.tick() * p.v[0] * velocity * .3;
    y = sat(y, .30) * 1.995262315;
    if (!mesh.alive)
      active = false;
    return y;
  }
  if (engine == 7) {
    auto y = captured.tick(rng);
    if (!captured.alive)
      active = false;
    return y;
  }
  const auto dc = 1 - std::exp(-1 / ((.004 + p.v[1] * .050) * sr)),
             pdc = 1 - std::exp(-1 / ((.00025 + p.v[9] * .0022) * sr));
  env -= env * dc;
  if (env < .00001)
    env = 0;
  pitchEnv -= pitchEnv * pdc;
  if (pitchEnv < .00001)
    pitchEnv = 0;
  clickEnv = std::max(0.0, clickEnv - 1 / (.0011 * sr));
  const auto fast = std::pow(env, 3.8), mid = std::pow(env, 1.7),
             slow = std::pow(env, .78), focus = std::pow(env, 5.5),
             imp = clickEnv <= 0 ? 0.0 : (clickEnv > .5 ? 1.0 : -1.0);
  auto clk = imp * (1 + p.v[5] * 1.6) * p.v[4] * .18 * (1 + accent * .6);
  clk *= pitchEnv > .6 ? 1.25 + accent * .4 : 1;
  clk *= .72 + fast * .55;
  clk = sat(clk, .7);
  const auto base = 320 + p.v[2] * 2100, ratio = 1.12 + p.v[3] * .80,
             boost = pitchEnv > .65 ? 1 + accent * 3.8 : 1,
             snapPitch = 1 + pitchEnv * p.v[9] * (1.2 + accent * 4) * boost;
  phase1 += base * snapPitch / sr;
  phase2 += base * ratio * snapPitch / sr;
  phase1 -= std::floor(phase1);
  phase2 -= std::floor(phase2);
  const auto tri = phase1 < .5 ? phase1 * 4 - 1 : 3 - phase1 * 4,
             sq = phase2 < .5 ? 1.0 : -1.0,
             rm = tri * sq * p.v[7] *
                  (1 + accent * (2.2 + accent * p.v[8] * .35)),
             core = tri * (1 - p.v[6] * .55) + sq * (p.v[6] * .35),
             cavity = sq * .75 - tri * .58,
             met = rm + (sq - tri) * .12 * p.v[7],
             coreSig = core * (.78 + p.v[6] * .25) * mid,
             cavitySig = cavity * (.22 + p.v[6] * .38) * fast,
             metalSig = met * (.30 + p.v[7] * .90) * slow;
  auto body = (coreSig - cavitySig + metalSig) * (1 - accent * .42) +
              metalSig * accent * .18;
  if (p.v[10] > 0) {
    const auto sm = p.v[10],
               drive = 1.1 + accent * .8 + pitchEnv * accent + focus * .25,
               bs = body * drive / (1 + std::abs(body * drive));
    body = body * (1 - sm) + bs * sm;
  }
  const auto raw = clk + body + body * focus * (.10 + p.v[5] * .04);
  satMem += (raw - satMem) * .25;
  auto y = (raw + satMem * .35) * p.v[0] * velocity;
  y = sat(y, .40);
  if (!std::isfinite(y) || !std::isfinite(satMem)) {
    reset();
    return 0;
  }
  if (std::abs(y) < 1e-5 && std::abs(satMem) < 1e-5) ++silenceCount;
  else silenceCount = 0;
  if (silenceCount >= 2048) {
    active = false;
    satMem = 0;
    return 0;
  }
  if (env <= 0 && pitchEnv <= 0 && clickEnv <= 0 && std::abs(y) < .00003 &&
      std::abs(satMem) < .00003) {
    active = false;
    satMem = 0;
    return 0;
  }
  return y;
}
} // namespace lr608
