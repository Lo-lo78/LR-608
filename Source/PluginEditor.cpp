// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginEditor.h"
#include "GeneratedPages.h"
#include "GeneratedParameters.h"
#include "ScreenReaderAnnouncer.h"
#include <BinaryData.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace
{
using ValueEditorShortcut = std::function<bool (const juce::KeyPress&, juce::Component*)>;

juce::String parameterNameWithoutElement (juce::String name)
{
    static constexpr const char* prefixes[] {
        "Kick ", "Snare 1 ", "Snare 2 ", "Snare2 ", "Snare ", "Clap ", "Rim ",
        "Low Tom ", "Mid Tom ", "High Tom ", "LowTom ", "MidTom ", "HighTom ",
        "HiHat ", "Crash ", "Ride ", "Cymbal ", "Maracas ", "Cowbell ",
        "Clave/Cowbell ", "Clave ", "Zap ", "Master "
    };
    for (const auto* prefix : prefixes)
        if (name.startsWithIgnoreCase (prefix))
            return name.substring (static_cast<int> (std::char_traits<char>::length (prefix)));
    return name;
}

juce::String mappedName (juce::StringRef name,
                         std::initializer_list<std::pair<const char*, const char*>> names)
{
    for (const auto& entry : names)
        if (juce::String (name) == entry.first)
            return entry.second;
    return {};
}

// These are the contextual labels from the original LR-608.lua controller.
// A physical JSFX slider changes meaning with the synthesis engine; the VST
// now exposes that meaning directly to both the visual grid and accessibility.
juce::String contextualParameterName (int engine, juce::String structuralName)
{
    const auto& info = lr608::slotEngines[juce::jlimit (0, lr608::slotEngineCount - 1, engine)];
    const auto sub = info.subEngine;
    const auto commonName = parameterNameWithoutElement (structuralName);

    if (info.family == lr608::SlotFamily::kick)
    {
        const auto suffix = mappedName (structuralName, {
            {"Kick Level","Level"},{"Kick Tune","Minimum Pitch"},{"Kick Decay","Amp Decay"},{"Kick Curve","Amp Curve"},
            {"Kick Punch Amount","Pitch Envelope"},{"Kick Punch Time","Pitch Decay"},{"Kick Curve Pitch","Shift Mix"},{"Kick Curve Pitch Decay","Pitch Curve"},
            {"Kick Click Level","Click Boost"},{"Kick Click Tone","Click Frequency"},{"Kick Click Resonance","Noise Resonance"},
            {"Kick Noise Level","Noise Level"},{"Kick Noise Tone","Noise Tone"},{"Kick Noise Decay","Noise Decay"},
            {"Kick Body Drive","Output Drive"},{"Kick Body Drive Mix","Drive Mix"},{"Kick Body Phase","Body Start Phase"},{"Kick Click Phase","Modulator Start Phase"},
            {"Kick LFO Volume Depth","Body Noise Balance"},{"Kick LFO Pitch Depth","FM Depth"},{"Kick LFO Frequency","FM Ratio"},
            {"Kick LFO Wave","Frequency Shift"},{"Kick LFO Delay","Amp Attack"},{"Kick LFO Smooth","Damping"},{"Kick LFO Wave Shape","Saturation Asymmetry"}
        });
        if (suffix.isEmpty()) return commonName; // compressor controls are common
        if (sub == 7)
        {
            const auto linn=mappedName(structuralName,{{"Kick Level","Level"},{"Kick Tune","Membrane Tune"},{"Kick Punch Amount","Pitch Envelope"},{"Kick Punch Time","Pitch Decay"},{"Kick Decay","Membrane Decay"},{"Kick Curve","Membrane Damping"},{"Kick Curve Pitch","Pitch Curve"},{"Kick Curve Pitch Decay","Pitch Curve Decay"},{"Kick Click Level","Beater Level"},{"Kick Click Tone","Beater Tone"},{"Kick Click Resonance","Beater Character"},{"Kick Noise Level","Shell Air Level"},{"Kick Noise Tone","Shell Air Tone"},{"Kick Noise Decay","Shell Air Decay"},{"Kick Body Drive","Input Drive"},{"Kick Body Drive Mix","Drive Mix"},{"Kick Body Phase","Membrane Start Phase"},{"Kick Click Phase","Beater Start Phase"},{"Kick LFO Volume Depth","LFO Volume Depth"},{"Kick LFO Pitch Depth","LFO Pitch Depth"},{"Kick LFO Frequency","LFO Rate"},{"Kick LFO Wave","LFO Wave"},{"Kick LFO Delay","LFO Delay"},{"Kick LFO Smooth","LFO Smooth"},{"Kick LFO Wave Shape","LFO Shape"}});
            return linn.isNotEmpty()?"Linn Acoustic "+linn:commonName;
        }
        if (sub >= 3) return "Saike Type " + juce::String (sub - 3) + " " + suffix;
        if (sub == 2)
        {
            const auto label = mappedName (structuralName, {
                {"Kick Level","909 Kick Level"},{"Kick Tune","909 Kick Tune"},{"Kick Decay","909 Kick Decay"},{"Kick Curve","909 Kick Compression"},
                {"Kick Punch Amount","909 Kick Tune Depth"},{"Kick Punch Time","909 Kick Tune Decay"},{"Kick Curve Pitch","909 Kick Pitch"},{"Kick Curve Pitch Decay","909 Kick Tune Shape"},
                {"Kick Click Level","909 Kick Attack"},{"Kick Click Tone","909 Kick Pulse Tone"},{"Kick Click Resonance","909 Kick Pulse Length"},
                {"Kick Noise Level","909 Kick Noise Level"},{"Kick Noise Tone","909 Kick Noise Tone"},{"Kick Noise Decay","909 Kick Noise Decay"},
                {"Kick Body Drive","909 Kick Distortion"},{"Kick Body Drive Mix","909 Kick Distortion Mix"},{"Kick Body Phase","909 Kick VCO Start Phase"},{"Kick Click Phase","909 Kick Pulse Phase"},
                {"Kick LFO Volume Depth","909 Kick Body Attack"},{"Kick LFO Pitch Depth","909 Kick Accent Pitch"},{"Kick LFO Frequency","909 Kick Body Harmonics"},
                {"Kick LFO Wave","909 Kick VCO Shape"},{"Kick LFO Delay","909 Kick Pulse Width"},{"Kick LFO Smooth","909 Kick Circuit Damping"},{"Kick LFO Wave Shape","909 Kick Diode Curve"}
            });
            return label.isNotEmpty() ? label : commonName;
        }
        const auto simmons = mappedName (structuralName, {
            {"Kick Tune","Tone"},{"Kick Curve","Envelope Shape"},{"Kick Punch Amount","Bend"},{"Kick Punch Time","Bend Time"},
            {"Kick Curve Pitch","Second Bend"},{"Kick Curve Pitch Decay","Bend Shape"},{"Kick Click Resonance","Click Length"},
            {"Kick Noise Decay","Noise Tail"},{"Kick Body Drive","Analog Drive"},{"Kick LFO Volume Depth","Noise Movement"},
            {"Kick LFO Pitch Depth","VCO Mod Depth"},{"Kick LFO Frequency","VCO Mod Rate"},{"Kick LFO Wave","VCO Mod Wave"},
            {"Kick LFO Delay","VCO Mod Delay"},{"Kick LFO Smooth","VCO Mod Smooth"},{"Kick LFO Wave Shape","VCO Mod Shape"}
        });
        return juce::String (sub == 0 ? "808 Kick " : "Simmons Kick ")
             + (sub == 1 && simmons.isNotEmpty() ? simmons : commonName);
    }

    if (info.family == lr608::SlotFamily::snare1 || info.family == lr608::SlotFamily::snare2)
    {
        const auto suffix = mappedName (structuralName, {
            {"Snare Level","Level"},{"Snare Click Level","Transient"},{"Snare Body Decay","Pitch Decay"},{"Snare Body Tune","Minimum Pitch"},
            {"Snare Body Level","Body Level"},{"Snare Body Pitch Decay","Pitch Envelope"},{"Snare Body Mid","Amplitude Decay"},{"Snare Body High","Body Brightness"},
            {"Snare Noise Level","Noise Level"},{"Snare Noise Decay","Noise Decay"},{"Snare Noise Tone","Noise Tone"},{"Snare Noise Resonance","Noise Resonance"},
            {"Snare Noise Tone Env","Shared Filter Tone"},{"Snare Noise Attack","Noise Attack"},{"Snare Noise Color","Body Noise Balance"},{"Snare Noise Degrade","Noise Retrigger"},
            {"Snare Filter Env Amount","Body Noise Coupling"},{"Snare Drive","Drive"},{"Snare Drive Mix","Drive Mix"},{"Snare Noise Ring Freq","Frequency Shift"},
            {"Snare Noise Ring Depth","Frequency Shift Mix"},{"Snare Noise Ring Wave","Variation"},{"Snare Noise Ring Smooth","Start Smoothing"},
            {"Snare Noise Ring Delay","Amplitude Attack"},{"Snare Noise Ring Wave Shape","Wave Asymmetry"},{"Snare Comp Threshold","Comp Threshold"},
            {"Snare Comp Ratio","Comp Ratio"},{"Snare Comp Gain","Comp Gain"},{"Snare Comp Attack (ms)","Comp Attack"},{"Snare Comp Release (ms)","Comp Release"},{"Snare Comp Mix","Comp Mix"}
        });
        if (sub == 0 || sub == 1)
        {
            const auto simmons = mappedName (structuralName, {
                {"Snare Body Tune","Tone"},{"Snare Body Level","Tone Level"},{"Snare Body Pitch Decay","Bend"},{"Snare Body Mid","Bend Time"},{"Snare Body High","Tone Shape"},
                {"Snare Noise Resonance","Filter Resonance"},{"Snare Noise Tone Env","Filter Sweep"},{"Snare Noise Color","Noise Texture"},{"Snare Noise Degrade","Noise Flour"},
                {"Snare Filter Env Amount","Filter Dynamics"},{"Snare Drive","Analog Drive"},{"Snare Noise Ring Freq","VCO Mod Rate"},{"Snare Noise Ring Depth","VCO Mod Depth"},
                {"Snare Noise Ring Wave","VCO Mod Wave"},{"Snare Noise Ring Smooth","VCO Mod Smooth"},{"Snare Noise Ring Delay","VCO Mod Delay"},{"Snare Noise Ring Wave Shape","VCO Mod Shape"}
            });
            return juce::String (sub == 0 ? "808 Snare " : "Simmons Snare ")
                 + (sub == 1 && simmons.isNotEmpty() ? simmons : commonName);
        }
        if (sub == 2)
        {
            const auto acoustic = mappedName (structuralName, {
                {"Snare Level","Acoustic Snare Level"},{"Snare Click Level","Acoustic Twack"},{"Snare Body Decay","Acoustic Membrane Decay"},
                {"Snare Body Tune","Acoustic Membrane Tune"},{"Snare Body Level","Acoustic Membrane Level"},{"Snare Body Pitch Decay","Acoustic Mode Spread"},
                {"Snare Body Mid","Acoustic Mid Modes"},{"Snare Body High","Acoustic High Modes"},{"Snare Noise Level","Acoustic Wire Level"},
                {"Snare Noise Decay","Acoustic Wire Decay"},{"Snare Noise Tone","Acoustic Wire Tone"},{"Snare Noise Resonance","Acoustic Modal Resonance"},
                {"Snare Noise Tone Env","Acoustic Air Level"},{"Snare Noise Attack","Acoustic Wire Attack"},{"Snare Noise Color","Acoustic Noise Balance"},
                {"Snare Noise Degrade","Acoustic Sparse Noise"},{"Snare Filter Env Amount","Acoustic Membrane Coupling"},{"Snare Drive","Acoustic Extra Drive"},
                {"Snare Drive Mix","Acoustic Extra Drive Mix"},{"Snare Noise Ring Freq","Acoustic Low Wire Frequency"},{"Snare Noise Ring Depth","Acoustic Strike Variation"},
                {"Snare Noise Ring Wave","Acoustic Mode Set"},{"Snare Noise Ring Smooth","Acoustic Modal Damping"},{"Snare Noise Ring Delay","Acoustic Crack Time"},
                {"Snare Noise Ring Wave Shape","Acoustic Mode Balance"}
            });
            return acoustic.isNotEmpty() ? acoustic : commonName;
        }
        if(sub==8)
        {
            const auto linn=mappedName(structuralName,{
                {"Snare Level","Level"},{"Snare Click Level","Click Level"},{"Snare Body Decay","Body Decay"},{"Snare Body Tune","Body Tune"},
                {"Snare Body Level","Membrane Level"},{"Snare Body Pitch Decay","Pitch Envelope Amount"},{"Snare Body Mid","Mid Modes"},{"Snare Body High","High Modes"},
                {"Snare Noise Level","Wire Level"},{"Snare Noise Decay","Wire Decay"},{"Snare Noise Tone","Wire Tone"},{"Snare Noise Resonance","Wire Resonance"},
                {"Snare Noise Tone Env","Late Wire Brightness"},{"Snare Noise Attack","Wire Bloom"},{"Snare Noise Color","Wire Color"},{"Snare Noise Degrade","Digital Playback"},
                {"Snare Filter Env Amount","Body Wire Coupling"},{"Snare Drive","Drive"},{"Snare Drive Mix","Drive Mix"},{"Snare Noise Ring Freq","Noise Ring Frequency"},
                {"Snare Noise Ring Depth","Noise Ring Depth"},{"Snare Noise Ring Wave","Noise Ring Wave"},{"Snare Noise Ring Smooth","Noise Ring Smooth"},{"Snare Noise Ring Delay","Noise Ring Delay"},
                {"Snare Noise Ring Wave Shape","Pitch Decay Time"}
            });
            return "Linn Snare "+(linn.isNotEmpty()?linn:commonName);
        }
        auto selectedSuffix = suffix;
        if (sub == 6)
        {
            const auto hybrid = mappedName (structuralName, {
                {"Snare Body Decay","Body Decay"},{"Snare Body Tune","Body Tune"},{"Snare Body Pitch Decay","FM Amount"},{"Snare Body Mid","Second Oscillator"},
                {"Snare Body High","Oscillator Ratio"},{"Snare Noise Level","Wire Level"},{"Snare Noise Decay","Wire Decay"},{"Snare Noise Tone","Wire Tone"},
                {"Snare Noise Resonance","Wire Resonance"},{"Snare Noise Tone Env","FM Ratio"},{"Snare Noise Attack","Wire Attack"},{"Snare Noise Color","Noise Color"},
                {"Snare Filter Env Amount","FM Envelope"},{"Snare Noise Ring Delay","Attack"},{"Snare Noise Ring Wave Shape","Asymmetry"}
            });
            if (hybrid.isNotEmpty()) selectedSuffix = hybrid;
        }
        const auto prefix = sub == 6 ? "Hybrid FM Snare 1 "
                          : sub == 7 ? "Saike 909 Variant Snare 1 "
                                     : "Saike Type " + juce::String (sub - 2) + " Snare 1 ";
        return prefix + (selectedSuffix.isNotEmpty() ? selectedSuffix : commonName);
    }

    if (info.family == lr608::SlotFamily::clap)
    {
        const auto labels = mappedName (structuralName, {
            {"Clap Level","Level"},{"Clap Decay","Tail"},{"Clap Tone","Tone"},{"Clap Hit Level","Attack"},{"Clap Filter Tone","Tune"},
            {"Clap Resonance","Resonance"},{"Clap Character","Character"},{"Clap Filter Decay Follow","Sweep"},
            {"Clap Sequence Balance","Balance"},{"Clap Body Size","Body"},{"Clap Dirt","Dirt"}
        });
        const auto index = structuralName == "Clap Level" ? 0 : structuralName == "Clap Decay" ? 1 : structuralName == "Clap Tone" ? 2
                         : structuralName == "Clap Hit Level" ? 3 : structuralName == "Clap Filter Tone" ? 4 : structuralName == "Clap Resonance" ? 5
                         : structuralName == "Clap Character" ? 6 : structuralName == "Clap Filter Decay Follow" ? 7 : structuralName == "Clap Sequence Balance" ? 8
                         : structuralName == "Clap Body Size" ? 9 : structuralName == "Clap Dirt" ? 10 : -1;
        static constexpr const char* exact[][11] = {
            {"909 Ensemble Level","909 Ensemble Tail","909 Ensemble Tone","909 Hand Attack","909 Ensemble Tune","909 Ensemble Resonance","909 Hands","909 Ensemble Sweep","909 Dry Tail Balance","909 Hand Spread","909 Logic Noise"},
            {"909 Clap Level","909 Tail Decay","909 Noise Tone","909 Pop Level","909 Filter Tune","909 Filter Resonance","909 Burst Spacing","909 Filter Sweep","909 Burst Tail Balance","909 Clap Body","909 VCA Trash"},
            {"Roland Clap Level","Roland Clap Reverb Decay","Roland Clap Tone","Roland Clap Burst Level","Roland Clap Filter Tune","Roland Clap Resonance","Roland Clap 808 909 Character","Roland Clap Filter Sweep","Roland Clap Burst Reverb Balance","Roland Clap Body","Roland Clap VCA Dirt"}
        };
        if (index >= 0 && sub < 3) return exact[sub][index];
        if(index>=0&&sub==6)return "Linn Clap "+labels;
        return index >= 0 ? "Saike Clap " + labels : commonName;
    }

    if (info.family == lr608::SlotFamily::lowTom || info.family == lr608::SlotFamily::midTom || info.family == lr608::SlotFamily::highTom)
    {
        if (sub >= 2) return "Saike Type " + juce::String (sub - 2) + " " + structuralName;
        auto ending = structuralName;
        if (sub == 1)
        {
            ending = ending.replace (" Tune", " Tone").replace (" Pitch Decay", " Bend Time").replace (" Pitch Amount", " Bend")
                           .replace (" Wave Morph", " Noise Texture").replace (" Saturation", " Analog Drive")
                           .replace (" Noise LPF", " Noise Tone").replace (" Noise Decay", " Noise Tail");
        }
        else ending = ending.replace (" Click Level", " Click");
        return juce::String (sub == 0 ? "808 " : "Simmons ") + ending;
    }

    auto prefixed = [&] (const char* const* prefixes, int count, juce::String semantic)
    {
        return juce::String (prefixes[juce::jlimit (0, count - 1, sub)]) + " " + semantic;
    };
    if (info.family == lr608::SlotFamily::rim)
    {
        static constexpr const char* prefixes[]{"LR-608","Saike Type 0","Saike Type 1","Saike Type 2","Acoustic Cross-Stick","Acoustic Pop Rimshot"};
        auto semantic = mappedName (structuralName, {{"Rim Level","Rim Level"},{"Rim Decay","Rim Decay"},{"Rim Tone","Rim Tune"},{"Rim Click Level","Rim Exciter"},{"Rim Second Mode Level","Rim Body"},{"Rim Second Mode Tune","Rim Mode Spread"}});
        if (sub > 0) semantic = mappedName (structuralName, {{"Rim Level","Gain dB"},{"Rim Decay","Decay"},{"Rim Tone","Tune"},{"Rim Click Level","Pan"},{"Rim Second Mode Level","Second Mode Level"},{"Rim Second Mode Tune","Second Mode Tune"}});
        if(sub==4)semantic=mappedName(structuralName,{{"Rim Level","Gain dB"},{"Rim Decay","Decay"},{"Rim Tone","Stick Tune"},{"Rim Click Level","Stick Character"},{"Rim Second Mode Level","Shell Level"},{"Rim Second Mode Tune","Shell Tune"}});
        if(sub==5)semantic=mappedName(structuralName,{{"Rim Level","Gain dB"},{"Rim Decay","Decay"},{"Rim Tone","Body Tone"},{"Rim Click Level","Crack Character"},{"Rim Second Mode Level","Snare Wire Level"},{"Rim Second Mode Tune","Shell and Wire Tune"}});
        return semantic.isNotEmpty() ? prefixed (prefixes, 6, semantic) : commonName;
    }
    if (info.family == lr608::SlotFamily::hatClosed || info.family == lr608::SlotFamily::hatOpen)
    {
        static constexpr const char* prefixes[]{"LR-608","Ring Alloy","Noise PM","Modal Shell","Saike Type 0","Saike Type 1","Saike Type 2"};
        auto semantic = structuralName == "HiHat Resonance Mod Speed (Hz)" ? "HiHat Resonance Mod Speed" : structuralName;
        return prefixed (prefixes, 7, semantic);
    }
    if (info.family == lr608::SlotFamily::crash || info.family == lr608::SlotFamily::ride)
    {
        static constexpr const char* prefixes[]{"LR-608","Saike Type 0","Saike Type 1","Saike Type 2"};
        auto semantic = structuralName == "Cymbal Brightness" ? "Cymbal Tone" : structuralName == "Ride Pitch Relax" ? "Duty or Relax" : structuralName == "Ride Relax Speed" ? "Attack" : structuralName;
        return prefixed (prefixes, 4, semantic);
    }
    if (info.family == lr608::SlotFamily::maracas)
    {
        static constexpr const char* prefixes[]{"LR-608","Saike Type 0","Saike Type 1","Saike Type 2"};
        auto semantic = mappedName (structuralName, {{"Maracas Level","Level"},{"Maracas Attack","Attack"},{"Maracas Decay","Decay"},{"Maracas Noise Color","Tune or Color"},{"Maracas S&H Rate","Motion Rate"},{"Maracas S&H Amount","Motion Amount"},{"Maracas Bounce","Bounce"},{"Maracas Ghost Softness","Softness"}});
        return prefixed (prefixes, 4, semantic.isNotEmpty() ? semantic : commonName);
    }
    if (info.family == lr608::SlotFamily::cowbell)
    {
        static constexpr const char* prefixes[]{"LR-608","Saike Type 0","Saike Type 1","Saike Type 2","Saike Type 3","Timbales Physical","Timbales Wave Mesh"};
        auto semantic = mappedName (structuralName, {{"Clave/Cowbell Level","Level"},{"Clave/Cowbell Decay","Decay"},{"Clave/Cowbell Base Pitch","Tune"},{"Clave/Cowbell Inharmonic Ratio","Inharmonic Ratio"},{"Clave/Cowbell Click Amount","Click Amount"},{"Clave/Cowbell Click Sharpness","Click Sharpness"},{"Clave/Cowbell Metal Body Balance","Metal Body Balance"},{"Clave/Cowbell Ring Mod Amount","Ring Mod Amount"},{"Clave/Cowbell Accent Bite","Accent Bite"},{"Clave/Cowbell Pitch Snap Amount","Pitch Snap Amount"},{"Clave/Cowbell Saturation Mix","Saturation Mix"}});
        if (sub == 5 && structuralName == "Clave/Cowbell Click Amount") semantic = "Pitch Env Amount";
        if (sub == 5 && structuralName == "Clave/Cowbell Click Sharpness") semantic = "Pitch Env Velocity Time";
        return prefixed (prefixes, 7, semantic.isNotEmpty() ? semantic : commonName);
    }
    if (info.family == lr608::SlotFamily::zap)
    {
        static constexpr const char* prefixes[]{"LR-608","Clocked Alarm","Particle Beacon","Modal UFO","Karplus Wire","FM Siren","Grain Laser","Chaos Relay","Hyper Spring","Bouncing Coin","Karplus Wire Tune -"};
        auto semantic = mappedName (structuralName, {{"Zap Level","Level"},{"Zap Fast Decay","Fast Decay"},{"Zap Slow Decay","Slow Decay"},{"Zap Curve","Curve"},{"Zap Tune","Tune"},{"Zap Click Level","Click Level"},{"ZAP LFO Frequency","LFO Frequency"},{"ZAP LFO Pitch Depth","LFO Pitch Depth"},{"ZAP LFO Wave","LFO Wave"},{"ZAP LFO Smooth","LFO Smooth"},{"ZAP LFO Delay","LFO Delay"},{"ZAP LFO Wave Shape","LFO Wave Shape"},{"Zap Ring Mix","Ring Mix"},{"Zap Ring Frequency","Ring Frequency"},{"Zap Ring Wave","Ring Wave"},{"Zap Ring Env Freq Mod","Ring Envelope"},{"Zap Comp Threshold","Comp Threshold"},{"Zap Comp Ratio","Comp Ratio"},{"Zap Comp Gain","Comp Gain"},{"Zap Comp Attack (ms)","Comp Attack"},{"Zap Comp Release (ms)","Comp Release"},{"Zap Comp Mix","Comp Mix"}});
        return prefixed (prefixes, 11, semantic.isNotEmpty() ? semantic : commonName);
    }
    return commonName;
}

class ShortcutValueTextEditor final : public juce::TextEditor
{
public:
    ShortcutValueTextEditor (const juce::String& name, ValueEditorShortcut callback)
        : juce::TextEditor (name), shortcut (std::move (callback)) {}
    bool keyPressed (const juce::KeyPress& key) override
    {
        if ((key.getModifiers().isAltDown() || key.getKeyCode() == juce::KeyPress::returnKey
             || key.getKeyCode() == juce::KeyPress::escapeKey)
            && shortcut && shortcut (key, this))
            return true;
        return juce::TextEditor::keyPressed (key);
    }
private:
    ValueEditorShortcut shortcut;
};

class ShortcutSliderLabel final : public juce::Label
{
public:
    explicit ShortcutSliderLabel (ValueEditorShortcut callback) : shortcut (std::move (callback)) {}
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override {}
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
    { return createIgnoredAccessibilityHandler (*this); }
protected:
    juce::TextEditor* createEditorComponent() override
    {
        auto* textEditor = new ShortcutValueTextEditor (getName(), shortcut);
        textEditor->setInputRestrictions (0, "-0123456789.");
        textEditor->applyFontToAllText (getLookAndFeel().getLabelFont (*this));
        return textEditor;
    }
private:
    ValueEditorShortcut shortcut;
};

class ShortcutLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    explicit ShortcutLookAndFeel (ValueEditorShortcut callback) : shortcut (std::move (callback)) {}
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = new ShortcutSliderLabel (shortcut);
        label->setJustificationType (juce::Justification::centred);
        label->setKeyboardType (juce::TextInputTarget::decimalKeyboard);
        label->setColour (juce::Label::textColourId, slider.findColour (juce::Slider::textBoxTextColourId));
        label->setColour (juce::Label::backgroundColourId, slider.findColour (juce::Slider::textBoxBackgroundColourId));
        label->setColour (juce::Label::outlineColourId, slider.findColour (juce::Slider::textBoxOutlineColourId));
        return label;
    }
