// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <vector>

namespace lr608
{
class PresetManager
{
public:
    struct PatchSnapshot
    {
        juce::ValueTree completeState;
        juce::String currentPresetRelativePath;
        bool hadCurrentPreset = false;
        bool isValid() const noexcept { return completeState.isValid(); }
    };
    struct BrowserEntry { juce::File file; juce::String name; bool isDirectory = false; };

    explicit PresetManager (juce::AudioProcessorValueTreeState&, juce::File root = {});
    void setStateCallbacks (std::function<juce::ValueTree()> capture,
                            std::function<void(const juce::ValueTree&)> restore,
                            std::function<void()> importLegacyKit);
    juce::Result ensureLibraryExists();
    juce::Result loadFactoryInit();
    juce::File getLibraryRoot() const { return root; }
    std::vector<BrowserEntry> listDirectory (const juce::File&) const;
    std::vector<juce::File> allPresetFiles() const;
    bool isInsideLibrary (const juce::File&) const;
    bool isValidPresetFile (const juce::File&) const;
    juce::Result savePreset (const juce::String&, const juce::File&, juce::File&,
                             bool overwriteExisting = false);
    juce::Result deletePreset (const juce::File&);
    juce::Result loadPreset (const juce::File&, juce::String& name, int& number);
    juce::Result loadRelativePreset (int direction, juce::String& name, int& number);
    PatchSnapshot capturePatchSnapshot() const;
    void restorePatchSnapshot (const PatchSnapshot&);
    juce::Result previewPreset (const juce::File&);
    juce::Result commitPresetPreview (const juce::File&);
    static int getEmbeddedFactoryPresetCount();

private:
    struct ParsedPreset
    {
        juce::String name;
        std::vector<float> values;
        juce::ValueTree completeState;
    };
    juce::AudioProcessorValueTreeState& state;
    juce::File root;
    std::function<juce::ValueTree()> captureState;
    std::function<void(const juce::ValueTree&)> restoreState;
    std::function<void()> importLegacy;
    static std::vector<ParsedPreset> parseReaperLibrary (const juce::String&);
    static bool parseTextPreset (const juce::String&, ParsedPreset&);
    static juce::String serialiseLegacyPreset (const juce::String&, const std::vector<float>&);
    static juce::String serialiseCompletePreset (const juce::String&, const juce::ValueTree&);
    static juce::String nameWithoutExtension (const juce::File&);
    static bool naturalFileLess (const juce::File&, const juce::File&);
    void applyValues (const std::vector<float>&);
    juce::Result applyParsed (const ParsedPreset&);
    void rememberCurrentPreset (const juce::File&);
    juce::File recalledCurrentPreset() const;
};
}
