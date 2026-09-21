// SPDX-License-Identifier: AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "GeneratedParameters.h"
#include "GeneratedPages.h"
#include <limits>

namespace
{
int catalogIndex(juce::StringRef);
constexpr int slotEngineArchitectureVersion = 4;
constexpr const char* universalSlotParameterIds[] {
    "slotPan", "slotVoiceOverlap",
    "slotLowPassCutoff", "slotLowPassResonance", "slotHighPassCutoff", "slotHighPassResonance",
    "slotDelayDry", "slotDelayWet", "slotDelayVolume", "slotDelayDivision", "slotDelayFeedback",
    "slotDelayGlide", "slotDelayFilter", "slotDelayLeftOffset", "slotDelayRightOffset",
    "slider250", "slider251"
};

int migrateLegacyEngineIndex (int oldIndex)
{
    oldIndex = juce::jlimit (0, 84, oldIndex);
    if (oldIndex <= 83) return lr608::historicalEngineToCurrent (oldIndex);
    return lr608::offEngineIndex;
}

int migrateVersion2EngineIndex(int oldIndex)
{
    oldIndex=juce::jlimit(0,87,oldIndex);
    if(oldIndex<=21)return oldIndex;
    if(oldIndex<=86)return oldIndex+1;
    return 88; // Off in architecture version 3.
}

int migrateVersion3EngineIndex(int oldIndex)
{
    oldIndex=juce::jlimit(0,88,oldIndex);
    if(oldIndex<=15)return oldIndex;
    if(oldIndex<=87)return oldIndex+1;
    return lr608::offEngineIndex;
}

juce::ValueTree migrateSlotEngineArchitecture (const juce::ValueTree& source)
{
    auto state = source.createCopy();
    const auto sourceVersion=int(state.getProperty("slotEngineArchitectureVersion",1));
    if (sourceVersion >= slotEngineArchitectureVersion)
        return state;
    const auto migrate=[sourceVersion](int engine)
    {
        if(sourceVersion<=1)return migrateLegacyEngineIndex(engine);
        if(sourceVersion==2)return migrateVersion3EngineIndex(migrateVersion2EngineIndex(engine));
        return migrateVersion3EngineIndex(engine);
    };

    for (int slot = 0; slot < lr608::slotCount; ++slot)
    {
        const auto id = lr608::slotEngineId (slot);
        if (state.hasProperty (juce::Identifier (id)))
            state.setProperty (juce::Identifier (id), migrate (juce::roundToInt (double (state.getProperty (juce::Identifier (id))))), nullptr);
        for (int child = 0; child < state.getNumChildren(); ++child)
        {
            auto parameter = state.getChild (child);
            if (parameter.getProperty ("id").toString() == id && parameter.hasProperty ("value"))
                parameter.setProperty ("value", migrate (juce::roundToInt (double (parameter.getProperty ("value")))), nullptr);
        }
    }
    state.setProperty ("slotEngineArchitectureVersion", slotEngineArchitectureVersion, nullptr);
    return state;
}
}

juce::AudioProcessor::BusesProperties LR608AudioProcessor::makeBuses()
{
    auto buses = juce::AudioProcessor::BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), false)
        .withOutput ("Main", juce::AudioChannelSet::stereo(), true);
    for (const auto* name : { "Kick", "Snare 1", "Snare 2", "Clap", "Rim",
                              "HiHat", "Crash", "Ride", "Low Tom", "Mid Tom",
                              "High Tom", "Zap", "Clave Cowbell", "Maracas" })
        buses = buses.withOutput (name, juce::AudioChannelSet::stereo(), true);
    for(int bus=15;bus<lr608::OutputStage::stemCount;++bus)
        buses=buses.withOutput("Stereo "+juce::String(bus*2+1)+"/"+juce::String(bus*2+2),juce::AudioChannelSet::stereo(),true);
    return buses;
}

LR608AudioProcessor::LR608AudioProcessor (juce::File presetRoot)
    : AudioProcessor (makeBuses()),
      parameters (*this, nullptr, "LR608State", lr608::createParameterLayout()),
      presetManager (parameters, std::move (presetRoot)),
      kickEngineBanks (parameters),
      snareEngineBanks (parameters),
      clapRimEngineBanks (parameters),
      tomEngineBanks (parameters),
      hatCymbalEngineBanks (parameters),
      maracasEngineBanks (parameters),
      cowbellEngineBanks (parameters),
      zapEngineBanks (parameters)
{
    voicePool=std::make_unique<std::array<lr608::PolyphonicVoice,128>>();
    initialiseSlotArchitecture();
    presetManager.setStateCallbacks([this]{return capturePresetState();},
                                    [this](const auto&state){restorePresetState(state);},
                                    [this]{importLegacyPresetAsKit();});
    presetManager.loadFactoryInit();
    refreshSlotTriggerCache();
    for(const auto& descriptor:lr608::generated::parameters)parameters.addParameterListener(descriptor.id,this);
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        parameters.addParameterListener(lr608::slotEngineId(slot),this);
        parameters.addParameterListener(lr608::slotNoteId(slot),this);
        parameters.addParameterListener(lr608::slotChokeTriggerId(slot),this);
        parameters.addParameterListener(lr608::slotChokeTargetId(slot),this);
    }
}

LR608AudioProcessor::~LR608AudioProcessor(){for(const auto&descriptor:lr608::generated::parameters)parameters.removeParameterListener(descriptor.id,this);for(int slot=0;slot<lr608::slotCount;++slot){parameters.removeParameterListener(lr608::slotEngineId(slot),this);parameters.removeParameterListener(lr608::slotNoteId(slot),this);parameters.removeParameterListener(lr608::slotChokeTriggerId(slot),this);parameters.removeParameterListener(lr608::slotChokeTargetId(slot),this);}}

void LR608AudioProcessor::parameterChanged(const juce::String&id,float value)
{
    if(const auto index=catalogIndex(id);index>=0){slotValues[juce::jlimit(0,lr608::slotCount-1,currentProxySlot.load())][index].store(value,std::memory_order_relaxed);return;}
    if(!id.startsWith("slot")||id.length()<8)return;
    const auto slot=id.substring(4,7).getIntValue()-1;
    if(!juce::isPositiveAndBelow(slot,lr608::slotCount))return;
    const auto plain=juce::roundToInt(value);
    if(id.endsWith("Engine"))slotEngineIndices[slot].store(juce::jlimit(0,lr608::slotEngineCount-1,plain),std::memory_order_relaxed);
    else if(id.endsWith("Note"))slotMidiNotes[slot].store(juce::jlimit(0,127,plain),std::memory_order_relaxed);
    else if(id.endsWith("ChokeTrigger"))slotChokeTriggers[slot].store(plain-1,std::memory_order_relaxed);
    else if(id.endsWith("ChokeTarget"))slotChokeTargets[slot].store(plain-1,std::memory_order_relaxed);
}

void LR608AudioProcessor::refreshSlotTriggerCache()
{
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        slotEngineIndices[slot].store(juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(parameters.getRawParameterValue(lr608::slotEngineId(slot))->load())),std::memory_order_relaxed);
        slotMidiNotes[slot].store(juce::jlimit(0,127,juce::roundToInt(parameters.getRawParameterValue(lr608::slotNoteId(slot))->load())),std::memory_order_relaxed);
        slotChokeTriggers[slot].store(juce::roundToInt(parameters.getRawParameterValue(lr608::slotChokeTriggerId(slot))->load())-1,std::memory_order_relaxed);
        slotChokeTargets[slot].store(juce::roundToInt(parameters.getRawParameterValue(lr608::slotChokeTargetId(slot))->load())-1,std::memory_order_relaxed);
    }
}

void LR608AudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    timingEngine.reset();
    for(auto& voice:*voicePool)voice.prepare(sampleRate);
    for(auto& delay:slotDelays)delay.prepare(sampleRate);
    voiceCounter=0;activeVoiceCount=0;outputIdleLatched=false;
    voiceStealCounter.store(0);
    outputStage.prepare (sampleRate);
}

namespace { int catalogIndex(juce::StringRef id){for(int i=0;i<lr608::slotParameterValueCount;++i)if(id==juce::StringRef(lr608::generated::parameters[i].id))return i;return-1;} }