private:
    ValueEditorShortcut shortcut;
};

constexpr int stepWidths[] { 1, 5, 10, 15, 20 };
constexpr int parameterPageStep = 5;
constexpr int valuePageStep = 40;
// A restrained hybrid of classic analogue and early-digital drum-machine colours.
// These are deliberately high-contrast: the visual skin must remain as readable as
// the keyboard/screen-reader interface is usable.
const auto background = juce::Colour::fromRGB (0x12, 0x15, 0x18);
const auto panel = juce::Colour::fromRGB (0x20, 0x25, 0x2a);
const auto panelEdge = juce::Colour::fromRGB (0x59, 0x61, 0x69);
const auto foreground = juce::Colour::fromRGB (0xf2, 0xea, 0xd9);
const auto mutedText = juce::Colour::fromRGB (0xb8, 0xbe, 0xc3);
const auto orange = juce::Colour::fromRGB (0xe7, 0x79, 0x35);
const auto red = juce::Colour::fromRGB (0xc9, 0x4e, 0x45);
const auto cyan = juce::Colour::fromRGB (0x4f, 0xc3, 0xc8);
const auto yellow = juce::Colour::fromRGB (0xe9, 0xb9, 0x49);
const juce::Identifier presetBrowserDirectoryState{"presetBrowserDirectory"};
const juce::Identifier presetBrowserSelectionState{"presetBrowserSelection"};
const juce::Identifier presetBrowserRowState{"presetBrowserRow"};
}

struct LR608AudioProcessorEditor::SlotReportModel final : public juce::ListBoxModel
{
    SlotReportModel(LR608AudioProcessorEditor& editor,bool isFilled):owner(editor),filled(isFilled){}
    int getNumRows() override{return int((filled?owner.slotReportFilledEntries:owner.slotReportEmptyEntries).size());}
    void paintListBoxItem(int row,juce::Graphics&g,int width,int height,bool selected) override
    {
        const auto&entries=filled?owner.slotReportFilledEntries:owner.slotReportEmptyEntries;
        if(!juce::isPositiveAndBelow(row,int(entries.size())))return;
        if(selected)g.fillAll(foreground);g.setColour(selected?background:foreground);g.setFont(juce::FontOptions(15.0f,juce::Font::bold));
        g.drawText(entries[std::size_t(row)].text,8,0,width-16,height,juce::Justification::centredLeft,true);
    }
    juce::String getNameForRow(int row) override
    {
        const auto&entries=filled?owner.slotReportFilledEntries:owner.slotReportEmptyEntries;
        return juce::isPositiveAndBelow(row,int(entries.size()))?entries[std::size_t(row)].text:juce::String();
    }
    void selectedRowsChanged(int row) override
    {
        auto&list=filled?owner.slotReportFilled:owner.slotReportEmpty;
        if(owner.slotReportOpen&&list.hasKeyboardFocus(true))owner.announceSlotReportEntry(filled,row);
    }
    void returnKeyPressed(int row) override{owner.activateSlotReportEntry(filled,row);}
    LR608AudioProcessorEditor& owner;
    bool filled=false;
};

