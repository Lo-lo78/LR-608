// SPDX-License-Identifier: AGPL-3.0-or-later
#include "KickOtherVoices.h"

#include <algorithm>
#include <cmath>

namespace lr608
{
namespace
{
constexpr double pi = 3.14159265358979323846;
double signum (double x) { return x < 0.0 ? -1.0 : x > 0.0 ? 1.0 : 0.0; }
double saikeTanh (double x) { return 2.0 / (1.0 + std::exp (-2.0 * x)) - 1.0; }
}

void KickOtherVoices::Svf::init (double frequency, double inverseQ, double sampleRate)
{
    frequency = std::clamp (frequency, 20.0, sampleRate * 0.45);
    const auto g = std::tan (pi * frequency / sampleRate);
    const auto k = std::max (0.015, inverseQ);
    a1 = 1.0 / (1.0 + g * (g + k)); a2 = g * a1; a3 = g * a2; ic1 = ic2 = 0.0;
}

double KickOtherVoices::Svf::bandPass (double input)
{
    const auto v3 = input - ic2;
    const auto v1 = a1 * ic1 + a2 * v3;
    const auto v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.0 * v1 - ic1; ic2 = 2.0 * v2 - ic2;
    return v1;
}

void KickOtherVoices::Bell::init (double frequency, double q, double gainDb, double sampleRate)
{
    const auto a = std::pow (10.0, gainDb / 40.0);
    const auto g = std::tan (pi * std::clamp (frequency, 20.0, sampleRate * 0.45) / sampleRate);
    const auto k = 1.0 / (std::max (0.05, q) * a);
    a1 = 1.0 / (1.0 + g * (g + k)); a2 = g * a1; a3 = g * a2;
    m1 = k * (a * a - 1.0); ic1 = ic2 = 0.0;
}

double KickOtherVoices::Bell::tick (double input)
{
    const auto v3 = input - ic2;
    const auto v1 = a1 * ic1 + a2 * v3;
    const auto v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.0 * v1 - ic1; ic2 = 2.0 * v2 - ic2;
    return input + m1 * v1;
}

double KickOtherVoices::Elliptic::tick (double x)
{
    static constexpr double b[] { 0.03974403712835188, 0.11443117839583584, 0.4102732984609602,
        0.8255281436307241, 1.6689828207164152, 2.5256753272317622, 3.6193770241123127,
        4.250403515943048, 4.641846929462009, 4.25040351594302, 3.6193770241123016,
        2.525675327231766, 1.6689828207164181, 0.8255281436307251,
        0.41027329846095995, 0.11443117839583594 };
    static constexpr double a[] { 1.2209793606380654, -6.918940386446262, 7.438409047076798,
        -20.47654014058037, 19.21733444638215, -33.69411950162771, 27.235417392156258,
        -33.46680351213294, 22.8021725145997, -20.29444701618275, 11.231790923026374,
        -7.173357397659418, 2.9956603900306376, -1.2866484319363045,
        0.3305293493933626, -0.07745428581611816 };
    auto y = b[0] * x + s[0];
    for (int i = 0; i < 15; ++i) s[i] = b[i + 1] * x + a[i] * y + s[i + 1];
    s[15] = 0.0397440371283519 * x + a[15] * y;
    return y;
}

void KickOtherVoices::Shifter::init (double shift, double sampleRate)
{
    const auto ratio = 48000.0 / sampleRate;
    const auto block = 200.0 * pi;
    dt1 = 2.0 * pi * (0.250 + 0.001 * ratio);
    dt2 = dt1 + 2.0 * pi * shift / sampleRate;
    if (t1 > block) t1 -= block; if (t2 > block) t2 -= block;
    coefficient1 = 2.0 * std::cos (dt1); t1 += dt1;
    cos11 = std::sin (-dt1 + t1); cos12 = std::sin (-2.0 * dt1 + t1);
    sin11 = -std::cos (-dt1 + t1); sin12 = -std::cos (-2.0 * dt1 + t1); t1 -= dt1;
    coefficient2 = 2.0 * std::cos (dt2); t2 += dt2;
    cos21 = std::sin (-dt2 + t2); cos22 = std::sin (-2.0 * dt2 + t2);
    sin21 = -std::cos (-dt2 + t2); sin22 = -std::cos (-2.0 * dt2 + t2); t2 -= dt2;
}

double KickOtherVoices::Shifter::tick (double x)
{
    t1 += dt1; t2 += dt2;
    const auto ct1 = coefficient1 * cos11 - cos12; cos12 = cos11; cos11 = ct1;
    const auto ct2 = coefficient2 * cos21 - cos22; cos22 = cos21; cos21 = ct2;
    const auto st1 = coefficient1 * sin11 - sin12; sin12 = sin11; sin11 = st1;
    const auto st2 = coefficient2 * sin21 - sin22; sin22 = sin21; sin21 = st2;
    return 2.0 * (l1.tick (x * ct1) * ct2 + l2.tick (x * st1) * st2);
}

void KickOtherVoices::prepare (double sampleRate) { sr = std::max (1.0, sampleRate); reset(); }

void KickOtherVoices::reset()
{
    active = false; env = pitchEnv = clickEnv = impulseEnv = noiseEnv = 0.0;
    phase = impulsePhase = lfoPhase = lfoEnv = lfoSmooth = 0.0;
    noiseHold = bodyLp = noiseLp1 = noiseLp2 = noiseLp3 = noiseLp4 = noiseDc = noiseCount = 0.0;
    dcX = dcY = rms = runningDb = 0.0; rng = 0x608u;
    pitchValue = pitchTime = ampValue = ampTime = saikeNoiseLevel = phase2 = lastY = postLp = 0.0;
    smoothCount = 0; linnPhase.fill(0);linnLp.fill(0);linnFilterEnv=linnBeaterLp1=linnBeaterLp2=linnAirLp1=linnAirLp2=linnDigitalPhase=linnDigitalHold=linnHitColor=linnSampleHold=0;linnHitDetune=1;
}

double KickOtherVoices::randomBipolar()
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (rng / double (0xffffffffu)) * 2.0 - 1.0;
}