void LR608AudioProcessor::buildEngineDefaults()
{
    const auto setPlain=[this](const char*id,int plain){if(auto*p=parameters.getParameter(id))p->setValueNotifyingHost(p->convertTo0to1(float(plain)));};
    for(int engine=0;engine<lr608::slotEngineCount;++engine)
    {
        if(lr608::isOffEngine(engine)){engineDefaults[engine].fill(0.0f);continue;}
        const auto&i=lr608::slotEngines[engine];
        switch(i.family)
        {
            case lr608::SlotFamily::kick:setPlain("slider247",i.subEngine);kickEngineBanks.switchTo(i.subEngine);break;
            case lr608::SlotFamily::snare1:setPlain("slider248",i.subEngine);snareEngineBanks.switchTo(0,i.subEngine);break;
            case lr608::SlotFamily::snare2:setPlain("slider198",i.subEngine);snareEngineBanks.switchTo(1,i.subEngine);break;
            case lr608::SlotFamily::clap:setPlain("slider207",i.subEngine);clapRimEngineBanks.switchClap(i.subEngine);break;
            case lr608::SlotFamily::rim:setPlain("slider209",i.subEngine);clapRimEngineBanks.switchRim(i.subEngine);break;
            case lr608::SlotFamily::lowTom:case lr608::SlotFamily::midTom:case lr608::SlotFamily::highTom:setPlain("slider219",i.subEngine);tomEngineBanks.switchTo(i.subEngine);break;
            case lr608::SlotFamily::hatClosed:case lr608::SlotFamily::hatOpen:setPlain("slider217",i.subEngine);hatCymbalEngineBanks.switchHat(i.subEngine);break;
            case lr608::SlotFamily::crash:case lr608::SlotFamily::ride:setPlain("slider214",i.subEngine);hatCymbalEngineBanks.switchCymbal(i.subEngine);break;
            case lr608::SlotFamily::maracas:setPlain("slider216",i.subEngine);maracasEngineBanks.switchTo(i.subEngine);break;
            case lr608::SlotFamily::cowbell:setPlain("slider215",i.subEngine);cowbellEngineBanks.switchTo(i.subEngine);break;
            case lr608::SlotFamily::zap:setPlain("slider199",i.subEngine);zapEngineBanks.switchTo(i.subEngine);break;
        }
        for(int p=0;p<lr608::slotParameterValueCount;++p)engineDefaults[engine][p]=parameters.getRawParameterValue(lr608::generated::parameters[p].id)->load();
    }
    setPlain("slider247",0);kickEngineBanks.switchTo(0);
    setPlain("slider248",0);snareEngineBanks.switchTo(0,0);
    setPlain("slider198",0);snareEngineBanks.switchTo(1,0);
    setPlain("slider207",3);clapRimEngineBanks.switchClap(3);
    setPlain("slider209",2);clapRimEngineBanks.switchRim(2);
    setPlain("slider219",0);tomEngineBanks.switchTo(0);
    setPlain("slider217",0);hatCymbalEngineBanks.switchHat(0);
    setPlain("slider214",1);hatCymbalEngineBanks.switchCymbal(1);
    setPlain("slider216",3);maracasEngineBanks.switchTo(3);
    setPlain("slider215",2);cowbellEngineBanks.switchTo(2);
    setPlain("slider199",0);zapEngineBanks.switchTo(0);
}

void LR608AudioProcessor::initialiseSlotArchitecture()
{
    buildEngineDefaults();
    for(int slot=0;slot<lr608::slotCount;++slot){const auto engine=lr608::defaultSlotEngine(slot);for(int p=0;p<lr608::slotParameterValueCount;++p)slotValues[slot][p].store(engineDefaults[engine][p]);slotGrid[slot].store(0);slotOutputs[slot].store(0);}
    currentProxySlot.store(0);loadSlotToProxy(0);
}

void LR608AudioProcessor::captureSlotParameter(int slot,juce::StringRef id)
{
    if(!juce::isPositiveAndBelow(slot,lr608::slotCount))return;if(const auto index=catalogIndex(id);index>=0)slotValues[slot][index].store(parameters.getRawParameterValue(id)->load());
}

void LR608AudioProcessor::captureSlotFromProxy(int slot)
{
    if(!juce::isPositiveAndBelow(slot,lr608::slotCount))return;const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));if(lr608::isOffEngine(engine))return;for(const auto*id:universalSlotParameterIds)captureSlotParameter(slot,id);const auto&page=lr608::generated::pages[lr608::slotEngines[engine].page];for(std::size_t i=0;i<page.parameterCount;++i)if(!lr608::isEngineSelectorId(page.parameterIds[i]))captureSlotParameter(slot,page.parameterIds[i]);
}

void LR608AudioProcessor::loadSlotToProxy(int slot)
{
    if(!juce::isPositiveAndBelow(slot,lr608::slotCount))return;currentProxySlot.store(slot);const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));if(lr608::isOffEngine(engine))return;for(const auto*id:universalSlotParameterIds)if(auto*p=parameters.getParameter(id))p->setValueNotifyingHost(p->convertTo0to1(slotValues[slot][catalogIndex(id)].load()));const auto&page=lr608::generated::pages[lr608::slotEngines[engine].page];for(std::size_t i=0;i<page.parameterCount;++i){if(lr608::isEngineSelectorId(page.parameterIds[i]))continue;if(const auto index=catalogIndex(page.parameterIds[i]);index>=0)if(auto*p=parameters.getParameter(page.parameterIds[i]))p->setValueNotifyingHost(p->convertTo0to1(slotValues[slot][index].load()));}
}

void LR608AudioProcessor::setSlotEngine(int slot,int engine)
{
    if(!juce::isPositiveAndBelow(slot,lr608::slotCount))return;engine=juce::jlimit(0,lr608::slotEngineCount-1,engine);if(auto*p=parameters.getParameter(lr608::slotEngineId(slot))){p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(float(engine)));p->endChangeGesture();}for(int i=0;i<lr608::slotParameterValueCount;++i)slotValues[slot][i].store(engineDefaults[engine][i]);slotGrid[slot].store(0);slotDelays[std::size_t(slot)].reset();
}

LR608AudioProcessor::SlotSoundSnapshot LR608AudioProcessor::copySlotSound(int slot)
{
    SlotSoundSnapshot snapshot;if(!juce::isPositiveAndBelow(slot,lr608::slotCount))return snapshot;
    if(currentProxySlot.load()==slot)captureSlotFromProxy(slot);
    snapshot.engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));
    for(int parameter=0;parameter<lr608::slotParameterValueCount;++parameter)snapshot.values[std::size_t(parameter)]=slotValues[slot][parameter].load();
    return snapshot;
}

bool LR608AudioProcessor::pasteSlotSound(int slot,const SlotSoundSnapshot&snapshot)
{
    if(!juce::isPositiveAndBelow(slot,lr608::slotCount)||!snapshot.isValid()||!juce::isPositiveAndBelow(snapshot.engine,lr608::offEngineIndex))return false;
    const auto targetEngine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));
    if(!lr608::isOffEngine(targetEngine)&&lr608::slotFamilyGroup(lr608::slotEngines[targetEngine].family)!=lr608::slotFamilyGroup(lr608::slotEngines[snapshot.engine].family))return false;
    const auto rememberedGrid=slotGrid[slot].load();setSlotEngine(slot,snapshot.engine);slotGrid[slot].store(rememberedGrid);
    for(int parameter=0;parameter<lr608::slotParameterValueCount;++parameter)slotValues[slot][parameter].store(snapshot.values[std::size_t(parameter)]);
    if(currentProxySlot.load()==slot)loadSlotToProxy(slot);
    return true;
}

juce::String LR608AudioProcessor::copyMidiKeyToText(int note,bool cut,int&layerCount)
{
    layerCount=0;
    if(!juce::isPositiveAndBelow(note,128))return {};
    juce::ValueTree root("LR608MidiKeyClipboard");
    root.setProperty("version",1,nullptr);root.setProperty("engineArchitectureVersion",slotEngineArchitectureVersion,nullptr);root.setProperty("sourceNote",note,nullptr);
    std::vector<int> sourceSlots;
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        const auto engine=slotEngineIndices[slot].load(std::memory_order_relaxed);
        if(slotMidiNotes[slot].load(std::memory_order_relaxed)!=note||lr608::isOffEngine(engine))continue;
        if(currentProxySlot.load()==slot)captureSlotFromProxy(slot);
        juce::ValueTree layer("Layer");
        layer.setProperty("engine",engine,nullptr);layer.setProperty("output",slotOutputs[slot].load(),nullptr);
        layer.setProperty("chokeTrigger",juce::roundToInt(parameters.getRawParameterValue(lr608::slotChokeTriggerId(slot))->load()),nullptr);
        layer.setProperty("chokeTarget",juce::roundToInt(parameters.getRawParameterValue(lr608::slotChokeTargetId(slot))->load()),nullptr);
        for(int p=0;p<lr608::slotParameterValueCount;++p)layer.setProperty(juce::Identifier("p"+juce::String(p)),slotValues[slot][p].load(),nullptr);
        root.addChild(layer,-1,nullptr);sourceSlots.push_back(slot);
    }
    layerCount=int(sourceSlots.size());
    if(layerCount==0)return {};
    auto xml=root.createXml();if(xml==nullptr){layerCount=0;return {};}
    const auto text="LR-608 MIDI KEY CLIPBOARD 1\n"+xml->toString();
    if(cut)for(const auto slot:sourceSlots)setSlotEngine(slot,lr608::offEngineIndex);
    return text;
}