LR608AudioProcessorEditor::LR608AudioProcessorEditor (LR608AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    visualLookAndFeel = std::make_unique<juce::LookAndFeel_V4>();
    visualLookAndFeel->setColour (juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB (0x20, 0x25, 0x2a));
    visualLookAndFeel->setColour (juce::PopupMenu::textColourId, foreground);
    visualLookAndFeel->setColour (juce::PopupMenu::highlightedBackgroundColourId, orange);
    visualLookAndFeel->setColour (juce::PopupMenu::highlightedTextColourId, background);
    setLookAndFeel (visualLookAndFeel.get());
    shortcutLookAndFeel = std::make_unique<ShortcutLookAndFeel> (
        [this] (const juce::KeyPress& key, juce::Component* source) { return keyPressed (key, source); });
    setSize (900, 570);
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    setWantsKeyboardFocus (true);
    addKeyListener (this);

    title.setText ("LR-608", juce::dontSendNotification);
    title.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    title.setJustificationType (juce::Justification::centred);
    title.setAccessible (false);
    status.setAccessible (true);
    status.setTitle ("Status");
    status.setJustificationType (juce::Justification::centred);

    for (std::size_t index = 0; index < std::size (lr608::generated::pages); ++index)
        pageSelector.addItem (lr608::generated::pages[index].name, static_cast<int> (index + 1));
    pageSelector.setAccessible(false);

    for(int slot=lr608::slotCount-1;slot>=0;--slot)slotSelector.addItem(slotDisplayName(slot),slot+1);
    // Display Off first, followed by the engine catalogue in its natural order.
    // Item IDs remain engineIndex + 1, so presets and saved automation retain
    // exactly the same engine indices.
    engineSelector.addItem(lr608::slotEngines[lr608::offEngineIndex].name,lr608::offEngineIndex+1);
    for(int engine=0;engine<lr608::offEngineIndex;++engine)engineSelector.addItem(lr608::slotEngines[engine].name,engine+1);
    for(int note=0;note<128;++note)noteSelector.addItem(juce::String(note),note+1);
    chokeTriggerSelector.addItem("Off",1);chokeTargetSelector.addItem("Off",1);
    for(int note=0;note<128;++note){chokeTriggerSelector.addItem(juce::String(note),note+2);chokeTargetSelector.addItem(juce::String(note),note+2);}
    for(int output=lr608::OutputStage::stemCount-1;output>=0;--output)outputSelector.addItem("Stereo "+juce::String(output*2+1)+"/"+juce::String(output*2+2),output+1);
    slotSelector.setTitle("Slot"); engineSelector.setTitle("Engine"); noteSelector.setTitle("MIDI Note");chokeTriggerSelector.setTitle("MIDI Choke Trigger");chokeTargetSelector.setTitle("MIDI Choke Target");outputSelector.setTitle("Output");
    slotSelector.setDescription("Slot. F2 edits the Slot name. Left and Right select a column. Alt plus navigation changes the value");
    engineSelector.setDescription("Engine for the current slot. The MIDI Note remains unchanged");
    noteSelector.setDescription("MIDI Note belonging to the current slot. Up to 8 active Slot layers are allowed per note");
    chokeTriggerSelector.setDescription("MIDI Note that triggers this Slot's choke link. Off disables it");
    chokeTargetSelector.setDescription("MIDI Note whose active voices are choked. Off disables it");
    outputSelector.setDescription("Stereo output belonging to the current slot");
    slotSelector.setExplicitFocusOrder(1);engineSelector.setExplicitFocusOrder(2);noteSelector.setExplicitFocusOrder(3);chokeTriggerSelector.setExplicitFocusOrder(4);chokeTargetSelector.setExplicitFocusOrder(5);outputSelector.setExplicitFocusOrder(6);
    slotSelector.onChange=[this]{if(updatingSlotBar||globalOpen)return;processor.captureSlotFromProxy(selectedSlot);const auto item=parameterSelector.getSelectedItemIndex();if(item>=0){rememberedGridIndices[selectedSlot]=item;processor.setSlotGridPosition(selectedSlot,item);}selectedSlot=juce::jlimit(0,lr608::slotCount-1,slotSelector.getSelectedId()-1);saveUiPosition(false);syncSlotBar(true);};
    engineSelector.onChange=[this]{if(updatingSlotBar||globalOpen)return;processor.captureSlotFromProxy(selectedSlot);processor.setSlotEngine(selectedSlot,engineSelector.getSelectedId()-1);syncSlotBar(true);saveUiPosition(false);};
    noteSelector.onChange=[this]{if(updatingSlotBar||globalOpen)return;const auto note=noteSelector.getSelectedItemIndex();if(!processor.canAssignSlotToMidiNote(selectedSlot,note)){syncSlotBar(false);announce("MIDI Note "+juce::String(note)+" already has 8 layers");return;}if(auto*p=processor.parameters.getParameter(lr608::slotNoteId(selectedSlot))){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(note)));p->endChangeGesture();}saveUiPosition(false);};
    chokeTriggerSelector.onChange=[this]{if(updatingSlotBar||globalOpen)return;if(auto*p=processor.parameters.getParameter(lr608::slotChokeTriggerId(selectedSlot))){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(chokeTriggerSelector.getSelectedItemIndex())));p->endChangeGesture();}saveUiPosition(false);};
    chokeTargetSelector.onChange=[this]{if(updatingSlotBar||globalOpen)return;if(auto*p=processor.parameters.getParameter(lr608::slotChokeTargetId(selectedSlot))){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(chokeTargetSelector.getSelectedItemIndex())));p->endChangeGesture();}saveUiPosition(false);};
    outputSelector.onChange=[this]{if(updatingSlotBar||globalOpen)return;processor.setSlotOutput(selectedSlot,outputSelector.getSelectedId()-1);saveUiPosition(false);};

    parameterSelector.setTitle ("Parameter");
    parameterSelector.setExplicitFocusOrder (7);
    parameterSelector.onChange = [this] { selectParameter(); };
    parameterValue.setLookAndFeel (shortcutLookAndFeel.get());
    parameterValue.setSliderStyle (juce::Slider::LinearHorizontal);
    parameterValue.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 190, 30);
    parameterValue.setWantsKeyboardFocus (true);
    parameterValue.setExplicitFocusOrder (8);
    parameterValue.onValueChange = [this]
    {
        updateParameterLabel();
        const auto item = parameterSelector.getSelectedItemIndex();
        if (! juce::isPositiveAndBelow (item, static_cast<int> (visibleCatalogIndices.size())))
            return;
        handleContextualParameterChange (
            lr608::generated::parameters[visibleCatalogIndices[item]].id);
        if(!globalOpen)processor.captureSlotParameter(selectedSlot,lr608::generated::parameters[visibleCatalogIndices[item]].id);
    };
    parameterValue.setTitle ("Value");

    reset.setDescription ("Resets the selected parameter. Shortcut Backspace");
    reset.onClick = [this] { resetSelected(); };
    reset.setExplicitFocusOrder (9);
    initialize.setDescription ("Initializes the complete LR-608 kit. Shortcut Alt I");
    initialize.onClick = [this] { initializeAll(); };
    initialize.setExplicitFocusOrder (10);
    clearSlots.setDescription ("Clear all 128 Slots by setting every Engine to Off");
    clearSlots.onClick = [this] { clearAllSlots(); };
    clearSlots.setExplicitFocusOrder (11);
    previousPreset.setDescription ("Loads the previous preset. Shortcut Alt minus");
    previousPreset.onClick = [this] { changePreset (-1); };
    previousPreset.setExplicitFocusOrder (12);
    nextPreset.setDescription ("Loads the next preset. Shortcut Alt plus");
    nextPreset.onClick = [this] { changePreset (1); };
    nextPreset.setExplicitFocusOrder (13);
    loadPreset.setDescription("Opens the accessible preset browser. Shortcut Alt B");
    loadPreset.setExplicitFocusOrder(14);loadPreset.onClick=[this]{togglePresetBrowser();};
    savePreset.setDescription("Saves the complete LR-608 kit with a name. Shortcut Alt S");
    savePreset.setExplicitFocusOrder(15);savePreset.onClick=[this]{showPresetSave();};
    help.setExplicitFocusOrder (16);
    help.setDescription("Opens the HTML help language menu. Shortcut Alt H");
    help.onClick = [this] { showHelpLanguageMenu(); };

    for (auto* component : std::array<juce::Component*, 18> {
                              &title, &status, &slotSelector, &engineSelector,&noteSelector,&chokeTriggerSelector,&chokeTargetSelector,&outputSelector,&parameterSelector,
                              &parameterValue, &reset, &initialize, &clearSlots, &previousPreset,
                              &nextPreset, &loadPreset,&savePreset,&help })
    {
        component->setColour (juce::Label::textColourId, foreground);
        addAndMakeVisible (*component);
        component->addKeyListener (this);
    }

    for (auto* box : std::array<juce::ComboBox*, 7> { &slotSelector, &engineSelector,
                                                       &noteSelector, &chokeTriggerSelector,
                                                       &chokeTargetSelector, &outputSelector,
                                                       &parameterSelector })
    {
        box->setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromRGB (0x2a, 0x30, 0x35));
        box->setColour (juce::ComboBox::textColourId, foreground);
        box->setColour (juce::ComboBox::outlineColourId, panelEdge);
        box->setColour (juce::ComboBox::arrowColourId, orange);
        box->setColour (juce::ComboBox::focusedOutlineColourId, cyan);
    }

    for (auto* button : std::array<juce::TextButton*, 8> { &reset, &initialize, &clearSlots, &previousPreset,
                                                           &nextPreset, &loadPreset, &savePreset,
                                                           &help })
    {
        button->setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (0x2a, 0x30, 0x35));
        button->setColour (juce::TextButton::buttonOnColourId, orange);
        button->setColour (juce::TextButton::textColourOffId, foreground);
        button->setColour (juce::TextButton::textColourOnId, background);
    }
    initialize.setColour (juce::TextButton::buttonColourId, red.darker (0.12f));
    clearSlots.setColour (juce::TextButton::buttonColourId, orange.darker (0.48f));
    loadPreset.setColour (juce::TextButton::buttonColourId, cyan.darker (0.58f));
    savePreset.setColour (juce::TextButton::buttonColourId, cyan.darker (0.58f));
    help.setColour (juce::TextButton::buttonColourId, yellow.darker (0.58f));

    parameterValue.setColour (juce::Slider::backgroundColourId, juce::Colour::fromRGB (0x2a, 0x30, 0x35));
    parameterValue.setColour (juce::Slider::trackColourId, cyan.darker (0.18f));
    parameterValue.setColour (juce::Slider::thumbColourId, orange);
    parameterValue.setColour (juce::Slider::textBoxTextColourId, foreground);
    parameterValue.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB (0x18, 0x1c, 0x20));
    parameterValue.setColour (juce::Slider::textBoxOutlineColourId, panelEdge);
    status.setColour (juce::Label::backgroundColourId, juce::Colour::fromRGB (0x18, 0x1c, 0x20));
    status.setColour (juce::Label::textColourId, foreground);

    presetBrowserPath.setTitle("Preset browser folder");presetBrowserPath.setJustificationType(juce::Justification::centredLeft);
    presetBrowser.setTitle("Preset browser");presetBrowser.setDescription("Folders and valid LR-608 presets. Selection previews the complete kit. Enter opens or confirms. Delete asks to remove a selected user preset. Backspace goes to the parent folder. Alt C cancels and restores the previous kit.");presetBrowser.setAccessible(true);presetBrowser.setMultipleSelectionEnabled(false);presetBrowser.setRowHeight(32);presetBrowser.setOutlineThickness(1);
    presetBrowserBack.setDescription("Goes to the parent preset folder. Shortcut Backspace");presetBrowserBack.onClick=[this]{goToParentPresetFolder();};
    presetBrowserClose.setDescription("Closes the browser and restores the previous kit. Shortcut Alt C");presetBrowserClose.onClick=[this]{closePresetBrowser();};
    presetDeleteLabel.setTitle("Delete preset confirmation");presetDeleteLabel.setJustificationType(juce::Justification::centred);
    presetDeleteYes.setDescription("Permanently deletes the selected preset. Shortcut Y");presetDeleteYes.onClick=[this]{presetDeleteChoiceYes=true;confirmPresetDelete();};
    presetDeleteNo.setDescription("Cancels preset deletion. Default choice. Shortcut N");presetDeleteNo.onClick=[this]{dismissPresetDeleteConfirmation();};
    presetSaveLabel.setText("Preset name",juce::dontSendNotification);presetSaveLabel.setTitle("Save preset");
    presetSaveName.setTitle("Preset name");presetSaveName.setDescription("Type a name for the complete LR-608 kit");presetSaveName.setReturnKeyStartsNewLine(false);presetSaveName.onReturnKey=[this]{commitPresetSave();};presetSaveName.onEscapeKey=[this]{closePresetSave();};
    presetSaveConfirm.onClick=[this]{commitPresetSave();};presetSaveCancel.setDescription("Closes Save preset without saving");presetSaveCancel.onClick=[this]{closePresetSave();};
    presetOverwriteLabel.setTitle("Overwrite preset confirmation");presetOverwriteLabel.setJustificationType(juce::Justification::centred);
    presetOverwriteYes.setDescription("Overwrites the existing preset");presetOverwriteYes.onClick=[this]{confirmPresetOverwrite();};
    presetOverwriteNo.setDescription("Does not overwrite and returns to the preset name. Default choice");presetOverwriteNo.onClick=[this]{dismissPresetOverwriteConfirmation();};
    presetOverwriteClose.setDescription("Closes Save preset and cancels the save");presetOverwriteClose.onClick=[this]{closePresetSave();};
    for(auto*component:std::array<juce::Component*,15>{&presetBrowserPath,&presetBrowser,&presetBrowserBack,&presetBrowserClose,&presetDeleteLabel,&presetDeleteYes,&presetDeleteNo,&presetSaveLabel,&presetSaveName,&presetSaveConfirm,&presetSaveCancel,&presetOverwriteLabel,&presetOverwriteYes,&presetOverwriteNo,&presetOverwriteClose}){addAndMakeVisible(*component);component->addKeyListener(this);component->setVisible(false);}

    slotNameLabel.setText("Slot name",juce::dontSendNotification);slotNameLabel.setTitle("Rename Slot");slotNameLabel.setJustificationType(juce::Justification::centred);
    slotNameEditor.setTitle("Slot name");slotNameEditor.setDescription("Type a name for Slot "+juce::String(selectedSlot+1)+". An empty name removes the identity");slotNameEditor.setReturnKeyStartsNewLine(false);slotNameEditor.setInputRestrictions(48);slotNameEditor.onReturnKey=[this]{commitSlotName();};slotNameEditor.onEscapeKey=[this]{closeSlotNameEditor();};
    slotNameConfirm.setDescription("Saves the Slot name. Shortcut Enter");slotNameConfirm.onClick=[this]{commitSlotName();};slotNameCancel.setDescription("Closes without changing the Slot name. Shortcut Escape");slotNameCancel.onClick=[this]{closeSlotNameEditor();};
    for(auto*component:std::array<juce::Component*,4>{&slotNameLabel,&slotNameEditor,&slotNameConfirm,&slotNameCancel}){addAndMakeVisible(*component);component->addKeyListener(this);component->setVisible(false);}

    slotReportFilledModel=std::make_unique<SlotReportModel>(*this,true);slotReportEmptyModel=std::make_unique<SlotReportModel>(*this,false);
    slotReportFilled.setModel(slotReportFilledModel.get());slotReportEmpty.setModel(slotReportEmptyModel.get());
    slotReportSummary.setTitle("Slot report");slotReportSummary.setJustificationType(juce::Justification::centred);
    slotReportFilledLabel.setText("Filled Slots",juce::dontSendNotification);slotReportFilledLabel.setTitle("Filled Slots");slotReportFilledLabel.setJustificationType(juce::Justification::centred);
    slotReportEmptyLabel.setText("Empty Slots",juce::dontSendNotification);slotReportEmptyLabel.setTitle("Empty Slots");slotReportEmptyLabel.setJustificationType(juce::Justification::centred);
    slotReportFilled.setTitle("Filled Slots");slotReportFilled.setDescription("Filled Slots and grouped MIDI Note layers. Enter goes to the first Slot in the selected entry");
    slotReportEmpty.setTitle("Empty Slots");slotReportEmpty.setDescription("Empty Slots. Enter goes to the selected Slot");
    for(auto*list:{&slotReportFilled,&slotReportEmpty}){list->setAccessible(true);list->setMultipleSelectionEnabled(false);list->setRowHeight(34);list->setOutlineThickness(1);}
    slotReportClose.setDescription("Closes the Slot report. Shortcut Escape");slotReportClose.onClick=[this]{closeSlotReport();};
    for(auto*component:std::array<juce::Component*,6>{&slotReportSummary,&slotReportFilledLabel,&slotReportEmptyLabel,&slotReportFilled,&slotReportEmpty,&slotReportClose}){addAndMakeVisible(*component);component->addKeyListener(this);component->setVisible(false);}

    for (auto* label : std::array<juce::Label*, 8> { &presetBrowserPath, &presetDeleteLabel,
                                                     &presetSaveLabel, &presetOverwriteLabel,
                                                     &slotNameLabel, &slotReportSummary,
                                                     &slotReportFilledLabel, &slotReportEmptyLabel })
        label->setColour (juce::Label::textColourId, foreground);

    for (auto* editor : std::array<juce::TextEditor*, 2> { &presetSaveName, &slotNameEditor })
    {
        editor->setColour (juce::TextEditor::backgroundColourId, juce::Colour::fromRGB (0x18, 0x1c, 0x20));
        editor->setColour (juce::TextEditor::textColourId, foreground);
        editor->setColour (juce::TextEditor::outlineColourId, panelEdge);
        editor->setColour (juce::TextEditor::focusedOutlineColourId, cyan);
        editor->setColour (juce::TextEditor::highlightColourId, orange.withAlpha (0.72f));
        editor->setColour (juce::TextEditor::highlightedTextColourId, background);
    }

    for (auto* list : std::array<juce::ListBox*, 3> { &presetBrowser, &slotReportFilled, &slotReportEmpty })
    {
        list->setColour (juce::ListBox::backgroundColourId, juce::Colour::fromRGB (0x18, 0x1c, 0x20));
        list->setColour (juce::ListBox::outlineColourId, panelEdge);
    }

    for (auto* button : std::array<juce::TextButton*, 12> {
             &presetBrowserBack, &presetBrowserClose, &presetDeleteYes, &presetDeleteNo,
             &presetSaveConfirm, &presetSaveCancel, &presetOverwriteYes, &presetOverwriteNo,
             &presetOverwriteClose, &slotNameConfirm, &slotNameCancel, &slotReportClose,
             })
    {
        button->setColour (juce::TextButton::buttonColourId, juce::Colour::fromRGB (0x2a, 0x30, 0x35));
        button->setColour (juce::TextButton::buttonOnColourId, orange);
        button->setColour (juce::TextButton::textColourOffId, foreground);
        button->setColour (juce::TextButton::textColourOnId, background);
    }

    selectedSlot=juce::jlimit(0,lr608::slotCount-1,int(processor.parameters.state.getProperty("uiSelectedSlot",0)));
    selectedSlotColumn=0;
    positionWasInGrid=false;
    for(int slot=0;slot<lr608::slotCount;++slot)rememberedGridIndices[slot]=processor.getSlotGridPosition(slot);
    globalGridIndex=juce::jmax(0,int(processor.parameters.state.getProperty("uiGlobalGrid",0)));
    syncSlotBar(true);
    lastMidiNavigationEvent=processor.getMidiNavigationEvent();
    midiCycleSlots.fill(-1);
    startTimerHz(30);
}

LR608AudioProcessorEditor::~LR608AudioProcessorEditor()
{
    removeKeyListener (this);
    stopTimer();
    cancelPendingUpdate();
    pendingShortcutFocusTarget = nullptr;
    parameterValue.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
    shortcutLookAndFeel.reset();
    visualLookAndFeel.reset();
}

void LR608AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    g.setColour (juce::Colour::fromRGB (0x31, 0x37, 0x3d));
    g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (10.0f), 8.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (10.5f), 8.0f, 1.5f);

    // Four small identity bands suggest the mixed analogue/digital ancestry
    // without copying the faceplate of any one vintage instrument.
    const auto stripeY = 18.0f;
    const auto stripeX = 28.0f;
    const auto stripeWidth = (getWidth() - 56.0f) / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        const juce::Colour colours[] { orange, foreground.darker (0.18f), red, cyan };
        g.setColour (colours[i]);
        g.fillRect (stripeX + stripeWidth * i, stripeY, stripeWidth, 4.0f);
    }

    const auto overlayOpen = presetBrowserOpen || presetSaveOpen || presetDeleteConfirmationOpen
                          || presetOverwriteConfirmationOpen || slotNameEditorOpen || slotReportOpen;
    if (overlayOpen)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (26.0f), 7.0f);
        g.setColour (panelEdge);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (26.5f), 7.0f, 1.0f);
        return;
    }

    if (! globalOpen)
    {
        auto slotPanel = slotSelector.getBounds().getUnion (outputSelector.getBounds()).expanded (10, 27);
        slotPanel.setTop (slotSelector.getY() - 23);
        slotPanel.setBottom (slotSelector.getBottom() + 10);
        g.setColour (panel);
        g.fillRoundedRectangle (slotPanel.toFloat(), 6.0f);
        g.setColour (panelEdge);
        g.drawRoundedRectangle (slotPanel.toFloat(), 6.0f, 1.0f);

        const char* headings[] { "SLOT", "ENGINE", "MIDI NOTE", "CHOKE TRIGGER", "CHOKE TARGET", "OUTPUT" };
        const juce::Component* columns[] { &slotSelector, &engineSelector, &noteSelector,
                                           &chokeTriggerSelector, &chokeTargetSelector, &outputSelector };
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.setColour (mutedText);
        for (int i = 0; i < 6; ++i)
            g.drawText (headings[i], columns[i]->getX() + 5, columns[i]->getY() - 21,
                        columns[i]->getWidth() - 10, 18, juce::Justification::centredLeft, true);
    }

    auto parameterPanel = parameterSelector.getBounds().getUnion (parameterValue.getBounds()).expanded (10, 28);
    parameterPanel.setTop (parameterSelector.getY() - 23);
    parameterPanel.setBottom (parameterValue.getBottom() + 10);
    g.setColour (panel);
    g.fillRoundedRectangle (parameterPanel.toFloat(), 6.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (parameterPanel.toFloat(), 6.0f, 1.0f);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.setColour (mutedText);
    g.drawText (globalOpen ? "GLOBAL PARAMETERS" : "ENGINE PARAMETERS",
                parameterSelector.getX() + 5, parameterSelector.getY() - 21,
                parameterSelector.getWidth() - 10, 18, juce::Justification::centredLeft, true);

    g.setFont (juce::FontOptions (12.5f));
    g.setColour (mutedText);
    const auto guide = globalOpen
        ? juce::String ("Choose a global parameter, then drag the control or use the keyboard.  Enter confirms - Escape cancels")
        : juce::String ("1  Choose a Slot    2  Choose an Engine    3  Set MIDI Note and Output    4  Shape the sound below");
    g.drawText (guide, 34, title.getBottom() - 2, getWidth() - 68, 22,
                juce::Justification::centred, true);

    if (! globalOpen)
    {
        g.setFont (juce::FontOptions (12.0f));
        g.setColour (mutedText);
        g.drawText ("Keyboard:  Alt+D Slot area   |   Alt+L Parameters   |   Alt+B Presets   |   Alt+H Complete guide",
                    34, status.getY() - 42, getWidth() - 68, 24,
                    juce::Justification::centred, true);
    }
}

