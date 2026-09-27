// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <cmath>

namespace lr608
{
class Refined1176
{
public:
    void prepare (double sampleRate) { sr = std::max (1.0, sampleRate); reset(); }
    void reset() { detector = gainReductionDb = 0.0; }
    void configure (double thresholdDb, int ratioIndex, double makeupDb,
                    double attackMs, double releaseMs, double wetPercent)
    {
        mix = std::clamp (wetPercent * 0.01, 0.0, 1.0);
        threshold = thresholdDb - 3.0; // preserve LR-608 historical early onset
        static constexpr double ratios[] { 4.0, 8.0, 12.0, 20.0, 20.0 };
        ratioReduction = 1.0 - 1.0 / ratios[std::clamp (ratioIndex, 0, 4)];
        makeup = std::exp (makeupDb * dbToLog);
        attackCoefficient = std::exp (-1.0 / (std::max (0.0001, attackMs * 0.001) * sr));
        releaseSeconds = std::max (0.001, releaseMs * 0.001);
        detectorAttackCoefficient = std::exp (-1.0 / (0.00015 * sr));
        detectorReleaseCoefficient = std::exp (-1.0 / (0.006 * sr));
    }

    double process (double x)
    {
        if (mix <= 1.0e-6) { reset(); return x; }
        const auto gain = updateGain (std::abs (x));
        return x + (x * gain - x) * mix;
    }

    void processStereo (double& left, double& right)
    {
        if (mix <= 1.0e-6) { reset(); return; }
        const auto gain = updateGain (std::max (std::abs (left), std::abs (right)));
        left += (left * gain - left) * mix;
        right += (right * gain - right) * mix;
    }

    double activity() const noexcept { return std::max (detector, std::abs (gainReductionDb)); }

private:
    double updateGain (double level)
    {
        detector = level > detector
            ? level + detectorAttackCoefficient * (detector - level)
            : level + detectorReleaseCoefficient * (detector - level);

        const auto detectorDb = logToDb * std::log (std::max (detector, 1.0e-7));
        const auto xDb = detectorDb - threshold;
        constexpr double kneeDb = 6.0;
        constexpr double halfKnee = kneeDb * 0.5;
        double overDb = 0.0;
        if (xDb >= halfKnee) overDb = xDb;
        else if (xDb > -halfKnee)
        {
            const auto kneeX = xDb + halfKnee;
            overDb = (kneeX * kneeX) / (2.0 * kneeDb);
        }

        const auto targetGr = overDb * ratioReduction;
        if (targetGr > gainReductionDb)
            gainReductionDb += (targetGr - gainReductionDb) * (1.0 - attackCoefficient);
        else
        {
            const auto grNorm = std::clamp (gainReductionDb / 12.0, 0.0, 1.0);
            const auto releaseScale = 1.0 - 0.68 * grNorm;
            const auto releaseNow = std::max (0.001, releaseSeconds * releaseScale);
            const auto releaseCoefficient = std::exp (-1.0 / (releaseNow * sr));
            gainReductionDb += (targetGr - gainReductionDb) * (1.0 - releaseCoefficient);
        }
        return std::exp (-gainReductionDb * dbToLog) * makeup;
    }

    static constexpr double dbToLog = 0.11512925464970229;
    static constexpr double logToDb = 8.6858896380650366;
    double sr = 44100.0, mix = 0.0, threshold = -21.0, ratioReduction = 0.75, makeup = 1.0;
    double attackCoefficient = 0.0, releaseSeconds = 0.08;
    double detectorAttackCoefficient = 0.0, detectorReleaseCoefficient = 0.0;
    double detector = 0.0, gainReductionDb = 0.0;
};
}