juce::Result LR608AudioProcessor::pasteMidiKeyFromText(int note,const juce::String&text,int&layerCount)
{
    layerCount=0;
    if(!juce::isPositiveAndBelow(note,128))return juce::Result::fail("Press a destination MIDI key first");
    constexpr auto header="LR-608 MIDI KEY CLIPBOARD 1\n";
    if(!text.startsWith(header))return juce::Result::fail("Clipboard does not contain an LR-608 MIDI key");
    auto xml=juce::XmlDocument::parse(text.substring(int(std::char_traits<char>::length(header))));
    if(xml==nullptr)return juce::Result::fail("Invalid LR-608 clipboard data");
    const auto root=juce::ValueTree::fromXml(*xml);
    if(!root.hasType("LR608MidiKeyClipboard")||int(root.getProperty("version",0))!=1||root.getNumChildren()==0)
        return juce::Result::fail("Invalid LR-608 clipboard data");
    std::vector<MidiKeyLayer> layers;
    for(int child=0;child<root.getNumChildren();++child)
    {
        const auto node=root.getChild(child);MidiKeyLayer layer;
        layer.sound.engine=int(node.getProperty("engine",-1));
        const auto clipboardVersion=int(root.getProperty("engineArchitectureVersion",2));
        if(clipboardVersion==2)layer.sound.engine=migrateVersion3EngineIndex(migrateVersion2EngineIndex(layer.sound.engine));
        else if(clipboardVersion==3)layer.sound.engine=migrateVersion3EngineIndex(layer.sound.engine);
        layer.output=int(node.getProperty("output",0));layer.chokeTrigger=int(node.getProperty("chokeTrigger",0));layer.chokeTarget=int(node.getProperty("chokeTarget",0));
        if(!node.hasType("Layer")||!juce::isPositiveAndBelow(layer.sound.engine,lr608::offEngineIndex)||!juce::isPositiveAndBelow(layer.output,lr608::OutputStage::stemCount)||!juce::isPositiveAndBelow(layer.chokeTrigger,129)||!juce::isPositiveAndBelow(layer.chokeTarget,129))
            return juce::Result::fail("Invalid LR-608 clipboard layer");
        for(int p=0;p<lr608::slotParameterValueCount;++p)
        {
            const auto id=juce::Identifier("p"+juce::String(p));
            if(!node.hasProperty(id))
            {
                if(p>=lr608::slotVoiceOverlapParameterIndex){layer.sound.values[std::size_t(p)]=float(lr608::generated::parameters[p].defaultValue);continue;}
                return juce::Result::fail("Incomplete LR-608 clipboard layer");
            }
            const auto value=float(node.getProperty(id));if(!std::isfinite(value))return juce::Result::fail("Invalid LR-608 clipboard value");
            layer.sound.values[std::size_t(p)]=value;
        }
        layers.push_back(layer);
    }
    const auto existingLayers=getActiveSlotCountForMidiNote(note);
    const auto freeLayers=8-existingLayers;
    if(int(layers.size())>freeLayers)
        return juce::Result::fail(freeLayers<=0
            ? "Destination MIDI Note "+juce::String(note)+" already has 8 layers"
            : "Destination MIDI Note "+juce::String(note)+" has only "+juce::String(freeLayers)+(freeLayers==1?" free layer":" free layers"));
    std::vector<int> destinations;
    for(int pass=0;pass<2;++pass)for(int slot=0;slot<lr608::slotCount;++slot)
        if(lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed))&&((slotMidiNotes[slot].load(std::memory_order_relaxed)==note)==(pass==0)))destinations.push_back(slot);
    if(destinations.size()<layers.size())return juce::Result::fail("Not enough Off Slots for all copied layers");
    const auto setPlain=[this](const juce::String&id,int value){if(auto*p=parameters.getParameter(id))p->setValueNotifyingHost(p->convertTo0to1(float(value)));};
    for(std::size_t index=0;index<layers.size();++index)
    {
        const auto slot=destinations[index];const auto&layer=layers[index];setSlotEngine(slot,layer.sound.engine);
        for(int p=0;p<lr608::slotParameterValueCount;++p)slotValues[slot][p].store(layer.sound.values[std::size_t(p)]);
        setSlotOutput(slot,layer.output);setPlain(lr608::slotNoteId(slot),note);setPlain(lr608::slotChokeTriggerId(slot),layer.chokeTrigger);setPlain(lr608::slotChokeTargetId(slot),layer.chokeTarget);
        if(currentProxySlot.load()==slot)loadSlotToProxy(slot);
    }
    layerCount=int(layers.size());return juce::Result::ok();
}

bool LR608AudioProcessor::clearSlot(int slot)
{
    if(!juce::isPositiveAndBelow(slot,lr608::slotCount)
        ||lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed)))return false;
    const auto setOff=[this](const juce::String&id){if(auto*p=parameters.getParameter(id))p->setValueNotifyingHost(p->convertTo0to1(0.0f));};
    setSlotEngine(slot,lr608::offEngineIndex);setSlotOutput(slot,0);
    setOff(lr608::slotChokeTriggerId(slot));setOff(lr608::slotChokeTargetId(slot));
    return true;
}

int LR608AudioProcessor::clearMidiKey(int note)
{
    if(!juce::isPositiveAndBelow(note,128))return 0;
    int cleared=0;
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        if(slotMidiNotes[slot].load(std::memory_order_relaxed)!=note||lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed)))continue;
        if(clearSlot(slot))++cleared;
    }
    return cleared;
}
int LR608AudioProcessor::getSlotGridPosition(int slot)const{return juce::isPositiveAndBelow(slot,lr608::slotCount)?slotGrid[slot].load():0;}
void LR608AudioProcessor::setSlotGridPosition(int slot,int value){if(juce::isPositiveAndBelow(slot,lr608::slotCount))slotGrid[slot].store(juce::jmax(0,value));}
int LR608AudioProcessor::getSlotOutput(int slot)const{return juce::isPositiveAndBelow(slot,lr608::slotCount)?slotOutputs[slot].load():0;}
void LR608AudioProcessor::setSlotOutput(int slot,int output){if(juce::isPositiveAndBelow(slot,lr608::slotCount))slotOutputs[slot].store(juce::jlimit(0,lr608::OutputStage::stemCount-1,output));}
int LR608AudioProcessor::findFirstSlotForMidiNote(int note)const{if(!juce::isPositiveAndBelow(note,128))return-1;for(int slot=0;slot<lr608::slotCount;++slot)if(slotMidiNotes[slot].load(std::memory_order_relaxed)==note&&!lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed)))return slot;return-1;}
int LR608AudioProcessor::findNextSlotForMidiNote(int note,int afterSlot)const{if(!juce::isPositiveAndBelow(note,128))return-1;const auto matches=[this,note](int slot){return juce::isPositiveAndBelow(slot,lr608::slotCount)&&slotMidiNotes[slot].load(std::memory_order_relaxed)==note&&!lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed));};if(matches(afterSlot))for(int slot=afterSlot+1;slot<lr608::slotCount;++slot)if(matches(slot))return slot;return findFirstSlotForMidiNote(note);}
int LR608AudioProcessor::getActiveSlotCountForMidiNote(int note,int excludedSlot)const
{
    if(!juce::isPositiveAndBelow(note,128))return 0;
    int count=0;
    for(int slot=0;slot<lr608::slotCount;++slot)
        if(slot!=excludedSlot&&slotMidiNotes[slot].load(std::memory_order_relaxed)==note
            &&!lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed)))
            ++count;
    return count;
}
int LR608AudioProcessor::getActiveSlotLayerNumber(int note,int targetSlot)const
{
    if(!juce::isPositiveAndBelow(note,128)||!juce::isPositiveAndBelow(targetSlot,lr608::slotCount))return 0;
    int layer=0;
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        if(slotMidiNotes[slot].load(std::memory_order_relaxed)!=note
            ||lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed)))continue;
        ++layer;
        if(slot==targetSlot)return layer;
    }
    return 0;
}
bool LR608AudioProcessor::canAssignSlotToMidiNote(int slot,int note)const
{
    return juce::isPositiveAndBelow(slot,lr608::slotCount)&&juce::isPositiveAndBelow(note,128)
        &&getActiveSlotCountForMidiNote(note,slot)<lr608::maxMidiNoteLayers;
}
int LR608AudioProcessor::findAssignableMidiNote(int startNote,int direction,int slot)const
{
    if(direction==0)return canAssignSlotToMidiNote(slot,startNote)?startNote:-1;
    for(int note=juce::jlimit(0,127,startNote);juce::isPositiveAndBelow(note,128);note+=direction>0?1:-1)
        if(canAssignSlotToMidiNote(slot,note))return note;
    return -1;
}
juce::String LR608AudioProcessor::getSlotName(int slot)const{return juce::isPositiveAndBelow(slot,lr608::slotCount)?slotNames[std::size_t(slot)]:juce::String();}
void LR608AudioProcessor::setSlotName(int slot,const juce::String&name){if(juce::isPositiveAndBelow(slot,lr608::slotCount))slotNames[std::size_t(slot)]=name.trim().substring(0,48);}
int LR608AudioProcessor::getActiveVoiceCount()const{int count=0;for(const auto&voice:*voicePool)if(voice.isActive())++count;return count;}
int LR608AudioProcessor::getActiveVoiceCountForMidiNote(int note)const{int count=0;for(const auto&voice:*voicePool)if(voice.isActive()&&voice.getMidiNote()==note)++count;return count;}