void LR608AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (30);
    title.setBounds (area.removeFromTop (42));
    area.removeFromTop (30);
    area.removeFromTop (18);
    auto slotBar=area.removeFromTop(38);const auto columnWidth=slotBar.getWidth()/6;
    slotSelector.setBounds(slotBar.removeFromLeft(columnWidth));engineSelector.setBounds(slotBar.removeFromLeft(columnWidth));noteSelector.setBounds(slotBar.removeFromLeft(columnWidth));chokeTriggerSelector.setBounds(slotBar.removeFromLeft(columnWidth));chokeTargetSelector.setBounds(slotBar.removeFromLeft(columnWidth));outputSelector.setBounds(slotBar);
    area.removeFromTop (32);
    parameterSelector.setBounds (area.removeFromTop (42));
    area.removeFromTop (12);
    parameterValue.setBounds (area.removeFromTop (82));
    area.removeFromTop (10);
    auto actions=area.removeFromTop(36);reset.setBounds(actions.removeFromLeft(180));actions.removeFromLeft(16);initialize.setBounds(actions.removeFromLeft(130));actions.removeFromLeft(10);clearSlots.setBounds(actions.removeFromLeft(130));help.setBounds(actions.removeFromRight(100));
    area.removeFromTop(10);
    auto buttons=area.removeFromTop(36);const auto width=(buttons.getWidth()-30)/4;previousPreset.setBounds(buttons.removeFromLeft(width));buttons.removeFromLeft(10);nextPreset.setBounds(buttons.removeFromLeft(width));buttons.removeFromLeft(10);loadPreset.setBounds(buttons.removeFromLeft(width));buttons.removeFromLeft(10);savePreset.setBounds(buttons);
    status.setBounds (area.removeFromBottom (44));

    auto overlay=getLocalBounds().reduced(36);presetBrowserPath.setBounds(overlay.removeFromTop(38));overlay.removeFromTop(10);auto browserButtons=overlay.removeFromBottom(36);presetBrowserBack.setBounds(browserButtons.removeFromLeft(130));presetBrowserClose.setBounds(browserButtons.removeFromRight(130));overlay.removeFromBottom(10);presetBrowser.setBounds(overlay);
    auto deleteArea=getLocalBounds().withSizeKeepingCentre(560,190);presetDeleteLabel.setBounds(deleteArea.removeFromTop(76));deleteArea.removeFromTop(18);auto deleteButtons=deleteArea.removeFromTop(38);presetDeleteYes.setBounds(deleteButtons.removeFromLeft(180));presetDeleteNo.setBounds(deleteButtons.removeFromRight(180));
    auto saveArea=getLocalBounds().withSizeKeepingCentre(520,190);presetSaveLabel.setBounds(saveArea.removeFromTop(36));saveArea.removeFromTop(8);presetSaveName.setBounds(saveArea.removeFromTop(38));saveArea.removeFromTop(20);auto saveButtons=saveArea.removeFromTop(36);presetSaveConfirm.setBounds(saveButtons.removeFromLeft(150));presetSaveCancel.setBounds(saveButtons.removeFromRight(150));
    auto overwriteArea=getLocalBounds().withSizeKeepingCentre(560,190);presetOverwriteLabel.setBounds(overwriteArea.removeFromTop(76));overwriteArea.removeFromTop(18);auto overwriteButtons=overwriteArea.removeFromTop(38);presetOverwriteYes.setBounds(overwriteButtons.removeFromLeft(140));overwriteButtons.removeFromLeft(20);presetOverwriteClose.setBounds(overwriteButtons.removeFromRight(140));overwriteButtons.removeFromRight(20);presetOverwriteNo.setBounds(overwriteButtons);
    auto slotNameArea=getLocalBounds().withSizeKeepingCentre(520,190);slotNameLabel.setBounds(slotNameArea.removeFromTop(36));slotNameArea.removeFromTop(8);slotNameEditor.setBounds(slotNameArea.removeFromTop(38));slotNameArea.removeFromTop(20);auto slotNameButtons=slotNameArea.removeFromTop(36);slotNameConfirm.setBounds(slotNameButtons.removeFromLeft(150));slotNameCancel.setBounds(slotNameButtons.removeFromRight(150));
    auto reportArea=getLocalBounds().reduced(30);slotReportSummary.setBounds(reportArea.removeFromTop(38));reportArea.removeFromTop(6);auto reportHeadings=reportArea.removeFromTop(34);const auto reportGap=12;const auto reportWidth=(reportHeadings.getWidth()-reportGap)/2;slotReportFilledLabel.setBounds(reportHeadings.removeFromLeft(reportWidth));reportHeadings.removeFromLeft(reportGap);slotReportEmptyLabel.setBounds(reportHeadings);auto reportButtonArea=reportArea.removeFromBottom(38);slotReportClose.setBounds(reportButtonArea.withSizeKeepingCentre(150,38));reportArea.removeFromBottom(8);auto filledArea=reportArea.removeFromLeft(reportWidth);reportArea.removeFromLeft(reportGap);slotReportFilled.setBounds(filledArea);slotReportEmpty.setBounds(reportArea);
}

void LR608AudioProcessorEditor::focusGained (FocusChangeType) { scheduleInitialFocusTransfer(); }
void LR608AudioProcessorEditor::focusOfChildComponentChanged(FocusChangeType)
{
    if(globalOpen)return;
    auto* focused=juce::Component::getCurrentlyFocusedComponent();
    if(focused==&slotSelector){selectedSlotColumn=0;saveUiPosition(false);}
    else if(focused==&engineSelector){selectedSlotColumn=1;saveUiPosition(false);}
    else if(focused==&noteSelector){selectedSlotColumn=2;saveUiPosition(false);}
    else if(focused==&chokeTriggerSelector){selectedSlotColumn=3;saveUiPosition(false);}
    else if(focused==&chokeTargetSelector){selectedSlotColumn=4;saveUiPosition(false);}
    else if(focused==&outputSelector){selectedSlotColumn=5;saveUiPosition(false);}
    else if(focused==&parameterSelector||focused==&parameterValue||(focused!=nullptr&&parameterValue.isParentOf(focused)))saveUiPosition(true);
}
void LR608AudioProcessorEditor::visibilityChanged() { if (isVisible()) scheduleInitialFocusTransfer(); }

std::unique_ptr<juce::AccessibilityHandler> LR608AudioProcessorEditor::createAccessibilityHandler()
{
    return createIgnoredAccessibilityHandler (*this);
}

void LR608AudioProcessorEditor::scheduleInitialFocusTransfer()
{
    if (initialFocusTransferPending)
        return;
    initialFocusTransferPending = true;
    juce::Timer::callAfterDelay (150, [safe = juce::Component::SafePointer (this)]
    {
        if (safe == nullptr)
            return;
        safe->initialFocusTransferPending = false;
        safe->performInitialFocusTransfer();
    });
}

void LR608AudioProcessorEditor::performInitialFocusTransfer()
{
    if (! isShowing() || presetBrowserOpen || presetSaveOpen || slotNameEditorOpen || slotReportOpen)
        return;
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused == &slotSelector)
        return;
    auto* peer = getPeer();
    auto* target = static_cast<juce::Component*> (&slotSelector);
    auto* handler = target->getAccessibilityHandler();
    const auto nativeAccessibilityReady = handler != nullptr
        && handler->getNativeImplementation() != nullptr;
    // This is the critical LJuno guard: merely selecting LR-608 in an FX Chain
    // must never pull focus away from another plug-in or from the chain itself.
    if (peer == nullptr || ! peer->isFocused() || ! nativeAccessibilityReady)
    {
        if (++initialFocusTransferAttempts < 8)
            scheduleInitialFocusTransfer();
        return;
    }
    initialFocusTransferAttempts = 0;
    target->grabKeyboardFocus();
    if (target->hasKeyboardFocus (false) && handler != nullptr)
        handler->grabFocus();
    if (! target->hasKeyboardFocus (false) && ++initialFocusTransferAttempts < 8)
        scheduleInitialFocusTransfer();
}

void LR608AudioProcessorEditor::requestShortcutFocus (juce::Component& target)
{
    pendingShortcutFocusTarget = &target;
    triggerAsyncUpdate();
}

void LR608AudioProcessorEditor::handleAsyncUpdate()
{
    auto* target = pendingShortcutFocusTarget;
    pendingShortcutFocusTarget = nullptr;
    if (target == nullptr || ! isParentOf (target) || ! target->isShowing())
        return;
    parameterValue.hideTextBox (false);
    juce::AccessibilityHandler::clearCurrentlyFocusedHandler();
    target->grabKeyboardFocus();
    if (auto* handler = target->getAccessibilityHandler())
        handler->grabFocus();
}

void LR608AudioProcessorEditor::announce (const juce::String& message)
{
    status.setText (message, juce::dontSendNotification);
    lr608::announceToActiveScreenReader (status, message);
}

void LR608AudioProcessorEditor::announceMidiNavigation (const juce::String& message)
{
    status.setText (message, juce::dontSendNotification);
    lr608::announceToActiveScreenReader (status, message, true);
}

void LR608AudioProcessorEditor::saveUiPosition(bool inGrid)
{
    positionWasInGrid=inGrid;
    processor.parameters.state.setProperty("uiSelectedSlot",selectedSlot,nullptr);
    processor.parameters.state.setProperty("uiSelectedColumn",selectedSlotColumn,nullptr);
    processor.parameters.state.setProperty("uiInGrid",inGrid,nullptr);
    const auto item=parameterSelector.getSelectedItemIndex();
    if(!globalOpen&&item>=0){rememberedGridIndices[selectedSlot]=item;processor.setSlotGridPosition(selectedSlot,item);}
}

void LR608AudioProcessorEditor::syncSlotBar(bool updateGrid, bool notifyGrid)
{
    const juce::ScopedValueSetter guard(updatingSlotBar,true);
    const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(selectedSlot))->load()));
    const auto note=juce::jlimit(0,127,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotNoteId(selectedSlot))->load()));
    const auto chokeTrigger=juce::jlimit(0,128,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTriggerId(selectedSlot))->load()));
    const auto chokeTarget=juce::jlimit(0,128,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTargetId(selectedSlot))->load()));
    const auto output=processor.getSlotOutput(selectedSlot);
    slotSelector.setSelectedId(selectedSlot+1,juce::dontSendNotification);
    engineSelector.setSelectedId(engine+1,juce::dontSendNotification);
    noteSelector.setSelectedItemIndex(note,juce::dontSendNotification);
    chokeTriggerSelector.setSelectedItemIndex(chokeTrigger,juce::dontSendNotification);
    chokeTargetSelector.setSelectedItemIndex(chokeTarget,juce::dontSendNotification);
    outputSelector.setSelectedId(output+1,juce::dontSendNotification);
    if(updateGrid&&!globalOpen){if(!lr608::isOffEngine(engine)){processor.loadSlotToProxy(selectedSlot);pageSelector.setSelectedItemIndex(lr608::slotEngines[engine].page,juce::dontSendNotification);}updateParameterList();setListIndex(rememberedGridIndices[selectedSlot],notifyGrid);}
}

void LR608AudioProcessorEditor::focusSlotColumn(int column)
{
    selectedSlotColumn=juce::jlimit(0,5,column);saveUiPosition(false);
    std::array<juce::Component*,6> columns{&slotSelector,&engineSelector,&noteSelector,&chokeTriggerSelector,&chokeTargetSelector,&outputSelector};
    requestShortcutFocus(*columns[std::size_t(selectedSlotColumn)]);
}

void LR608AudioProcessorEditor::changeSlotBarValue(int direction,bool pageStep,bool boundary,bool maximum)
{
    std::array<juce::ComboBox*,6> columns{&slotSelector,&engineSelector,&noteSelector,&chokeTriggerSelector,&chokeTargetSelector,&outputSelector};auto* box=columns[std::size_t(juce::jlimit(0,5,selectedSlotColumn))];
    const auto count=box->getNumItems();const auto previous=box->getSelectedItemIndex();auto target=previous;
    // Engine keeps its established keyboard direction (Up advances through the
    // engine catalogue) even though the popup is now displayed naturally as
    // Off, Kick ... Zap from top to bottom. The three MIDI columns use the same
    // increasing-on-Up convention.
    const auto valueDirection=selectedSlotColumn>=1&&selectedSlotColumn<=4?-direction:direction;
    if(boundary)
    {
        const auto reverseEngineOrMidiBoundary=selectedSlotColumn>=1&&selectedSlotColumn<=4;
        const auto selectLast=reverseEngineOrMidiBoundary?!maximum:maximum;
        target=selectLast?count-1:0;
    }
    else target+=valueDirection*(pageStep?((selectedSlotColumn>=2&&selectedSlotColumn<=4)?12:5):1);
    target=juce::jlimit(0,count-1,target);
    if(selectedSlotColumn==2&&target!=previous)
    {
        const auto searchDirection=boundary?(target==count-1?-1:1):(valueDirection<0?-1:1);
        target=processor.findAssignableMidiNote(target,searchDirection,selectedSlot);
        if(target<0){announce("No MIDI Note available in this direction. Maximum 8 layers per note");return;}
    }
    if(target==previous)return;
    box->setSelectedItemIndex(target,juce::sendNotificationSync);
    const auto value=selectedSlotColumn==0?slotDisplayName(selectedSlot):selectedSlotColumn==1?juce::String(lr608::slotEngines[juce::jlimit(0,lr608::slotEngineCount-1,engineSelector.getSelectedId()-1)].name):box->getText();
    const char* names[]{"Slot, ","Engine, ","MIDI Note, ","MIDI Choke Trigger, ","MIDI Choke Target, ","Output, "};announce(juce::String(names[selectedSlotColumn])+value);
}

void LR608AudioProcessorEditor::openGlobal()
{
    if(globalOpen)return;
    auto* focused=juce::Component::getCurrentlyFocusedComponent();
    positionWasInGrid=focused==&parameterSelector||focused==&parameterValue||(focused!=nullptr&&parameterValue.isParentOf(focused));
    positionBeforeGlobalColumn=selectedSlotColumn;positionBeforeGlobalSlot=selectedSlot;
    globalSnapshot.clear();for(const auto*id:{"slider249","slider253","slider254","slider256"})globalSnapshot.emplace_back(id,processor.parameters.getRawParameterValue(id)->load());
    globalOpen=true;slotSelector.setVisible(false);engineSelector.setVisible(false);noteSelector.setVisible(false);chokeTriggerSelector.setVisible(false);chokeTargetSelector.setVisible(false);outputSelector.setVisible(false);initialize.setVisible(false);clearSlots.setVisible(false);previousPreset.setVisible(false);nextPreset.setVisible(false);help.setVisible(false);title.setText("Global",juce::dontSendNotification);parameterSelector.setTitle("Global");pageSelector.setSelectedItemIndex(11,juce::dontSendNotification);updateParameterList();setListIndex(globalGridIndex);requestShortcutFocus(parameterSelector);announce("Global");
}

void LR608AudioProcessorEditor::closeGlobal(bool accept)
{
    if(!globalOpen)return;
    if(!accept)for(const auto&entry:globalSnapshot)if(auto*p=processor.parameters.getParameter(entry.first)){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(entry.second));p->endChangeGesture();}
    const auto globalItem=parameterSelector.getSelectedItemIndex();if(globalItem>=0){globalGridIndex=globalItem;processor.parameters.state.setProperty("uiGlobalGrid",globalItem,nullptr);}
    globalOpen=false;title.setText("LR-608",juce::dontSendNotification);parameterSelector.setTitle("Parameter");slotSelector.setVisible(true);engineSelector.setVisible(true);noteSelector.setVisible(true);chokeTriggerSelector.setVisible(true);chokeTargetSelector.setVisible(true);outputSelector.setVisible(true);initialize.setVisible(true);clearSlots.setVisible(true);previousPreset.setVisible(true);nextPreset.setVisible(true);help.setVisible(true);selectedSlot=positionBeforeGlobalSlot;selectedSlotColumn=positionBeforeGlobalColumn;syncSlotBar(true);saveUiPosition(positionWasInGrid);
    if(positionWasInGrid)requestShortcutFocus(parameterSelector);else focusSlotColumn(selectedSlotColumn);
    announce(accept?"Global confirmed":"Global cancelled");
}

void LR608AudioProcessorEditor::announcePage()
{
    const auto index = pageSelector.getSelectedItemIndex();
    if (! juce::isPositiveAndBelow (index, static_cast<int> (std::size (lr608::generated::pages))))
        return;
    announce (juce::String (lr608::generated::pages[index].name));
}