void KickOtherVoices::trigger (int engine, int midiVelocity, const Kick808Parameters& p, std::uint32_t randomSeed)
{
    if (randomSeed != 0) rng = randomSeed;
    activeEngine = std::clamp (engine, 1, 7); active = true;
    velocity = std::clamp (midiVelocity / 127.0, 0.0, 1.0);
    const auto threshold = p.accentThreshold / 127.0;
    accent = velocity > threshold
        ? std::pow ((velocity - threshold) / std::max (0.001, 1.0 - threshold), 1.6) * p.accentCharacter : 0.0;
    env = pitchEnv = clickEnv = impulseEnv = noiseEnv = 1.0;
    phase = impulsePhase = lfoPhase = lfoEnv = lfoSmooth = 0.0;
    noiseHold = randomBipolar(); noiseCount = 0.0;
    bodyLp = noiseLp1 = noiseLp2 = noiseLp3 = noiseLp4 = noiseDc = 0.0;
    dcCoefficient = std::exp (-2.0 * pi * 5.0 / sr);
    rmsCoefficient = std::exp (-1.0 / (0.01 * sr));
    const auto db2log = std::log (10.0) / 20.0;
    compThresholdLinear = std::exp ((p.compThreshold - 3.0) * db2log);
    compAttackCoefficient = std::exp (-1.0 / (std::max (0.0001, p.compAttack * 0.001) * sr));
    compReleaseCoefficient = std::exp (-1.0 / (std::max (0.001, p.compRelease * 0.001) * sr));
    static constexpr int ratios[] { 4, 8, 12, 20, 20 };
    compRatioReduction = 1.0 - 1.0 / ratios[std::clamp (p.compRatio, 0, 4)];
    compMakeupGain = std::exp (p.compMakeup * db2log);
    if(activeEngine==7){const auto ph=(p.bodyPhase+1)*.5+.75;linnPhase={ph-std::floor(ph),ph*1.31+.09,ph*.73+.21,ph*1.67+.37};for(auto&x:linnPhase)x-=std::floor(x);linnFilterEnv=1;linnBeaterLp1=linnBeaterLp2=linnAirLp1=linnAirLp2=0;linnDigitalPhase=1;linnDigitalHold=0;linnLp.fill(0);linnHitDetune=1+randomBipolar()*.0025;linnHitColor=randomBipolar();linnSampleHold=randomBipolar();}
    else if (activeEngine >= 3) resetSaike (p);
}

double KickOtherVoices::finish (double signal, const Kick808Parameters& p)
{
    signal *= p.level * velocity * (activeEngine == 7 ? 0.96 : activeEngine >= 3 ? 1.270925175 : 1.0);
    const auto oldX = dcX; dcX = signal;
    dcY = dcX - oldX + dcCoefficient * dcY;
    signal = dcY;
    if (p.compMix > 0.000001)
    {
        const auto db2log = std::log (10.0) / 20.0;
        rms = signal * signal + rmsCoefficient * (rms - signal * signal);
        const auto over = std::max (0.0, (20.0 / std::log (10.0)) *
            std::log (std::max (std::sqrt (std::max (0.0, rms)), 1.0e-7) / compThresholdLinear));
        const auto coefficient = over > runningDb ? compAttackCoefficient : compReleaseCoefficient;
        runningDb += (over - runningDb) * (1.0 - coefficient);
        const auto gain = std::exp (-runningDb * compRatioReduction * db2log) * compMakeupGain;
        signal += (signal * gain - signal) * (p.compMix / 100.0);
    }
    else rms = runningDb = 0.0;
    return signal;
}