juce::ValueTree LR608AudioProcessor::capturePresetState()
{
    storeSlotStates();
    auto result=parameters.copyState();
    result.setProperty("slotEngineArchitectureVersion",slotEngineArchitectureVersion,nullptr);
    for(const auto*property:{"uiSelectedSlot","uiSelectedColumn","uiInGrid","uiGlobalGrid","presetBrowserDirectory","presetBrowserSelection","presetBrowserRow","currentPresetRelativePath"})
        result.removeProperty(juce::Identifier(property),nullptr);
    return result;
}

void LR608AudioProcessor::restorePresetState(const juce::ValueTree&complete)
{
    if(!complete.isValid()||complete.getType()!=parameters.state.getType())return;
    const auto selected=int(parameters.state.getProperty("uiSelectedSlot",0));
    const auto column=int(parameters.state.getProperty("uiSelectedColumn",0));
    const auto inGrid=bool(parameters.state.getProperty("uiInGrid",false));
    const auto globalGrid=int(parameters.state.getProperty("uiGlobalGrid",0));
    const auto browserDirectory=parameters.state.getProperty("presetBrowserDirectory").toString();
    const auto browserSelection=parameters.state.getProperty("presetBrowserSelection").toString();
    const auto browserRow=int(parameters.state.getProperty("presetBrowserRow",0));
    const auto currentPreset=parameters.state.getProperty("currentPresetRelativePath").toString();
    parameters.replaceState(migrateSlotEngineArchitecture(complete));
    parameters.state.setProperty("uiSelectedSlot",juce::jlimit(0,lr608::slotCount-1,selected),nullptr);
    parameters.state.setProperty("uiSelectedColumn",column,nullptr);
    parameters.state.setProperty("uiInGrid",inGrid,nullptr);
    parameters.state.setProperty("uiGlobalGrid",globalGrid,nullptr);
    if(browserDirectory.isNotEmpty())parameters.state.setProperty("presetBrowserDirectory",browserDirectory,nullptr);
    if(browserSelection.isNotEmpty())parameters.state.setProperty("presetBrowserSelection",browserSelection,nullptr);
    parameters.state.setProperty("presetBrowserRow",browserRow,nullptr);
    if(currentPreset.isNotEmpty())parameters.state.setProperty("currentPresetRelativePath",currentPreset,nullptr);
    kickEngineBanks.resetFromCurrentState();snareEngineBanks.resetFromCurrentState();clapRimEngineBanks.resetFromCurrentState();tomEngineBanks.resetFromCurrentState();hatCymbalEngineBanks.resetFromCurrentState();maracasEngineBanks.resetFromCurrentState();cowbellEngineBanks.resetFromCurrentState();zapEngineBanks.resetFromCurrentState();
    restoreSlotStates();
}

void LR608AudioProcessor::importLegacyPresetAsKit()
{
    static constexpr int bases[]{0,8,8,17,24,30,35,40,45,52,59,63,67,71,78};
    static constexpr const char*selectors[]{"slider247","slider248","slider198","slider207","slider209","slider219","slider219","slider219","slider217","slider217","slider214","slider214","slider216","slider215","slider199"};
    static constexpr const char*snare1Ids[]{"slider020","slider021","slider022","slider023","slider024","slider029","slider116","slider117","slider026","slider025","slider027","slider028","slider019","slider092","slider094","slider096","slider098","slider108","slider139","slider132","slider133","slider134","slider135","slider136","slider137","slider156","slider157","slider158","slider159","slider160","slider161"};
    static constexpr const char*snare2Ids[]{"slider221","slider222","slider223","slider224","slider225","slider230","slider232","slider233","slider227","slider226","slider228","slider229","slider220","slider093","slider095","slider097","slider099","slider231","slider240","slider234","slider235","slider236","slider237","slider238","slider239","slider241","slider242","slider243","slider244","slider245","slider246"};
    // Preserve the complete 128-Slot instrument bank that existed before the
    // optional Off engine was introduced.  Legacy JSFX kits overwrite their
    // first 19 Slots below; the remaining Slots retain their original engine,
    // MIDI note and per-engine init values.
    for(int slot=0;slot<lr608::slotCount;++slot){setSlotEngine(slot,lr608::defaultSlotEngine(slot));setSlotOutput(slot,0);if(auto*p=parameters.getParameter(lr608::slotNoteId(slot)))p->setValueNotifyingHost(p->convertTo0to1(float(lr608::defaultSlotNote(slot))));if(auto*p=parameters.getParameter(lr608::slotChokeTriggerId(slot)))p->setValueNotifyingHost(0);if(auto*p=parameters.getParameter(lr608::slotChokeTargetId(slot)))p->setValueNotifyingHost(0);}
    for(int slot=0;slot<15;++slot)
    {
        const auto sub=juce::roundToInt(parameters.getRawParameterValue(selectors[slot])->load());
        const auto engine=bases[slot]+sub;setSlotEngine(slot,engine);
        const auto routeParameter=slot==2?"routeSnare2":lr608::generated::parameters[lr608::slotEngines[engine].routeParameterIndex].id;
        setSlotOutput(slot,juce::roundToInt(parameters.getRawParameterValue(routeParameter)->load()));
        if(auto*p=parameters.getParameter(lr608::slotNoteId(slot)))p->setValueNotifyingHost(p->convertTo0to1(float(lr608::defaultSlotNote(slot))));
        if(slot==2)
        {
            // Snare 2 is no longer exposed as a separate Engine family, but
            // every legacy Factory preset keeps its actual Snare 2 sound.
            // The two engines have the same 31 semantic controls, stored under
            // different legacy slider IDs, so migrate them one by one.
            for(std::size_t parameter=0;parameter<std::size(snare1Ids);++parameter)
                if(const auto destination=catalogIndex(snare1Ids[parameter]);destination>=0)
                    slotValues[slot][destination].store(parameters.getRawParameterValue(snare2Ids[parameter])->load());
            slotValues[slot][catalogIndex("slider250")].store(parameters.getRawParameterValue("slider250")->load());
            slotValues[slot][catalogIndex("slider251")].store(parameters.getRawParameterValue("slider251")->load());
        }
        else captureSlotFromProxy(slot);
    }
    static constexpr int extraBases[]{30,45,35,40};
    static constexpr const char*extraSelectors[]{"slider219","slider217","slider219","slider219"};
    static constexpr int extraNotes[]{43,44,47,50};
    for(int extra=0;extra<4;++extra)
    {
        const auto slot=15+extra;
        const auto engine=extraBases[extra]+juce::roundToInt(parameters.getRawParameterValue(extraSelectors[extra])->load());setSlotEngine(slot,engine);
        setSlotOutput(slot,juce::roundToInt(parameters.getRawParameterValue(lr608::generated::parameters[lr608::slotEngines[engine].routeParameterIndex].id)->load()));
        if(auto*p=parameters.getParameter(lr608::slotNoteId(slot)))p->setValueNotifyingHost(p->convertTo0to1(float(extraNotes[extra])));
        captureSlotFromProxy(slot);
    }
    // The original JSFX has a fixed hi-hat choke: closed (42) and pedal (44)
    // both truncate every voice started by open hi-hat note 46. Factory kits
    // retain that behaviour using the new generic per-Slot Trigger/Target pairs.
    const auto setChoke=[](juce::AudioProcessorValueTreeState& state,int slot,int trigger,int target)
    {
        if(auto*p=state.getParameter(lr608::slotChokeTriggerId(slot)))p->setValueNotifyingHost(p->convertTo0to1(float(trigger+1)));
        if(auto*p=state.getParameter(lr608::slotChokeTargetId(slot)))p->setValueNotifyingHost(p->convertTo0to1(float(target+1)));
    };
    setChoke(parameters,8,42,46);
    setChoke(parameters,16,44,46);
    const auto selected=juce::jlimit(0,lr608::slotCount-1,int(parameters.state.getProperty("uiSelectedSlot",0)));
    loadSlotToProxy(selected);
}

double LR608AudioProcessor::getTailLengthSeconds() const
{
    for(int slot=0;slot<lr608::slotCount;++slot)
        if(slotValues[slot][lr608::slotDelayWetParameterIndex].load(std::memory_order_relaxed)>0.0f
           &&slotValues[slot][lr608::slotDelayFeedbackParameterIndex].load(std::memory_order_relaxed)>=99.999f)
            return std::numeric_limits<double>::infinity();
    return 32.0;
}

