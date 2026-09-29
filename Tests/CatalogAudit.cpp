#include "GeneratedParameters.h"
#include "GeneratedPages.h"
#include "SlotArchitecture.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

int main()
{
    if (lr608::slotCount != 128 || lr608::slotEngineCount != 91)
    {
        std::cerr << "Unexpected Slot/engine architecture count\n";
        return EXIT_FAILURE;
    }
    for(int slot=15;slot<lr608::slotCount;++slot)
    {
        const auto oldEngine=(slot-15)%84;
        const auto historicalExpected=oldEngine<=6?oldEngine:oldEngine<=14?oldEngine+1:oldEngine<=20?oldEngine+2:oldEngine<=24?oldEngine+3:oldEngine+5;
        // Captured Timbales is visible before Zap, but is intentionally skipped
        // by the historical factory/default slot cycle.
        const auto expected=historicalExpected>=78?historicalExpected+1:historicalExpected;
        if(lr608::defaultSlotEngine(slot)!=expected)
        {
            std::cerr<<"Historical post-kit Slot cycle changed at Slot "<<slot+1<<'\n';
            return EXIT_FAILURE;
        }
    }
    std::set<std::string> engineNames;
    for (const auto& engine : lr608::slotEngines)
    {
        if (! engineNames.emplace (engine.name).second
            || engine.page < 0 || engine.page >= int (std::size (lr608::generated::pages)))
        {
            std::cerr << "Invalid or duplicate specific engine entry\n";
            return EXIT_FAILURE;
        }
    }
    if (std::size (lr608::generated::parameters) != 396
        || std::size (lr608::generated::pages) != 12)
    {
        std::cerr << "Unexpected catalogue/page count\n";
        return EXIT_FAILURE;
    }
    std::set<std::string> ids;
    for (int index = 0; index < 256; ++index)
    {
        const auto& parameter = lr608::generated::parameters[index];
        const auto knownZapDefaultAnomaly = parameter.sliderNumber == 77
                                         && parameter.defaultValue == 3.04
                                         && parameter.maximum == 3.0;
        if (parameter.sliderNumber != index + 1 || parameter.step <= 0.0
            || ((! knownZapDefaultAnomaly)
                && (parameter.minimum > parameter.defaultValue
                    || parameter.defaultValue > parameter.maximum)))
        {
            std::cerr << "Invalid descriptor slider " << parameter.sliderNumber << '\n';
            return EXIT_FAILURE;
        }
        ids.emplace (parameter.id);
    }
    for (int index = 256; index < 270; ++index)
    {
        const auto& parameter = lr608::generated::parameters[index];
        if (parameter.sliderNumber != 0 || parameter.minimum != 0.0
            || parameter.maximum != 14.0 || parameter.step != 1.0)
            return EXIT_FAILURE;
        ids.emplace (parameter.id);
    }
    const auto& pan=lr608::generated::parameters[270];
    if(pan.sliderNumber!=257||std::string(pan.id)!="slotPan"||pan.minimum!=-1.0||pan.maximum!=1.0||pan.defaultValue!=0.0)
    {
        std::cerr << "Invalid universal Pan descriptor\n";
        return EXIT_FAILURE;
    }
    ids.emplace(pan.id);
    const auto& overlap=lr608::generated::parameters[271];
    if(overlap.sliderNumber!=258||std::string(overlap.id)!="slotVoiceOverlap"||overlap.minimum!=1.0||overlap.maximum!=2.0||overlap.step!=1.0||overlap.defaultValue!=2.0)
    {
        std::cerr << "Invalid per-Slot Voice Overlap descriptor\n";
        return EXIT_FAILURE;
    }
    ids.emplace(overlap.id);
    const auto& pingPong=lr608::generated::parameters[lr608::slotPingPongPanDepthParameterIndex];
    if(std::string(pingPong.id)!="slotPingPongPanDepth"||pingPong.minimum!=-1.0||pingPong.maximum!=1.0||pingPong.step!=0.01||pingPong.defaultValue!=0.0)
    {
        std::cerr << "Invalid Ping Pong Pan Depth descriptor\n";
        return EXIT_FAILURE;
    }
    ids.emplace(pingPong.id);
    const std::array<const char*,4> filterIds{"slotLowPassCutoff","slotLowPassResonance","slotHighPassCutoff","slotHighPassResonance"};
    for(int index=272;index<276;++index)
    {
        const auto&filter=lr608::generated::parameters[index];
        if(std::string(filter.id)!=filterIds[std::size_t(index-272)]||filter.step<=0||filter.minimum>filter.defaultValue||filter.defaultValue>filter.maximum)
        {
            std::cerr<<"Invalid per-Slot filter descriptor\n";
            return EXIT_FAILURE;
        }
        ids.emplace(filter.id);
    }
    const std::array<const char*,9> delayIds{"slotDelayWet","slotDelayDivision","slotDelayFeedback","slotDelayGlide","slotDelayFilter","slotDelayLeftOffset","slotDelayRightOffset","slotDelayFilterResonance","slotDelayPitch"};
    for(int index=276;index<285;++index)
    {
        const auto&delay=lr608::generated::parameters[index];
        if(std::string(delay.id)!=delayIds[std::size_t(index-276)]||delay.step<=0||delay.minimum>delay.defaultValue||delay.defaultValue>delay.maximum)
        {
            std::cerr<<"Invalid per-Slot delay descriptor\n";
            return EXIT_FAILURE;
        }
        ids.emplace(delay.id);
    }
    if(lr608::generated::parameters[lr608::slotDelayWetParameterIndex].defaultValue!=0.0
       ||lr608::generated::parameters[lr608::slotDelayFilterParameterIndex].defaultValue!=0.5
       ||lr608::generated::parameters[lr608::slotDelayFilterResonanceParameterIndex].defaultValue!=0.707
       ||lr608::generated::parameters[lr608::slotDelayPitchParameterIndex].defaultValue!=0.0)
    {
        std::cerr<<"Delay defaults are not backward-compatible\n";
        return EXIT_FAILURE;
    }
    if(std::string(lr608::slotEngines[16].name)!="Snare Linn"||lr608::slotEngines[16].subEngine!=8)
    {
        std::cerr<<"Snare Linn engine is missing or misplaced\n";
        return EXIT_FAILURE;
    }
    if(std::string(lr608::slotEngines[23].name)!="Clap Linn"||lr608::slotEngines[23].subEngine!=6)
    {
        std::cerr<<"Clap Linn engine is missing or misplaced\n";
        return EXIT_FAILURE;
    }
    const std::array<const char*,4> degradeIds{"slotDegradeAmount","slotDegradeBits","slotDegradeHold","slotDegradeJitter"};
    for(int index=285;index<289;++index)
    {
        const auto&degrade=lr608::generated::parameters[index];
        if(std::string(degrade.id)!=degradeIds[std::size_t(index-285)]||degrade.step<=0||degrade.minimum>degrade.defaultValue||degrade.defaultValue>degrade.maximum)
        {
            std::cerr<<"Invalid per-Slot Degrade descriptor\n";
            return EXIT_FAILURE;
        }
        ids.emplace(degrade.id);
    }
    if(lr608::generated::parameters[lr608::slotDegradeAmountParameterIndex].defaultValue!=0.0
       ||lr608::generated::parameters[lr608::slotDegradeBitsParameterIndex].defaultValue!=8.0
       ||lr608::generated::parameters[lr608::slotDegradeHoldParameterIndex].defaultValue!=4.0
       ||lr608::generated::parameters[lr608::slotDegradeJitterParameterIndex].defaultValue!=0.0)
    {
        std::cerr<<"Degrade defaults are not backward-compatible\n";
        return EXIT_FAILURE;
    }
    const std::array<const char*,2> filterEnvIds{"slotFilterEnvelopeDepth","slotFilterEnvelopeDecay"};
    for(int index=289;index<291;++index){const auto&e=lr608::generated::parameters[index];if(std::string(e.id)!=filterEnvIds[std::size_t(index-289)]||e.step<=0||e.minimum>e.defaultValue||e.defaultValue>e.maximum){std::cerr<<"Invalid filter envelope descriptor\n";return EXIT_FAILURE;}ids.emplace(e.id);}
    if(lr608::generated::parameters[lr608::slotFilterEnvelopeDepthParameterIndex].defaultValue!=0.0||lr608::generated::parameters[lr608::slotFilterEnvelopeDecayParameterIndex].defaultValue!=0.0){std::cerr<<"Filter envelope defaults are not backward-compatible\n";return EXIT_FAILURE;}
    {const auto&a=lr608::generated::parameters[lr608::slotFilterEnvelopeAttackParameterIndex];if(std::string(a.id)!="slotFilterEnvelopeAttack"||a.minimum!=0.0||a.maximum<30.0||a.step<=0.0||std::abs(a.defaultValue-0.004)>1.0e-12){std::cerr<<"Invalid FX envelope attack descriptor\n";return EXIT_FAILURE;}ids.emplace(a.id);}
    for(int index=291;index<348;++index){const auto&c=lr608::generated::parameters[index];if(std::string(c.id).rfind("capturedTimbale",0)!=0||c.step<=0||c.minimum>c.defaultValue||c.defaultValue>c.maximum){std::cerr<<"Invalid captured Timbale descriptor\n";return EXIT_FAILURE;}ids.emplace(c.id);}
    const std::array<const char*,3> kickLabIds{"kickNoiseResonance","kickNoiseGranulation","kickClickBodyCoupling"};
    for(int index=348;index<351;++index){const auto&k=lr608::generated::parameters[index];if(std::string(k.id)!=kickLabIds[std::size_t(index-348)]||k.step<=0||k.minimum>k.defaultValue||k.defaultValue>k.maximum){std::cerr<<"Invalid Kick 808 LAB descriptor\n";return EXIT_FAILURE;}ids.emplace(k.id);}
    if(lr608::generated::parameters[lr608::kickNoiseResonanceParameterIndex].defaultValue!=0.0||lr608::generated::parameters[lr608::kickNoiseGranulationParameterIndex].defaultValue!=0.0||lr608::generated::parameters[lr608::kickClickBodyCouplingParameterIndex].defaultValue!=0.11){std::cerr<<"Kick 808 LAB defaults are not backward-compatible\n";return EXIT_FAILURE;}
    const std::array<const char*,12> snareLabIds{
        "snare1BodyMidTune", "snare1BodyHighTune", "snare1NoiseGranulation",
        "snare1BodyMidDecay", "snare1BodyHighDecay", "snare1ClickBodyCoupling",
        "snare2BodyMidTune", "snare2BodyHighTune", "snare2NoiseGranulation",
        "snare2BodyMidDecay", "snare2BodyHighDecay", "snare2ClickBodyCoupling"
    };
    for(int index=351;index<363;++index)
    {
        const auto&s=lr608::generated::parameters[index];
        if(std::string(s.id)!=snareLabIds[std::size_t(index-351)]||s.step<=0||s.minimum>s.defaultValue||s.defaultValue>s.maximum)
        {
            std::cerr<<"Invalid Snare LAB descriptor\n";
            return EXIT_FAILURE;
        }
        ids.emplace(s.id);
    }
    const std::array<const char*,12> simmonsLabIds{
        "lowTomNoiseResonance", "lowTomNoiseGranulation", "lowTomClickBodyCoupling",
        "midTomNoiseResonance", "midTomNoiseGranulation", "midTomClickBodyCoupling",
        "highTomNoiseResonance", "highTomNoiseGranulation", "highTomClickBodyCoupling",
        "lowTomPitchDecayCurve", "midTomPitchDecayCurve", "highTomPitchDecayCurve"
    };
    for(int index=363;index<375;++index)
    {
        const auto&t=lr608::generated::parameters[index];
        if(std::string(t.id)!=simmonsLabIds[std::size_t(index-363)]||t.step<=0||t.minimum>t.defaultValue||t.defaultValue>t.maximum)
        {
            std::cerr<<"Invalid Simmons LAB descriptor\n";
            return EXIT_FAILURE;
        }
        ids.emplace(t.id);
    }
    const auto& saikeBodyAmpDecay = lr608::generated::parameters[375];
    if (std::string(saikeBodyAmpDecay.id) != "snare1SaikeBodyAmpDecay"
        || saikeBodyAmpDecay.minimum != 0.001
        || saikeBodyAmpDecay.maximum != 10.0
        || saikeBodyAmpDecay.step != 0.001
        || saikeBodyAmpDecay.defaultValue != 10.0)
    {
        std::cerr << "Invalid Saike Body Amp Decay descriptor\n";
        return EXIT_FAILURE;
    }
    ids.emplace(saikeBodyAmpDecay.id);
    const std::array<const char*,3> metalGranulationIds{
        "hiHatNoiseGranulation", "crashNoiseGranulation", "rideNoiseGranulation"
    };
    for (int index = 376; index < 379; ++index)
    {
        const auto& g = lr608::generated::parameters[index];
        if (std::string(g.id) != metalGranulationIds[std::size_t(index - 376)]
            || g.minimum != 0.0 || g.maximum != 1000.0 || g.step != 0.1 || g.defaultValue != 0.0)
        {
            std::cerr << "Invalid hat/cymbal Noise Granulation descriptor\n";
            return EXIT_FAILURE;
        }
        ids.emplace(g.id);
    }
    const std::array<const char*,15> lfoIds{
        "slotLfoRate", "slotLfoWave", "slotLfoSmooth", "slotLfoVolumeDepth",
        "slotLfoLowPassDepth", "slotLfoHighPassDepth", "slotLfoPanDepth",
        "slotLfoOneShot", "slotLfoOneShotPercent", "slotLfoPhase", "slotLfoEnvRate",
        "slotLfoDelay", "slotLfoSquarePwm", "slotLfoUpperSquash", "slotLfoLowerSquash"
    };
    for(int offset=0;offset<int(lfoIds.size());++offset)
    {
        const auto index=lr608::slotLfoRateParameterIndex+offset;
        const auto& lfo=lr608::generated::parameters[index];
        if(std::string(lfo.id)!=lfoIds[std::size_t(offset)]
           || lfo.step<=0 || lfo.minimum>lfo.defaultValue || lfo.defaultValue>lfo.maximum)
        {
            std::cerr<<"Invalid trigger LFO descriptor\n";
            return EXIT_FAILURE;
        }
        ids.emplace(lfo.id);
    }
    if(lr608::generated::parameters[lr608::slotLfoVolumeDepthParameterIndex].defaultValue!=0.0
       ||lr608::generated::parameters[lr608::slotLfoLowPassDepthParameterIndex].defaultValue!=0.0
       ||lr608::generated::parameters[lr608::slotLfoHighPassDepthParameterIndex].defaultValue!=0.0
       ||lr608::generated::parameters[lr608::slotLfoPanDepthParameterIndex].defaultValue!=0.0)
    {
        std::cerr<<"Trigger LFO depths are not backward-compatible\n";
        return EXIT_FAILURE;
    }
    if (ids.size() != 396)
    {
        std::cerr << "Duplicate parameter IDs\n";
        return EXIT_FAILURE;
    }
    std::set<std::string> exposed;
    for (const auto& page : lr608::generated::pages)
    {
        if (page.parameterCount == 0 || page.rowsPerColumn <= 0)
        {
            std::cerr << "Invalid page " << page.name << '\n';
            return EXIT_FAILURE;
        }
        for (std::size_t index = 0; index < page.parameterCount; ++index)
        {
            if (! ids.contains (page.parameterIds[index]))
            {
                std::cerr << "Unknown page ID " << page.parameterIds[index] << '\n';
                return EXIT_FAILURE;
            }
            exposed.emplace (page.parameterIds[index]);
        }
    }
    if (exposed.size() != 350)
    {
        std::cerr << "Unexpected exposed count " << exposed.size() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "LR-608 catalogue: 256 JSFX + 14 routing parameters + Slot Pan, Voice Overlap, four musical filters, nine stereo delay controls and four Degrade controls, a shared exponential FX Attack/Decay envelope and a zero-cost-when-disabled triggered per-voice LFO, 12 pages, 350 page controls; legacy Tom Pan and Output Mode controls hidden\n";
    return EXIT_SUCCESS;
}