float KickOtherVoices::render (const Kick808Parameters& p, double tempo)
{
    if (! active) return 0.0f;
    if (activeEngine == 1) return renderSimmons (p);
    if (activeEngine == 2) return render909 (p);
    if (activeEngine == 7) return renderLinn (p, tempo);
    return renderSaike (p);
}

float KickOtherVoices::renderSimmons (const Kick808Parameters& p)
{
    pitchEnv *= std::exp (-1.0 / (std::max (0.004, p.punchTime) * sr));
    if (pitchEnv < 0.00001) pitchEnv = 0.0;
    const auto base = 30.0 + std::pow (std::max (0.0, p.tune), 1.65) * 410.0;
    const auto bend = p.punchAmount * (2.75 + accent * 0.42);
    const auto bendShape = 0.42 + std::max (0.0, p.curvePitchDecay) * 2.7;
    const auto secondBend = p.curvePitch * 0.82 * std::pow (pitchEnv, bendShape);
    lfoPhase += std::max (0.125, p.lfoFrequency) / sr; lfoPhase -= std::floor (lfoPhase);
    const auto shape = p.lfoShape;
    const auto triBase = 1.0 - 4.0 * std::abs (lfoPhase - 0.5);
    const double wave[] { std::sin ((lfoPhase + (shape - 0.5) * 0.22) * 2.0 * pi),
        signum (triBase) * std::pow (std::abs (triBase), 0.35 + shape * 1.7),
        lfoPhase * 2.0 - 1.0, 1.0 - lfoPhase * 2.0,
        lfoPhase < std::clamp (shape, 0.05, 0.95) ? 1.0 : -1.0, noiseHold };
    lfoSmooth += (wave[std::clamp (p.lfoWave, 0, 5)] - lfoSmooth) * (0.001 + (1.0 - p.lfoSmooth) * 0.075);
    lfoEnv += (1.0 - lfoEnv) * (1.0 - std::exp (-1.0 / (std::max (0.001, p.lfoDelay * 0.001) * sr)));
    const auto lfo = lfoSmooth * lfoEnv;
    auto frequency = base * std::pow (2.0, bend * std::pow (pitchEnv, 1.45) + secondBend + lfo * p.lfoPitchDepth * 0.055);
    frequency = std::clamp (frequency, 18.0, sr * 0.38);
    phase += frequency / sr; phase -= std::floor (phase);
    auto triangle = 1.0 - 4.0 * std::abs ((phase + (p.bodyPhase + 1.0) * 0.5
                                            - std::floor (phase + (p.bodyPhase + 1.0) * 0.5)) - 0.5);
    const auto bodyCut = std::min (sr * 0.42, 180.0 + base * 2.3);
    bodyLp += (triangle - bodyLp) * (2.0 * pi * bodyCut) / (sr + 2.0 * pi * bodyCut);
    auto body = bodyLp * 0.84 + triangle * 0.16; body /= 1.0 + std::abs (body) * 0.08;
    env -= env * (1.0 - std::exp (-1.0 / (std::max (0.008, p.decay) * sr)));
    if (env < 0.00001) env = 0.0;
    auto amplitude = std::pow (env, 0.52 + p.curve * 1.38) * (1.0 + accent * env * 0.38);
    amplitude *= std::max (0.0, 1.0 - lfo * p.lfoVolumeDepth * 0.18);
    const auto texture = std::clamp (std::abs (p.lfoVolumeDepth) * 0.36 + p.noiseTone * 0.18, 0.0, 1.0);
    if (--noiseCount <= 0.0) { noiseHold = randomBipolar(); noiseCount = std::max (1.0, std::floor (sr / (11.0 + std::pow (texture, 1.6) * 105.0))); }
    noiseLp4 += (noiseHold - noiseLp4) * (0.00055 + texture * 0.0035);
    noiseEnv *= std::exp (-1.0 / (std::max (0.003, p.noiseDecay * 0.001) * sr));
    if (noiseEnv < 0.00001) noiseEnv = 0.0;
    const auto white = randomBipolar(), tone = std::clamp (p.noiseTone * 0.5, 0.0, 1.0);
    auto highCut = (1200.0 + std::pow (tone, 1.25) * 12500.0) * (0.58 + 0.42 * std::sqrt (noiseEnv));
    highCut = std::clamp (highCut * (1.0 + noiseLp4 * texture * 0.09), 700.0, sr * 0.42);
    const auto onePole = [this] (double cutoff) { return (2.0 * pi * cutoff) / (sr + 2.0 * pi * cutoff); };
    noiseLp1 += (white - noiseLp1) * onePole (highCut);
    noiseLp2 += (white - noiseLp2) * onePole (420.0 + texture * 1900.0);
    noiseLp3 += (white - noiseLp3) * onePole (70.0 + texture * 310.0);
    const auto air = noiseLp1 - noiseLp2, flour = noiseLp2 - noiseLp3;
    const auto flourMix = 0.38 + texture * 0.44;
    auto coloured = air * (1.0 - flourMix * 0.66) + flour * flourMix;
    coloured *= 0.88 + noiseLp4 * texture * 0.12; coloured /= 1.0 + std::abs (coloured) * 0.16;
    noiseDc += (coloured - noiseDc) * 0.0012;
    const auto noise = (coloured - noiseDc) * noiseEnv * p.noiseLevel * 0.58 * (1.0 + accent * 0.35);
    impulseEnv *= std::exp (-1.0 / ((0.0007 + p.clickResonance * 0.0065) * sr));
    clickEnv *= std::exp (-1.0 / (0.0042 * sr));
    if (impulseEnv < 0.00001) impulseEnv = 0.0; if (clickEnv < 0.00001) clickEnv = 0.0;
    impulsePhase += (650.0 + p.clickTone * 900.0) / sr; impulsePhase -= std::floor (impulsePhase);
    const auto read = impulsePhase + (p.clickPhase + 1.0) * 0.5;
    const auto click = (std::sin ((read - std::floor (read)) * 2.0 * pi) * 0.58 + air * 0.42)
        * std::pow (impulseEnv, 2.0) * p.clickLevel * 0.54 * (1.0 + accent * 0.70);
    auto raw = body * amplitude * 1.06 + noise + click;
    const auto drive = 1.0 + std::pow (p.bodyDrive * 0.001, 1.25) * 7.5;
    const auto saturated = raw * drive / (1.0 + std::abs (raw * drive));
    raw = raw * (1.0 - p.bodyDriveMix * 0.72) + saturated * p.bodyDriveMix * 0.72;
    if (env <= 0.0 && pitchEnv <= 0.0 && clickEnv <= 0.0 && impulseEnv <= 0.0 && noiseEnv <= 0.0) active = false;
    return static_cast<float> (finish (active ? raw : 0.0, p));
}