bool LR608AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    if (! (input.isDisabled() || input == juce::AudioChannelSet::stereo()))
        return false;
    for (int bus = 0; bus < getBusCount (false); ++bus)
    {
        const auto channels = layouts.getChannelSet (false, bus);
        if (! (channels.isDisabled() || channels == juce::AudioChannelSet::stereo()))
            return false;
    }
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void LR608AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    {
        buffer.clear();
        bool hasDelayTail=false;
        for(const auto&delay:slotDelays)if(delay.isActive()){hasDelayTail=true;break;}
        // At true silence the only mandatory operation is clearing the synth
        // outputs. Delay tails are part of the Slot insert path, so they keep
        // the renderer alive even after the source voice has finished.
        if(midi.isEmpty()&&activeVoiceCount==0&&!timingEngine.needsSampleClock()&&!hasDelayTail)
        {
            if(!outputIdleLatched){outputStage.reset();outputIdleLatched=true;}
            return;
        }
        lr608::TimingSettings timing;timing.sampleRate=currentSampleRate;timing.rollDivision=juce::roundToInt(parameters.getRawParameterValue("slider249")->load());timing.shufflePercent=parameters.getRawParameterValue("slider253")->load();timing.secondHitReductionPercent=parameters.getRawParameterValue("slider254")->load();timing.cowbellEngine=0;
        if(auto*hostPlayHead=getPlayHead())if(const auto position=hostPlayHead->getPosition()){if(const auto bpm=position->getBpm())timing.tempo=*bpm;if(const auto signature=position->getTimeSignature()){timing.timeSignatureNumerator=signature->numerator;timing.timeSignatureDenominator=signature->denominator;}if(const auto ppq=position->getPpqPosition()){timing.hostTimelineValid=true;timing.hostPpqPosition=*ppq;if(const auto barStart=position->getPpqPositionOfLastBarStart())timing.hostBarStartPpq=*barStart;else timing.hostBarStartPpq=std::floor(*ppq/(timing.timeSignatureNumerator*4.0/timing.timeSignatureDenominator))*(timing.timeSignatureNumerator*4.0/timing.timeSignatureDenominator);}timing.hostPlaying=position->getIsPlaying();}
        timingEngine.setSettings(timing);

        // One stereo insert delay per Slot. Parameters belong to the Slot/engine
        // snapshot exactly like Pan and the musical filters; old presets receive
        // these defaults, with Wet at zero, and therefore keep the old sound.
        // The exact pre-delay renderer remains the fast path while every insert
        // is at its transparent defaults.
        bool delayInsertNeeded=false;
        std::array<bool,lr608::slotCount> slotInsertNeeded{};
        for(int slot=0;slot<lr608::slotCount;++slot)
        {
            lr608::SlotDelay::Settings settings;
            settings.dryPercent=slotValues[slot][lr608::slotDelayDryParameterIndex].load(std::memory_order_relaxed);
            settings.wetPercent=slotValues[slot][lr608::slotDelayWetParameterIndex].load(std::memory_order_relaxed);
            settings.outputDb=slotValues[slot][lr608::slotDelayVolumeParameterIndex].load(std::memory_order_relaxed);
            const auto insertNeeded=settings.wetPercent>1.0e-9||std::abs(settings.dryPercent-100.0)>1.0e-9||std::abs(settings.outputDb)>1.0e-9;
            slotInsertNeeded[std::size_t(slot)]=insertNeeded;
            delayInsertNeeded=delayInsertNeeded||insertNeeded;
            settings.divisionIndex=juce::roundToInt(slotValues[slot][lr608::slotDelayDivisionParameterIndex].load(std::memory_order_relaxed));
            settings.feedbackPercent=slotValues[slot][lr608::slotDelayFeedbackParameterIndex].load(std::memory_order_relaxed);
            settings.glideMs=slotValues[slot][lr608::slotDelayGlideParameterIndex].load(std::memory_order_relaxed);
            settings.filter=slotValues[slot][lr608::slotDelayFilterParameterIndex].load(std::memory_order_relaxed);
            settings.leftOffsetMs=slotValues[slot][lr608::slotDelayLeftOffsetParameterIndex].load(std::memory_order_relaxed);
            settings.rightOffsetMs=slotValues[slot][lr608::slotDelayRightOffsetParameterIndex].load(std::memory_order_relaxed);
            settings.tempo=timing.tempo;
            slotDelays[std::size_t(slot)].setSettings(settings);
        }
        hasDelayTail=false;for(const auto&delay:slotDelays)if(delay.isActive()){hasDelayTail=true;break;}
        if(midi.isEmpty()&&activeVoiceCount==0&&!timingEngine.needsSampleClock()&&!hasDelayTail)
        {
            if(!outputIdleLatched){outputStage.reset();outputIdleLatched=true;}
            return;
        }
        const auto masterDb=double(parameters.getRawParameterValue("slider256")->load());
        std::array<float*,lr608::OutputStage::stemCount> outputLeft{},outputRight{};
        for(int bus=0;bus<getBusCount(false);++bus)if(getBus(false,bus)->isEnabled()){auto destination=getBusBuffer(buffer,false,bus);outputLeft[std::size_t(bus)]=destination.getWritePointer(0);outputRight[std::size_t(bus)]=destination.getWritePointer(1);}
        auto trigger=[&](lr608::DrumTrigger hit)
        {
            lastPlayedMidiNote.store(hit.note,std::memory_order_relaxed);
            midiNavigationEvent.store(((++midiNavigationSerial)<<8)|std::uint64_t(juce::jlimit(0,127,hit.note)),std::memory_order_release);
            for(int slot=0;slot<lr608::slotCount;++slot)
            {
                if(lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed)))continue;
                const auto chokeTrigger=slotChokeTriggers[slot].load(std::memory_order_relaxed),chokeTarget=slotChokeTargets[slot].load(std::memory_order_relaxed);
                if(chokeTrigger!=hit.note||!juce::isPositiveAndBelow(chokeTarget,128))continue;
                for(int active=0;active<activeVoiceCount;++active){auto&voice=(*voicePool)[std::size_t(activeVoiceIndices[std::size_t(active)])];if(voice.isActive()&&voice.getMidiNote()==chokeTarget)voice.choke();}
            }
            bool hasMatchingSlot=false;
            for(int slot=0;slot<lr608::slotCount;++slot)
                if(slotMidiNotes[slot].load(std::memory_order_relaxed)==hit.note&&!lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed))){hasMatchingSlot=true;break;}
            if(!hasMatchingSlot)return;

            const auto hitGeneration=++voiceCounter;
            for(int slot=0;slot<lr608::slotCount;++slot)
            {
                if(slotMidiNotes[slot].load(std::memory_order_relaxed)!=hit.note||lr608::isOffEngine(slotEngineIndices[slot].load(std::memory_order_relaxed)))continue;
                const auto overlap=juce::jlimit(1,2,juce::roundToInt(slotValues[slot][lr608::slotVoiceOverlapParameterIndex].load(std::memory_order_relaxed)));
                std::array<std::uint64_t,2> generations{};
                int generationCount=0;
                std::uint64_t oldestGeneration=std::numeric_limits<std::uint64_t>::max();
                for(int active=0;active<activeVoiceCount;++active)
                {
                    const auto&voice=(*voicePool)[std::size_t(activeVoiceIndices[std::size_t(active)])];
                    if(!voice.isActive()||voice.getSlot()!=slot)continue;
                    const auto age=voice.getAge();
                    bool known=false;for(int generation=0;generation<generationCount;++generation)if(generations[std::size_t(generation)]==age){known=true;break;}
                    if(!known&&generationCount<2)generations[std::size_t(generationCount++)]=age;
                    oldestGeneration=std::min(oldestGeneration,age);
                }
                if(generationCount>=overlap)
                {
                    for(int active=0;active<activeVoiceCount;)
                    {
                        auto&voice=(*voicePool)[std::size_t(activeVoiceIndices[std::size_t(active)])];
                        if(voice.isActive()&&voice.getSlot()==slot&&(overlap==1||voice.getAge()==oldestGeneration))
                        {
                            voice.reset();
                            activeVoiceIndices[std::size_t(active)]=activeVoiceIndices[std::size_t(--activeVoiceCount)];
                            continue;
                        }
                        ++active;
                    }
                }
                int targetIndex=-1;
                for(int voice=0;voice<int(voicePool->size());++voice)if(!(*voicePool)[std::size_t(voice)].isActive()){targetIndex=voice;break;}
                if(targetIndex<0){targetIndex=int(std::distance(voicePool->begin(),std::min_element(voicePool->begin(),voicePool->end(),[](const auto&a,const auto&b){return a.getAge()<b.getAge();})));voiceStealCounter.fetch_add(1,std::memory_order_relaxed);}
                else activeVoiceIndices[std::size_t(activeVoiceCount++)]=targetIndex;
                (*voicePool)[std::size_t(targetIndex)].start(slotEngineIndices[slot].load(std::memory_order_relaxed),hit.velocity,slotOutputs[slot].load(),hit.note,slot,slotValues[slot],timing.tempo,hitGeneration);
            }
        };
        juce::MidiBuffer incomingMidi;
        incomingMidi.swapWith(midi);
        auto iterator=incomingMidi.findNextSamplePosition(0);
        for(int sample=0;sample<buffer.getNumSamples();++sample)
        {
            auto emitMidi=[&](lr608::GeneratedDrumMidi event){const auto channel=juce::jlimit(1,16,event.channel+1);midi.addEvent(event.noteOn?juce::MidiMessage::noteOn(channel,event.note,juce::uint8(event.velocity)):juce::MidiMessage::noteOff(channel,event.note),sample);};
            while(iterator!=incomingMidi.cend()&&(*iterator).samplePosition==sample){const auto message=(*iterator).getMessage();const auto*bytes=message.getRawData();bool consumed=false;if(message.getRawDataSize()>=3)consumed=timingEngine.handleMidi(bytes[0],bytes[1],bytes[2],trigger,emitMidi);if(!consumed)midi.addEvent(message,sample);++iterator;}
            timingEngine.tick(trigger,emitMidi);

            std::array<lr608::StereoSample,lr608::OutputStage::stemCount>buses{};
            bool anySlotProcessing=false;
            if(!delayInsertNeeded)
            {
                // Backward-compatible fast path: identical summing/routing to
                // the renderer that existed before the per-Slot delay.
                for(int active=0;active<activeVoiceCount;)
                {
                    auto&voice=(*voicePool)[std::size_t(activeVoiceIndices[std::size_t(active)])];
                    if(!voice.isActive()){activeVoiceIndices[std::size_t(active)]=activeVoiceIndices[std::size_t(--activeVoiceCount)];continue;}
                    anySlotProcessing=true;
                    const auto rendered=voice.render();auto&bus=buses[juce::jlimit(0,lr608::OutputStage::stemCount-1,voice.getRoute())];bus.left+=rendered.left;bus.right+=rendered.right;
                    if(!voice.isActive()){activeVoiceIndices[std::size_t(active)]=activeVoiceIndices[std::size_t(--activeVoiceCount)];continue;}
                    ++active;
                }
            }
            else
            {
                // Delay-enabled path: sum the polyphonic voices of each Slot,
                // then run the Slot through its final stereo insert before bus routing.
                std::array<lr608::StereoSample,lr608::slotCount> slotSamples{};
                std::array<bool,lr608::slotCount> touched{};
                std::array<int,lr608::slotCount> slotsToProcess{};
                int slotsToProcessCount=0;
                const auto touchSlot=[&](int slot)
                {
                    if(touched[std::size_t(slot)])return;
                    touched[std::size_t(slot)]=true;
                    slotsToProcess[std::size_t(slotsToProcessCount++)]=slot;
                };
                for(int slot=0;slot<lr608::slotCount;++slot)
                    if(slotInsertNeeded[std::size_t(slot)]&&slotDelays[std::size_t(slot)].isActive())touchSlot(slot);
                for(int active=0;active<activeVoiceCount;)
                {
                    auto&voice=(*voicePool)[std::size_t(activeVoiceIndices[std::size_t(active)])];
                    if(!voice.isActive()){activeVoiceIndices[std::size_t(active)]=activeVoiceIndices[std::size_t(--activeVoiceCount)];continue;}
                    const auto rendered=voice.render();
                    const auto slot=juce::jlimit(0,lr608::slotCount-1,voice.getSlot());
                    if(slotInsertNeeded[std::size_t(slot)])
                    {
                        touchSlot(slot);
                        slotSamples[std::size_t(slot)].left+=rendered.left;
                        slotSamples[std::size_t(slot)].right+=rendered.right;
                    }
                    else
                    {
                        // Slots whose insert is still at its defaults remain on
                        // the exact legacy voice->bus path, even while another
                        // Slot is using its delay.
                        const auto route=juce::jlimit(0,lr608::OutputStage::stemCount-1,voice.getRoute());
                        buses[std::size_t(route)].left+=rendered.left;
                        buses[std::size_t(route)].right+=rendered.right;
                        anySlotProcessing=true;
                    }
                    if(!voice.isActive()){activeVoiceIndices[std::size_t(active)]=activeVoiceIndices[std::size_t(--activeVoiceCount)];continue;}
                    ++active;
                }
                anySlotProcessing=anySlotProcessing||slotsToProcessCount>0;
                for(int item=0;item<slotsToProcessCount;++item)
                {
                    const auto slot=slotsToProcess[std::size_t(item)];
                    const auto rendered=slotDelays[std::size_t(slot)].process(slotSamples[std::size_t(slot)]);
                    const auto route=juce::jlimit(0,lr608::OutputStage::stemCount-1,slotOutputs[slot].load(std::memory_order_relaxed));
                    buses[std::size_t(route)].left+=rendered.left;
                    buses[std::size_t(route)].right+=rendered.right;
                }
            }
            if(!anySlotProcessing)
            {
                if(activeVoiceCount==0&&!timingEngine.needsSampleClock())
                {
                    if(!outputIdleLatched){outputStage.reset();outputIdleLatched=true;}
                }
                continue;
            }
            outputIdleLatched=false;
            outputStage.process(buses,true,masterDb,0);
            for(int bus=0;bus<getBusCount(false);++bus)if(outputLeft[std::size_t(bus)]!=nullptr&&(buses[bus].left!=0||buses[bus].right!=0)){outputLeft[std::size_t(bus)][sample]+=float(buses[bus].left);outputRight[std::size_t(bus)][sample]+=float(buses[bus].right);}
        }
        return;
    }

