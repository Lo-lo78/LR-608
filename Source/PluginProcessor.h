// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterCatalog.h"
#include "PresetManager.h"
#include "DrumTimingEngine.h"
#include "Kick808Voice.h"
#include "KickOtherVoices.h"
#include "KickEngineBanks.h"
#include "SnareEngineBanks.h"
#include "SnareVoice.h"
#include "ClapRimEngineBanks.h"
#include "ClapRimVoices.h"
#include "TomEngineBanks.h"
#include "TomVoice.h"
#include "HatCymbalEngineBanks.h"
#include "HatCymbalVoices.h"
#include "MaracasEngineBanks.h"
#include "MaracasVoice.h"
#include "CowbellEngineBanks.h"
#include "CowbellVoice.h"
#include "ZapEngineBanks.h"
#include "ZapVoice.h"
#include "SlotArchitecture.h"
#include "PolyphonicVoice.h"
#include <atomic>
#include "OutputStage.h"

class LR608AudioProcessor final : public juce::AudioProcessor,
                                  private juce::AudioProcessorValueTreeState::Listener
{
public:
    struct SlotSoundSnapshot { int engine=-1; std::array<float,lr608::slotParameterValueCount> values{}; bool isValid()const noexcept{return engine>=0;} };
    struct MidiKeyLayer { SlotSoundSnapshot sound; int output=0,chokeTrigger=0,chokeTarget=0; };
    explicit LR608AudioProcessor (juce::File presetRoot = {});
    ~LR608AudioProcessor() override;
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 32.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    lr608::PresetManager presetManager;
    lr608::KickEngineBanks kickEngineBanks;
    lr608::SnareEngineBanks snareEngineBanks;
    lr608::ClapRimEngineBanks clapRimEngineBanks;
    lr608::TomEngineBanks tomEngineBanks;
    lr608::HatCymbalEngineBanks hatCymbalEngineBanks;
    lr608::MaracasEngineBanks maracasEngineBanks;
    lr608::CowbellEngineBanks cowbellEngineBanks;
    lr608::ZapEngineBanks zapEngineBanks;
    void captureSlotFromProxy(int);
    void captureSlotParameter(int,juce::StringRef);
    void loadSlotToProxy(int);
    void setSlotEngine(int,int);
    SlotSoundSnapshot copySlotSound(int);
    bool pasteSlotSound(int,const SlotSoundSnapshot&);
    int getLastPlayedMidiNote() const { return lastPlayedMidiNote.load(std::memory_order_relaxed); }
    std::uint64_t getMidiNavigationEvent() const { return midiNavigationEvent.load(std::memory_order_acquire); }
    int findFirstSlotForMidiNote(int) const;
    int findNextSlotForMidiNote(int,int) const;
    int getActiveSlotCountForMidiNote(int,int excludedSlot=-1) const;
    int getActiveSlotLayerNumber(int,int) const;
    bool canAssignSlotToMidiNote(int,int) const;
    int findAssignableMidiNote(int,int,int) const;
    juce::String getSlotName(int) const;
    void setSlotName(int,const juce::String&);
    juce::String copyMidiKeyToText(int,bool,int&);
    juce::Result pasteMidiKeyFromText(int,const juce::String&,int&);
    bool clearSlot(int);
    int clearMidiKey(int);
    int getSlotGridPosition(int)const;
    void setSlotGridPosition(int,int);
    int getSlotOutput(int) const;
    void setSlotOutput(int,int);
    int getActiveVoiceCount() const;
    int getActiveVoiceCountForMidiNote(int) const;
    std::uint64_t getVoiceStealCount() const { return voiceStealCounter.load(); }
    juce::ValueTree capturePresetState();
    void restorePresetState(const juce::ValueTree&);
    void importLegacyPresetAsKit();

private:
    static BusesProperties makeBuses();
    double currentSampleRate = 44100.0;
    lr608::DrumTimingEngine timingEngine;
    lr608::OutputStage outputStage;
    std::unique_ptr<std::array<lr608::PolyphonicVoice,128>> voicePool;
    std::array<int,128> activeVoiceIndices{};
    int activeVoiceCount=0;
    bool outputIdleLatched=false;
    std::array<std::array<std::atomic<float>,lr608::slotParameterValueCount>,lr608::slotCount> slotValues{};
    std::array<std::array<float,lr608::slotParameterValueCount>,lr608::slotEngineCount> engineDefaults{};
    std::array<std::atomic<int>,lr608::slotCount> slotGrid{};
    std::array<std::atomic<int>,lr608::slotCount> slotOutputs{};
    std::array<std::atomic<int>,lr608::slotCount> slotMidiNotes{};
    std::array<std::atomic<int>,lr608::slotCount> slotEngineIndices{};
    std::array<std::atomic<int>,lr608::slotCount> slotChokeTriggers{};
    std::array<std::atomic<int>,lr608::slotCount> slotChokeTargets{};
    std::array<juce::String,lr608::slotCount> slotNames{};
    std::atomic<int> currentProxySlot{0};
    std::atomic<int> lastPlayedMidiNote{-1};
    std::atomic<std::uint64_t> midiNavigationEvent{0};
    std::uint64_t midiNavigationSerial=0;
    std::uint64_t voiceCounter=0;
    std::atomic<std::uint64_t> voiceStealCounter{0};
    void initialiseSlotArchitecture();
    void buildEngineDefaults();
    void storeSlotStates();
    void restoreSlotStates();
    void refreshSlotTriggerCache();
    void parameterChanged(const juce::String&,float) override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LR608AudioProcessor)
};