void LR608AudioProcessorEditor::updateParameterList()
{
    attachment.reset();
    visibleCatalogIndices.clear();
    visibleNames.clear();
    parameterSelector.clear (juce::dontSendNotification);
    const auto selectedEngine=juce::jlimit(0,lr608::slotEngineCount-1,engineSelector.getSelectedId()-1);
    if(!globalOpen&&lr608::isOffEngine(selectedEngine))
    {
        parameterSelector.setDescription("No parameters. Engine Off");
        parameterValue.setEnabled(false);reset.setEnabled(false);
        return;
    }
    parameterValue.setEnabled(true);reset.setEnabled(true);
    const auto pageIndex = pageSelector.getSelectedItemIndex();
    if (pageIndex < 0)
        return;
    const auto& page = lr608::generated::pages[pageIndex];
    const auto family=!globalOpen?lr608::slotEngines[selectedEngine].family:lr608::SlotFamily::kick;
    const auto tomFamily=family==lr608::SlotFamily::lowTom||family==lr608::SlotFamily::midTom||family==lr608::SlotFamily::highTom;
    if(!globalOpen&&!tomFamily)
    {
        const auto catalog=lr608::slotPanParameterIndex;
        visibleCatalogIndices.push_back(catalog);visibleNames.emplace_back("Pan");
        auto label=visibleNames.back();if(auto*parameter=processor.parameters.getParameter("slotPan"))label+=", "+parameter->getCurrentValueAsText();
        parameterSelector.addItem(label,static_cast<int>(visibleCatalogIndices.size()));
    }
    if(!globalOpen)
    {
        const auto catalog=lr608::slotVoiceOverlapParameterIndex;
        visibleCatalogIndices.push_back(catalog);visibleNames.emplace_back("Voice Overlap");
        auto label=visibleNames.back();if(auto*parameter=processor.parameters.getParameter("slotVoiceOverlap"))label+=", "+parameter->getCurrentValueAsText();
        parameterSelector.addItem(label,static_cast<int>(visibleCatalogIndices.size()));
    }
    if(!globalOpen)
    {
        for(const auto catalog:{lr608::slotLowPassCutoffParameterIndex,lr608::slotLowPassResonanceParameterIndex,lr608::slotHighPassCutoffParameterIndex,lr608::slotHighPassResonanceParameterIndex})
        {
            visibleCatalogIndices.push_back(catalog);visibleNames.emplace_back(lr608::generated::parameters[catalog].name);
            auto label=visibleNames.back();if(auto*parameter=processor.parameters.getParameter(lr608::generated::parameters[catalog].id))label+=", "+parameter->getCurrentValueAsText();
            parameterSelector.addItem(label,static_cast<int>(visibleCatalogIndices.size()));
        }
    }
    if(!globalOpen)
    {
        for(int catalog=lr608::slotDelayDryParameterIndex;catalog<=lr608::slotDelayRightOffsetParameterIndex;++catalog)
        {
            visibleCatalogIndices.push_back(catalog);visibleNames.emplace_back(lr608::generated::parameters[catalog].name);
            auto label=visibleNames.back();if(auto*parameter=processor.parameters.getParameter(lr608::generated::parameters[catalog].id))label+=", "+parameter->getCurrentValueAsText();
            parameterSelector.addItem(label,static_cast<int>(visibleCatalogIndices.size()));
        }
    }
    if(!globalOpen)
    {
        for(const auto catalog:{249,250})
        {
            visibleCatalogIndices.push_back(catalog);visibleNames.emplace_back(lr608::generated::parameters[catalog].name);
            auto label=visibleNames.back();if(auto*parameter=processor.parameters.getParameter(lr608::generated::parameters[catalog].id))label+=", "+parameter->getCurrentValueAsText();
            parameterSelector.addItem(label,static_cast<int>(visibleCatalogIndices.size()));
        }
    }
    for (std::size_t item = 0; item < page.parameterCount; ++item)
    {
        for (int catalog = 0; catalog < static_cast<int> (std::size (lr608::generated::parameters)); ++catalog)
        {
            const auto& descriptor = lr608::generated::parameters[catalog];
            if (juce::String (descriptor.id) != page.parameterIds[item])
                continue;
            if(globalOpen&&(juce::String(descriptor.id)=="slider250"||juce::String(descriptor.id)=="slider251"))continue;
            if(lr608::isEngineSelectorId(descriptor.id))continue;
            if(!globalOpen&&catalog==lr608::slotEngines[selectedEngine].routeParameterIndex)continue;
            const auto structuralName=juce::String(page.parameterNames[item]);
            if(pageIndex==5)
            {
                const auto low=structuralName.startsWith("Low Tom")||structuralName.startsWith("LowTom");
                const auto mid=structuralName.startsWith("Mid Tom")||structuralName.startsWith("MidTom");
                const auto high=structuralName.startsWith("High Tom")||structuralName.startsWith("HighTom");
                if((family==lr608::SlotFamily::lowTom&&!low)||(family==lr608::SlotFamily::midTom&&!mid)||(family==lr608::SlotFamily::highTom&&!high))continue;
            }
            if(pageIndex==7)
            {
                const auto crash=structuralName.startsWith("Crash "),ride=structuralName.startsWith("Ride "),common=structuralName.startsWith("Cymbal ");
                if((family==lr608::SlotFamily::crash&&!(crash||common))||(family==lr608::SlotFamily::ride&&!(ride||common)))continue;
            }
            visibleCatalogIndices.push_back (catalog);
            auto spokenName = globalOpen
                            ? parameterNameWithoutElement (juce::String (page.parameterNames[item]))
                            : contextualParameterName (selectedEngine, juce::String (page.parameterNames[item]));
            visibleNames.emplace_back (spokenName);
            auto label = visibleNames.back();
            if (auto* parameter = processor.parameters.getParameter (descriptor.id))
                label += ", " + parameter->getCurrentValueAsText();
            parameterSelector.addItem (label, static_cast<int> (visibleCatalogIndices.size()));
            break;
        }
    }
    parameterSelector.setSelectedItemIndex (0, juce::dontSendNotification);
    selectParameter();
}

void LR608AudioProcessorEditor::selectParameter()
{
    const auto item = parameterSelector.getSelectedItemIndex();
    if (! juce::isPositiveAndBelow (item, static_cast<int> (visibleCatalogIndices.size())))
        return;
    const auto& descriptor = lr608::generated::parameters[visibleCatalogIndices[item]];
    attachment.reset();
    parameterValue.setTitle ("Value. " + visibleNames[item]);
    parameterValue.setName ("Value. " + visibleNames[item]);
    parameterValue.setDescription ("Value for " + visibleNames[item]
                                   + ". Alt+V. Enter returns to Parameter");
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.parameters, descriptor.id, parameterValue);
    const auto& page = lr608::generated::pages[pageSelector.getSelectedItemIndex()];
    parameterSelector.setDescription (
        "Parameter, row " + juce::String (item % page.rowsPerColumn + 1)
        + ", column " + juce::String (item / page.rowsPerColumn + 1)
        + ". Alt+navigation keys, Enter for Value, Backspace resets");
}

void LR608AudioProcessorEditor::updateParameterLabel()
{
    const auto item = parameterSelector.getSelectedItemIndex();
    if (! juce::isPositiveAndBelow (item, static_cast<int> (visibleCatalogIndices.size())))
        return;
    auto label = visibleNames[item];
    const auto catalog=visibleCatalogIndices[item];
    const auto id=juce::String(lr608::generated::parameters[catalog].id);
    if (auto* parameter = processor.parameters.getParameter (id))
        label += ", " + parameter->getCurrentValueAsText();
    parameterSelector.changeItemText (item + 1, label);
    parameterSelector.setSelectedId (item + 1, juce::dontSendNotification);
}

void LR608AudioProcessorEditor::selectRelativePage (int delta)
{
    const auto next = juce::jlimit (1, static_cast<int> (std::size (lr608::generated::pages)),
                                    pageSelector.getSelectedId() + delta);
    if (next == pageSelector.getSelectedId())
        return;
    pageSelector.setSelectedId (next, juce::dontSendNotification);
    updateParameterList();
    announcePage();
    requestShortcutFocus (parameterSelector);
}

void LR608AudioProcessorEditor::selectPageByInitial (juce::juce_wchar initial)
{
    std::vector<int> matches;
    const auto wanted = juce::CharacterFunctions::toLowerCase (initial);
    for (int page = 0; page < static_cast<int> (std::size (lr608::generated::pages)); ++page)
    {
        const auto name = juce::String (lr608::generated::pages[page].name);
        if (name.isNotEmpty() && juce::CharacterFunctions::toLowerCase (name[0]) == wanted)
            matches.push_back (page + 1);
    }
    if (matches.empty())
        return;
    auto target = matches.front();
    if (const auto found = std::find (matches.begin(), matches.end(), pageSelector.getSelectedId());
        found != matches.end())
        target = std::next (found) == matches.end() ? matches.front() : *std::next (found);
    pageSelector.setSelectedId (target, juce::dontSendNotification);
    updateParameterList();
    announcePage();
    requestShortcutFocus (parameterSelector);
}

void LR608AudioProcessorEditor::setListIndex (int target, bool notifyAccessibility)
{
    if (! visibleCatalogIndices.empty())
    {
        const auto bounded=juce::jlimit (0, static_cast<int> (visibleCatalogIndices.size()) - 1, target);
        if(bounded==parameterSelector.getSelectedItemIndex())return;
        parameterSelector.setSelectedItemIndex (bounded,notifyAccessibility?juce::sendNotificationSync:juce::dontSendNotification);
        if(!notifyAccessibility)selectParameter();
        if(!globalOpen)saveUiPosition(true);
    }
}

void LR608AudioProcessorEditor::moveInGrid (int rowDelta, int columnDelta)
{
    const auto current = parameterSelector.getSelectedItemIndex();
    const auto count = static_cast<int> (visibleCatalogIndices.size());
    if (! juce::isPositiveAndBelow (current, count))
        return;
    const auto rows = lr608::generated::pages[pageSelector.getSelectedItemIndex()].rowsPerColumn;
    const auto row = current % rows;
    auto target = current;
    if (rowDelta < 0 && row > 0) --target;
    else if (rowDelta > 0 && row + 1 < rows && current + 1 < count) ++target;
    else if (columnDelta < 0 && current >= rows) target -= rows;
    else if (columnDelta > 0 && current + rows < count) target += rows;
    setListIndex (target);
}

bool LR608AudioProcessorEditor::selectNextParameterStartingWith (juce::juce_wchar character)
{
    const auto initial = juce::CharacterFunctions::toLowerCase (character);
    if (! juce::CharacterFunctions::isLetterOrDigit (initial) || visibleNames.empty())
        return false;

    const auto count = static_cast<int> (visibleNames.size());
    const auto current = parameterSelector.getSelectedItemIndex();
    for (int offset = 1; offset <= count; ++offset)
    {
        const auto target = (juce::jmax (-1, current) + offset) % count;
        const auto name = visibleNames[static_cast<std::size_t> (target)].trimStart();
        if (name.isNotEmpty()
            && juce::CharacterFunctions::toLowerCase (name[0]) == initial)
        {
            setListIndex (target);
            return true;
        }
    }
    return true;
}

void LR608AudioProcessorEditor::changeStepWidth (int direction)
{
    const auto next = juce::jlimit (0, static_cast<int> (std::size (stepWidths)) - 1,
                                    stepWidthIndex + direction);
    if (next != stepWidthIndex)
    {
        stepWidthIndex = next;
        announce ("Step " + juce::String (stepWidths[stepWidthIndex]));
    }
}

void LR608AudioProcessorEditor::changeValue (int direction, bool pageStep)
{
    const auto item = parameterSelector.getSelectedItemIndex();
    if (! juce::isPositiveAndBelow (item, static_cast<int> (visibleCatalogIndices.size())))
        return;
    const auto catalog=visibleCatalogIndices[item];
    const auto& descriptor = lr608::generated::parameters[catalog];
    if (auto* parameter = processor.parameters.getParameter (descriptor.id))
    {
        const auto current = parameter->convertFrom0to1 (parameter->getValue());
        const auto isChoice = juce::String (descriptor.choices).isNotEmpty();
        const auto multiplier = isChoice ? 1.0 : stepWidths[stepWidthIndex] * (pageStep ? valuePageStep : 1);
        auto next = juce::jlimit (descriptor.minimum, descriptor.maximum,
                                 current + descriptor.step * multiplier * direction);
        next = descriptor.minimum + std::round ((next - descriptor.minimum) / descriptor.step) * descriptor.step;
        if(std::abs(next-current)<descriptor.step*0.001)return;
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (next)));
        parameter->endChangeGesture();
        handleContextualParameterChange (descriptor.id);
        updateParameterLabel();
        announce (parameter->getCurrentValueAsText());
    }
}

void LR608AudioProcessorEditor::setValueBoundary (bool maximum)
{
    const auto item = parameterSelector.getSelectedItemIndex();
    if (item < 0) return;
    const auto catalog=visibleCatalogIndices[item];
    const auto& descriptor = lr608::generated::parameters[catalog];
    if (auto* parameter = processor.parameters.getParameter (descriptor.id))
    {
        const auto current=parameter->convertFrom0to1(parameter->getValue());
        const auto target=maximum?descriptor.maximum:descriptor.minimum;
        if(std::abs(current-target)<descriptor.step*0.001)return;
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (
            static_cast<float> (target)));
        parameter->endChangeGesture();
        handleContextualParameterChange (descriptor.id);
        updateParameterLabel();
        announce (parameter->getCurrentValueAsText());
    }
}

void LR608AudioProcessorEditor::focusValueAndAnnounce()
{
    const auto item = parameterSelector.getSelectedItemIndex();
    if (! juce::isPositiveAndBelow (item, static_cast<int> (visibleCatalogIndices.size())))
        return;
    auto message = "Value. " + visibleNames[item];
    const auto catalog=visibleCatalogIndices[item];const auto id=juce::String(lr608::generated::parameters[catalog].id);
    if (auto* parameter = processor.parameters.getParameter (id))
        message += ", " + parameter->getCurrentValueAsText();
    requestShortcutFocus (parameterValue);
    lr608::announceToActiveScreenReader (parameterValue, message);
    status.setText (message, juce::dontSendNotification);
}

void LR608AudioProcessorEditor::copyMidiKey(bool cut)
{
    if(!requireMidiKeyboardMode())return;
    if(globalOpen){announce("MIDI key copy is available in the Slot area");return;}
    const auto note=midiEditingNote;if(note<0){announce("Press a MIDI key before copying");return;}
    int layers=0;const auto text=processor.copyMidiKeyToText(note,cut,layers);
    if(text.isEmpty()){announce("MIDI Note "+juce::String(note)+" is empty");return;}
    juce::SystemClipboard::copyTextToClipboard(text);
    if(cut)syncSlotBar(true);
    announce(juce::String(cut?"Cut ":"Copied ")+juce::String(layers)+(layers==1?" layer from MIDI Note ":" layers from MIDI Note ")+juce::String(note));
}

void LR608AudioProcessorEditor::pasteMidiKey()
{
    if(!requireMidiKeyboardMode())return;
    if(globalOpen){announce("MIDI key paste is available in the Slot area");return;}
    const auto note=midiEditingNote;if(note<0){announce("Press an empty destination MIDI key before pasting");return;}
    int layers=0;const auto result=processor.pasteMidiKeyFromText(note,juce::SystemClipboard::getTextFromClipboard(),layers);
    if(result.failed()){announce(result.getErrorMessage());return;}
    syncSlotBar(true);saveUiPosition(false);
    announce("Pasted "+juce::String(layers)+(layers==1?" layer to MIDI Note ":" layers to MIDI Note ")+juce::String(note));
}

void LR608AudioProcessorEditor::clearMidiKey()
{
    if(!requireMidiKeyboardMode())return;
    if(globalOpen){announce("MIDI key delete is available in the Slot area");return;}
    const auto note=midiEditingNote;if(note<0){announce("Press a MIDI key before Delete");return;}
    const auto cleared=processor.clearMidiKey(note);
    if(cleared==0){announce("MIDI Note "+juce::String(note)+" is already empty");return;}
    syncSlotBar(true);saveUiPosition(false);
    announce("Deleted "+juce::String(cleared)+(cleared==1?" layer from MIDI Note ":" layers from MIDI Note ")+juce::String(note));
}

void LR608AudioProcessorEditor::clearCurrentMidiLayer()
{
    if(!requireMidiKeyboardMode())return;
    if(globalOpen){announce("MIDI layer delete is available in the Slot area");return;}
    const auto note=midiEditingNote;
    if(note<0){announce("Press a MIDI key before Alt Delete");return;}
    const auto engine=juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(selectedSlot))->load());
    const auto slotNote=juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotNoteId(selectedSlot))->load());
    if(slotNote!=note||lr608::isOffEngine(engine)){announce("MIDI Note "+juce::String(note)+" has no selected layer");return;}

    const auto deletedLayer=processor.getActiveSlotLayerNumber(note,selectedSlot);
    const auto deletedSlot=selectedSlot;
    if(!processor.clearSlot(deletedSlot)){announce("MIDI Note "+juce::String(note)+" has no selected layer");return;}

    std::vector<int> remaining;
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        const auto candidateNote=juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotNoteId(slot))->load());
        const auto candidateEngine=juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(slot))->load());
        if(candidateNote==note&&!lr608::isOffEngine(candidateEngine))remaining.push_back(slot);
    }
    if(!remaining.empty())selectedSlot=remaining[std::size_t(juce::jlimit(0,int(remaining.size())-1,deletedLayer-1))];
    midiCycleSlots[std::size_t(note)]=remaining.empty()?-1:selectedSlot;
    refreshSlotNames();syncSlotBar(true);saveUiPosition(false);
    announce("Deleted Layer "+juce::String(deletedLayer)+" from MIDI Note "+juce::String(note)
             +(remaining.empty()?". MIDI Note is now empty":". "+juce::String(remaining.size())+(remaining.size()==1?" layer remains":" layers remain")));
}

void LR608AudioProcessorEditor::toggleMidiKeyboardMode()
{
    midiKeyboardModeEnabled=!midiKeyboardModeEnabled;midiEditingNote=-1;midiCycleSlots.fill(-1);
    // Notes received while the mode was Off must never become an editing
    // source or destination when it is enabled later.
    lastMidiNavigationEvent=processor.getMidiNavigationEvent();
    announce(midiKeyboardModeEnabled?"MIDI keyboard editing on. Play a MIDI key":"MIDI keyboard editing off");
}

bool LR608AudioProcessorEditor::requireMidiKeyboardMode()
{
    if(midiKeyboardModeEnabled)return true;
    announce("MIDI keyboard editing off. Press Alt M to enable it");return false;
}

void LR608AudioProcessorEditor::changePreset (int direction)
{
    juce::String name;
    int number = 0;
    const auto result = processor.presetManager.loadRelativePreset (direction, name, number);
    if (result.failed())
    {
        announce (result.getErrorMessage());
        return;
    }
    refreshAfterPresetChange();
    announce ("Preset " + juce::String (number) + ". " + name);
}

void LR608AudioProcessorEditor::refreshAfterPresetChange()
{
    selectedSlot=juce::jlimit(0,lr608::slotCount-1,int(processor.parameters.state.getProperty("uiSelectedSlot",selectedSlot)));
    for(int slot=0;slot<lr608::slotCount;++slot)rememberedGridIndices[slot]=processor.getSlotGridPosition(slot);
    refreshSlotNames();
    syncSlotBar(true);
}

juce::String LR608AudioProcessorEditor::slotDisplayName(int slot)const
{
    const auto number=juce::String(slot+1),name=processor.getSlotName(slot);
    return name.isEmpty()?number:name+", "+number;
}

void LR608AudioProcessorEditor::refreshSlotNames()
{
    for(int slot=0;slot<lr608::slotCount;++slot)slotSelector.changeItemText(slot+1,slotDisplayName(slot));
    // JUCE does not refresh the selected label in changeItemText. Without
    // this, the next arrow treats the ComboBox as unselected and jumps to 128.
    slotSelector.setSelectedId(selectedSlot+1,juce::dontSendNotification);
}

void LR608AudioProcessorEditor::showSlotNameEditor()
{
    if(slotNameEditorOpen||slotReportOpen||globalOpen||presetBrowserOpen||presetSaveOpen)return;
    slotNameEditorOpen=true;setMainControlsEnabled(false);
    slotNameLabel.setText("Name for Slot "+juce::String(selectedSlot+1),juce::dontSendNotification);
    slotNameEditor.setDescription("Type a name for Slot "+juce::String(selectedSlot+1)+". Leave it empty to remove the name");
    slotNameEditor.setText(processor.getSlotName(selectedSlot),false);slotNameEditor.selectAll();
    for(auto*c:std::array<juce::Component*,4>{&slotNameLabel,&slotNameEditor,&slotNameConfirm,&slotNameCancel}){c->setVisible(true);c->toFront(false);}
    repaint();slotNameEditor.grabKeyboardFocus();announce("Rename Slot "+juce::String(selectedSlot+1));
}