#if 0 // Superseded by the slot-specific 128-voice renderer above.
    lr608::TimingSettings timing;
    timing.sampleRate = currentSampleRate;
    timing.rollDivision = juce::roundToInt (parameters.getRawParameterValue ("slider249")->load());
    timing.shufflePercent = parameters.getRawParameterValue ("slider253")->load();
    timing.secondHitReductionPercent = parameters.getRawParameterValue ("slider254")->load();
    timing.cowbellEngine = juce::roundToInt (parameters.getRawParameterValue ("slider215")->load());
    if (auto* hostPlayHead = getPlayHead())
        if (const auto position = hostPlayHead->getPosition())
        {
            if (const auto bpm = position->getBpm()) timing.tempo = *bpm;
            if (const auto signature = position->getTimeSignature())
            {
                timing.timeSignatureNumerator = signature->numerator;
                timing.timeSignatureDenominator = signature->denominator;
            }
        }
    timingEngine.setSettings (timing);

    const auto value = [this] (const char* id) { return double (parameters.getRawParameterValue (id)->load()); };
    lr608::Kick808Parameters kick {
        value ("slider011"), value ("slider012"), value ("slider013"), value ("slider014"),
        value ("slider015"), value ("slider016"), value ("slider017"), value ("slider018"),
        value ("slider067"), value ("slider078"), value ("slider079"), value ("slider100"),
        value ("slider101"), value ("slider102"), value ("slider103"), juce::roundToInt (value ("slider104")),
        value ("slider105"), value ("slider106"), value ("slider107"), value ("slider138"),
        value ("slider140"), value ("slider141"), value ("slider142"), value ("slider148"),
        value ("slider149"), value ("slider150"), value ("slider152"), value ("slider153"),
        value ("slider154"), value ("slider155"), juce::roundToInt (value ("slider151")),
        value ("slider250"), value ("slider251"), value ("slider256")
    };
    const auto kickEngine = juce::roundToInt (value ("slider247"));
    if (kickEngine != lastKickEngine)
    {
        // Never let an existing tail run through another engine's parameter
        // map. The JSFX applies a guarded fade here; hard reset is the safe
        // native checkpoint until that crossfade is ported.
        kick808.reset();
        kickOthers.reset();
        lastKickEngine = kickEngine;
    }
    const auto kickRoute = juce::jlimit (0, getBusCount (false) - 1,
                                         juce::roundToInt (value ("routeKick")));

    constexpr const char* snareIds[2][31] {
        { "slider020","slider021","slider022","slider023","slider024","slider029","slider116","slider117","slider026","slider025","slider027","slider028","slider019","slider092","slider094","slider096","slider098","slider108","slider139","slider132","slider133","slider134","slider135","slider136","slider137","slider156","slider157","slider158","slider159","slider160","slider161" },
        { "slider221","slider222","slider223","slider224","slider225","slider230","slider232","slider233","slider227","slider226","slider228","slider229","slider220","slider093","slider095","slider097","slider099","slider231","slider240","slider234","slider235","slider236","slider237","slider238","slider239","slider241","slider242","slider243","slider244","slider245","slider246" }
    };
    lr608::SnareParameters snareParameters[2];
    for (int slot = 0; slot < 2; ++slot)
    {
        for (int parameter = 0; parameter < 31; ++parameter)
            snareParameters[slot].v[parameter] = value (snareIds[slot][parameter]);
        snareParameters[slot].accentThreshold = value ("slider250");
        snareParameters[slot].accentCharacter = value ("slider251");
    }
    const int snareEngines[2] { juce::roundToInt(value("slider248")), juce::roundToInt(value("slider198")) };
    if (snareEngines[0] != lastSnare1Engine) { snare1.reset(); lastSnare1Engine = snareEngines[0]; }
    if (snareEngines[1] != lastSnare2Engine) { snare2.reset(); lastSnare2Engine = snareEngines[1]; }
    const int snareRoutes[2] {
        juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeSnare1"))),
        juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeSnare2"))) };
    constexpr const char* clapIds[]{"slider030","slider031","slider032","slider033","slider034","slider208","slider035","slider036","slider037","slider038","slider039"};
    constexpr const char* rimIds[]{"slider071","slider070","slider069","slider068","slider124","slider125","slider143","slider144","slider145","slider146","slider147","slider252","slider162","slider163","slider164","slider165","slider166","slider167"};
    lr608::ClapParameters clapParameters; for(int i=0;i<11;++i)clapParameters.v[i]=value(clapIds[i]);
    lr608::RimParameters rimParameters; for(int i=0;i<18;++i)rimParameters.v[i]=value(rimIds[i]);
    clapParameters.accentThreshold=rimParameters.accentThreshold=value("slider250");
    clapParameters.accentCharacter=rimParameters.accentCharacter=value("slider251"); rimParameters.tempo=timing.tempo;
    const auto clapEngine=juce::roundToInt(value("slider207")),rimEngine=juce::roundToInt(value("slider209"));
    if(clapEngine!=lastClapEngine){clap.reset();lastClapEngine=clapEngine;} if(rimEngine!=lastRimEngine){rim.reset();lastRimEngine=rimEngine;}
    const auto clapRoute=juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeClap")));
    const auto rimRoute=juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeRim")));

    constexpr const char* tomIds[3][18] {
        {"slider064","slider040","slider041","slider042","slider043","slider044","slider045","slider109","slider118","slider046","slider047","slider121","slider174","slider175","slider176","slider177","slider178","slider179"},
        {"slider065","slider048","slider049","slider050","slider051","slider052","slider053","slider110","slider119","slider054","slider055","slider122","slider180","slider181","slider182","slider183","slider184","slider185"},
        {"slider066","slider056","slider057","slider058","slider059","slider060","slider061","slider111","slider120","slider062","slider063","slider123","slider186","slider187","slider188","slider189","slider190","slider191"}
    };
    lr608::TomParameters tomParameters[3];
    for(int tom=0;tom<3;++tom){for(int p=0;p<18;++p)tomParameters[tom].v[p]=value(tomIds[tom][p]);tomParameters[tom].accentThreshold=value("slider250");tomParameters[tom].accentCharacter=value("slider251");}
    const auto tomEngine=juce::roundToInt(value("slider219"));
    if(tomEngine!=lastTomEngine){lowTom.reset();midTom.reset();highTom.reset();lastTomEngine=tomEngine;}
    const int tomRoutes[]{juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeLowTom"))),juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeMidTom"))),juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeHighTom")))};
    constexpr const char* hatIds[]{"slider005","slider001","slider002","slider003","slider004","slider114","slider218","slider115","slider090","slider091"};lr608::HatParameters hatParameters;for(int p=0;p<10;++p)hatParameters.v[p]=value(hatIds[p]);
    constexpr const char* cymIds[]{"slider009","slider010","slider006","slider007","slider008","slider112","slider113"};lr608::CymbalParameters cymbalParameters;for(int p=0;p<7;++p)cymbalParameters.v[p]=value(cymIds[p]);
    const auto hatEngine=juce::roundToInt(value("slider217")),cymbalEngine=juce::roundToInt(value("slider214"));if(hatEngine!=lastHatEngine){hiHat.reset();lastHatEngine=hatEngine;}if(cymbalEngine!=lastCymbalEngine){crash.reset();ride.reset();lastCymbalEngine=cymbalEngine;}
    const int cymRoutes[]{juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeHiHat"))),juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeCrash"))),juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeRide")))};
    constexpr const char* maracasIds[]{"slider168","slider169","slider170","slider171","slider172","slider173","slider205","slider206"};lr608::MaracasParameters maracasParameters;for(int p=0;p<8;++p)maracasParameters.v[p]=value(maracasIds[p]);maracasParameters.accentThreshold=value("slider250");maracasParameters.accentCharacter=value("slider251");const auto maracasEngine=juce::roundToInt(value("slider216"));if(maracasEngine!=lastMaracasEngine){maracas.reset();lastMaracasEngine=maracasEngine;}const auto maracasRoute=juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeMaracas")));
    constexpr const char* cowbellIds[]{"slider080","slider081","slider082","slider083","slider084","slider085","slider086","slider087","slider088","slider089","slider204"};lr608::CowbellParameters cowbellParameters;for(int p=0;p<11;++p)cowbellParameters.v[p]=value(cowbellIds[p]);cowbellParameters.accentThreshold=value("slider250");cowbellParameters.accentCharacter=value("slider251");const auto cowbellEngine=juce::roundToInt(value("slider215"));if(cowbellEngine!=lastCowbellEngine){cowbell.reset();lastCowbellEngine=cowbellEngine;}const auto cowbellRoute=juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeCowbell")));
    constexpr const char* zapIds[]{"slider072","slider073","slider074","slider075","slider076","slider077","slider126","slider127","slider128","slider129","slider130","slider131","slider210","slider211","slider212","slider213","slider192","slider193","slider194","slider195","slider196","slider197"};lr608::ZapParameters zapParameters;for(int p=0;p<22;++p)zapParameters.v[p]=value(zapIds[p]);zapParameters.tempo=timing.tempo;const auto zapEngine=juce::roundToInt(value("slider199"));if(zapEngine!=lastZapEngine){zap.reset();lastZapEngine=zapEngine;}const auto zapRoute=juce::jlimit(0,getBusCount(false)-1,juce::roundToInt(value("routeZap")));
    std::array<int,lr608::slotCount> slotEngines{},slotNotes{};
    for(int slot=0;slot<lr608::slotCount;++slot){slotEngines[slot]=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));slotNotes[slot]=juce::jlimit(0,127,juce::roundToInt(parameters.getRawParameterValue(lr608::slotNoteId(slot))->load()));}

    // Every instrument now owns an explicit route.  The old Stereo/
    // Multichannel switch remains in state only for legacy preset loading.
    getBusBuffer (buffer, false, 0).clear();
    for (int bus = 1; bus < getBusCount (false); ++bus)
        getBusBuffer (buffer, false, bus).clear();

    // Keep the original MIDI stream intact while dispatching synthesis at the
    // event's actual offset. Voice renderers are connected to this callback as
    // each authoritative JSFX family is ported.
    auto triggerEngine = [&] (int selectedEngine, int velocity)
    {
        if(selectedEngine==1){snare1.trigger(snareEngines[0],velocity,snareParameters[0]);return;}
        if(selectedEngine==2){snare2.trigger(snareEngines[1],velocity,snareParameters[1]);return;}
        if(selectedEngine==3){clap.trigger(clapEngine,velocity,clapParameters);return;}
        if(selectedEngine==4){rim.trigger(rimEngine,velocity,rimParameters);return;}
        if(selectedEngine==5){lowTom.trigger(tomEngine,velocity,tomParameters[0]);return;}
        if(selectedEngine==6){midTom.trigger(tomEngine,velocity,tomParameters[1]);return;}
        if(selectedEngine==7){highTom.trigger(tomEngine,velocity,tomParameters[2]);return;}
        if(selectedEngine==8){hiHat.trigger(hatEngine,false,velocity,hatParameters);return;}
        if(selectedEngine==9){hiHat.trigger(hatEngine,true,velocity,hatParameters);return;}
        if(selectedEngine==10){crash.trigger(cymbalEngine,velocity,cymbalParameters);return;}
        if(selectedEngine==11){ride.trigger(cymbalEngine,velocity,cymbalParameters);return;}
        if(selectedEngine==12){maracas.trigger(maracasEngine,velocity,maracasParameters);return;}
        if(selectedEngine==13){cowbell.trigger(cowbellEngine,velocity,cowbellParameters);return;}
        if(selectedEngine==14){zap.trigger(zapEngine,velocity,zapParameters);return;}
        if (kickEngine == 0)
        {
            kickOthers.reset();
            kick808.trigger (velocity, kick);
        }
        else
        {
            kick808.reset();
            kickOthers.trigger (kickEngine, velocity, kick);
        }
    };
    auto triggerVoice = [&] (lr608::DrumTrigger hit)
    {
        for(int slot=0;slot<lr608::slotCount;++slot)
            if(slotNotes[slot]==hit.note)triggerEngine(slotEngines[slot],hit.velocity);
    };
    auto iterator = midi.findNextSamplePosition (0);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        while (iterator != midi.cend() && (*iterator).samplePosition == sample)
        {
            const auto message = (*iterator).getMessage();
            const auto* bytes = message.getRawData();
            if (message.getRawDataSize() >= 3)
                timingEngine.handleMidi (bytes[0], bytes[1], bytes[2], triggerVoice);
            ++iterator;
        }
        timingEngine.tick (triggerVoice);
        const auto kickSample = kick808.render (kick, timing.tempo) + kickOthers.render (kick, timing.tempo);
        const auto snare1Sample = snare1.render (snareParameters[0]);
        const auto snare2Sample = snare2.render (snareParameters[1]);
        const auto clapSample = clap.render (clapParameters);
        const auto rimSample = rim.render (rimParameters);
        const auto lowTomSample=lowTom.render(tomParameters[0]),midTomSample=midTom.render(tomParameters[1]),highTomSample=highTom.render(tomParameters[2]);
        const auto hatSample=hiHat.render(hatParameters),crashSample=crash.render(cymbalParameters),rideSample=ride.render(cymbalParameters),maracasSample=maracas.render(maracasParameters),cowbellSample=cowbell.render(cowbellParameters),zapSample=zap.render(zapParameters);
        std::array<lr608::StereoSample, lr608::OutputStage::stemCount> stems {};
        stems[0] = { kickSample, kickSample };
        stems[1] = { snare1Sample, snare1Sample };
        stems[2] = { snare2Sample, snare2Sample };
        stems[3] = { clapSample.left, clapSample.right };
        stems[4] = { rimSample.left, rimSample.right };
        stems[8] = { lowTomSample.left, lowTomSample.right };
        stems[9] = { midTomSample.left, midTomSample.right };
        stems[10] = { highTomSample.left, highTomSample.right };
        stems[5]={hatSample,hatSample};stems[6]={crashSample,crashSample};stems[7]={rideSample,rideSample};
        stems[13]={maracasSample,maracasSample};
        stems[12]={cowbellSample,cowbellSample};
        stems[11]={zapSample,zapSample};
        outputStage.process (stems, true, value ("slider256"),
                             juce::roundToInt (value ("slider199")));
        if ((stems[0].left != 0.0 || stems[0].right != 0.0)
            && getBus (false, kickRoute)->isEnabled())
        {
            auto destination = getBusBuffer (buffer, false, kickRoute);
            destination.addSample (0, sample, static_cast<float> (stems[0].left));
            destination.addSample (1, sample, static_cast<float> (stems[0].right));
        }
        for (int slot = 0; slot < 2; ++slot)
        {
            const auto& stem = stems[slot + 1];
            if ((stem.left != 0.0 || stem.right != 0.0) && getBus(false, snareRoutes[slot])->isEnabled())
            {
                auto destination = getBusBuffer(buffer, false, snareRoutes[slot]);
                destination.addSample(0, sample, static_cast<float>(stem.left));
                destination.addSample(1, sample, static_cast<float>(stem.right));
            }
        }
        const int extraRoutes[]{clapRoute,rimRoute};
        for(int stemIndex=3;stemIndex<=4;++stemIndex)
        {
            const auto& stem=stems[stemIndex]; const auto route=extraRoutes[stemIndex-3];
            if((stem.left!=0||stem.right!=0)&&getBus(false,route)->isEnabled())
            {
                auto destination=getBusBuffer(buffer,false,route);
                destination.addSample(0,sample,static_cast<float>(stem.left));
                destination.addSample(1,sample,static_cast<float>(stem.right));
            }
        }
        for(int tom=0;tom<3;++tom){const auto& stem=stems[tom+8];if((stem.left!=0||stem.right!=0)&&getBus(false,tomRoutes[tom])->isEnabled()){auto destination=getBusBuffer(buffer,false,tomRoutes[tom]);destination.addSample(0,sample,static_cast<float>(stem.left));destination.addSample(1,sample,static_cast<float>(stem.right));}}
        for(int cym=0;cym<3;++cym){const auto&stem=stems[cym+5];if((stem.left!=0||stem.right!=0)&&getBus(false,cymRoutes[cym])->isEnabled()){auto destination=getBusBuffer(buffer,false,cymRoutes[cym]);destination.addSample(0,sample,static_cast<float>(stem.left));destination.addSample(1,sample,static_cast<float>(stem.right));}}
        {const auto&stem=stems[13];if((stem.left!=0||stem.right!=0)&&getBus(false,maracasRoute)->isEnabled()){auto destination=getBusBuffer(buffer,false,maracasRoute);destination.addSample(0,sample,static_cast<float>(stem.left));destination.addSample(1,sample,static_cast<float>(stem.right));}}
        {const auto&stem=stems[12];if((stem.left!=0||stem.right!=0)&&getBus(false,cowbellRoute)->isEnabled()){auto destination=getBusBuffer(buffer,false,cowbellRoute);destination.addSample(0,sample,static_cast<float>(stem.left));destination.addSample(1,sample,static_cast<float>(stem.right));}}
        {const auto&stem=stems[11];if((stem.left!=0||stem.right!=0)&&getBus(false,zapRoute)->isEnabled()){auto destination=getBusBuffer(buffer,false,zapRoute);destination.addSample(0,sample,static_cast<float>(stem.left));destination.addSample(1,sample,static_cast<float>(stem.right));}}
    }