float KickOtherVoices::render909 (const Kick808Parameters& p)
{
    pitchEnv *= std::exp (-1.0 / (std::max (0.001, p.punchTime) * sr));
    if (pitchEnv < 0.000001) pitchEnv = 0.0;
    const auto base = (35.0 + std::pow (std::max (0.0, p.tune), 1.35) * 96.0) * std::pow (2.0, p.curvePitch * 1.25);
    const auto depth = 0.10 + p.punchAmount * (3.15 + accent * 0.35);
    const auto tuneShape = 0.28 + std::max (0.0, p.curvePitchDecay) * 3.4;
    auto frequency = base * std::pow (2.0, depth * std::pow (pitchEnv, tuneShape)
        + accent * p.lfoPitchDepth * 0.085 * std::pow (pitchEnv, 1.6));
    frequency = std::clamp (frequency, 18.0, sr * 0.36);
    phase += frequency / sr; phase -= std::floor (phase);
    auto read = phase + (p.bodyPhase + 1.0) * 0.5; read -= std::floor (read);
    const auto triangle = 1.0 - 4.0 * std::abs (read - 0.5);
    const auto sine = std::sin (read * 2.0 * pi), rounded = std::sin (triangle * pi * 0.5);
    const auto parabolic = triangle * (1.5 - 0.5 * triangle * triangle);
    const auto hard = signum (triangle) * std::pow (std::abs (triangle), 0.56);
    const double waves[] { sine, rounded, triangle, hard, signum (triangle), parabolic };
    auto wave = waves[std::clamp (p.lfoWave, 0, 5)];
    const auto diode = std::clamp (p.lfoShape, 0.01, 0.99);
    wave = wave * (1.0 - diode * 0.52) + rounded * diode * 0.52;
    const auto harmonics = std::clamp (std::log (std::max (0.125, p.lfoFrequency) / 0.125) / std::log (8192.0), 0.0, 1.0);
    wave += (wave * wave - 0.34) * harmonics * 0.13;
    const auto cutoff = std::min (sr * 0.42, 260.0 + base * (4.0 + p.lfoSmooth * 5.0));
    bodyLp += (wave - bodyLp) * (2.0 * pi * cutoff) / (sr + 2.0 * pi * cutoff);
    const auto body = bodyLp * (0.76 + p.lfoSmooth * 0.18) + wave * (0.24 - p.lfoSmooth * 0.12);
    env *= std::exp (-1.0 / (std::max (0.004, p.decay) * (1.0 + accent * 0.07) * sr));
    if (env < 0.000001) env = 0.0;
    const auto attackNorm = std::clamp ((p.lfoVolumeDepth + 2.0) * 0.25, 0.0, 1.0);
    const auto bodyAttack = 0.00003 + std::pow (attackNorm, 2.2) * 0.018;
    lfoEnv += (1.0 - lfoEnv) * (1.0 - std::exp (-1.0 / (bodyAttack * sr)));
    const auto bodySignal = body * std::pow (env, 0.48 + p.curve * 2.15) * lfoEnv
        * (1.0 + accent * (0.22 + env * 0.30));
    const auto width = std::clamp ((p.lfoDelay - 100.0) / 4900.0, 0.0, 1.0);
    impulseEnv *= std::exp (-1.0 / ((0.00025 + p.clickResonance * p.clickResonance * 0.0075 + width * 0.004) * sr));
    clickEnv *= std::exp (-1.0 / ((0.0012 + p.clickResonance * 0.006) * sr));
    if (impulseEnv < 0.000001) impulseEnv = 0.0; if (clickEnv < 0.000001) clickEnv = 0.0;
    impulsePhase += (620.0 + p.clickTone * 920.0) / sr; impulsePhase -= std::floor (impulsePhase);
    auto clickRead = impulsePhase + (p.clickPhase + 1.0) * 0.5; clickRead -= std::floor (clickRead);
    const auto pulse = std::sin (clickRead * 2.0 * pi) * std::pow (impulseEnv, 1.65);
    const auto white = randomBipolar();
    const auto highCut = std::min (sr * 0.42, 900.0 + std::pow (std::max (0.0, p.noiseTone * 0.5), 1.15) * 10500.0);
    noiseLp1 += (white - noiseLp1) * (2.0 * pi * highCut) / (sr + 2.0 * pi * highCut);
    const auto midCut = 220.0 + p.clickResonance * 1500.0;
    noiseLp2 += (noiseLp1 - noiseLp2) * (2.0 * pi * midCut) / (sr + 2.0 * pi * midCut);
    noiseEnv *= std::exp (-1.0 / (std::max (0.001, p.noiseDecay * 0.001) * sr));
    if (noiseEnv < 0.000001) noiseEnv = 0.0;
    auto attack = (pulse * p.clickLevel * 0.58 + (noiseLp1 - noiseLp2) * noiseEnv * p.noiseLevel * 0.54)
        * (1.0 + accent * 0.62) * (0.82 + clickEnv * 0.18);
    auto raw = bodySignal + attack - body * std::pow (impulseEnv, 0.42) * p.clickLevel * 0.075;
    const auto drive = 1.0 + std::pow (p.bodyDrive * 0.001, 1.28) * 18.0;
    const auto driven = raw * drive;
    auto clipped = driven >= 0.0 ? driven / (1.0 + std::max (0.0, driven) * 0.86)
                                 : driven / (1.0 + std::max (0.0, -driven) * 0.72);
    clipped /= 1.0 + std::abs (clipped) * 0.18;
    raw = raw * (1.0 - p.bodyDriveMix) + clipped * p.bodyDriveMix;
    if (env <= 0.0 && pitchEnv <= 0.0 && clickEnv <= 0.0 && impulseEnv <= 0.0 && noiseEnv <= 0.0) active = false;
    return static_cast<float> (finish (active ? raw : 0.0, p));
}

