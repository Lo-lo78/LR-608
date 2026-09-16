// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Kick808Voice.h"

#include <algorithm>
#include <cmath>

namespace lr608
{
namespace
{
constexpr double pi = 3.14159265358979323846;
double sign (double x) { return x < 0.0 ? -1.0 : x > 0.0 ? 1.0 : 0.0; }
}

void Kick808Voice::prepare (double sampleRate) { sr = std::max (1.0, sampleRate); reset(); }

void Kick808Voice::reset()
{
    active = false; env = pitchEnv = clickEnv = impulseEnv = noiseEnv = 0.0;
    phase = impulsePhase = lfoPhase = lfoPreviousPhase = lfoEnv = lfoSmoothState = 0.0;
    clickX1 = clickX2 = clickY1 = clickY2 = impulseX1 = impulseX2 = impulseY1 = impulseY2 = 0.0;
    dcX = dcY = rms = runningDb = 0.0; rng = 0x608u;
}

double Kick808Voice::randomBipolar()
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (rng / double (0xffffffffu)) * 2.0 - 1.0;
}

void Kick808Voice::trigger (int midiVelocity, const Kick808Parameters& p, std::uint32_t randomSeed)
{
    if (randomSeed != 0) rng = randomSeed;
    active = true;
    velocity = std::clamp (midiVelocity / 127.0, 0.0, 1.0);
    const auto threshold = p.accentThreshold / 127.0;
    accent = velocity > threshold
        ? std::pow ((velocity - threshold) / std::max (0.001, 1.0 - threshold), 1.6) * p.accentCharacter
        : 0.0;
    env = pitchEnv = clickEnv = impulseEnv = noiseEnv = 1.0;
    phase = impulsePhase = lfoPhase = lfoPreviousPhase = lfoEnv = lfoSmoothState = 0.0;
    lfoSampleHold = randomBipolar();
    clickX1 = clickX2 = clickY1 = clickY2 = impulseX1 = impulseX2 = impulseY1 = impulseY2 = 0.0;

    pitchEnvStep = 1.0 / (std::max (0.001, p.punchTime) * sr);
    lfoEnvRate = 1.0 - std::exp (-1.0 / (std::max (0.001, p.lfoDelay * 0.001) * sr));
    lfoSmoothRate = 1.0 - std::exp (-1.0 / ((0.00005 + p.lfoSmooth * 0.02) * sr));
    endFrequency = 30.0 + std::max (0.0, p.tune) * 80.0;
    startFrequency = endFrequency * (1.0 + p.punchAmount * 8.0);
    envLoss = 1.0 - std::exp (-1.0 / (std::max (p.decay, 0.001) * sr));
    amplitudeExponent = 1.0 + p.curve * 6.0 + accent * 1.8;
    pitchCurveReciprocal = 1.0 / std::max (0.001, p.curvePitchDecay);
    clickEnvStep = 1.0 / (0.0034 * sr);
    impulseEnvStep = 1.0 / (0.0016 * sr);
    impulsePhaseStep = 2200.0 / sr;

    const auto impulseFrequency = 450.0 + p.clickTone * 2200.0;
    const auto impulseQ = 0.7 + p.clickResonance * p.clickResonance * 2.4;
    const auto iw = 2.0 * pi * impulseFrequency / sr;
    const auto ia = std::sin (iw) / (2.0 * impulseQ), ia0 = 1.0 + ia;
    impulseB0 = ia / ia0;
    impulseA1 = -2.0 * std::cos (iw) / ia0;
    impulseA2 = (1.0 - ia) / ia0;

    const auto clickFrequency = 900.0 + p.noiseTone * 2600.0;
    const auto clickQ = 0.9 + p.clickResonance * 0.8;
    const auto cw = 2.0 * pi * clickFrequency / sr;
    const auto ca = std::sin (cw) / (2.0 * clickQ), ca0 = 1.0 + ca;
    clickB0 = ca / ca0;
    clickA1 = -2.0 * std::cos (cw) / ca0;
    clickA2 = (1.0 - ca) / ca0;
    noiseEnvLoss = 1.0 - std::exp (-1.0 / (std::max (p.noiseDecay * 0.001, 0.001) * sr));
    bodyDriveCurve = std::pow (p.bodyDrive * 0.01, 1.4);
    dcCoefficient = std::exp (-2.0 * pi * 5.0 / sr);
    rmsCoefficient = std::exp (-1.0 / (0.01 * sr));
    const auto db2log = std::log (10.0) / 20.0;
    compThresholdLinear = std::exp ((p.compThreshold - 3.0) * db2log);
    compAttackCoefficient = std::exp (-1.0 / (std::max (0.0001, p.compAttack * 0.001) * sr));
    compReleaseCoefficient = std::exp (-1.0 / (std::max (0.001, p.compRelease * 0.001) * sr));
    const int ratios[] { 4, 8, 12, 20, 20 };
    compRatioReduction = 1.0 - 1.0 / ratios[std::clamp (p.compRatio, 0, 4)];
    compMakeupGain = std::exp (p.compMakeup * db2log);
}