void LR608AudioProcessorEditor::closeSlotNameEditor()
{
    if(!slotNameEditorOpen)return;slotNameEditorOpen=false;
    for(auto*c:std::array<juce::Component*,4>{&slotNameLabel,&slotNameEditor,&slotNameConfirm,&slotNameCancel})c->setVisible(false);
    setMainControlsEnabled(true);repaint();requestShortcutFocus(slotSelector);
}

void LR608AudioProcessorEditor::commitSlotName()
{
    if(!slotNameEditorOpen)return;const auto name=slotNameEditor.getText().trim().substring(0,48);processor.setSlotName(selectedSlot,name);refreshSlotNames();closeSlotNameEditor();
    announce(name.isEmpty()?"Slot "+juce::String(selectedSlot+1)+" name removed":"Slot "+name+", "+juce::String(selectedSlot+1));
}

void LR608AudioProcessorEditor::handleSlotReportShortcut()
{
    const auto now=juce::Time::getMillisecondCounterHiRes();
    if(lastSlotReportShortcutMs>0.0&&now-lastSlotReportShortcutMs<=700.0)
    {
        lastSlotReportShortcutMs=0.0;openSlotReport();return;
    }
    lastSlotReportShortcutMs=now;
    int filled=0;for(int slot=0;slot<lr608::slotCount;++slot)if(!lr608::isOffEngine(juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(slot))->load())))++filled;
    announce(juce::String(filled)+" Filled Slots. "+juce::String(lr608::slotCount-filled)+" Empty Slots");
}

void LR608AudioProcessorEditor::refreshSlotReport()
{
    slotReportFilledEntries.clear();slotReportEmptyEntries.clear();
    std::array<bool,128> noteAdded{};int filledCount=0;
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));
        if(lr608::isOffEngine(engine)){slotReportEmptyEntries.push_back({"Slot, "+slotDisplayName(slot)+". Empty",slot});continue;}
        ++filledCount;
    }
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));
        if(lr608::isOffEngine(engine))continue;
        const auto note=juce::jlimit(0,127,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotNoteId(slot))->load()));
        const auto layers=processor.getActiveSlotCountForMidiNote(note);
        if(layers>1)
        {
            if(noteAdded[std::size_t(note)])continue;noteAdded[std::size_t(note)]=true;
            juce::String slotNumbers;int first=-1;
            for(int member=0;member<lr608::slotCount;++member)
            {
                const auto memberEngine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(member))->load()));
                const auto memberNote=juce::jlimit(0,127,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotNoteId(member))->load()));
                if(lr608::isOffEngine(memberEngine)||memberNote!=note)continue;if(first<0)first=member;
                if(slotNumbers.isNotEmpty())slotNumbers+=", ";slotNumbers+=slotDisplayName(member);
            }
            const auto firstEngine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(first))->load()));
            slotReportFilledEntries.push_back({"Layers, "+juce::String(layers)+". MIDI Note "+juce::String(note)+". Slots, "+slotNumbers+". First Engine, "+lr608::slotEngines[firstEngine].name,first});
        }
        else slotReportFilledEntries.push_back({"Slot, "+slotDisplayName(slot)+". Engine, "+lr608::slotEngines[engine].name+". MIDI Note "+juce::String(note),slot});
    }
    slotReportSummary.setText(juce::String(filledCount)+" Filled Slots. "+juce::String(lr608::slotCount-filledCount)+" Empty Slots",juce::dontSendNotification);
    slotReportFilledLabel.setText("Filled Slots, "+juce::String(filledCount)+". "+juce::String(slotReportFilledEntries.size())+" report entries",juce::dontSendNotification);
    slotReportEmptyLabel.setText("Empty Slots, "+juce::String(lr608::slotCount-filledCount),juce::dontSendNotification);
    slotReportFilled.updateContent();slotReportEmpty.updateContent();
}

void LR608AudioProcessorEditor::openSlotReport()
{
    if(slotReportOpen||globalOpen||presetBrowserOpen||presetSaveOpen||slotNameEditorOpen)return;
    refreshSlotReport();slotReportOpen=true;setMainControlsEnabled(false);
    for(auto*c:std::array<juce::Component*,6>{&slotReportSummary,&slotReportFilledLabel,&slotReportEmptyLabel,&slotReportFilled,&slotReportEmpty,&slotReportClose}){c->setVisible(true);c->toFront(false);}
    if(!slotReportFilledEntries.empty())slotReportFilled.selectRow(0);if(!slotReportEmptyEntries.empty())slotReportEmpty.selectRow(0);repaint();
    juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]
    {
        if(safe==nullptr||!safe->slotReportOpen)return;
        if(!safe->slotReportFilledEntries.empty())safe->slotReportFilled.grabKeyboardFocus();else safe->slotReportEmpty.grabKeyboardFocus();
        safe->announce("Slot report. "+safe->slotReportSummary.getText());
    });
}

void LR608AudioProcessorEditor::closeSlotReport()
{
    if(!slotReportOpen)return;slotReportOpen=false;lastSlotReportShortcutMs=0.0;
    for(auto*c:std::array<juce::Component*,6>{&slotReportSummary,&slotReportFilledLabel,&slotReportEmptyLabel,&slotReportFilled,&slotReportEmpty,&slotReportClose})c->setVisible(false);
    setMainControlsEnabled(true);repaint();selectedSlotColumn=0;requestShortcutFocus(slotSelector);
}

void LR608AudioProcessorEditor::announceSlotReportEntry(bool filled,int row)
{
    const auto&entries=filled?slotReportFilledEntries:slotReportEmptyEntries;
    if(juce::isPositiveAndBelow(row,int(entries.size())))announce(entries[std::size_t(row)].text);
}

void LR608AudioProcessorEditor::activateSlotReportEntry(bool filled,int row)
{
    const auto&entries=filled?slotReportFilledEntries:slotReportEmptyEntries;
    if(!juce::isPositiveAndBelow(row,int(entries.size())))return;const auto destination=entries[std::size_t(row)].destinationSlot;
    processor.captureSlotFromProxy(selectedSlot);const auto item=parameterSelector.getSelectedItemIndex();if(item>=0){rememberedGridIndices[selectedSlot]=item;processor.setSlotGridPosition(selectedSlot,item);}
    selectedSlot=destination;selectedSlotColumn=0;saveUiPosition(false);syncSlotBar(true);closeSlotReport();
    const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(destination))->load()));
    announce("Slot, "+slotDisplayName(destination)+(lr608::isOffEngine(engine)?". Empty":". Engine, "+juce::String(lr608::slotEngines[engine].name)));
}

void LR608AudioProcessorEditor::timerCallback()
{
    const auto event=processor.getMidiNavigationEvent();if(event==0||event==lastMidiNavigationEvent)return;lastMidiNavigationEvent=event;
    if(!midiKeyboardModeEnabled)return;
    const auto note=int(event&0xffu);midiEditingNote=note;
    if(globalOpen||presetBrowserOpen||presetSaveOpen||slotNameEditorOpen||slotReportOpen||!isShowing())return;
    const auto slot=processor.findNextSlotForMidiNote(note,midiCycleSlots[std::size_t(note)]);
    if(slot<0)
    {
        midiCycleSlots[std::size_t(note)]=-1;
        announceMidiNavigation("MIDI Note "+juce::String(note)+". Empty");
        return;
    }
    midiCycleSlots[std::size_t(note)]=slot;
    auto*focused=juce::Component::getCurrentlyFocusedComponent();
    const auto inGrid=focused==&parameterSelector||focused==&parameterValue||(focused!=nullptr&&parameterValue.isParentOf(focused));
    const auto oldGridItem=parameterSelector.getSelectedItemIndex();
    const auto oldCatalog=juce::isPositiveAndBelow(oldGridItem,int(visibleCatalogIndices.size()))?visibleCatalogIndices[std::size_t(oldGridItem)]:-1;
    if(slot!=selectedSlot)
    {
        processor.captureSlotFromProxy(selectedSlot);
        if(inGrid)
        {
            const auto item=parameterSelector.getSelectedItemIndex();
            if(item>=0){rememberedGridIndices[selectedSlot]=item;processor.setSlotGridPosition(selectedSlot,item);}
        }
        selectedSlot=slot;syncSlotBar(true,false);
        if(inGrid)
        {
            const auto sameParameter=std::find(visibleCatalogIndices.begin(),visibleCatalogIndices.end(),oldCatalog);
            setListIndex(sameParameter!=visibleCatalogIndices.end()?int(std::distance(visibleCatalogIndices.begin(),sameParameter)):oldGridItem,false);
        }
        saveUiPosition(inGrid);
    }
    const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));
    const auto engineAnnouncement="Engine, "+juce::String(lr608::slotEngines[engine].name);
    const auto layerCount=processor.getActiveSlotCountForMidiNote(note);
    const auto layerNumber=processor.getActiveSlotLayerNumber(note,slot);
    auto message=(layerCount>1?"Layer "+juce::String(layerNumber)+". ":juce::String())+"MIDI Note "+juce::String(note)+". Slot, "+slotDisplayName(slot)+". "+engineAnnouncement;
    const auto location=currentFocusValueAnnouncement();if(location.isNotEmpty()&&location!="Slot, "+slotDisplayName(slot)&&location!=engineAnnouncement)message+=". "+location;
    announceMidiNavigation(message);
}

juce::String LR608AudioProcessorEditor::currentFocusValueAnnouncement() const
{
    auto*focused=juce::Component::getCurrentlyFocusedComponent();
    if(focused==&slotSelector)return "Slot, "+slotDisplayName(selectedSlot);
    if(focused==&engineSelector)return "Engine, "+engineSelector.getText();
    if(focused==&noteSelector)return "MIDI Note, "+noteSelector.getText();
    if(focused==&chokeTriggerSelector)return "MIDI Choke Trigger, "+chokeTriggerSelector.getText();
    if(focused==&chokeTargetSelector)return "MIDI Choke Target, "+chokeTargetSelector.getText();
    if(focused==&outputSelector)return "Output, "+outputSelector.getText();
    const auto item=parameterSelector.getSelectedItemIndex();
    if(focused==&parameterSelector&&juce::isPositiveAndBelow(item,int(visibleNames.size())))return visibleNames[std::size_t(item)]+", "+processor.parameters.getParameter(lr608::generated::parameters[visibleCatalogIndices[std::size_t(item)]].id)->getCurrentValueAsText();
    if((focused==&parameterValue||(focused!=nullptr&&parameterValue.isParentOf(focused)))&&juce::isPositiveAndBelow(item,int(visibleNames.size())))return "Value. "+visibleNames[std::size_t(item)]+", "+processor.parameters.getParameter(lr608::generated::parameters[visibleCatalogIndices[std::size_t(item)]].id)->getCurrentValueAsText();
    if(focused!=nullptr)return focused->getTitle().isNotEmpty()?focused->getTitle():focused->getName();
    return {};
}

void LR608AudioProcessorEditor::setMainControlsEnabled(bool enabled)
{
    for(auto*control:std::array<juce::Component*,16>{&slotSelector,&engineSelector,&noteSelector,&chokeTriggerSelector,&chokeTargetSelector,&outputSelector,&parameterSelector,&parameterValue,&reset,&initialize,&clearSlots,&previousPreset,&nextPreset,&loadPreset,&savePreset,&help})control->setEnabled(enabled);
}

void LR608AudioProcessorEditor::togglePresetBrowser(){if(presetBrowserOpen)closePresetBrowser();else openPresetBrowser();}
void LR608AudioProcessorEditor::openPresetBrowser()
{
    if(presetBrowserOpen)return;if(const auto result=processor.presetManager.ensureLibraryExists();result.failed()){announce(result.getErrorMessage());return;}if(presetSaveOpen)closePresetSave();
    presetBrowserOriginalPatch=processor.presetManager.capturePatchSnapshot();presetBrowserPreviewFile={};presetBrowserHasPreview=false;const auto root=processor.presetManager.getLibraryRoot();presetBrowserDirectory=root;const auto remembered=processor.parameters.state.getProperty(presetBrowserDirectoryState).toString();if(remembered.isNotEmpty()&&remembered!="."){const auto candidate=root.getChildFile(remembered);if(candidate.isDirectory()&&processor.presetManager.isInsideLibrary(candidate))presetBrowserDirectory=candidate;}
    presetBrowserOpen=true;setMainControlsEnabled(false);for(auto*c:std::array<juce::Component*,4>{&presetBrowserPath,&presetBrowser,&presetBrowserBack,&presetBrowserClose}){c->setVisible(true);c->toFront(false);}refreshPresetBrowser(int(processor.parameters.state.getProperty(presetBrowserRowState,0)));const auto selection=processor.parameters.state.getProperty(presetBrowserSelectionState).toString();if(selection.isNotEmpty()){const auto file=root.getChildFile(selection);for(int row=0;row<int(presetBrowserEntries.size());++row)if(presetBrowserEntries[std::size_t(row)].file==file){const juce::ScopedValueSetter guard(suppressPresetBrowserAnnouncement,true);presetBrowser.selectRow(row);presetBrowser.scrollToEnsureRowIsOnscreen(row);break;}}
    repaint();juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]{if(safe!=nullptr&&safe->presetBrowserOpen)safe->focusPresetBrowserAndAnnounce();});
}
void LR608AudioProcessorEditor::closePresetBrowser(bool focusGrid,bool restoreOriginal)
{
    if(!presetBrowserOpen)return;const auto root=processor.presetManager.getLibraryRoot();processor.parameters.state.setProperty(presetBrowserDirectoryState,presetBrowserDirectory==root?juce::String("."):presetBrowserDirectory.getRelativePathFrom(root),nullptr);const auto row=presetBrowser.getSelectedRow();processor.parameters.state.setProperty(presetBrowserRowState,std::max(0,row),nullptr);if(juce::isPositiveAndBelow(row,int(presetBrowserEntries.size())))processor.parameters.state.setProperty(presetBrowserSelectionState,presetBrowserEntries[std::size_t(row)].file.getRelativePathFrom(root),nullptr);else processor.parameters.state.removeProperty(presetBrowserSelectionState,nullptr);
    if(restoreOriginal&&presetBrowserHasPreview&&presetBrowserOriginalPatch.isValid())processor.presetManager.restorePatchSnapshot(presetBrowserOriginalPatch);presetBrowserOriginalPatch={};presetBrowserPreviewFile={};presetBrowserHasPreview=false;presetDeleteConfirmationOpen=false;presetDeleteChoiceYes=false;presetDeleteFile={};presetBrowserOpen=false;for(auto*c:std::array<juce::Component*,7>{&presetBrowserPath,&presetBrowser,&presetBrowserBack,&presetBrowserClose,&presetDeleteLabel,&presetDeleteYes,&presetDeleteNo})c->setVisible(false);setMainControlsEnabled(true);refreshAfterPresetChange();repaint();if(focusGrid)requestShortcutFocus(parameterSelector);else requestShortcutFocus(loadPreset);
}
void LR608AudioProcessorEditor::refreshPresetBrowser(int row)
{
    const juce::ScopedValueSetter guard(suppressPresetBrowserAnnouncement,true);presetBrowserEntries=processor.presetManager.listDirectory(presetBrowserDirectory);const auto relative=presetBrowserDirectory.getRelativePathFrom(processor.presetManager.getLibraryRoot());presetBrowserPath.setText(relative=="."?"Preset browser. LR-608":"Preset browser. Folder "+relative,juce::dontSendNotification);presetBrowser.updateContent();if(presetBrowserEntries.empty()){presetBrowser.deselectAllRows();announce("Empty folder");}else{presetBrowser.selectRow(juce::jlimit(0,int(presetBrowserEntries.size())-1,row));presetBrowser.scrollToEnsureRowIsOnscreen(presetBrowser.getSelectedRow());}
}
void LR608AudioProcessorEditor::focusPresetBrowserAndAnnounce(){if(!presetBrowserOpen)return;presetBrowser.grabKeyboardFocus();const auto row=presetBrowser.getSelectedRow();if(previewPresetBrowserRow(row))announcePresetBrowserRow(row,true);}
void LR608AudioProcessorEditor::selectPresetBrowserRow(int row){const auto count=int(presetBrowserEntries.size());if(count<=0)return;const auto target=juce::jlimit(0,count-1,row);if(target==presetBrowser.getSelectedRow())return;presetBrowser.grabKeyboardFocus();presetBrowser.selectRow(target);presetBrowser.scrollToEnsureRowIsOnscreen(target);}
void LR608AudioProcessorEditor::announcePresetBrowserRow(int row,bool includeFolder){if(!juce::isPositiveAndBelow(row,int(presetBrowserEntries.size())))return;auto message=getNameForRow(row);if(includeFolder){const auto relative=presetBrowserDirectory.getRelativePathFrom(processor.presetManager.getLibraryRoot());message=(relative=="."?juce::String("Preset browser. LR-608"):juce::String("Preset browser. Folder ")+relative)+". "+message;}announce(message);}
bool LR608AudioProcessorEditor::previewPresetBrowserRow(int row){if(!juce::isPositiveAndBelow(row,int(presetBrowserEntries.size())))return false;const auto&entry=presetBrowserEntries[std::size_t(row)];if(entry.isDirectory||entry.file==presetBrowserPreviewFile)return true;if(const auto result=processor.presetManager.previewPreset(entry.file);result.failed()){announce(result.getErrorMessage());return false;}presetBrowserPreviewFile=entry.file;presetBrowserHasPreview=true;refreshAfterPresetChange();return true;}
void LR608AudioProcessorEditor::activatePresetBrowserRow(int row){if(!juce::isPositiveAndBelow(row,int(presetBrowserEntries.size())))return;const auto&entry=presetBrowserEntries[std::size_t(row)];if(entry.isDirectory){presetBrowserDirectory=entry.file;refreshPresetBrowser();focusPresetBrowserAndAnnounce();return;}if(!previewPresetBrowserRow(row))return;if(const auto result=processor.presetManager.commitPresetPreview(entry.file);result.failed()){announce(result.getErrorMessage());return;}closePresetBrowser(true,false);announce("Preset "+entry.name);}
void LR608AudioProcessorEditor::goToParentPresetFolder(){const auto root=processor.presetManager.getLibraryRoot();if(presetBrowserDirectory==root)return;const auto parent=presetBrowserDirectory.getParentDirectory();if(!processor.presetManager.isInsideLibrary(parent))return;const auto previous=presetBrowserDirectory.getFileName();presetBrowserDirectory=parent;{const juce::ScopedValueSetter guard(suppressPresetBrowserAnnouncement,true);refreshPresetBrowser();for(int row=0;row<int(presetBrowserEntries.size());++row)if(presetBrowserEntries[std::size_t(row)].isDirectory&&presetBrowserEntries[std::size_t(row)].name==previous){presetBrowser.selectRow(row);break;}}focusPresetBrowserAndAnnounce();}

