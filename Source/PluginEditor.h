// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>
#include <array>
#include "PluginProcessor.h"

class LR608AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::KeyListener,
                                         private juce::ListBoxModel,
                                         private juce::AsyncUpdater,
                                         private juce::Timer
{
public:
    explicit LR608AudioProcessorEditor (LR608AudioProcessor&);
    ~LR608AudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void focusGained (FocusChangeType) override;
    void focusOfChildComponentChanged (FocusChangeType) override;
    void visibilityChanged() override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    struct SlotReportModel;
    struct SlotReportEntry { juce::String text; int destinationSlot=-1; };
    LR608AudioProcessor& processor;
    std::unique_ptr<juce::LookAndFeel_V4> visualLookAndFeel;
    std::unique_ptr<juce::LookAndFeel_V4> shortcutLookAndFeel;
    juce::Label title;
    juce::Label status;
    juce::ComboBox pageSelector;
    juce::ComboBox slotSelector;
    juce::ComboBox engineSelector;
    juce::ComboBox noteSelector;
    juce::ComboBox chokeTriggerSelector;
    juce::ComboBox chokeTargetSelector;
    juce::ComboBox outputSelector;
    juce::ComboBox parameterSelector;
    juce::Slider parameterValue;
    juce::TextButton reset { "Reset parameter" };
    juce::TextButton initialize { "Initialize" };
    juce::TextButton clearSlots { "Clear All Slots" };
    juce::TextButton previousPreset { "Previous preset" };
    juce::TextButton nextPreset { "Next preset" };
    juce::TextButton loadPreset { "Browser" };
    juce::TextButton savePreset { "Save preset" };
    juce::TextButton help { "Help" };
    juce::Label presetBrowserPath;
    juce::ListBox presetBrowser { "Preset browser", this };
    juce::TextButton presetBrowserBack { "Back" };
    juce::TextButton presetBrowserClose { "Close" };
    juce::Label presetDeleteLabel;
    juce::TextButton presetDeleteYes { "Yes" };
    juce::TextButton presetDeleteNo { "No" };
    juce::Label presetSaveLabel;
    juce::TextEditor presetSaveName;
    juce::TextButton presetSaveConfirm { "Save" };
    juce::TextButton presetSaveCancel { "Close" };
    juce::Label presetOverwriteLabel;
    juce::TextButton presetOverwriteYes { "Yes" };
    juce::TextButton presetOverwriteNo { "No" };
    juce::TextButton presetOverwriteClose { "Close" };
    juce::Label slotNameLabel;
    juce::TextEditor slotNameEditor;
    juce::TextButton slotNameConfirm { "Save name" };
    juce::TextButton slotNameCancel { "Close" };
    juce::Label slotReportSummary;
    juce::Label slotReportFilledLabel;
    juce::Label slotReportEmptyLabel;
    std::unique_ptr<SlotReportModel> slotReportFilledModel;
    std::unique_ptr<SlotReportModel> slotReportEmptyModel;
    juce::ListBox slotReportFilled { "Filled Slots" };
    juce::ListBox slotReportEmpty { "Empty Slots" };
    juce::TextButton slotReportClose { "Close" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    std::vector<int> visibleCatalogIndices;
    std::vector<juce::String> visibleNames;
    std::vector<lr608::PresetManager::BrowserEntry> presetBrowserEntries;
    juce::File presetBrowserDirectory;
    juce::File presetBrowserPreviewFile;
    lr608::PresetManager::PatchSnapshot presetBrowserOriginalPatch;
    bool presetBrowserOpen=false,presetBrowserHasPreview=false,suppressPresetBrowserAnnouncement=false;
    bool presetDeleteConfirmationOpen=false,presetDeleteChoiceYes=false;
    bool presetSaveOpen=false,presetOverwriteConfirmationOpen=false,presetSaveFromBrowser=false;
    bool slotNameEditorOpen=false;
    bool slotReportOpen=false;
    bool midiKeyboardModeEnabled=false;
    int midiEditingNote=-1;
    std::array<int,128> midiCycleSlots{};
    juce::File presetOverwriteFile;
    juce::File presetSaveReturnFile;
    juce::File presetDeleteFile;
    int presetDeleteRow=0;
    int stepWidthIndex = 0;
    int selectedSlot = 0;
    int selectedSlotColumn = 0;
    std::array<int, lr608::slotCount> rememberedGridIndices {};
    std::array<int, lr608::slotCount> rememberedFxGridIndices {};
    int globalGridIndex = 0;
    bool updatingSlotBar = false;
    bool globalOpen = false;
    bool slotFxPage = false;
    bool positionWasInGrid = false;
    int positionBeforeGlobalColumn = 0;
    int positionBeforeGlobalSlot = 0;
    std::vector<std::pair<juce::String, float>> globalSnapshot;
    bool initialFocusTransferPending = false;
    int initialFocusTransferAttempts = 0;
    juce::Component* pendingShortcutFocusTarget = nullptr;
    std::uint64_t lastMidiNavigationEvent = 0;
    double lastSlotReportShortcutMs = 0.0;
    std::vector<SlotReportEntry> slotReportFilledEntries;
    std::vector<SlotReportEntry> slotReportEmptyEntries;

    void updateParameterList();
    void syncSlotBar (bool updateGrid, bool notifyGrid = true);
    void changeSlotBarValue (int direction, bool pageStep, bool boundary, bool maximum);
    void focusSlotColumn (int);
    void saveUiPosition (bool inGrid);
    void openGlobal();
    void closeGlobal (bool accept);
    void selectParameter();
    void updateParameterLabel();
    void selectRelativePage (int);
    void selectPageByInitial (juce::juce_wchar);
    void announcePage();
    void toggleSlotParameterPage();
    int currentGridRows() const;
    void announce (const juce::String&);
    void announceMidiNavigation (const juce::String&);
    void resetSelected();
    void initializeAll();
    void clearAllSlots();
    void moveInGrid (int rowDelta, int columnDelta);
    bool selectNextParameterStartingWith (juce::juce_wchar);
    void setListIndex (int, bool notifyAccessibility = true);
    void changeStepWidth (int);
    void changeValue (int direction, bool pageStep);
    void setValueBoundary (bool maximum);
    void focusValueAndAnnounce();
    void copyMidiKey(bool cut);
    void pasteMidiKey();
    void clearCurrentMidiLayer();
    void clearMidiKey();
    void toggleMidiKeyboardMode();
    bool requireMidiKeyboardMode();
    void changePreset (int direction);
    void refreshAfterPresetChange();
    void refreshSlotNames();
    juce::String slotDisplayName(int) const;
    void showSlotNameEditor();
    void closeSlotNameEditor();
    void commitSlotName();
    void handleSlotReportShortcut();
    void refreshSlotReport();
    void openSlotReport();
    void closeSlotReport();
    void activateSlotReportEntry(bool filled,int row);
    void announceSlotReportEntry(bool filled,int row);
    void setMainControlsEnabled(bool);
    void togglePresetBrowser();
    void openPresetBrowser();
    void closePresetBrowser(bool focusParameterGrid=false,bool restoreOriginalPatch=true);
    void refreshPresetBrowser(int selectedRow=0);
    void focusPresetBrowserAndAnnounce();
    void selectPresetBrowserRow(int);
    void announcePresetBrowserRow(int,bool);
    bool previewPresetBrowserRow(int);
    void activatePresetBrowserRow(int);
    void goToParentPresetFolder();
    void showPresetDeleteConfirmation();
    void setPresetDeleteChoice(bool);
    void dismissPresetDeleteConfirmation();
    void confirmPresetDelete();
    void showPresetSave();
    void closePresetSave();
    void commitPresetSave();
    void showPresetOverwriteConfirmation(const juce::File&);
    void dismissPresetOverwriteConfirmation();
    void confirmPresetOverwrite();
    void showHelpLanguageMenu();
    void openHelp(const juce::String&);
    void handleContextualParameterChange (juce::StringRef id);
    void scheduleInitialFocusTransfer();
    void performInitialFocusTransfer();
    void requestShortcutFocus (juce::Component&);
    void handleAsyncUpdate() override;
    void timerCallback() override;
    juce::String currentFocusValueAnnouncement() const;
    int getNumRows() override;
    void paintListBoxItem(int,juce::Graphics&,int,int,bool) override;
    juce::String getNameForRow(int) override;
    void selectedRowsChanged(int) override;
    void returnKeyPressed(int) override;
    bool keyPressed (const juce::KeyPress&, juce::Component*) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LR608AudioProcessorEditor)
};
