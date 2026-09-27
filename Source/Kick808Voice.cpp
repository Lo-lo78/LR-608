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
double clamp01 (double x) { return std::clamp (x, 0.0, 1.0); }
}

// Kick 808 migration checkpoint (2026-09-27):
// Ported from LR-608_Kick808_LAB_TEST11_ClickBodyCoupling.jsfx.
// Approved changes: fixed-short click-resonator tail, refined TEST7 1176,
// independent Noise Resonance, organic TEST10 Noise Granulation (0..100),
// and exposed Click/Body Coupling (default 0.11). TEST3/TEST5 resonator
// variants and TEST8 resonant granulator were explicitly rejected.

void Kick808Voice::prepare (double sampleRate) { sr = std::max (1.0, sampleRate); reset(); }

void Kick808Voice::reset()
{
    active = false;
    env = pitchEnv = clickEnv = impulseEnv = noiseEnv = 0.0;
    phase = impulsePhase = lfoPhase = lfoPreviousPhase = lfoEnv = lfoSmoothState = 0.0;
    clickX1 = clickX2 = clickY1 = clickY2 = impulseX1 = impulseX2 = impulseY1 = impulseY2 = 0.0;
    dcX = dcY = detectorEnvelope = runningDb = 0.0;
    impulseTailEnvelope = 0.0;
    noiseContactE1 = noiseContactE2 = noiseContactE3 = noiseContactE4 = noiseContactE5 = 0.0;
    noiseContactC1 = noiseContactC2 = noiseContactC3 = noiseContactC4 = noiseContactC5 = 1.0;
    noiseContactMean = 0.0;
    rng = 0x608u;
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
    impulseTailEnvelope = 1.0;
    phase = impulsePhase = lfoPhase = lfoPreviousPhase = lfoEnv = lfoSmoothState = 0.0;
    lfoSampleHold = randomBipolar();
    clickX1 = clickX2 = clickY1 = clickY2 = impulseX1 = impulseX2 = impulseY1 = impulseY2 = 0.0;
    noiseContactE1 = noiseContactE2 = noiseContactE3 = noiseContactE4 = noiseContactE5 = 0.0;
    noiseContactMean = 0.0;
    detectorEnvelope = runningDb = 0.0;

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
    impulseTailStep = 1.0 / std::max (1.0, 0.0025 * sr); // fixed short tail at every resonance value

    const auto impulseFrequency = 450.0 + p.clickTone * 2200.0;
    const auto impulseQ = 0.7 + p.clickResonance * p.clickResonance * 2.4;
    const auto iw = 2.0 * pi * impulseFrequency / sr;
    const auto ia = std::sin (iw) / (2.0 * impulseQ), ia0 = 1.0 + ia;
    impulseB0 = ia / ia0;
    impulseA1 = -2.0 * std::cos (iw) / ia0;
    impulseA2 = (1.0 - ia) / ia0;

    const auto clickFrequency = 900.0 + p.noiseTone * 2600.0;
    const auto noiseResonance = std::clamp (p.noiseResonance, 0.0, 1.0);
    const auto clickQ = 0.9 + p.clickResonance * 0.8 + noiseResonance * noiseResonance * 4.8;
    const auto cw = 2.0 * pi * clickFrequency / sr;
    const auto ca = std::sin (cw) / (2.0 * clickQ), ca0 = 1.0 + ca;
    clickB0 = ca / ca0;
    clickA1 = -2.0 * std::cos (cw) / ca0;
    clickA2 = (1.0 - ca) / ca0;
    noiseEnvLoss = 1.0 - std::exp (-1.0 / (std::max (p.noiseDecay * 0.001, 0.001) * sr));

    // TEST10 organic flour/contact mesh. This is temporal modulation of the
    // existing broadband noise, not another filter or resonant sound source.
    noiseGranulationAmount = std::clamp (p.noiseGranulation * 0.01, 0.0, 1.0);
    const auto rate1 = 1050.0 + noiseGranulationAmount * 1250.0;
    const auto rate2 = 1320.0 + noiseGranulationAmount * 1580.0;
    const auto rate3 = 1680.0 + noiseGranulationAmount * 1900.0;
    const auto rate4 = 2110.0 + noiseGranulationAmount * 2220.0;
    const auto rate5 = 2640.0 + noiseGranulationAmount * 2550.0;
    noiseContactStep1 = std::max (1.0, sr / rate1);
    noiseContactStep2 = std::max (1.0, sr / rate2);
    noiseContactStep3 = std::max (1.0, sr / rate3);
    noiseContactStep4 = std::max (1.0, sr / rate4);
    noiseContactStep5 = std::max (1.0, sr / rate5);
    const auto decay = [this] (double seconds) { return std::exp (-1.0 / std::max (1.0, seconds * sr)); };
    noiseContactD1Fast = decay (0.00022); noiseContactD2Fast = decay (0.00029);
    noiseContactD3Fast = decay (0.00037); noiseContactD4Fast = decay (0.00046); noiseContactD5Fast = decay (0.00057);
    noiseContactD1Slow = decay (0.00105); noiseContactD2Slow = decay (0.00122);
    noiseContactD3Slow = decay (0.00142); noiseContactD4Slow = decay (0.00165); noiseContactD5Slow = decay (0.00192);
    noiseContactMeanCoefficient = 1.0 - std::exp (-1.0 / std::max (1.0, 0.028 * sr));
    if (noiseGranulationAmount > 1.0e-9)
    {
        const auto uniform = [this] { return (randomBipolar() + 1.0) * 0.5; };
        noiseContactC1 = 1.0 + uniform() * noiseContactStep1;
        noiseContactC2 = 1.0 + uniform() * noiseContactStep2;
        noiseContactC3 = 1.0 + uniform() * noiseContactStep3;
        noiseContactC4 = 1.0 + uniform() * noiseContactStep4;
        noiseContactC5 = 1.0 + uniform() * noiseContactStep5;
    }
    else
        noiseContactC1 = noiseContactC2 = noiseContactC3 = noiseContactC4 = noiseContactC5 = 1.0;

    bodyDriveCurve = std::pow (p.bodyDrive * 0.01, 1.4);
    dcCoefficient = std::exp (-2.0 * pi * 5.0 / sr);

    // TEST7 refined 1176-style detector: fast peak detector -> soft knee ->
    // user Attack -> program-dependent release. The historical -3 dB early
    // onset is kept, but the old fixed 10 ms RMS pre-detector is removed.
    detectorAttackCoefficient = std::exp (-1.0 / (0.00015 * sr));
    detectorReleaseCoefficient = std::exp (-1.0 / (0.006 * sr));
    compThresholdDb = p.compThreshold - 3.0;
    compKneeDb = 6.0;
    compAttackCoefficient = std::exp (-1.0 / (std::max (0.0001, p.compAttack * 0.001) * sr));
    compReleaseSeconds = std::max (0.001, p.compRelease * 0.001);
    const int ratios[] { 4, 8, 12, 20, 20 };
    compRatioReduction = 1.0 - 1.0 / ratios[std::clamp (p.compRatio, 0, 4)];
    compMakeupGain = std::exp (p.compMakeup * (std::log (10.0) / 20.0));
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
    else
    {
        lfoEnv = lfoSmoothState = 0.0;
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
    if (clickEnv <= 0.0 && impulseTailEnvelope > 0.0)
        impulseTailEnvelope = std::max (0.0, impulseTailEnvelope - impulseTailStep);

    impulseEnv = std::max (0.0, impulseEnv - impulseEnvStep);
    impulsePhase += impulsePhaseStep; impulsePhase -= std::floor (impulsePhase);
    auto impulse = std::sin ((impulsePhase + (p.clickPhase + 1.0) * 0.5) * 2.0 * pi);
    impulse = sign (impulse) * std::pow (std::abs (impulse), 0.28) * impulseEnv * p.clickLevel * 1.1 * (1.0 + accent * 0.7);

    auto impulseBp = impulseB0 * impulse - impulseB0 * impulseX2
                   - impulseA1 * impulseY1 - impulseA2 * impulseY2;
    impulseBp *= 1.0 + p.clickResonance * 0.9;
    impulseBp /= 1.0 + std::abs (impulseBp) * 0.8;
    impulseBp *= impulseTailEnvelope;
    impulseX2 = impulseX1; impulseX1 = impulse; impulseY2 = impulseY1; impulseY1 = impulseBp;
    const auto impulseFinal = impulse * 0.18 + impulseBp * 0.95;

    auto noise = randomBipolar();
    if (noiseGranulationAmount > 1.0e-9)
    {
        const auto tailness = 1.0 - std::sqrt (clamp01 (noiseEnv));
        const auto speed = 0.10 + 0.90 * std::sqrt (std::max (0.0, noiseEnv));
        const auto uniform = [this] { return (randomBipolar() + 1.0) * 0.5; };
        auto contact = [&] (double& counter, double& envelope, double step, double low, double span, double tailBoost, double r0, double rSpan)
        {
            counter -= 1.0;
            if (counter <= 0.0)
            {
                envelope += (low + uniform() * span) * (1.0 + tailBoost * tailness);
                counter = (step / speed) * (r0 + uniform() * rSpan);
            }
        };
        contact (noiseContactC1, noiseContactE1, noiseContactStep1, 0.46, 0.54, 0.18, 0.52, 1.18);
        contact (noiseContactC2, noiseContactE2, noiseContactStep2, 0.43, 0.57, 0.16, 0.50, 1.22);
        contact (noiseContactC3, noiseContactE3, noiseContactStep3, 0.40, 0.60, 0.14, 0.48, 1.26);
        contact (noiseContactC4, noiseContactE4, noiseContactStep4, 0.37, 0.63, 0.12, 0.46, 1.30);
        contact (noiseContactC5, noiseContactE5, noiseContactStep5, 0.34, 0.66, 0.10, 0.44, 1.34);

        noiseContactE1 *= noiseContactD1Fast + (noiseContactD1Slow - noiseContactD1Fast) * tailness;
        noiseContactE2 *= noiseContactD2Fast + (noiseContactD2Slow - noiseContactD2Fast) * tailness;
        noiseContactE3 *= noiseContactD3Fast + (noiseContactD3Slow - noiseContactD3Fast) * tailness;
        noiseContactE4 *= noiseContactD4Fast + (noiseContactD4Slow - noiseContactD4Fast) * tailness;
        noiseContactE5 *= noiseContactD5Fast + (noiseContactD5Slow - noiseContactD5Fast) * tailness;

        const auto sum = noiseContactE1 * 0.26 + noiseContactE2 * 0.23 + noiseContactE3 * 0.20
                       + noiseContactE4 * 0.17 + noiseContactE5 * 0.14;
        noiseContactMean += (sum - noiseContactMean) * noiseContactMeanCoefficient;
        const auto normalized = std::clamp (sum / std::max (0.08, noiseContactMean), 0.08, 2.65);
        const auto depth = noiseGranulationAmount * (0.30 + 0.68 * tailness);
        const auto modulation = std::clamp (1.0 + depth * (normalized - 1.0), 0.10, 2.25);
        noise *= modulation;
    }

    noiseEnv -= noiseEnv * noiseEnvLoss;
    if (noiseEnv < 0.00001) noiseEnv = 0.0;
    const auto clickBp = clickB0 * noise - clickB0 * clickX2
                       - clickA1 * clickY1 - clickA2 * clickY2;
    clickX2 = clickX1; clickX1 = noise; clickY2 = clickY1; clickY1 = clickBp;
    const auto noiseClick = clickBp * std::pow (noiseEnv, 2.4) * p.noiseLevel * 0.34 * (1.0 + accent * 0.25);

    const auto clickFocus = std::pow (clickEnv, 0.42);
    body *= 1.0 + accent * pitchEnv * 0.4;
    const auto clickRaw = impulseFinal + noiseClick + (-body) * clickFocus * p.clickBodyCoupling;
    const auto clickDrive = 1.45 + clickFocus * 1.15 + accent * 0.45;
    const auto clickSaturated = clickRaw * clickDrive / (1.0 + std::abs (clickRaw * clickDrive));
    const auto click = clickRaw * 0.22 + clickSaturated * 0.78;

    const auto bodySignal = (-body) * amplitude;
    const auto drive = 1.0 + bodyDriveCurve * (2.0 + pitchEnv * 2.5 + accent * 2.0);
    const auto driven = bodySignal * drive;
    const auto bodyFinal = bodySignal * (1.0 - p.bodyDriveMix)
                         + (driven / (1.0 + std::abs (driven))) * p.bodyDriveMix;
    auto signal = (bodyFinal + click) * p.level * velocity;

    const auto previousX = dcX; dcX = signal;
    dcY = dcX - previousX + dcCoefficient * dcY;
    signal = dcY;

    if (p.compMix > 0.000001)
    {
        const auto peak = std::abs (signal);
        if (peak > detectorEnvelope)
            detectorEnvelope = peak + detectorAttackCoefficient * (detectorEnvelope - peak);
        else
            detectorEnvelope = peak + detectorReleaseCoefficient * (detectorEnvelope - peak);

        const auto log2db = 20.0 / std::log (10.0);
        const auto detectorDb = log2db * std::log (std::max (detectorEnvelope, 1.0e-7));
        const auto xDb = detectorDb - compThresholdDb;
        const auto halfKnee = compKneeDb * 0.5;
        double overDb = 0.0;
        if (xDb >= halfKnee)
            overDb = xDb;
        else if (xDb > -halfKnee)
        {
            const auto kneeX = xDb + halfKnee;
            overDb = (kneeX * kneeX) / std::max (1.0e-6, 2.0 * compKneeDb);
        }
        const auto targetGr = overDb * compRatioReduction;
        if (targetGr > runningDb)
            runningDb += (targetGr - runningDb) * (1.0 - compAttackCoefficient);
        else
        {
            const auto grNorm = std::clamp (runningDb / 12.0, 0.0, 1.0);
            const auto releaseScale = 1.0 - 0.68 * grNorm;
            const auto releaseNow = std::max (0.001, compReleaseSeconds * releaseScale);
            const auto releaseCoefficient = std::exp (-1.0 / (releaseNow * sr));
            runningDb += (targetGr - runningDb) * (1.0 - releaseCoefficient);
        }
        const auto db2log = std::log (10.0) / 20.0;
        const auto gain = std::exp (-runningDb * db2log) * compMakeupGain;
        signal += (signal * gain - signal) * (p.compMix / 100.0);
    }
    else
        detectorEnvelope = runningDb = 0.0;

    if (env <= 0.0 && pitchEnv <= 0.0 && clickEnv <= 0.0 && impulseEnv <= 0.0 && noiseEnv <= 0.0)
    {
        active = false;
        impulseTailEnvelope = 0.0;
        signal = 0.0;
    }
    return static_cast<float> (signal);
}
}
