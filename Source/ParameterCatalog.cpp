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

juce::String delayTimeToText (double plainValue)
{
    const auto code = juce::jlimit (0, 1035, juce::roundToInt (plainValue));
    if (code <= 1020)
        return "1/" + juce::String (1024 - code);

    const auto beats = code - 1019; // 1021 -> 2 beats, 1023 -> one 4/4 bar.
    if (beats < 4)
        return "0." + juce::String (beats);
    if (beats == 4)
        return "1";

    // User-facing bar.beat notation: 1.1 ... 1.4, 2.1 ... 2.4, 3.1 ... 3.4.
    const auto bars = (beats - 1) / 4;
    const auto beatInBar = ((beats - 1) % 4) + 1;
    return juce::String (bars) + "." + juce::String (beatInBar);
}

double delayTimeFromText (const juce::String& source)
{
    const auto text = source.trim();
    if (text.startsWith ("1/"))
    {
        const auto denominator = juce::jlimit (4, 1024, text.substring (2).getIntValue());
        return double (1024 - denominator);
    }

    const auto dot = text.indexOfChar ('.');
    if (dot >= 0)
    {
        const auto bars = juce::jlimit (0, 3, text.substring (0, dot).getIntValue());
        const auto beat = juce::jlimit (1, 4, text.substring (dot + 1).getIntValue());
        return double (1019 + bars * 4 + beat);
    }

    // Plain integers 1..4 mean exact bars.
    const auto bars = juce::jlimit (1, 4, text.getIntValue());
    return double (1019 + bars * 4);
}
}

juce::String valueToText (int sliderNumber, double plainValue)
{
    if (sliderNumber == 266)
        return delayTimeToText (plainValue);

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
                .withValueFromStringFunction ([sliderNumber] (const juce::String& text)
                {
                    return sliderNumber == 266 ? static_cast<float> (delayTimeFromText (text))
                                               : text.getFloatValue();
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