float KickOtherVoices::renderLinn (const Kick808Parameters& p, double tempo)
{
    pitchEnv*=std::exp(-1/(std::max(.008,p.punchTime)*sr));if(pitchEnv<1e-6)pitchEnv=0;
    env*=std::exp(-1/(std::max(.035,p.decay)*(1+accent*.055)*sr));if(env<1e-6)env=0;
    const auto filterTime=.040+std::max(.035,p.decay)*.45+std::clamp(p.noiseDecay,1.0,500.0)*.0008;
    linnFilterEnv*=std::exp(-1/(filterTime*sr));if(linnFilterEnv<1e-6)linnFilterEnv=0;
    double lfo=0;
    if(std::abs(p.lfoVolumeDepth)>1e-6||std::abs(p.lfoPitchDepth)>1e-6){lfoEnv+=(1-lfoEnv)*(1-std::exp(-1/(std::max(.001,p.lfoDelay*.001)*sr)));const auto old=lfoPhase;lfoPhase+=std::max(.125,p.lfoFrequency)/(sr*60/std::max(1.0,tempo));lfoPhase-=std::floor(lfoPhase);if(lfoPhase<old)linnSampleHold=randomBipolar();const auto sh=std::clamp(p.lfoShape,.01,.99),triBase=lfoPhase<.5?lfoPhase*4-1:3-lfoPhase*4;const double wave[]{std::sin((lfoPhase+(sh-.5)*.35)*2*pi),signum(triBase)*std::pow(std::abs(triBase),.25+sh*3.75),std::clamp(lfoPhase*(1+sh)*2-1,-1.0,1.0),std::clamp(1-lfoPhase*(1+sh)*2,-1.0,1.0),lfoPhase<sh?1.0:-1.0,linnSampleHold};lfoSmooth+=(wave[std::clamp(p.lfoWave,0,5)]-lfoSmooth)*(1-std::exp(-1/((.00005+p.lfoSmooth*.02)*sr)));lfo=lfoSmooth*lfoEnv;}else lfoEnv=lfoSmooth=0;
    auto base=52*std::pow(2.0,p.tune*.72)*linnHitDetune;base=std::clamp(base,18.0,125.0);const auto bend=1+p.punchAmount*(.55+accent*.10)*std::pow(pitchEnv,1.05+p.curvePitchDecay*1.8);const auto curvePitch=p.curvePitch*.62*std::pow(env,.22+p.curvePitchDecay*3.2);auto fund=base*bend*std::pow(2.0,curvePitch+lfo*p.lfoPitchDepth*.15);fund=std::clamp(fund,12.0,sr*.18);const auto spread=std::clamp(.20+p.curve*.55,0.0,1.0);const double ratio[]{1,1.53+spread*.12+linnHitColor*.004,2.09+spread*.11-linnHitColor*.006,2.31+spread*.26+linnHitColor*.009};for(int i=0;i<4;++i){linnPhase[i]+=fund*ratio[i]/sr;linnPhase[i]-=std::floor(linnPhase[i]);}const auto curve=.54+p.curve*1.75,damping=.30+p.curvePitchDecay*1.5;auto body=std::sin(linnPhase[0]*2*pi)*std::pow(env,curve)*.82+std::sin(linnPhase[1]*2*pi)*std::pow(env,curve+.72+damping*.30)*(.22+spread*.12)+std::sin(linnPhase[2]*2*pi)*std::pow(env,curve+1.36+damping*.55)*(.13+spread*.09)+std::sin(linnPhase[3]*2*pi)*std::pow(env,curve+2.05+damping*.75)*(.075+spread*.065);body*=1+accent*(.18+env*.16);
    const auto white=randomBipolar();const auto coef=[](double f,double s){return 2*pi*f/(s+2*pi*f);};const auto beaterHi=std::min(sr*.42,650+p.clickTone*1050+p.noiseTone*1350),beaterLo=90+p.clickResonance*1850;linnBeaterLp1+=(white-linnBeaterLp1)*coef(beaterHi,sr);linnBeaterLp2+=(white-linnBeaterLp2)*coef(beaterLo,sr);const auto beaterBand=linnBeaterLp1-linnBeaterLp2;const auto airHi=360+p.noiseTone*420,airLo=48+p.clickResonance*105;linnAirLp1+=(white-linnAirLp1)*coef(airHi,sr);linnAirLp2+=(white-linnAirLp2)*coef(airLo,sr);noiseEnv*=std::exp(-1/(std::max(.001,p.noiseDecay*.001)*sr));if(noiseEnv<1e-6)noiseEnv=0;clickEnv*=std::exp(-1/((.0008+p.clickResonance*p.clickResonance*.028)*sr));if(clickEnv<1e-6)clickEnv=0;impulseEnv*=std::exp(-1/((.00065+p.clickResonance*.0032)*sr));if(impulseEnv<1e-6)impulseEnv=0;impulsePhase+=(520+p.clickTone*610)/sr;impulsePhase-=std::floor(impulsePhase);auto cp=impulsePhase+(p.clickPhase+1)*.5;cp-=std::floor(cp);const auto beaterTone=std::sin(cp*2*pi)*std::pow(impulseEnv,1.65-p.clickResonance*.65)*(.38+p.clickResonance*1.85);const auto beater=(beaterBand*(.76-p.clickResonance*.26)+beaterTone*(.24+p.clickResonance*.76))*clickEnv*p.clickLevel*.52*(1+accent*.58);const auto air=(linnAirLp1-linnAirLp2)*noiseEnv*p.noiseLevel*.42*(.88+linnHitColor*.08);auto raw=(body+beater+air)*std::max(.02,1+lfo*p.lfoVolumeDepth*.46);const auto driven=raw*(1+p.bodyDrive*.024),sat=driven/(1+std::abs(driven)*(.30+std::max(0.0,driven)*.035));raw=std::clamp(raw*(1-p.bodyDriveMix)+sat*p.bodyDriveMix,-1.0,1.0);
    linnDigitalPhase+=27000/sr;if(linnDigitalPhase>=1){linnDigitalPhase-=std::floor(linnDigitalPhase);const auto encoded=signum(raw)*std::pow(std::abs(raw),.58);auto q=std::floor((encoded*.5+.5)*255+.5)/255;q=std::clamp(q*2-1,-1.0,1.0);linnDigitalHold=signum(q)*std::pow(std::abs(q),1/.58);}const auto cutEnd=390+std::max(0.0,p.noiseTone)*360+p.curve*520,cutStart=cutEnd+1400+std::max(0.0,p.noiseTone)*5200+p.clickTone*210,cut=std::clamp(cutEnd+(cutStart-cutEnd)*std::pow(linnFilterEnv,.34+p.curve*.92),160.0,sr*.42),c=coef(cut,sr);linnLp[0]+=(linnDigitalHold-linnLp[0])*c;for(int i=1;i<4;++i)linnLp[i]+=(linnLp[i-1]-linnLp[i])*c;const auto out=linnLp[3]*.97+linnDigitalHold*.03;
    if(env<=0&&pitchEnv<=0&&clickEnv<=0&&impulseEnv<=0&&noiseEnv<=0){active=false;linnFilterEnv=0;}
    return static_cast<float>(finish(active?out:0,p));
}