void LR608AudioProcessorEditor::showPresetDeleteConfirmation()
{
    if(!presetBrowserOpen||presetDeleteConfirmationOpen)return;
    const auto row=presetBrowser.getSelectedRow();if(!juce::isPositiveAndBelow(row,int(presetBrowserEntries.size())))return;
    const auto&entry=presetBrowserEntries[std::size_t(row)];
    if(entry.isDirectory){announce("Folders cannot be deleted here");return;}
    if(entry.file.isAChildOf(processor.presetManager.getLibraryRoot().getChildFile("Factory"))){announce("Factory presets cannot be deleted");return;}
    presetDeleteConfirmationOpen=true;presetDeleteChoiceYes=false;presetDeleteFile=entry.file;presetDeleteRow=row;
    for(auto*c:std::array<juce::Component*,4>{&presetBrowserPath,&presetBrowser,&presetBrowserBack,&presetBrowserClose})c->setVisible(false);
    presetDeleteLabel.setText("Permanently delete preset "+entry.name+"? Yes or No",juce::dontSendNotification);
    for(auto*c:std::array<juce::Component*,3>{&presetDeleteLabel,&presetDeleteYes,&presetDeleteNo}){c->setVisible(true);c->toFront(false);}
    repaint();juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]{if(safe!=nullptr&&safe->presetDeleteConfirmationOpen){safe->presetDeleteNo.grabKeyboardFocus();safe->announce(safe->presetDeleteLabel.getText()+". No selected");}});
}
void LR608AudioProcessorEditor::setPresetDeleteChoice(bool yes)
{
    if(!presetDeleteConfirmationOpen)return;presetDeleteChoiceYes=yes;(yes?presetDeleteYes:presetDeleteNo).grabKeyboardFocus();announce(yes?"Yes":"No");
}
void LR608AudioProcessorEditor::dismissPresetDeleteConfirmation()
{
    if(!presetDeleteConfirmationOpen)return;presetDeleteConfirmationOpen=false;presetDeleteChoiceYes=false;presetDeleteFile={};
    for(auto*c:std::array<juce::Component*,3>{&presetDeleteLabel,&presetDeleteYes,&presetDeleteNo})c->setVisible(false);
    for(auto*c:std::array<juce::Component*,4>{&presetBrowserPath,&presetBrowser,&presetBrowserBack,&presetBrowserClose}){c->setVisible(true);c->toFront(false);}
    repaint();requestShortcutFocus(presetBrowser);announce("Preset deletion cancelled");
}
void LR608AudioProcessorEditor::confirmPresetDelete()
{
    if(!presetDeleteConfirmationOpen||!presetDeleteChoiceYes){dismissPresetDeleteConfirmation();return;}
    const auto name=presetDeleteFile.getFileNameWithoutExtension();
    if(presetBrowserHasPreview&&presetBrowserOriginalPatch.isValid())processor.presetManager.restorePatchSnapshot(presetBrowserOriginalPatch);
    presetBrowserPreviewFile={};presetBrowserHasPreview=false;
    const auto result=processor.presetManager.deletePreset(presetDeleteFile);if(result.failed()){announce(result.getErrorMessage());return;}
    presetDeleteConfirmationOpen=false;presetDeleteChoiceYes=false;presetDeleteFile={};
    for(auto*c:std::array<juce::Component*,3>{&presetDeleteLabel,&presetDeleteYes,&presetDeleteNo})c->setVisible(false);
    for(auto*c:std::array<juce::Component*,4>{&presetBrowserPath,&presetBrowser,&presetBrowserBack,&presetBrowserClose}){c->setVisible(true);c->toFront(false);}
    refreshAfterPresetChange();refreshPresetBrowser(presetDeleteRow);repaint();focusPresetBrowserAndAnnounce();announce("Preset deleted. "+name);
}

void LR608AudioProcessorEditor::showPresetSave()
{
    if(presetSaveOpen)return;
    if(presetDeleteConfirmationOpen){announce("Finish or cancel preset deletion before saving");return;}
    if(const auto result=processor.presetManager.ensureLibraryExists();result.failed()){announce(result.getErrorMessage());return;}
    juce::String suggestedName;
    presetSaveFromBrowser=presetBrowserOpen;presetSaveReturnFile={};
    if(presetSaveFromBrowser)
    {
        const auto row=presetBrowser.getSelectedRow();
        if(juce::isPositiveAndBelow(row,int(presetBrowserEntries.size()))&&!presetBrowserEntries[std::size_t(row)].isDirectory)
        {
            if(!previewPresetBrowserRow(row))return;
            suggestedName=presetBrowserEntries[std::size_t(row)].name;
        }
        for(auto*c:std::array<juce::Component*,4>{&presetBrowserPath,&presetBrowser,&presetBrowserBack,&presetBrowserClose})c->setVisible(false);
    }
    presetSaveOpen=true;presetOverwriteConfirmationOpen=false;presetOverwriteFile={};setMainControlsEnabled(false);
    for(auto*c:std::array<juce::Component*,4>{&presetSaveLabel,&presetSaveName,&presetSaveConfirm,&presetSaveCancel}){c->setVisible(true);c->toFront(false);}
    presetSaveName.setText(suggestedName,false);presetSaveName.selectAll();repaint();
    juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]
    {
        if(safe!=nullptr&&safe->presetSaveOpen&&!safe->presetOverwriteConfirmationOpen)
        {
            safe->requestShortcutFocus(safe->presetSaveName);
            safe->announce("Save preset. Preset name");
        }
    });
}
void LR608AudioProcessorEditor::closePresetSave()
{
    if(!presetSaveOpen)return;
    const auto returnToBrowser=presetSaveFromBrowser&&presetBrowserOpen;
    const auto savedFile=presetSaveReturnFile;
    presetSaveOpen=false;presetOverwriteConfirmationOpen=false;presetOverwriteFile={};presetSaveFromBrowser=false;presetSaveReturnFile={};
    for(auto*c:std::array<juce::Component*,8>{&presetSaveLabel,&presetSaveName,&presetSaveConfirm,&presetSaveCancel,&presetOverwriteLabel,&presetOverwriteYes,&presetOverwriteNo,&presetOverwriteClose})c->setVisible(false);
    if(returnToBrowser)
    {
        for(auto*c:std::array<juce::Component*,4>{&presetBrowserPath,&presetBrowser,&presetBrowserBack,&presetBrowserClose}){c->setVisible(true);c->toFront(false);}
        if(savedFile.existsAsFile())
        {
            presetBrowserDirectory=processor.presetManager.getLibraryRoot();refreshPresetBrowser();
            for(int row=0;row<int(presetBrowserEntries.size());++row)if(presetBrowserEntries[std::size_t(row)].file==savedFile){const juce::ScopedValueSetter guard(suppressPresetBrowserAnnouncement,true);presetBrowser.selectRow(row);presetBrowser.scrollToEnsureRowIsOnscreen(row);break;}
        }
        setMainControlsEnabled(false);repaint();
        juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]{if(safe!=nullptr&&safe->presetBrowserOpen)safe->focusPresetBrowserAndAnnounce();});
        return;
    }
    setMainControlsEnabled(true);repaint();requestShortcutFocus(savePreset);
}
void LR608AudioProcessorEditor::commitPresetSave(){if(presetOverwriteConfirmationOpen)return;juce::File saved;const auto result=processor.presetManager.savePreset(presetSaveName.getText(),processor.presetManager.getLibraryRoot(),saved);if(result.failed()){if(saved.existsAsFile()){showPresetOverwriteConfirmation(saved);return;}announce(result.getErrorMessage());requestShortcutFocus(presetSaveName);return;}const auto name=saved.getFileNameWithoutExtension();if(presetSaveFromBrowser){presetBrowserOriginalPatch=processor.presetManager.capturePatchSnapshot();presetBrowserPreviewFile=saved;presetBrowserHasPreview=true;presetSaveReturnFile=saved;}closePresetSave();announce("Preset saved. "+name);}
void LR608AudioProcessorEditor::showPresetOverwriteConfirmation(const juce::File&file){presetOverwriteConfirmationOpen=true;presetOverwriteFile=file;for(auto*c:std::array<juce::Component*,4>{&presetSaveLabel,&presetSaveName,&presetSaveConfirm,&presetSaveCancel})c->setVisible(false);presetOverwriteLabel.setText("Preset "+file.getFileNameWithoutExtension()+" already exists. Overwrite?",juce::dontSendNotification);for(auto*c:std::array<juce::Component*,4>{&presetOverwriteLabel,&presetOverwriteYes,&presetOverwriteNo,&presetOverwriteClose}){c->setVisible(true);c->toFront(false);}repaint();juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]{if(safe!=nullptr&&safe->presetOverwriteConfirmationOpen){safe->presetOverwriteNo.grabKeyboardFocus();safe->announce(safe->presetOverwriteLabel.getText()+" No");}});}
void LR608AudioProcessorEditor::dismissPresetOverwriteConfirmation(){if(!presetOverwriteConfirmationOpen)return;presetOverwriteConfirmationOpen=false;presetOverwriteFile={};for(auto*c:std::array<juce::Component*,4>{&presetOverwriteLabel,&presetOverwriteYes,&presetOverwriteNo,&presetOverwriteClose})c->setVisible(false);for(auto*c:std::array<juce::Component*,4>{&presetSaveLabel,&presetSaveName,&presetSaveConfirm,&presetSaveCancel}){c->setVisible(true);c->toFront(false);}repaint();presetSaveName.grabKeyboardFocus();announce("Preset not overwritten. Enter another name");}
void LR608AudioProcessorEditor::confirmPresetOverwrite(){if(!presetOverwriteConfirmationOpen||!presetOverwriteFile.existsAsFile()){dismissPresetOverwriteConfirmation();return;}juce::File saved;const auto result=processor.presetManager.savePreset(presetSaveName.getText(),processor.presetManager.getLibraryRoot(),saved,true);if(result.failed()){announce(result.getErrorMessage());return;}const auto name=saved.getFileNameWithoutExtension();if(presetSaveFromBrowser){presetBrowserOriginalPatch=processor.presetManager.capturePatchSnapshot();presetBrowserPreviewFile=saved;presetBrowserHasPreview=true;presetSaveReturnFile=saved;}closePresetSave();announce("Preset overwritten. "+name);}

void LR608AudioProcessorEditor::showHelpLanguageMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader("Help language");
    menu.addItem(1,"English");menu.addItem(2,"Italiano");menu.addItem(3,juce::String::fromUTF8("Español"));menu.addItem(4,juce::String::fromUTF8("Português"));
    menu.addItem(5,juce::String::fromUTF8("Français"));menu.addItem(6,juce::String::fromUTF8("Русский"));menu.addItem(7,juce::String::fromUTF8("中文"));menu.addItem(8,juce::String::fromUTF8("日本語"));
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&help),[safe=juce::Component::SafePointer(this)](int result)
    {
        if(safe==nullptr)return;
        if(result==0){juce::Timer::callAfterDelay(50,[safe]{if(safe!=nullptr)safe->requestShortcutFocus(safe->help);});return;}
        static constexpr const char*codes[]{"en","it","es","pt","fr","ru","zh","ja"};
        if(juce::isPositiveAndBelow(result-1,int(std::size(codes))))safe->openHelp(codes[result-1]);
    });
}

void LR608AudioProcessorEditor::openHelp(const juce::String&languageCode)
{
    int size=0;const auto*data=BinaryData::getNamedResource("LR608Help_html",size);
    if(data==nullptr||size<=0){announce("Help file unavailable");return;}
    const juce::StringArray supported{"en","it","es","pt","fr","ru","zh","ja"};
    const auto language=supported.contains(languageCode)?languageCode:"en";
    auto html=juce::String::fromUTF8(data,size).replace("const requestedLanguage='en';","const requestedLanguage='"+language+"';");
    const auto folder=juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("LR-608 Help");
    if(folder.createDirectory().failed()){announce("Cannot create help folder");return;}
    const auto file=folder.getChildFile("LR-608 Help "+language+".html");
    if(!file.replaceWithText(html,false,false,"\n")){announce("Cannot write help file");return;}
    if(!file.startAsProcess())announce("Cannot open help in the default browser");
}

int LR608AudioProcessorEditor::getNumRows(){return int(presetBrowserEntries.size());}
void LR608AudioProcessorEditor::paintListBoxItem(int row,juce::Graphics&g,int width,int height,bool selected){if(!juce::isPositiveAndBelow(row,int(presetBrowserEntries.size())))return;if(selected)g.fillAll(foreground);g.setColour(selected?background:foreground);g.setFont(juce::FontOptions(16.0f,juce::Font::bold));g.drawText(getNameForRow(row),8,0,width-16,height,juce::Justification::centredLeft,true);}
juce::String LR608AudioProcessorEditor::getNameForRow(int row){if(!juce::isPositiveAndBelow(row,int(presetBrowserEntries.size())))return {};const auto&entry=presetBrowserEntries[std::size_t(row)];return juce::String(row+1)+". "+(entry.isDirectory?"Category ":"")+entry.name;}
void LR608AudioProcessorEditor::selectedRowsChanged(int row){if(!presetBrowserOpen||suppressPresetBrowserAnnouncement)return;if(previewPresetBrowserRow(row)&&presetBrowser.hasKeyboardFocus(true))announcePresetBrowserRow(row,false);}
void LR608AudioProcessorEditor::returnKeyPressed(int row){activatePresetBrowserRow(row);}

void LR608AudioProcessorEditor::handleContextualParameterChange (juce::StringRef id)
{
    if (id == juce::StringRef ("slider247"))
    {
        const auto switched = processor.kickEngineBanks.switchTo (juce::roundToInt (
            processor.parameters.getRawParameterValue ("slider247")->load()));
        if (! switched) return;
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer (this)]
        {
            if (safe == nullptr) return;
            const auto selected = safe->parameterSelector.getSelectedItemIndex();
            safe->updateParameterList();
            safe->setListIndex (selected);
        });
    }
    else if (lr608::KickEngineBanks::ownsParameter (id))
        processor.kickEngineBanks.captureCurrent();
    else if (id == juce::StringRef ("slider248") || id == juce::StringRef ("slider198"))
    {
        const auto slot = id == juce::StringRef ("slider248") ? 0 : 1;
        const auto* selector = slot == 0 ? "slider248" : "slider198";
        const auto switched = processor.snareEngineBanks.switchTo (slot, juce::roundToInt (
            processor.parameters.getRawParameterValue (selector)->load()));
        if (! switched) return;
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer (this)]
        {
            if (safe == nullptr) return;
            const auto selected = safe->parameterSelector.getSelectedItemIndex();
            safe->updateParameterList();
            safe->setListIndex (selected);
        });
    }
    else if (lr608::SnareEngineBanks::ownsParameter (0, id))
        processor.snareEngineBanks.captureCurrent (0);
    else if (lr608::SnareEngineBanks::ownsParameter (1, id))
        processor.snareEngineBanks.captureCurrent (1);
    else if (id == juce::StringRef ("slider207") || id == juce::StringRef ("slider209"))
    {
        const auto changed = id == juce::StringRef ("slider207")
            ? processor.clapRimEngineBanks.switchClap (juce::roundToInt (processor.parameters.getRawParameterValue ("slider207")->load()))
            : processor.clapRimEngineBanks.switchRim (juce::roundToInt (processor.parameters.getRawParameterValue ("slider209")->load()));
        if (! changed) return;
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer (this)]
        {
            if (safe == nullptr) return;
            const auto selected = safe->parameterSelector.getSelectedItemIndex();
            safe->updateParameterList(); safe->setListIndex (selected);
        });
    }
    else if (lr608::ClapRimEngineBanks::ownsClap (id)) processor.clapRimEngineBanks.captureClap();
    else if (lr608::ClapRimEngineBanks::ownsRim (id)) processor.clapRimEngineBanks.captureRim();
    else if (id == juce::StringRef ("slider219"))
    {
        const auto changed=processor.tomEngineBanks.switchTo(juce::roundToInt(processor.parameters.getRawParameterValue("slider219")->load()));
        if(!changed)return;
        juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]{if(safe==nullptr)return;const auto selected=safe->parameterSelector.getSelectedItemIndex();safe->updateParameterList();safe->setListIndex(selected);});
    }
    else if (lr608::TomEngineBanks::ownsParameter(id)) processor.tomEngineBanks.captureCurrent();
    else if(id==juce::StringRef("slider217")||id==juce::StringRef("slider214"))
    {
        const auto changed=id==juce::StringRef("slider217")?processor.hatCymbalEngineBanks.switchHat(juce::roundToInt(processor.parameters.getRawParameterValue("slider217")->load())):processor.hatCymbalEngineBanks.switchCymbal(juce::roundToInt(processor.parameters.getRawParameterValue("slider214")->load()));
        if(!changed)return;juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]{if(safe==nullptr)return;const auto selected=safe->parameterSelector.getSelectedItemIndex();safe->updateParameterList();safe->setListIndex(selected);});
    }
    else if(lr608::HatCymbalEngineBanks::ownsHat(id))processor.hatCymbalEngineBanks.captureHat();
    else if(lr608::HatCymbalEngineBanks::ownsCymbal(id))processor.hatCymbalEngineBanks.captureCymbal();
    else if(id==juce::StringRef("slider216"))
    {
        const auto changed=processor.maracasEngineBanks.switchTo(juce::roundToInt(processor.parameters.getRawParameterValue("slider216")->load()));
        if(!changed)return;juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]{if(safe==nullptr)return;const auto selected=safe->parameterSelector.getSelectedItemIndex();safe->updateParameterList();safe->setListIndex(selected);});
    }
    else if(lr608::MaracasEngineBanks::ownsParameter(id))processor.maracasEngineBanks.captureCurrent();
    else if(id==juce::StringRef("slider215"))
    {
        const auto changed=processor.cowbellEngineBanks.switchTo(juce::roundToInt(processor.parameters.getRawParameterValue("slider215")->load()));
        if(!changed)return;juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]{if(safe==nullptr)return;const auto selected=safe->parameterSelector.getSelectedItemIndex();safe->updateParameterList();safe->setListIndex(selected);});
    }
    else if(lr608::CowbellEngineBanks::ownsParameter(id))processor.cowbellEngineBanks.captureCurrent();
    else if(id==juce::StringRef("slider199"))
    {
        const auto changed=processor.zapEngineBanks.switchTo(juce::roundToInt(processor.parameters.getRawParameterValue("slider199")->load()));
        if(!changed)return;juce::MessageManager::callAsync([safe=juce::Component::SafePointer(this)]{if(safe==nullptr)return;const auto selected=safe->parameterSelector.getSelectedItemIndex();safe->updateParameterList();safe->setListIndex(selected);});
    }
    else if(lr608::ZapEngineBanks::ownsParameter(id))processor.zapEngineBanks.captureCurrent();
}

