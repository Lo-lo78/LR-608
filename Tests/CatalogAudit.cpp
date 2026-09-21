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
    if (lr608::slotCount != 128 || lr608::slotEngineCount != 90)
    {
        std::cerr << "Unexpected Slot/engine architecture count\n";
        return EXIT_FAILURE;
    }
    for(int slot=15;slot<lr608::slotCount;++slot)
    {
        const auto oldEngine=(slot-15)%84;
        const auto expected=oldEngine<=6?oldEngine:oldEngine<=14?oldEngine+1:oldEngine<=20?oldEngine+2:oldEngine<=24?oldEngine+3:oldEngine+5;
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
    if (std::size (lr608::generated::parameters) != 286
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
    const std::array<const char*,10> delayIds{"slotDelayDry","slotDelayWet","slotDelayVolume","slotDelayDivision","slotDelayFeedback","slotDelayGlide","slotDelayFilter","slotDelayLeftOffset","slotDelayRightOffset","slotDelayFilterResonance"};
    for(int index=276;index<286;++index)
    {
        const auto&delay=lr608::generated::parameters[index];
        if(std::string(delay.id)!=delayIds[std::size_t(index-276)]||delay.step<=0||delay.minimum>delay.defaultValue||delay.defaultValue>delay.maximum)
        {
            std::cerr<<"Invalid per-Slot delay descriptor\n";
            return EXIT_FAILURE;
        }
        ids.emplace(delay.id);
    }
    if(lr608::generated::parameters[lr608::slotDelayDryParameterIndex].defaultValue!=100.0
       ||lr608::generated::parameters[lr608::slotDelayWetParameterIndex].defaultValue!=0.0
       ||lr608::generated::parameters[lr608::slotDelayFilterParameterIndex].defaultValue!=0.5
       ||lr608::generated::parameters[lr608::slotDelayFilterResonanceParameterIndex].defaultValue!=0.707)
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
    if (ids.size() != 286)
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
    if (exposed.size() != 265)
    {
        std::cerr << "Unexpected exposed count " << exposed.size() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "LR-608 catalogue: 256 JSFX + 14 routing parameters + Slot Pan, Voice Overlap, four musical filters and ten stereo delay controls, 12 pages, 265 page controls; legacy Output Mode hidden\n";
    return EXIT_SUCCESS;
}