#endif
}

void LR608AudioProcessor::storeSlotStates()
{
    captureSlotFromProxy(currentProxySlot.load());
    if(auto old=parameters.state.getChildWithName("SlotStates");old.isValid())parameters.state.removeChild(old,nullptr);
    juce::ValueTree slots("SlotStates");
    for(int slot=0;slot<lr608::slotCount;++slot)
    {
        juce::ValueTree node("Slot");node.setProperty("index",slot,nullptr);node.setProperty("grid",slotGrid[slot].load(),nullptr);node.setProperty("output",slotOutputs[slot].load(),nullptr);node.setProperty("name",slotNames[std::size_t(slot)],nullptr);for(const auto*id:universalSlotParameterIds)node.setProperty(id,slotValues[slot][catalogIndex(id)].load(),nullptr);
        const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));if(lr608::isOffEngine(engine)){slots.addChild(node,-1,nullptr);continue;}const auto&page=lr608::generated::pages[lr608::slotEngines[engine].page];
        for(std::size_t item=0;item<page.parameterCount;++item)if(!lr608::isEngineSelectorId(page.parameterIds[item]))if(const auto p=catalogIndex(page.parameterIds[item]);p>=0)node.setProperty(juce::Identifier(page.parameterIds[item]),slotValues[slot][p].load(),nullptr);
        slots.addChild(node,-1,nullptr);
    }
    parameters.state.addChild(slots,-1,nullptr);
}