float Kick808Voice::render (const Kick808Parameters& p, double tempo)
{
    if (! active) return 0.0f;
    if (pitchEnv > 0.0) pitchEnv = std::max (0.0, pitchEnv - pitchEnvStep);

    double lfo = 0.0;
    if (std::abs (p.lfoVolumeDepth) > 1.0e-6 || std::abs (p.lfoPitchDepth) > 1.0e-6)
    {
        lfoEnv += (1.0 - lfoEnv) * lfoEnvRate;
        lfoPreviousPhase = lfoPhase;
        lfoPhase += p.lfoFrequency / (sr * 60.0 / std::max (1.0, tempo));
        lfoPhase -= std::floor (lfoPhase);
        if (lfoPhase < lfoPreviousPhase) lfoSampleHold = randomBipolar();
        const auto shape = p.lfoShape;
        const auto triangleBase = lfoPhase < 0.5 ? lfoPhase * 4.0 - 1.0 : 3.0 - lfoPhase * 4.0;
        const double waves[] {
            std::sin ((lfoPhase + (shape - 0.5) * 0.4) * 2.0 * pi),
            sign (triangleBase) * std::pow (std::abs (triangleBase), 1.0 + (shape - 0.5) * 6.0),
            std::clamp ((lfoPhase * (1.0 + shape)) * 2.0 - 1.0, -1.0, 1.0),
            std::clamp (1.0 - (lfoPhase * (1.0 + shape)) * 2.0, -1.0, 1.0),
            lfoPhase < std::clamp (shape, 0.05, 0.95) ? 1.0 : -1.0,
            lfoSampleHold
        };
        const auto raw = waves[std::clamp (p.lfoWave, 0, 5)];
        lfoSmoothState += (raw - lfoSmoothState) * lfoSmoothRate;
        lfo = lfoSmoothState * lfoEnv;
    }

    const auto pitchCurveSource = std::pow (env, pitchCurveReciprocal);
    const auto pitchCurveEnvelope = std::pow (pitchCurveSource, amplitudeExponent);
    auto frequency = endFrequency + (startFrequency - endFrequency) * std::pow (pitchEnv, 2.5 + accent * 1.4);
    frequency += (startFrequency - endFrequency) * p.curvePitch * pitchCurveEnvelope;
    frequency += lfo * p.lfoPitchDepth * (endFrequency * 0.4);
    frequency = std::max (1.0, frequency);
    auto body = std::sin ((phase + (p.bodyPhase + 1.0) * 0.5) * 2.0 * pi);
    phase += frequency / sr; phase -= std::floor (phase);
    env -= env * envLoss;
    if (env < 0.00001) env = 0.0;
    auto amplitude = std::pow (env, amplitudeExponent);
    amplitude *= 1.0 - ((lfo + 1.0) * 0.5) * p.lfoVolumeDepth;

    clickEnv = std::max (0.0, clickEnv - clickEnvStep);
    impulseEnv = std::max (0.0, impulseEnv - impulseEnvStep);
    impulsePhase += impulsePhaseStep; impulsePhase -= std::floor (impulsePhase);
    auto impulse = std::sin ((impulsePhase + (p.clickPhase + 1.0) * 0.5) * 2.0 * pi);
    impulse = sign (impulse) * std::pow (std::abs (impulse), 0.28) * impulseEnv * p.clickLevel * 1.1 * (1.0 + accent * 0.7);

    auto impulseBp = impulseB0 * impulse - impulseB0 * impulseX2
                   - impulseA1 * impulseY1 - impulseA2 * impulseY2;
    impulseBp *= 1.0 + p.clickResonance * 0.9;
    impulseBp /= 1.0 + std::abs (impulseBp) * 0.8;
    impulseX2 = impulseX1; impulseX1 = impulse; impulseY2 = impulseY1; impulseY1 = impulseBp;
    const auto impulseFinal = impulse * 0.18 + impulseBp * 0.95;

    const auto noise = randomBipolar();
    const auto clickBp = clickB0 * noise - clickB0 * clickX2
                       - clickA1 * clickY1 - clickA2 * clickY2;
    clickX2 = clickX1; clickX1 = noise; clickY2 = clickY1; clickY1 = clickBp;
    noiseEnv -= noiseEnv * noiseEnvLoss;
    if (noiseEnv < 0.00001) noiseEnv = 0.0;
    const auto noiseClick = clickBp * std::pow (noiseEnv, 2.4) * p.noiseLevel * 0.34 * (1.0 + accent * 0.25);

    const auto clickFocus = std::pow (clickEnv, 0.42);
    body *= 1.0 + accent * pitchEnv * 0.4;
    const auto clickRaw = impulseFinal + noiseClick + (-body) * clickFocus * 0.11;
    const auto clickDrive = 1.45 + clickFocus * 1.15 + accent * 0.45;
    const auto clickSaturated = clickRaw * clickDrive / (1.0 + std::abs (clickRaw * clickDrive));
    const auto click = clickRaw * 0.22 + clickSaturated * 0.78;
    const auto bodySignal = (-body) * amplitude;
    const auto drive = 1.0 + bodyDriveCurve * (2.0 + pitchEnv * 2.5 + accent * 2.0);
    const auto driven = bodySignal * drive;
    const auto bodyFinal = bodySignal * (1.0 - p.bodyDriveMix) +
                           (driven / (1.0 + std::abs (driven))) * p.bodyDriveMix;
    auto signal = (bodyFinal + click) * p.level * velocity;

    const auto previousX = dcX; dcX = signal;
    dcY = dcX - previousX + dcCoefficient * dcY;
    signal = dcY;
    if (p.compMix > 0.000001)
    {
        const auto db2log = std::log (10.0) / 20.0;
        rms = signal * signal + rmsCoefficient * (rms - signal * signal);
        const auto overDb = std::max (0.0, (20.0 / std::log (10.0)) *
            std::log (std::max (std::sqrt (std::max (0.0, rms)), 1.0e-7) /
                      compThresholdLinear));
        const auto coefficient = overDb > runningDb ? compAttackCoefficient : compReleaseCoefficient;
        runningDb += (overDb - runningDb) * (1.0 - coefficient);
        const auto gain = std::exp (-runningDb * compRatioReduction * db2log) * compMakeupGain;
        signal += (signal * gain - signal) * (p.compMix / 100.0);
    }
    else { rms = runningDb = 0.0; }

    if (env <= 0.0 && pitchEnv <= 0.0 && clickEnv <= 0.0 && impulseEnv <= 0.0 && noiseEnv <= 0.0)
    { active = false; signal = 0.0; }
    return static_cast<float> (signal);
}
}