void LR608AudioProcessorEditor::resetSelected()
{
    const auto item = parameterSelector.getSelectedItemIndex();
    if (item < 0) return;
    const auto catalog=visibleCatalogIndices[item];
    const auto& descriptor = lr608::generated::parameters[catalog];
    if (auto* parameter = processor.parameters.getParameter (descriptor.id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (
            static_cast<float> (juce::jlimit (descriptor.minimum, descriptor.maximum,
                                              descriptor.defaultValue))));
        parameter->endChangeGesture();
        handleContextualParameterChange (descriptor.id);
        updateParameterLabel();
        announce ("Reset. " + parameter->getCurrentValueAsText());
    }
}

void LR608AudioProcessorEditor::initializeAll()
{
    attachment.reset();
    for (const auto& descriptor : lr608::generated::parameters)
        if (auto* parameter = processor.parameters.getParameter (descriptor.id))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (
                static_cast<float> (juce::jlimit (descriptor.minimum, descriptor.maximum,
                                                  descriptor.defaultValue))));
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        processor.setSlotEngine(slot,lr608::defaultSlotEngine(slot));
        processor.setSlotOutput(slot,0);
        processor.setSlotName(slot,{});
        if(auto*p=processor.parameters.getParameter(lr608::slotNoteId(slot)))p->setValueNotifyingHost(p->convertTo0to1(float(lr608::defaultSlotNote(slot))));
        if(auto*p=processor.parameters.getParameter(lr608::slotChokeTriggerId(slot)))p->setValueNotifyingHost(0);
        if(auto*p=processor.parameters.getParameter(lr608::slotChokeTargetId(slot)))p->setValueNotifyingHost(0);
    }
    processor.kickEngineBanks.resetFromCurrentState();
    processor.snareEngineBanks.resetFromCurrentState();
    processor.clapRimEngineBanks.resetFromCurrentState();
    processor.tomEngineBanks.resetToFactory();
    processor.hatCymbalEngineBanks.resetToFactory();
    processor.maracasEngineBanks.resetToFactory();
    processor.cowbellEngineBanks.resetToFactory();
    processor.zapEngineBanks.resetToFactory();
    refreshSlotNames();syncSlotBar(true);selectParameter();
    updateParameterLabel();
    announce ("LR-608 initialized");
}

void LR608AudioProcessorEditor::clearAllSlots()
{
    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    const auto wasInGrid = focused == &parameterSelector || focused == &parameterValue
                        || (focused != nullptr && parameterValue.isParentOf (focused));
    attachment.reset();
    int cleared = 0;
    const auto setOff = [this] (const juce::String& id)
    {
        if (auto* parameter = processor.parameters.getParameter (id))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (0.0f));
    };

    for (int slot = 0; slot < lr608::slotCount; ++slot)
    {
        const auto engine = juce::roundToInt (
            processor.parameters.getRawParameterValue (lr608::slotEngineId (slot))->load());
        if (! lr608::isOffEngine (engine))
            ++cleared;

        processor.setSlotEngine (slot, lr608::offEngineIndex);
        processor.setSlotOutput (slot, 0);
        processor.setSlotName (slot, {});
        setOff (lr608::slotChokeTriggerId (slot));
        setOff (lr608::slotChokeTargetId (slot));
        rememberedGridIndices[slot] = 0;
        processor.setSlotGridPosition (slot, 0);
    }

    midiEditingNote = -1;
    midiCycleSlots.fill (-1);
    refreshSlotNames();
    syncSlotBar (true);
    saveUiPosition (wasInGrid);
    announce ("Clear Slots. "
              + (cleared == 0 ? juce::String ("All 128 Slots were already empty")
                              : juce::String (cleared) + (cleared == 1 ? " Slot cleared" : " Slots cleared")));
}

bool LR608AudioProcessorEditor::keyPressed (const juce::KeyPress& key,
                                             juce::Component* source)
{
    const auto code = key.getKeyCode();
    const auto alt = key.getModifiers().isAltDown();
    const auto ctrl = key.getModifiers().isCtrlDown()||key.getModifiers().isCommandDown();
    const auto character = juce::CharacterFunctions::toLowerCase (key.getTextCharacter());
    if(slotReportOpen)
    {
        if(code==juce::KeyPress::escapeKey||(alt&&character=='r')){closeSlotReport();return true;}
        if(code==juce::KeyPress::returnKey)
        {
            if(slotReportFilled.hasKeyboardFocus(true)){activateSlotReportEntry(true,slotReportFilled.getSelectedRow());return true;}
            if(slotReportEmpty.hasKeyboardFocus(true)){activateSlotReportEntry(false,slotReportEmpty.getSelectedRow());return true;}
        }
        return false;
    }
    if(slotNameEditorOpen){if(code==juce::KeyPress::escapeKey){closeSlotNameEditor();return true;}if(code==juce::KeyPress::returnKey){commitSlotName();return true;}return false;}
    if(code==juce::KeyPress::F2Key&&(source==&slotSelector||(source!=nullptr&&slotSelector.isParentOf(source)))){showSlotNameEditor();return true;}
    if(alt&&character=='m'){toggleMidiKeyboardMode();return true;}
    if(alt&&character=='b'){if(presetSaveOpen)closePresetSave();else togglePresetBrowser();return true;}
    if(alt&&character=='c'&&presetSaveOpen){closePresetSave();return true;}
    if(alt&&character=='c'&&presetBrowserOpen){closePresetBrowser();return true;}
    if(alt&&character=='s'){showPresetSave();return true;}
    if(presetDeleteConfirmationOpen)
    {
        if(code==juce::KeyPress::escapeKey){dismissPresetDeleteConfirmation();return true;}
        if(character=='y'){setPresetDeleteChoice(true);return true;}
        if(character=='n'){setPresetDeleteChoice(false);return true;}
        if(code==juce::KeyPress::leftKey||code==juce::KeyPress::upKey){setPresetDeleteChoice(true);return true;}
        if(code==juce::KeyPress::rightKey||code==juce::KeyPress::downKey){setPresetDeleteChoice(false);return true;}
        if(code==juce::KeyPress::returnKey){if(presetDeleteChoiceYes)confirmPresetDelete();else dismissPresetDeleteConfirmation();return true;}
        return false;
    }
    if(presetOverwriteConfirmationOpen){if(code==juce::KeyPress::escapeKey){dismissPresetOverwriteConfirmation();return true;}return false;}
    if(presetSaveOpen){if(code==juce::KeyPress::escapeKey){closePresetSave();return true;}if(code==juce::KeyPress::returnKey){commitPresetSave();return true;}return false;}
    if(presetBrowserOpen)
    {
        const auto count=int(presetBrowserEntries.size());
        if(code==juce::KeyPress::escapeKey){closePresetBrowser();return true;}
        if(code==juce::KeyPress::deleteKey){showPresetDeleteConfirmation();return true;}
        if(code==juce::KeyPress::backspaceKey){goToParentPresetFolder();return true;}
        if(code==juce::KeyPress::returnKey){activatePresetBrowserRow(presetBrowser.getSelectedRow());return true;}
        if(count>0&&code==juce::KeyPress::upKey){selectPresetBrowserRow(presetBrowser.getSelectedRow()-1);return true;}
        if(count>0&&code==juce::KeyPress::downKey){selectPresetBrowserRow(presetBrowser.getSelectedRow()+1);return true;}
        if(count>0&&code==juce::KeyPress::pageUpKey){selectPresetBrowserRow(presetBrowser.getSelectedRow()-10);return true;}
        if(count>0&&code==juce::KeyPress::pageDownKey){selectPresetBrowserRow(presetBrowser.getSelectedRow()+10);return true;}
        if(count>0&&code==juce::KeyPress::homeKey){selectPresetBrowserRow(0);return true;}
        if(count>0&&code==juce::KeyPress::endKey){selectPresetBrowserRow(count-1);return true;}
        return false;
    }
    if(alt&&character=='r'){if(globalOpen)announce("Slot report is available in the Slot area");else handleSlotReportShortcut();return true;}
    if(alt&&key.getModifiers().isShiftDown()&&code==juce::KeyPress::deleteKey&&!globalOpen){clearMidiKey();return true;}
    if(alt&&code==juce::KeyPress::deleteKey&&!globalOpen){clearCurrentMidiLayer();return true;}
    if(code==juce::KeyPress::deleteKey){announce("Delete alone does not erase sounds. Use Alt Delete for the selected layer or Alt Shift Delete for the complete MIDI Note");return true;}
    if(ctrl&&!alt&&character=='c'){copyMidiKey(false);return true;}
    if(ctrl&&!alt&&character=='x'){copyMidiKey(true);return true;}
    if(ctrl&&!alt&&character=='v'){pasteMidiKey();return true;}
    if(alt&&character=='i'&&!globalOpen){initialize.triggerClick();return true;}
    if(alt&&character=='c'){copyMidiKey(false);return true;}
    if(alt&&character=='x'){copyMidiKey(true);return true;}
    if(alt&&character=='v'){pasteMidiKey();return true;}
    if(alt&&character=='g'){if(globalOpen)closeGlobal(false);else openGlobal();return true;}
    if(globalOpen&&(code==juce::KeyPress::escapeKey||code==juce::KeyPress::returnKey)){closeGlobal(code==juce::KeyPress::returnKey);return true;}
    const auto slotBarSource=source==&slotSelector||source==&engineSelector||source==&noteSelector||source==&chokeTriggerSelector||source==&chokeTargetSelector||source==&outputSelector;
    if(slotBarSource)
    {
        selectedSlotColumn=source==&slotSelector?0:source==&engineSelector?1:source==&noteSelector?2:source==&chokeTriggerSelector?3:source==&chokeTargetSelector?4:5;
        if(code==juce::KeyPress::tabKey){saveUiPosition(true);requestShortcutFocus(parameterSelector);return true;}
        if(!alt&&code==juce::KeyPress::leftKey){if(selectedSlotColumn>0)focusSlotColumn(selectedSlotColumn-1);return true;}
        if(!alt&&code==juce::KeyPress::rightKey){if(selectedSlotColumn<5)focusSlotColumn(selectedSlotColumn+1);return true;}
        if(!alt&&code==juce::KeyPress::upKey){changeSlotBarValue(-1,false,false,false);return true;}
        if(!alt&&code==juce::KeyPress::downKey){changeSlotBarValue(1,false,false,false);return true;}
        if(!alt&&code==juce::KeyPress::pageUpKey){changeSlotBarValue(-1,true,false,false);return true;}
        if(!alt&&code==juce::KeyPress::pageDownKey){changeSlotBarValue(1,true,false,false);return true;}
        if(!alt&&code==juce::KeyPress::homeKey){changeSlotBarValue(0,false,true,false);return true;}
        if(!alt&&code==juce::KeyPress::endKey){changeSlotBarValue(0,false,true,true);return true;}
    }
    if (source == &pageSelector && ! alt
        && ! key.getModifiers().isCtrlDown() && ! key.getModifiers().isCommandDown()
        && juce::CharacterFunctions::isLetterOrDigit (character))
    {
        std::vector<int> matches;
        for (int page = 0; page < static_cast<int> (std::size (lr608::generated::pages)); ++page)
        {
            const auto name = juce::String (lr608::generated::pages[page].name).trimStart();
            if (name.isNotEmpty() && juce::CharacterFunctions::toLowerCase (name[0]) == character)
                matches.push_back (page + 1);
        }
        if (! matches.empty())
        {
            auto target = matches.front();
            if (const auto found = std::find (matches.begin(), matches.end(), pageSelector.getSelectedId());
                found != matches.end())
                target = std::next (found) == matches.end() ? matches.front() : *std::next (found);
            pageSelector.setSelectedId (target, juce::sendNotificationSync);
            return true;
        }
    }
    if (code == juce::KeyPress::returnKey)
        if (auto* editor = dynamic_cast<juce::TextEditor*> (source);
            editor != nullptr && parameterValue.isParentOf (editor))
        {
            requestShortcutFocus (parameterSelector);
            return true;
        }
    const auto focusControl = [this, source] (juce::Component& target)
    {
        if (dynamic_cast<juce::TextEditor*> (source) != nullptr)
            requestShortcutFocus (target);
        else
            target.grabKeyboardFocus();
    };
    if (alt)
    {
        if (character == '+' || code == juce::KeyPress::numberPadAdd
            || (code == '=' && key.getModifiers().isShiftDown()))
        { changePreset (1); return true; }
        if (character == '-' || code == '-' || code == juce::KeyPress::numberPadSubtract)
        { changePreset (-1); return true; }
        if (character == 'p') {selectedSlotColumn=0;changeSlotBarValue(-1,false,false,false);return true;}
        if (character == 'n') {selectedSlotColumn=0;changeSlotBarValue(1,false,false,false);return true;}
        if (character == 'd') { focusSlotColumn(selectedSlotColumn); return true; }
        if (character == 'l') {saveUiPosition(true);focusControl (parameterSelector); return true; }
        if (character == 'e') { parameterValue.showTextBox(); return true; }
        if (character == 'h') { help.triggerClick(); return true; }
        // Value-edit navigation is intentionally local to the Parameter grid.
        // Everywhere else these Alt combinations remain available to REAPER.
        if (source == &parameterSelector)
        {
            if (code == juce::KeyPress::upKey) { changeValue (1, false); return true; }
            if (code == juce::KeyPress::downKey) { changeValue (-1, false); return true; }
            if (code == juce::KeyPress::pageUpKey) { changeValue (1, true); return true; }
            if (code == juce::KeyPress::pageDownKey) { changeValue (-1, true); return true; }
            if (code == juce::KeyPress::leftKey) { changeStepWidth (-1); return true; }
            if (code == juce::KeyPress::rightKey) { changeStepWidth (1); return true; }
            if (code == juce::KeyPress::homeKey) { setValueBoundary (true); return true; }
            if (code == juce::KeyPress::endKey) { setValueBoundary (false); return true; }
        }
    }
    if (source == &parameterSelector)
    {
        if (! key.getModifiers().isAltDown()
            && ! key.getModifiers().isCtrlDown()
            && ! key.getModifiers().isCommandDown()
            && selectNextParameterStartingWith (key.getTextCharacter()))
            return true;
        if (code == juce::KeyPress::upKey) { moveInGrid (-1, 0); return true; }
        if (code == juce::KeyPress::downKey) { moveInGrid (1, 0); return true; }
        if (code == juce::KeyPress::leftKey) { moveInGrid (0, -1); return true; }
        if (code == juce::KeyPress::rightKey) { moveInGrid (0, 1); return true; }
        if (code == juce::KeyPress::pageUpKey) { setListIndex (parameterSelector.getSelectedItemIndex() - parameterPageStep); return true; }
        if (code == juce::KeyPress::pageDownKey) { setListIndex (parameterSelector.getSelectedItemIndex() + parameterPageStep); return true; }
        if (code == juce::KeyPress::returnKey) { focusValueAndAnnounce(); return true; }
        if (code == juce::KeyPress::backspaceKey) { resetSelected(); return true; }
        if(code==juce::KeyPress::escapeKey){focusSlotColumn(selectedSlotColumn);return true;}
        if (code == juce::KeyPress::homeKey || code == juce::KeyPress::endKey)
        {
            const auto rows = lr608::generated::pages[pageSelector.getSelectedItemIndex()].rowsPerColumn;
            const auto current = parameterSelector.getSelectedItemIndex();
            const auto first = (current / rows) * rows;
            setListIndex (code == juce::KeyPress::homeKey ? first : std::min (first + rows - 1,
                         static_cast<int> (visibleCatalogIndices.size()) - 1));
            return true;
        }
    }
    if (source == &parameterValue)
    {
        if (code == juce::KeyPress::returnKey) { parameterSelector.grabKeyboardFocus(); return true; }
        if (code == juce::KeyPress::upKey) { changeValue (1, false); return true; }
        if (code == juce::KeyPress::downKey) { changeValue (-1, false); return true; }
        if (code == juce::KeyPress::leftKey) { changeStepWidth (-1); return true; }
        if (code == juce::KeyPress::rightKey) { changeStepWidth (1); return true; }
        if (code == juce::KeyPress::pageUpKey) { changeValue (1, true); return true; }
        if (code == juce::KeyPress::pageDownKey) { changeValue (-1, true); return true; }
        if (code == juce::KeyPress::homeKey) { setValueBoundary (true); return true; }
        if (code == juce::KeyPress::endKey) { setValueBoundary (false); return true; }
        if (code == juce::KeyPress::backspaceKey) { resetSelected(); return true; }
    }
    return false;
}