void KickOtherVoices::resetSaike (const Kick808Parameters& p)
{
    const auto tf = 2302.58509299 / sr;
    pitchRise = tf * 0.5;
    pitchDecay = tf * 0.33 * std::exp (-4.605170185988092 * std::clamp (p.punchTime / 0.2, 0.0, 1.0));
    pitchAttackSamples = 0.01 * sr; pitchValue = pitchTime = 0.0;
    ampRise = tf;
    ampDecay = tf * 0.033 * std::exp (-4.605170185988092 * std::clamp (p.decay, 0.0, 1.0));
    ampAttackSamples = std::max (0.001, p.lfoDelay * 0.0001) * sr; ampValue = ampTime = 0.0;
    saikeNoiseLevel = 2.0;
    saikeNoiseDecay = tf * 13.0 * std::exp (-4.605170185988092 * std::clamp ((p.noiseDecay - 1.0) / 499.0, 0.0, 1.0));
    phase = (p.bodyPhase + 1.0) * 0.5; phase -= std::floor (phase);
    phase2 = (p.clickPhase + 1.0) * 0.5; phase2 -= std::floor (phase2);
    smoothCount = 5; lastY = postLp = 0.0;
    const auto noiseFrequency = 1900.0 * std::pow (2.0, (std::clamp (p.noiseTone, 0.0, 2.0) - 0.5) * 1.35);
    saikeNoiseFilter.init (noiseFrequency, 1.25 - std::clamp (p.clickResonance, 0.0, 1.0) * 0.95, sr);
    mudDip.init (480.0, 12.0, -50.0, sr);
    clickBoost.init (1800.0 + std::clamp (p.clickTone, 0.0, 6.0) * 650.0, 0.5,
                     std::clamp (p.clickLevel, 0.0, 4.0) * 3.15, sr);
    shifter.init (7.0, sr);
}