void LR608AudioProcessor::restoreSlotStates()
{
    for(auto& delay:slotDelays)delay.reset();
    const auto legacyAccentThreshold=parameters.getRawParameterValue("slider250")->load(),legacyAccentCharacter=parameters.getRawParameterValue("slider251")->load();
    for(int slot=0;slot<lr608::slotCount;++slot){const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));for(int p=0;p<lr608::slotParameterValueCount;++p)slotValues[slot][p].store(engineDefaults[engine][p]);slotGrid[slot].store(0);slotOutputs[slot].store(0);slotNames[std::size_t(slot)].clear();}
    if(const auto slots=parameters.state.getChildWithName("SlotStates");slots.isValid())for(int child=0;child<slots.getNumChildren();++child){const auto node=slots.getChild(child);const auto slot=int(node.getProperty("index",-1));if(!juce::isPositiveAndBelow(slot,lr608::slotCount))continue;slotGrid[slot].store(juce::jmax(0,int(node.getProperty("grid",0))));slotNames[std::size_t(slot)]=node.getProperty("name").toString().trim().substring(0,48);const auto engine=juce::jlimit(0,lr608::slotEngineCount-1,juce::roundToInt(parameters.getRawParameterValue(lr608::slotEngineId(slot))->load()));int legacyOutput=0;if(!lr608::isOffEngine(engine)){const auto routeName=juce::Identifier(lr608::generated::parameters[lr608::slotEngines[engine].routeParameterIndex].id);legacyOutput=int(node.getProperty(routeName,0));}const auto output=node.hasProperty("output")?int(node.getProperty("output")):legacyOutput;slotOutputs[slot].store(juce::jlimit(0,lr608::OutputStage::stemCount-1,output));if(!node.hasProperty("slider250"))slotValues[slot][catalogIndex("slider250")].store(legacyAccentThreshold);if(!node.hasProperty("slider251"))slotValues[slot][catalogIndex("slider251")].store(legacyAccentCharacter);for(int property=0;property<node.getNumProperties();++property){const auto name=node.getPropertyName(property);if(name==juce::Identifier("index")||name==juce::Identifier("grid")||name==juce::Identifier("output")||name==juce::Identifier("name"))continue;if(const auto p=catalogIndex(name.toString());p>=0)slotValues[slot][p].store(float(node.getProperty(name)));}}
    refreshSlotTriggerCache();
    const auto selected=juce::jlimit(0,lr608::slotCount-1,int(parameters.state.getProperty("uiSelectedSlot",0)));loadSlotToProxy(selected);
}

void LR608AudioProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    storeSlotStates();
    auto state = parameters.copyState();
    state.setProperty ("stateFormat", 1, nullptr);
    state.setProperty ("slotEngineArchitectureVersion", slotEngineArchitectureVersion, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destination);
}

void LR608AudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (migrateSlotEngineArchitecture (juce::ValueTree::fromXml (*xml)));
            kickEngineBanks.resetFromCurrentState();
            snareEngineBanks.resetFromCurrentState();
            clapRimEngineBanks.resetFromCurrentState();
            tomEngineBanks.resetFromCurrentState();
            hatCymbalEngineBanks.resetFromCurrentState();
            maracasEngineBanks.resetFromCurrentState();
            cowbellEngineBanks.resetFromCurrentState();
            zapEngineBanks.resetFromCurrentState();
            restoreSlotStates();
        }
}

juce::AudioProcessorEditor* LR608AudioProcessor::createEditor()
{
    return new LR608AudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LR608AudioProcessor();
}
