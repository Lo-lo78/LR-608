// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ParameterCatalog.h"
#include "GeneratedParameters.h"
#include "SlotArchitecture.h"

namespace lr608
{
namespace
{
juce::StringArray splitChoices (const char* encoded)
{
    return juce::StringArray::fromTokens (encoded, "|", "");
}
}

juce::String valueToText (int sliderNumber, double plainValue)
{
    for (const auto& descriptor : generated::parameters)
    {
        if (descriptor.sliderNumber != sliderNumber)
            continue;
        const auto choices = splitChoices (descriptor.choices);
        if (! choices.isEmpty())
            return choices[juce::jlimit (0, choices.size() - 1,
                juce::roundToInt (plainValue - descriptor.minimum))];
        const auto decimals = descriptor.step >= 1.0 ? 0
                            : descriptor.step >= 0.1 ? 1
                            : descriptor.step >= 0.01 ? 2
                            : descriptor.step >= 0.001 ? 3 : 6;
        return juce::String (plainValue, decimals);
    }
    return juce::String (plainValue);
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& descriptor : generated::parameters)
    {
        const auto id = juce::ParameterID { descriptor.id, 1 };
        const auto choices = splitChoices (descriptor.choices);
        if (! choices.isEmpty())
        {
            layout.add (std::make_unique<juce::AudioParameterChoice> (
                id, descriptor.name, choices,
                juce::roundToInt (descriptor.defaultValue - descriptor.minimum)));
            continue;
        }
        const auto sliderNumber = descriptor.sliderNumber;
        auto range = juce::NormalisableRange<float> (
            static_cast<float> (descriptor.minimum),
            static_cast<float> (descriptor.maximum),
            static_cast<float> (descriptor.step));
        const auto legalDefault = static_cast<float> (juce::jlimit (
            descriptor.minimum, descriptor.maximum, descriptor.defaultValue));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            id, descriptor.name, range, legalDefault,
            juce::AudioParameterFloatAttributes()
                .withStringFromValueFunction ([sliderNumber] (float value, int)
                {
                    return valueToText (sliderNumber, value);
                })
                .withValueFromStringFunction ([] (const juce::String& text)
                {
                    return text.getFloatValue();
                })));
    }
    juce::StringArray engines;
    for(const auto& engine:slotEngines)engines.add(engine.name);
    juce::StringArray chokeNoteChoices{"Off"};
    for(int note=0;note<128;++note)chokeNoteChoices.add("MIDI Note "+juce::String(note));
    for(int slot=0;slot<slotCount;++slot)
    {
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{slotEngineId(slot),1},"Slot "+juce::String(slot+1)+" Engine",
            engines,defaultSlotEngine(slot)));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{slotNoteId(slot),1},"Slot "+juce::String(slot+1)+" MIDI Note",
            0,127,defaultSlotNote(slot)));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{slotChokeTriggerId(slot),1},"Slot "+juce::String(slot+1)+" MIDI Choke Trigger",chokeNoteChoices,0));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{slotChokeTargetId(slot),1},"Slot "+juce::String(slot+1)+" MIDI Choke Target",chokeNoteChoices,0));
    }
    return layout;
}
}