float KickOtherVoices::renderSaike (const Kick808Parameters& p)
{
    if (pitchTime < pitchAttackSamples) { pitchValue += pitchRise * (1.0 - pitchValue); pitchTime += 1.0; }
    else pitchValue -= pitchDecay * pitchValue;
    if (pitchValue < 0.0000001) pitchValue = 0.0;
    if (ampTime < ampAttackSamples) { ampValue += ampRise * (1.0 - ampValue); ampTime += 1.0; }
    else ampValue -= ampDecay * ampValue;
    if (ampValue < 0.0000001) ampValue = 0.0;
    const auto pitchCurve = std::pow (std::max (0.0, pitchValue), 0.5 + std::clamp (p.curvePitchDecay, 0.0, 1.0) * 1.5);
    const auto ampCurve = std::pow (std::max (0.0, ampValue), 0.55 + std::clamp (p.curve, 0.0, 1.0) * 1.5);
    const auto baseRatio = p.tune < 0.0 ? (20.0 * std::pow (2.0, std::max (-2.0, p.tune))) / 22050.0
                                       : (20.0 + 80.0 * std::min (1.0, p.tune)) / 22050.0;
    const auto sampleRatio = 48000.0 / sr;
    const auto phaseStep = 0.5 * std::exp ((1.0 - std::clamp (p.punchAmount * 0.5, 0.0, 0.5) * pitchCurve)
                                           * std::log (baseRatio)) * sampleRatio;
    const auto type = activeEngine - 3;
    double body = 0.0;
    if (type < 3)
    {
        phase += phaseStep; phase -= std::floor (phase);
        const auto triangle = phase <= 0.5 ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase;
        const auto offset = 0.02 + std::clamp (p.lfoShape, 0.0, 1.0) * 0.16;
        if (type == 0) body = (0.91 * (saikeTanh (2.0 * triangle + offset) - offset) + 0.1 * triangle) * ampCurve;
        else if (type == 1) { body = triangle * ampCurve; body = 0.91 * (saikeTanh (2.0 * body + offset) - offset) + 0.1 * body; }
        else body = saikeTanh (2.0 * triangle) * ampCurve;
        if (type != 2) { body = mudDip.tick (body); if (p.clickLevel > 0.000001) body = clickBoost.tick (body); }
        if (p.lfoWave >= 1) body += (shifter.tick (body) - body) * std::clamp (std::abs (p.curvePitch), 0.0, 1.0);
        if (type == 2) { body = mudDip.tick (body); body = clickBoost.tick (body); }
    }
    else
    {
        body = std::sin (2.0 * pi * phase) * ampCurve;
        phase2 += std::clamp (p.lfoFrequency, 1.0, 32.0) * phaseStep; phase2 -= std::floor (phase2);
        phase += phaseStep + 0.001 * pitchCurve * std::sin (2.0 * pi * phase2) * sampleRatio
               * std::clamp (std::abs (p.lfoPitchDepth), 0.0, 4.0);
        phase -= std::floor (phase);
        if (p.lfoWave >= 1) body += shifter.tick ((saikeTanh (2.0 * body) - body) * 0.5)
                                         * std::clamp (std::abs (p.curvePitch), 0.0, 1.0);
        body = clickBoost.tick (body);
    }
    saikeNoiseLevel -= saikeNoiseDecay * saikeNoiseLevel;
    if (saikeNoiseLevel < 0.0000001) saikeNoiseLevel = 0.0;
    const auto noiseSource = saikeNoiseLevel * (randomBipolar() * 0.5) * std::max (0.0, p.noiseLevel);
    const auto noise = saikeNoiseFilter.bandPass (noiseSource);
    const auto balance = std::clamp (p.lfoVolumeDepth, -2.0, 2.0);
    auto y = body * std::pow (2.0, -balance * 0.25) + noise * std::pow (2.0, balance * 0.25);
    if (smoothCount > 0) { --smoothCount; y = 0.9 * lastY + 0.1 * y; }
    if (p.lfoSmooth > 0.000001)
    {
        const auto cutoff = 1800.0 + (1.0 - std::clamp (p.lfoSmooth, 0.0, 1.0)) * 18000.0;
        postLp += (y - postLp) * (2.0 * pi * cutoff) / (sr + 2.0 * pi * cutoff);
        y += (postLp - y) * p.lfoSmooth;
    }
    else postLp = y;
    if (p.bodyDriveMix > 0.000001)
    {
        auto driven = y * (1.0 + std::pow (std::max (0.0, p.bodyDrive) * 0.001, 1.2) * 10.0);
        driven /= 1.0 + std::abs (driven);
        y += (driven - y) * std::clamp (p.bodyDriveMix, 0.0, 1.0);
    }
    lastY = y;
    // As in the JSFX audio-sleep design, oscillator/filter residue must not own
    // voice lifetime. Some Saike shapes continuously feed a minute DC residue
    // into the filters after every audible envelope has already finished.
    active = ampValue > 0.00001 || pitchValue > 0.00001 || saikeNoiseLevel > 0.00001;
    if (! active)
    {
        saikeNoiseFilter.ic1 = saikeNoiseFilter.ic2 = 0.0;
        lastY = postLp = 0.0;
        y = 0.0;
    }
    return static_cast<float> (finish (active ? y : 0.0, p));
}
}
