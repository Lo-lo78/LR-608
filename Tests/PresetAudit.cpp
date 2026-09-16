#include "PluginProcessor.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

int main (int argc, char** argv)
{
    const auto count = lr608::PresetManager::getEmbeddedFactoryPresetCount();
    if (count != 14)
    {
        std::cerr << "Expected 14 embedded presets, got " << count << '\n';
        return EXIT_FAILURE;
    }
    const auto installDocuments = argc > 1
                               && juce::String (argv[1]) == "--install-documents";
    auto root = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("LR-608");
    if (! installDocuments)
    {
        root = juce::File::getCurrentWorkingDirectory().getChildFile ("preset-audit-output");
        // The audit owns this build-only directory. Remove leftovers from an
        // interrupted/failed run so nested Factory copies cannot affect counts.
        if (root.exists())
            root.deleteRecursively();
    }
    LR608AudioProcessor processor (root);
    const auto factoryEntries=processor.presetManager.listDirectory(root.getChildFile("Factory"));
    if(factoryEntries.empty()||factoryEntries.front().isDirectory||factoryEntries.front().name!="Init")
    {std::cerr<<"Init is not the first Factory browser preset\n";return EXIT_FAILURE;}
    const auto defaultPreset=processor.parameters.state.getProperty("currentPresetRelativePath").toString().replaceCharacter('\\','/');
    if(!defaultPreset.endsWith("Factory/001 - Init.LR608"))
    {std::cerr<<"Init was not loaded as the processor default\n";return EXIT_FAILURE;}
    if (processor.getBusCount (false) != 32 || processor.getTotalNumOutputChannels() != 64)
    {
        std::cerr << "Expected 32 enabled stereo output buses / 64 channels\n";
        return EXIT_FAILURE;
    }
    constexpr const char* routeIds[] = {
        "routeKick", "routeSnare1", "routeSnare2", "routeClap", "routeRim",
        "routeLowTom", "routeMidTom", "routeHighTom", "routeHiHat", "routeCrash",
        "routeRide", "routeMaracas", "routeCowbell", "routeZap"
    };
    for (const auto* id : routeIds)
    {
        const auto* parameter = processor.parameters.getParameter (id);
        if (parameter == nullptr || parameter->getNumSteps() != 15)
        {
            std::cerr << "Invalid routing parameter " << id << '\n';
            return EXIT_FAILURE;
        }
    }
    for(int slot=0;slot<lr608::slotCount;++slot)
        if(processor.parameters.getParameter(lr608::slotEngineId(slot))==nullptr||processor.parameters.getParameter(lr608::slotNoteId(slot))==nullptr)
        {std::cerr<<"Missing slot parameter "<<slot+1<<'\n';return EXIT_FAILURE;}
if(juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(0))->load())!=0||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotNoteId(0))->load())!=36||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(14))->load())!=78||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotNoteId(14))->load())!=52)
    {std::cerr<<"Slot factory layout failed\n";return EXIT_FAILURE;}
    processor.setSlotOutput(2,9);processor.setSlotEngine(2,66);if(processor.getSlotOutput(2)!=9){std::cerr<<"Engine change modified slot Output\n";return EXIT_FAILURE;}processor.setSlotEngine(2,7);processor.setSlotOutput(2,0);
    auto setPlain = [&] (const char* id, float plain)
    {
        auto* parameter = processor.parameters.getParameter (id);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plain));
    };
    {auto*e=processor.parameters.getParameter(lr608::slotEngineId(31));auto*n=processor.parameters.getParameter(lr608::slotNoteId(31));e->setValueNotifyingHost(e->convertTo0to1(14));n->setValueNotifyingHost(n->convertTo0to1(100));processor.setSlotOutput(31,31);processor.parameters.state.setProperty("uiSelectedSlot",31,nullptr);processor.parameters.state.setProperty("uiSelectedColumn",3,nullptr);processor.parameters.state.setProperty("uiInGrid",false,nullptr);juce::MemoryBlock saved;processor.getStateInformation(saved);auto restored=std::make_unique<LR608AudioProcessor>(root);restored->setStateInformation(saved.getData(),int(saved.getSize()));if(juce::roundToInt(restored->parameters.getRawParameterValue(lr608::slotEngineId(31))->load())!=14||juce::roundToInt(restored->parameters.getRawParameterValue(lr608::slotNoteId(31))->load())!=100||restored->getSlotOutput(31)!=31||int(restored->parameters.state.getProperty("uiSelectedSlot",-1))!=31||int(restored->parameters.state.getProperty("uiSelectedColumn",-1))!=3){std::cerr<<"Slot/UI position state recall failed\n";return EXIT_FAILURE;}}
    // Init is now authoritative for every currently selected engine. The
    // remaining bank checks verify untouched engines and edit retention.
    setPlain("slider064",9.25f); processor.tomEngineBanks.captureCurrent();
    setPlain("slider219",1); processor.tomEngineBanks.switchTo(1);
    if(std::abs(processor.parameters.getRawParameterValue("slider064")->load()-13.f)>.001f
       || std::abs(processor.parameters.getRawParameterValue("slider176")->load()-7.f)>.001f)
    { std::cerr<<"Tom Simmons factory Init failed\n"; return EXIT_FAILURE; }
    setPlain("slider064",14.5f); processor.tomEngineBanks.captureCurrent();
    setPlain("slider219",0); processor.tomEngineBanks.switchTo(0);
    setPlain("slider005",.83f);processor.hatCymbalEngineBanks.captureHat();setPlain("slider217",4);processor.hatCymbalEngineBanks.switchHat(4);
    if(std::abs(processor.parameters.getRawParameterValue("slider005")->load()-1.f)>.001f){std::cerr<<"HiHat factory Init failed\n";return EXIT_FAILURE;}
    setPlain("slider217",0);processor.hatCymbalEngineBanks.switchHat(0);if(std::abs(processor.parameters.getRawParameterValue("slider005")->load()-.83f)>.001f){std::cerr<<"HiHat bank retention failed\n";return EXIT_FAILURE;}
    setPlain("slider009",3.8f);processor.hatCymbalEngineBanks.captureCymbal();setPlain("slider214",2);processor.hatCymbalEngineBanks.switchCymbal(2);
    if(std::abs(processor.parameters.getRawParameterValue("slider009")->load()-2.f)>.001f){std::cerr<<"Cymbal factory Init failed\n";return EXIT_FAILURE;}
    setPlain("slider214",1);processor.hatCymbalEngineBanks.switchCymbal(1);if(std::abs(processor.parameters.getRawParameterValue("slider009")->load()-3.8f)>.001f){std::cerr<<"Cymbal bank retention failed\n";return EXIT_FAILURE;}
    setPlain("slider168",5.5f);processor.maracasEngineBanks.captureCurrent();setPlain("slider216",2);processor.maracasEngineBanks.switchTo(2);
    if(std::abs(processor.parameters.getRawParameterValue("slider168")->load()-4.f)>.001f||std::abs(processor.parameters.getRawParameterValue("slider170")->load()-.18f)>.001f){std::cerr<<"Maracas factory Init failed\n";return EXIT_FAILURE;}
    setPlain("slider216",3);processor.maracasEngineBanks.switchTo(3);if(std::abs(processor.parameters.getRawParameterValue("slider168")->load()-5.5f)>.001f){std::cerr<<"Maracas bank retention failed\n";return EXIT_FAILURE;}
    setPlain("slider080",5.5f);processor.cowbellEngineBanks.captureCurrent();setPlain("slider215",5);processor.cowbellEngineBanks.switchTo(5);if(std::abs(processor.parameters.getRawParameterValue("slider080")->load()-5.f)>.001f||std::abs(processor.parameters.getRawParameterValue("slider082")->load()-2.75f)>.001f){std::cerr<<"Cowbell factory Init failed\n";return EXIT_FAILURE;}setPlain("slider215",2);processor.cowbellEngineBanks.switchTo(2);if(std::abs(processor.parameters.getRawParameterValue("slider080")->load()-5.5f)>.001f){std::cerr<<"Cowbell bank retention failed\n";return EXIT_FAILURE;}
    setPlain("slider072",1.75f);processor.zapEngineBanks.captureCurrent();setPlain("slider199",8);processor.zapEngineBanks.switchTo(8);if(std::abs(processor.parameters.getRawParameterValue("slider072")->load()-2.f)>.001f||std::abs(processor.parameters.getRawParameterValue("slider074")->load()-1.55f)>.001f){std::cerr<<"Zap factory Init failed\n";return EXIT_FAILURE;}setPlain("slider199",0);processor.zapEngineBanks.switchTo(0);if(std::abs(processor.parameters.getRawParameterValue("slider072")->load()-1.75f)>.001f){std::cerr<<"Zap bank retention failed\n";return EXIT_FAILURE;}
    if(std::abs(processor.parameters.getRawParameterValue("slider064")->load()-9.25f)>.001f)
    { std::cerr<<"Tom 808 user bank retention failed\n"; return EXIT_FAILURE; }
    setPlain("slider219",1); processor.tomEngineBanks.switchTo(1);
    if(std::abs(processor.parameters.getRawParameterValue("slider064")->load()-14.5f)>.001f)
    { std::cerr<<"Tom Simmons user bank retention failed\n"; return EXIT_FAILURE; }
    setPlain("slider219",0); processor.tomEngineBanks.switchTo(0);
    setPlain ("slider020", 17.25f);
    processor.snareEngineBanks.captureCurrent (0);
    setPlain ("slider221", 9.75f);
    processor.snareEngineBanks.captureCurrent (1);
    if (! processor.snareEngineBanks.switchTo (0, 1)
        || ! processor.snareEngineBanks.switchTo (1, 2))
    {
        std::cerr << "Snare bank switch failed\n";
        return EXIT_FAILURE;
    }
    if (std::abs (processor.parameters.getRawParameterValue ("slider020")->load() - 12.45f) > 0.001f
        || std::abs (processor.parameters.getRawParameterValue ("slider221")->load() - 43.0f) > 0.001f)
    {
        std::cerr << "Snare factory Init failed\n";
        return EXIT_FAILURE;
    }
    if(!processor.snareEngineBanks.switchTo(0,8)||!processor.snareEngineBanks.switchTo(1,8)
       ||std::abs(processor.parameters.getRawParameterValue("slider020")->load()-18.0f)>.001f
       ||std::abs(processor.parameters.getRawParameterValue("slider023")->load())>.001f
       ||std::abs(processor.parameters.getRawParameterValue("slider137")->load()-.52f)>.001f
       ||std::abs(processor.parameters.getRawParameterValue("slider221")->load()-18.0f)>.001f)
    {
        std::cerr<<"Snare Linn factory Init failed\n";
        return EXIT_FAILURE;
    }
    processor.snareEngineBanks.switchTo (0, 0);
    processor.snareEngineBanks.switchTo (1, 0);
    if (std::abs (processor.parameters.getRawParameterValue ("slider020")->load() - 17.25f) > 0.001f
        || std::abs (processor.parameters.getRawParameterValue ("slider221")->load() - 9.75f) > 0.001f)
    {
        std::cerr << "Snare user bank retention failed\n";
        return EXIT_FAILURE;
    }
    setPlain ("slider011", 12.0f);
    processor.kickEngineBanks.captureCurrent();
    setPlain ("slider247", 1.0f);
    processor.kickEngineBanks.switchTo (1);
    if (std::abs (processor.parameters.getRawParameterValue ("slider011")->load() - 20.37f) > 0.001f
        || std::abs (processor.parameters.getRawParameterValue ("slider102")->load() - (-2.0f)) > 0.001f)
    {
        std::cerr << "Simmons factory bank was not recalled\n";
        return EXIT_FAILURE;
    }
    setPlain ("slider011", 21.0f);
    processor.kickEngineBanks.captureCurrent();
    setPlain ("slider247", 0.0f);
    processor.kickEngineBanks.switchTo (0);
    if (processor.kickEngineBanks.switchTo (0))
    {
        std::cerr << "Same-engine selection must be a no-op\n";
        return EXIT_FAILURE;
    }
    for (int pass = 0; pass < 100; ++pass)
    {
        const auto engine = pass % 7;
        setPlain ("slider247", static_cast<float> (engine));
        processor.kickEngineBanks.switchTo (engine);
    }
    setPlain ("slider247", 0.0f);
    processor.kickEngineBanks.switchTo (0);
    if (std::abs (processor.parameters.getRawParameterValue ("slider011")->load() - 12.0f) > 0.001f)
    {
        std::cerr << "Edited 808 bank was not retained\n";
        return EXIT_FAILURE;
    }
    setPlain ("slider247", 1.0f);
    processor.kickEngineBanks.switchTo (1);
    if (std::abs (processor.parameters.getRawParameterValue ("slider011")->load() - 21.0f) > 0.001f)
    {
        std::cerr << "Edited Simmons bank was not retained\n";
        return EXIT_FAILURE;
    }
    setPlain ("slider247", 0.0f);
    processor.kickEngineBanks.switchTo (0);
    processor.parameters.getParameter ("routeKick")->setValueNotifyingHost (2.0f / 14.0f);
    processor.parameters.getParameter ("routeSnare1")->setValueNotifyingHost (2.0f / 14.0f);
    processor.setSlotOutput(0,2);processor.setSlotOutput(1,2);
    if (processor.parameters.getRawParameterValue ("routeKick")->load() != 2.0f
        || processor.parameters.getRawParameterValue ("routeSnare1")->load() != 2.0f)
    {
        std::cerr << "Routing parameters cannot share an output independently\n";
        return EXIT_FAILURE;
    }
    processor.setRateAndBufferSizeDetails (48000.0, 512);
    processor.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> audio (64, 512);
    audio.clear();
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 36, (juce::uint8) 127), 32);
    processor.processBlock (audio, midi);
    double kickEnergy = 0.0, mainEnergy = 0.0, earlyEnergy = 0.0;
    for (int sample = 0; sample < 512; ++sample)
    {
        kickEnergy += std::abs (audio.getSample (4, sample)); // bus 2 = Output 5/6
        mainEnergy += std::abs (audio.getSample (0, sample));
        if (sample < 32) earlyEnergy += std::abs (audio.getSample (4, sample));
    }
    if (kickEnergy <= 0.01 || mainEnergy != 0.0 || earlyEnergy != 0.0)
    {
        std::cerr << "Kick 808 render/routing/sample-offset audit failed\n";
        return EXIT_FAILURE;
    }
    audio.clear(); midi.clear(); processor.loadSlotToProxy(5);processor.setSlotOutput(5,4); midi.addEvent(juce::MidiMessage::noteOn(1,41,(juce::uint8)127),16); processor.processBlock(audio,midi);
    double tomEnergy=0,tomEarly=0;for(int sample=0;sample<512;++sample){tomEnergy+=std::abs(audio.getSample(8,sample))+std::abs(audio.getSample(9,sample));if(sample<16)tomEarly+=std::abs(audio.getSample(8,sample))+std::abs(audio.getSample(9,sample));}
    if(tomEnergy<=.01||tomEarly!=0){std::cerr<<"Low Tom render/routing/sample-offset audit failed\n";return EXIT_FAILURE;}
    audio.clear(); midi.clear(); processor.loadSlotToProxy(6);processor.setSlotOutput(6,5); midi.addEvent(juce::MidiMessage::noteOn(1,45,(juce::uint8)127),24); processor.processBlock(audio,midi);
    double midTomEnergy=0,midTomWrongBus=0;for(int sample=0;sample<512;++sample){midTomEnergy+=std::abs(audio.getSample(10,sample))+std::abs(audio.getSample(11,sample));midTomWrongBus+=std::abs(audio.getSample(12,sample))+std::abs(audio.getSample(13,sample));}
    if(midTomEnergy<=.01||midTomWrongBus!=0){std::cerr<<"Mid Tom independent output audit failed\n";return EXIT_FAILURE;}
    audio.clear(); midi.clear(); processor.loadSlotToProxy(7);processor.setSlotOutput(7,6); midi.addEvent(juce::MidiMessage::noteOn(1,48,(juce::uint8)127),24); processor.processBlock(audio,midi);
    double highTomEnergy=0;for(int sample=0;sample<512;++sample)highTomEnergy+=std::abs(audio.getSample(12,sample))+std::abs(audio.getSample(13,sample));
    if(highTomEnergy<=.01){std::cerr<<"High Tom independent output audit failed\n";return EXIT_FAILURE;}
    audio.clear();midi.clear();processor.setSlotOutput(8,7);processor.setSlotOutput(10,8);processor.setSlotOutput(11,9);midi.addEvent(juce::MidiMessage::noteOn(1,42,(juce::uint8)127),0);midi.addEvent(juce::MidiMessage::noteOn(1,49,(juce::uint8)127),32);midi.addEvent(juce::MidiMessage::noteOn(1,51,(juce::uint8)127),64);processor.processBlock(audio,midi);
    double hatE=0,crashE=0,rideE=0;for(int sample=0;sample<512;++sample){hatE+=std::abs(audio.getSample(14,sample));crashE+=std::abs(audio.getSample(16,sample));rideE+=std::abs(audio.getSample(18,sample));}if(hatE<=.01||crashE<=.01||rideE<=.01){std::cerr<<"HiHat/Crash/Ride independent routing audit failed\n";return EXIT_FAILURE;}
    audio.clear();midi.clear();processor.setSlotOutput(12,10);midi.addEvent(juce::MidiMessage::noteOn(1,58,(juce::uint8)127),48);processor.processBlock(audio,midi);double maracasEnergy=0,maracasEarly=0;for(int sample=0;sample<512;++sample){maracasEnergy+=std::abs(audio.getSample(20,sample))+std::abs(audio.getSample(21,sample));if(sample<48)maracasEarly+=std::abs(audio.getSample(20,sample))+std::abs(audio.getSample(21,sample));}if(maracasEnergy<=.01||maracasEarly!=0){std::cerr<<"Maracas render/routing/sample-offset audit failed\n";return EXIT_FAILURE;}
    audio.clear();midi.clear();processor.setSlotOutput(13,11);midi.addEvent(juce::MidiMessage::noteOn(1,56,(juce::uint8)127),40);processor.processBlock(audio,midi);double cowEnergy=0,cowEarly=0;for(int sample=0;sample<512;++sample){cowEnergy+=std::abs(audio.getSample(22,sample))+std::abs(audio.getSample(23,sample));if(sample<40)cowEarly+=std::abs(audio.getSample(22,sample))+std::abs(audio.getSample(23,sample));}if(cowEnergy<=.01||cowEarly!=0){std::cerr<<"Cowbell render/routing/sample-offset audit failed\n";return EXIT_FAILURE;}
    audio.clear();midi.clear();processor.setSlotOutput(14,12);midi.addEvent(juce::MidiMessage::noteOn(1,52,(juce::uint8)127),36);processor.processBlock(audio,midi);double zapEnergy=0,zapEarly=0;for(int sample=0;sample<512;++sample){zapEnergy+=std::abs(audio.getSample(24,sample))+std::abs(audio.getSample(25,sample));if(sample<36)zapEarly+=std::abs(audio.getSample(24,sample))+std::abs(audio.getSample(25,sample));}if(zapEnergy<=.01||zapEarly!=0){std::cerr<<"Zap render/routing/sample-offset audit failed\n";return EXIT_FAILURE;}
    {auto layered=std::make_unique<LR608AudioProcessor>(root);auto set=[&](const juce::String&id,float plain){auto*p=layered->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};layered->setSlotOutput(0,2);set(lr608::slotNoteId(0),60);layered->setSlotEngine(1,74);layered->setSlotOutput(1,11);set(lr608::slotNoteId(1),60);if(juce::roundToInt(layered->parameters.getRawParameterValue(lr608::slotNoteId(1))->load())!=60){std::cerr<<"Engine change modified slot MIDI Note\n";return EXIT_FAILURE;}layered->setRateAndBufferSizeDetails(48000,512);layered->prepareToPlay(48000,512);juce::AudioBuffer<float>b(30,512);b.clear();juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)127),20);layered->processBlock(b,m);double ke=0,ce=0,early=0;for(int s=0;s<512;++s){ke+=std::abs(b.getSample(4,s));ce+=std::abs(b.getSample(22,s));if(s<20)early+=std::abs(b.getSample(4,s))+std::abs(b.getSample(22,s));}if(ke<=.01||ce<=.01||early!=0){std::cerr<<"Shared-note slot layering/remap audit failed\n";return EXIT_FAILURE;}}
    {auto panned=std::make_unique<LR608AudioProcessor>(root);auto set=[&](const juce::String&id,float plain){auto*p=panned->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};for(int slot=0;slot<lr608::slotCount;++slot)panned->setSlotEngine(slot,lr608::offEngineIndex);panned->setSlotEngine(0,0);set(lr608::slotNoteId(0),60);panned->loadSlotToProxy(0);set("slotPan",-1);panned->setRateAndBufferSizeDetails(48000,512);panned->prepareToPlay(48000,512);juce::AudioBuffer<float>b(30,512);juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)127),0);panned->processBlock(b,m);double left=0,right=0;for(int s=0;s<512;++s){left+=std::abs(b.getSample(0,s));right+=std::abs(b.getSample(1,s));}if(left<=.01||right>1.0e-5){std::cerr<<"Universal Pan hard-left audit failed\n";return EXIT_FAILURE;}panned->prepareToPlay(48000,512);set("slotPan",1);b.clear();m.clear();m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)127),0);panned->processBlock(b,m);left=right=0;for(int s=0;s<512;++s){left+=std::abs(b.getSample(0,s));right+=std::abs(b.getSample(1,s));}if(right<=.01||left>1.0e-5){std::cerr<<"Universal Pan hard-right audit failed\n";return EXIT_FAILURE;}}
    {auto filtered=std::make_unique<LR608AudioProcessor>(root);auto set=[&](const juce::String&id,float plain){auto*p=filtered->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};for(int slot=0;slot<lr608::slotCount;++slot)filtered->setSlotEngine(slot,lr608::offEngineIndex);filtered->setSlotEngine(0,0);set(lr608::slotNoteId(0),60);filtered->loadSlotToProxy(0);filtered->setRateAndBufferSizeDetails(48000,4096);filtered->prepareToPlay(48000,4096);juce::AudioBuffer<float>b(30,4096);juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)127),0);filtered->processBlock(b,m);double dry=0;for(int s=0;s<4096;++s)dry+=std::abs(b.getSample(0,s))+std::abs(b.getSample(1,s));set("slotHighPassCutoff",8000);set("slotHighPassResonance",2);filtered->prepareToPlay(48000,4096);b.clear();m.clear();m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)127),0);filtered->processBlock(b,m);double shaped=0;for(int s=0;s<4096;++s){const auto l=b.getSample(0,s),r=b.getSample(1,s);if(!std::isfinite(l)||!std::isfinite(r)){std::cerr<<"Per-Slot filter produced non-finite audio\n";return EXIT_FAILURE;}shaped+=std::abs(l)+std::abs(r);}if(dry<=.01||shaped>=dry*.6){std::cerr<<"Per-Slot 12 dB filter audit failed\n";return EXIT_FAILURE;}}
    {auto poly=std::make_unique<LR608AudioProcessor>(root);for(int slot=0;slot<lr608::slotCount;++slot){poly->setSlotEngine(slot,0);auto*n=poly->parameters.getParameter(lr608::slotNoteId(slot));n->setValueNotifyingHost(n->convertTo0to1(60));}poly->setRateAndBufferSizeDetails(48000,1);poly->prepareToPlay(48000,1);juce::AudioBuffer<float>b(30,1);juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)127),0);poly->processBlock(b,m);if(poly->getActiveVoiceCount()!=128||poly->getVoiceStealCount()!=0){std::cerr<<"128-voice allocation audit failed\n";return EXIT_FAILURE;}b.clear();m.clear();m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)127),0);poly->processBlock(b,m);if(poly->getActiveVoiceCount()!=128||poly->getVoiceStealCount()!=128){std::cerr<<"Predictable oldest-voice stealing audit failed\n";return EXIT_FAILURE;}}
    {
        auto capped=std::make_unique<LR608AudioProcessor>(root);
        auto set=[&](const juce::String&id,float plain){auto*p=capped->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};
        for(int slot=0;slot<lr608::slotCount;++slot){capped->setSlotEngine(slot,0);set(lr608::slotNoteId(slot),127);}
        for(int slot=0;slot<2;++slot)set(lr608::slotNoteId(slot),60);
        capped->loadSlotToProxy(0);set("slotVoiceOverlap",1);
        capped->loadSlotToProxy(1);
        if(juce::roundToInt(capped->parameters.getRawParameterValue("slotVoiceOverlap")->load())!=2){std::cerr<<"Voice Overlap default is not two\n";return EXIT_FAILURE;}
        capped->setRateAndBufferSizeDetails(48000,1);capped->prepareToPlay(48000,1);juce::AudioBuffer<float>b(30,1);
        for(int hit=0;hit<8;++hit){b.clear();juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)127),0);capped->processBlock(b,m);}
        if(capped->getActiveVoiceCountForMidiNote(60)!=3){std::cerr<<"Independent per-Slot Voice Overlap cap failed\n";return EXIT_FAILURE;}
        capped->loadSlotToProxy(1);set("slotVoiceOverlap",1);b.clear();juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(1,60,(juce::uint8)127),0);capped->processBlock(b,m);
        if(capped->getActiveVoiceCountForMidiNote(60)!=2){std::cerr<<"Live Voice Overlap reduction failed\n";return EXIT_FAILURE;}
    }
    {auto choke=std::make_unique<LR608AudioProcessor>(root);auto set=[&](const juce::String&id,float plain){auto*p=choke->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};set(lr608::slotNoteId(0),99);choke->setSlotEngine(1,55);set(lr608::slotNoteId(1),46);choke->setSlotEngine(2,55);set(lr608::slotNoteId(2),46);set(lr608::slotChokeTriggerId(0),121);set(lr608::slotChokeTargetId(0),47);set(lr608::slotChokeTriggerId(1),121);set(lr608::slotChokeTargetId(1),52);choke->setRateAndBufferSizeDetails(48000,256);choke->prepareToPlay(48000,256);juce::AudioBuffer<float>b(30,256);juce::MidiBuffer m;m.addEvent(juce::MidiMessage::noteOn(1,46,(juce::uint8)127),0);m.addEvent(juce::MidiMessage::noteOn(1,51,(juce::uint8)127),0);choke->processBlock(b,m);if(choke->getActiveVoiceCountForMidiNote(46)<2||choke->getActiveVoiceCountForMidiNote(51)<1){std::cerr<<"MIDI Choke target setup failed\n";return EXIT_FAILURE;}b.clear();m.clear();m.addEvent(juce::MidiMessage::noteOn(1,120,(juce::uint8)127),0);choke->processBlock(b,m);if(choke->getActiveVoiceCountForMidiNote(46)!=0||choke->getActiveVoiceCountForMidiNote(51)!=0){std::cerr<<"Multiple per-Slot MIDI Choke links failed\n";return EXIT_FAILURE;}set(lr608::slotChokeTriggerId(0),0);set(lr608::slotChokeTriggerId(1),0);b.clear();m.clear();m.addEvent(juce::MidiMessage::noteOn(1,46,(juce::uint8)127),0);choke->processBlock(b,m);b.clear();m.clear();m.addEvent(juce::MidiMessage::noteOn(1,120,(juce::uint8)127),0);choke->processBlock(b,m);if(choke->getActiveVoiceCountForMidiNote(46)<1){std::cerr<<"Disabled MIDI Choke link remained active\n";return EXIT_FAILURE;}}
    {auto accented=std::make_unique<LR608AudioProcessor>(root);auto set=[&](const juce::String&id,float plain){auto*p=accented->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};accented->setSlotEngine(0,0);accented->loadSlotToProxy(0);set("slider250",77);set("slider251",.45f);set("slotVoiceOverlap",1);set("slotLowPassCutoff",4200);set("slotLowPassResonance",1.35f);set("slotHighPassCutoff",75);set("slotHighPassResonance",.82f);accented->setSlotEngine(1,1);accented->loadSlotToProxy(1);set("slider250",121);set("slider251",2.35f);accented->loadSlotToProxy(0);if(std::abs(accented->parameters.getRawParameterValue("slider250")->load()-77)>.001f||std::abs(accented->parameters.getRawParameterValue("slider251")->load()-.45f)>.001f){std::cerr<<"Per-Slot Accent values leaked between elements\n";return EXIT_FAILURE;}const auto sound=accented->copySlotSound(0);accented->setSlotEngine(15,lr608::offEngineIndex);if(!accented->pasteSlotSound(15,sound)){std::cerr<<"Per-Slot Accent copy failed\n";return EXIT_FAILURE;}accented->loadSlotToProxy(15);if(std::abs(accented->parameters.getRawParameterValue("slider250")->load()-77)>.001f||std::abs(accented->parameters.getRawParameterValue("slider251")->load()-.45f)>.001f||juce::roundToInt(accented->parameters.getRawParameterValue("slotVoiceOverlap")->load())!=1||std::abs(accented->parameters.getRawParameterValue("slotLowPassCutoff")->load()-4200)>.001f||std::abs(accented->parameters.getRawParameterValue("slotLowPassResonance")->load()-1.35f)>.001f||std::abs(accented->parameters.getRawParameterValue("slotHighPassCutoff")->load()-75)>.001f||std::abs(accented->parameters.getRawParameterValue("slotHighPassResonance")->load()-.82f)>.001f){std::cerr<<"Copied sound lost per-Slot Accent, Voice Overlap or filter values\n";return EXIT_FAILURE;}}
    {const auto completeRoot=root.getChildFile("complete-state-audit-"+juce::Uuid().toString());auto complete=std::make_unique<LR608AudioProcessor>(completeRoot);auto set=[&](const juce::String&id,float plain){auto*p=complete->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};complete->setSlotEngine(73,83);complete->setSlotOutput(73,13);set(lr608::slotNoteId(73),99);complete->loadSlotToProxy(73);set("slider072",1.91f);set("slider253",37);complete->setSlotGridPosition(73,5);juce::File saved;if(complete->presetManager.savePreset("Complete Kit",completeRoot,saved).failed()||!saved.existsAsFile()){std::cerr<<"Complete preset save failed\n";return EXIT_FAILURE;}complete->setSlotEngine(73,0);complete->setSlotOutput(73,0);set(lr608::slotNoteId(73),1);set("slider253",0);juce::String loadedName;int loadedNumber=0;if(complete->presetManager.loadPreset(saved,loadedName,loadedNumber).failed()||juce::roundToInt(complete->parameters.getRawParameterValue(lr608::slotEngineId(73))->load())!=83||juce::roundToInt(complete->parameters.getRawParameterValue(lr608::slotNoteId(73))->load())!=99||complete->getSlotOutput(73)!=13||complete->getSlotGridPosition(73)!=5||std::abs(complete->parameters.getRawParameterValue("slider253")->load()-37)>0.001f){std::cerr<<"Complete 128-slot preset recall failed\n";return EXIT_FAILURE;}complete->loadSlotToProxy(73);if(std::abs(complete->parameters.getRawParameterValue("slider072")->load()-1.91f)>0.001f){std::cerr<<"Independent slot sound state was not recalled\n";return EXIT_FAILURE;}const auto entries=complete->presetManager.listDirectory(completeRoot);if(entries.size()<2||!entries.front().isDirectory||entries.front().name!="Factory"){std::cerr<<"Preset browser library layout failed\n";return EXIT_FAILURE;}const auto factoryInit=completeRoot.getChildFile("Factory").getChildFile("001 - Init.LR608");if(complete->presetManager.deletePreset(factoryInit).wasOk()||!factoryInit.existsAsFile()){std::cerr<<"Factory preset deletion was not protected\n";return EXIT_FAILURE;}if(complete->presetManager.deletePreset(saved).failed()||saved.existsAsFile()){std::cerr<<"User preset deletion failed\n";return EXIT_FAILURE;}complete.reset();completeRoot.deleteRecursively();}
    {const auto accentRoot=root.getChildFile("accent-state-audit-"+juce::Uuid().toString());auto savedProcessor=std::make_unique<LR608AudioProcessor>(accentRoot);auto set=[&](const juce::String&id,float plain){auto*p=savedProcessor->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};savedProcessor->loadSlotToProxy(0);set("slider250",84);set("slider251",3.25f);juce::File saved;if(savedProcessor->presetManager.savePreset("Accent State",accentRoot,saved).failed()){std::cerr<<"Per-Slot Accent preset save failed\n";return EXIT_FAILURE;}set("slider250",12);set("slider251",.1f);juce::String name;int number=0;if(savedProcessor->presetManager.loadPreset(saved,name,number).failed()){std::cerr<<"Per-Slot Accent preset load failed\n";return EXIT_FAILURE;}savedProcessor->loadSlotToProxy(0);if(std::abs(savedProcessor->parameters.getRawParameterValue("slider250")->load()-84)>.001f||std::abs(savedProcessor->parameters.getRawParameterValue("slider251")->load()-3.25f)>.001f){std::cerr<<"Per-Slot Accent preset recall failed\n";return EXIT_FAILURE;}savedProcessor.reset();accentRoot.deleteRecursively();}
    {const auto nameRoot=root.getChildFile("slot-name-audit-"+juce::Uuid().toString());auto named=std::make_unique<LR608AudioProcessor>(nameRoot);auto set=[&](const juce::String&id,float plain){auto*p=named->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};for(int slot=0;slot<lr608::slotCount;++slot)named->setSlotEngine(slot,lr608::offEngineIndex);named->setSlotEngine(2,0);set(lr608::slotNoteId(2),66);named->setSlotEngine(5,2);set(lr608::slotNoteId(5),66);if(named->findFirstSlotForMidiNote(66)!=2||named->findNextSlotForMidiNote(66,-1)!=2||named->findNextSlotForMidiNote(66,2)!=5||named->findNextSlotForMidiNote(66,5)!=2||named->getActiveSlotLayerNumber(66,2)!=1||named->getActiveSlotLayerNumber(66,5)!=2){std::cerr<<"Layered MIDI Note Slot cycle failed\n";return EXIT_FAILURE;}named->setSlotEngine(2,lr608::offEngineIndex);if(named->findFirstSlotForMidiNote(66)!=5||named->findNextSlotForMidiNote(66,2)!=5||named->getActiveSlotLayerNumber(66,5)!=1){std::cerr<<"MIDI Note navigation selected an Off Slot\n";return EXIT_FAILURE;}for(int slot=0;slot<8;++slot){named->setSlotEngine(slot,0);set(lr608::slotNoteId(slot),70);}named->setSlotEngine(8,0);set(lr608::slotNoteId(8),69);if(named->getActiveSlotCountForMidiNote(70)!=8||named->canAssignSlotToMidiNote(8,70)||named->findAssignableMidiNote(70,1,8)!=71||named->findAssignableMidiNote(70,-1,8)!=69||!named->canAssignSlotToMidiNote(0,70)){std::cerr<<"Eight-layer MIDI Note assignment limit failed\n";return EXIT_FAILURE;}named->setSlotName(19,"Acoustic layer");juce::File saved;if(named->presetManager.savePreset("Named Slot",nameRoot,saved).failed()){std::cerr<<"Named Slot preset save failed\n";return EXIT_FAILURE;}named->setSlotName(19,"Changed");juce::String name;int number=0;if(named->presetManager.loadPreset(saved,name,number).failed()||named->getSlotName(19)!="Acoustic layer"){std::cerr<<"Slot name preset recall failed\n";return EXIT_FAILURE;}named.reset();nameRoot.deleteRecursively();}
    {const auto chokeRoot=root.getChildFile("choke-state-audit-"+juce::Uuid().toString());auto savedProcessor=std::make_unique<LR608AudioProcessor>(chokeRoot);auto set=[&](const juce::String&id,float plain){auto*p=savedProcessor->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};set(lr608::slotChokeTriggerId(9),43);set(lr608::slotChokeTargetId(9),47);set(lr608::slotChokeTriggerId(19),43);set(lr608::slotChokeTargetId(19),52);juce::File saved;if(savedProcessor->presetManager.savePreset("Choke Matrix",chokeRoot,saved).failed()){std::cerr<<"MIDI Choke preset save failed\n";return EXIT_FAILURE;}set(lr608::slotChokeTriggerId(9),0);set(lr608::slotChokeTargetId(9),0);set(lr608::slotChokeTriggerId(19),0);set(lr608::slotChokeTargetId(19),0);juce::String name;int number=0;if(savedProcessor->presetManager.loadPreset(saved,name,number).failed()||juce::roundToInt(savedProcessor->parameters.getRawParameterValue(lr608::slotChokeTriggerId(9))->load())!=43||juce::roundToInt(savedProcessor->parameters.getRawParameterValue(lr608::slotChokeTargetId(9))->load())!=47||juce::roundToInt(savedProcessor->parameters.getRawParameterValue(lr608::slotChokeTriggerId(19))->load())!=43||juce::roundToInt(savedProcessor->parameters.getRawParameterValue(lr608::slotChokeTargetId(19))->load())!=52){std::cerr<<"MIDI Choke preset recall failed\n";return EXIT_FAILURE;}savedProcessor.reset();chokeRoot.deleteRecursively();}
    {const auto copyRoot=root.getChildFile("copy-audit-"+juce::Uuid().toString());auto copyProcessor=std::make_unique<LR608AudioProcessor>(copyRoot);auto set=[&](const juce::String&id,float plain){auto*p=copyProcessor->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};copyProcessor->setSlotEngine(0,1);copyProcessor->loadSlotToProxy(0);set("slider011",17.3f);const auto sound=copyProcessor->copySlotSound(0);copyProcessor->setSlotEngine(15,0);copyProcessor->setSlotOutput(15,6);set(lr608::slotNoteId(15),77);set(lr608::slotChokeTriggerId(15),121);set(lr608::slotChokeTargetId(15),48);if(!copyProcessor->pasteSlotSound(15,sound)){std::cerr<<"Compatible Slot sound paste failed\n";return EXIT_FAILURE;}copyProcessor->loadSlotToProxy(15);if(juce::roundToInt(copyProcessor->parameters.getRawParameterValue(lr608::slotEngineId(15))->load())!=1||std::abs(copyProcessor->parameters.getRawParameterValue("slider011")->load()-17.3f)>.001f||juce::roundToInt(copyProcessor->parameters.getRawParameterValue(lr608::slotNoteId(15))->load())!=77||copyProcessor->getSlotOutput(15)!=6||juce::roundToInt(copyProcessor->parameters.getRawParameterValue(lr608::slotChokeTriggerId(15))->load())!=121||juce::roundToInt(copyProcessor->parameters.getRawParameterValue(lr608::slotChokeTargetId(15))->load())!=48){std::cerr<<"Slot sound paste changed ownership values or lost sound values\n";return EXIT_FAILURE;}copyProcessor->setSlotEngine(1,8);if(copyProcessor->pasteSlotSound(1,sound)||juce::roundToInt(copyProcessor->parameters.getRawParameterValue(lr608::slotEngineId(1))->load())!=8){std::cerr<<"Cross-family Slot sound paste was not rejected\n";return EXIT_FAILURE;}copyProcessor.reset();copyRoot.deleteRecursively();}
    {
        const auto keyRoot=root.getChildFile("midi-key-copy-audit-"+juce::Uuid().toString());
        auto source=std::make_unique<LR608AudioProcessor>(keyRoot);
        const auto set=[&](const juce::String&id,float plain){auto*p=source->parameters.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(plain));};
        for(int slot=0;slot<lr608::slotCount;++slot)source->setSlotEngine(slot,lr608::offEngineIndex);
        source->setSlotEngine(0,1);set(lr608::slotNoteId(0),100);source->setSlotOutput(0,3);set(lr608::slotChokeTriggerId(0),43);set(lr608::slotChokeTargetId(0),47);
        source->loadSlotToProxy(0);set("slider011",19.25f);
        source->setSlotEngine(1,74);set(lr608::slotNoteId(1),100);source->setSlotOutput(1,12);
        int layers=0;const auto copied=source->copyMidiKeyToText(100,false,layers);
        if(copied.isEmpty()||layers!=2){std::cerr<<"MIDI key layered copy failed\n";return EXIT_FAILURE;}
        int pasted=0;if(source->pasteMidiKeyFromText(101,copied,pasted).failed()||pasted!=2){std::cerr<<"MIDI key paste into empty destination failed\n";return EXIT_FAILURE;}
        int destinationLayers=0;bool copiedKickValues=false;
        for(int slot=0;slot<lr608::slotCount;++slot)if(juce::roundToInt(source->parameters.getRawParameterValue(lr608::slotNoteId(slot))->load())==101&&juce::roundToInt(source->parameters.getRawParameterValue(lr608::slotEngineId(slot))->load())!=lr608::offEngineIndex)
        {++destinationLayers;if(juce::roundToInt(source->parameters.getRawParameterValue(lr608::slotEngineId(slot))->load())==1){source->loadSlotToProxy(slot);copiedKickValues=std::abs(source->parameters.getRawParameterValue("slider011")->load()-19.25f)<.001f&&source->getSlotOutput(slot)==3&&juce::roundToInt(source->parameters.getRawParameterValue(lr608::slotChokeTriggerId(slot))->load())==43&&juce::roundToInt(source->parameters.getRawParameterValue(lr608::slotChokeTargetId(slot))->load())==47;}}
        if(destinationLayers!=2||!copiedKickValues){std::cerr<<"MIDI key paste lost layer content\n";return EXIT_FAILURE;}
        for(int pass=0;pass<3;++pass){int added=0;if(source->pasteMidiKeyFromText(101,copied,added).failed()||added!=2){std::cerr<<"MIDI key paste did not add layers to an occupied destination\n";return EXIT_FAILURE;}}
        if(source->getActiveSlotCountForMidiNote(101)!=8){std::cerr<<"MIDI key additive paste lost existing layers\n";return EXIT_FAILURE;}
        int rejected=0;if(source->pasteMidiKeyFromText(101,copied,rejected).wasOk()||source->getActiveSlotCountForMidiNote(101)!=8){std::cerr<<"MIDI key paste exceeded the eight-layer limit\n";return EXIT_FAILURE;}
        auto firstDestination=source->findFirstSlotForMidiNote(101);
        if(firstDestination<0||!source->clearSlot(firstDestination)||source->getActiveSlotCountForMidiNote(101)!=7){std::cerr<<"Single MIDI layer Delete changed the wrong scope\n";return EXIT_FAILURE;}
        if(source->clearMidiKey(101)!=7){std::cerr<<"MIDI key Delete did not clear every remaining layer\n";return EXIT_FAILURE;}
        for(int slot=0;slot<lr608::slotCount;++slot)if(juce::roundToInt(source->parameters.getRawParameterValue(lr608::slotNoteId(slot))->load())==101&&juce::roundToInt(source->parameters.getRawParameterValue(lr608::slotEngineId(slot))->load())!=lr608::offEngineIndex){std::cerr<<"MIDI key Delete left an active layer\n";return EXIT_FAILURE;}
        int cutLayers=0;const auto cut=source->copyMidiKeyToText(100,true,cutLayers);if(cut.isEmpty()||cutLayers!=2){std::cerr<<"MIDI key cut failed\n";return EXIT_FAILURE;}
        for(int slot=0;slot<2;++slot)if(juce::roundToInt(source->parameters.getRawParameterValue(lr608::slotEngineId(slot))->load())!=lr608::offEngineIndex){std::cerr<<"MIDI key cut did not empty sources\n";return EXIT_FAILURE;}
        int moved=0;if(source->pasteMidiKeyFromText(102,cut,moved).failed()||moved!=2){std::cerr<<"MIDI key move paste failed\n";return EXIT_FAILURE;}
        source.reset();keyRoot.deleteRecursively();
    }
    const auto files = processor.presetManager.allPresetFiles();
    if (files.size() != 14)
    {
        std::cerr << "Expected 14 installed presets, got " << files.size() << '\n';
        return EXIT_FAILURE;
    }
    constexpr const char* migratedSnare1Ids[]{"slider020","slider021","slider022","slider023","slider024","slider029","slider116","slider117","slider026","slider025","slider027","slider028","slider019","slider092","slider094","slider096","slider098","slider108","slider139","slider132","slider133","slider134","slider135","slider136","slider137","slider156","slider157","slider158","slider159","slider160","slider161"};
    constexpr const char* legacySnare2Ids[]{"slider221","slider222","slider223","slider224","slider225","slider230","slider232","slider233","slider227","slider226","slider228","slider229","slider220","slider093","slider095","slider097","slider099","slider231","slider240","slider234","slider235","slider236","slider237","slider238","slider239","slider241","slider242","slider243","slider244","slider245","slider246"};
    for(const auto& factoryPreset:files)
    {
        juce::String factoryName;int factoryNumber=0;
        if(processor.presetManager.loadPreset(factoryPreset,factoryName,factoryNumber).failed()
           ||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTriggerId(8))->load())!=43
           ||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTargetId(8))->load())!=47
           ||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTriggerId(16))->load())!=45
           ||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTargetId(16))->load())!=47
           ||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(19))->load())!=lr608::defaultSlotEngine(19))
        {std::cerr<<"Factory preset missing JSFX hi-hat choke: "<<factoryPreset.getFileName()<<'\n';return EXIT_FAILURE;}
        const auto expectedSnareEngine=8+juce::roundToInt(processor.parameters.getRawParameterValue("slider198")->load());
        if(juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(2))->load())!=expectedSnareEngine)
        {std::cerr<<"Factory Snare 2 engine was not adapted: "<<factoryPreset.getFileName()<<'\n';return EXIT_FAILURE;}
        processor.loadSlotToProxy(2);
        for(std::size_t parameter=0;parameter<std::size(migratedSnare1Ids);++parameter)
            if(std::abs(processor.parameters.getRawParameterValue(migratedSnare1Ids[parameter])->load()-processor.parameters.getRawParameterValue(legacySnare2Ids[parameter])->load())>.001f)
            {std::cerr<<"Factory Snare 2 value was not adapted: "<<factoryPreset.getFileName()<<", parameter "<<parameter<<'\n';return EXIT_FAILURE;}
        const auto tomEngine=juce::roundToInt(processor.parameters.getRawParameterValue("slider219")->load());
        const auto hatEngine=juce::roundToInt(processor.parameters.getRawParameterValue("slider217")->load());
        const int expectedExtraEngines[]{30+tomEngine,45+hatEngine,35+tomEngine,40+tomEngine};
        for(int extra=0;extra<4;++extra)
            if(juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotEngineId(15+extra))->load())!=expectedExtraEngines[extra])
            {std::cerr<<"Factory duplicate instrument engine was not adapted: "<<factoryPreset.getFileName()<<", MIDI Note "<<(extra==0?43:extra==1?44:extra==2?47:50)<<'\n';return EXIT_FAILURE;}
        const int sourceSlots[]{5,8,6,7};
        for(int extra=0;extra<4;++extra)
        {
            const auto sourceSound=processor.copySlotSound(sourceSlots[extra]);
            const auto duplicateSound=processor.copySlotSound(15+extra);
            if(sourceSound.engine!=duplicateSound.engine||processor.getSlotOutput(sourceSlots[extra])!=processor.getSlotOutput(15+extra))
            {std::cerr<<"Factory duplicate instrument identity differs from its source: "<<factoryPreset.getFileName()<<'\n';return EXIT_FAILURE;}
            for(std::size_t value=0;value<sourceSound.values.size();++value)
                if(std::abs(sourceSound.values[value]-duplicateSound.values[value])>.0001f)
                {std::cerr<<"Factory duplicate instrument values differ from its source: "<<factoryPreset.getFileName()<<", value "<<value<<'\n';return EXIT_FAILURE;}
        }
    }
    {processor.setSlotEngine(15,83);auto*n=processor.parameters.getParameter(lr608::slotNoteId(15));n->setValueNotifyingHost(n->convertTo0to1(99));juce::String name;int number=0;if(processor.presetManager.loadPreset(files.front(),name,number).failed()){std::cerr<<"Legacy Factory kit migration failed\n";return EXIT_FAILURE;}constexpr int legacyNotes[]{36,38,40,39,37,41,45,48,42,46,49,51,58,56,52,43,44,47,50};for(int slot=0;slot<19;++slot)if(juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotNoteId(slot))->load())!=legacyNotes[slot]){std::cerr<<"Legacy JSFX MIDI map migration failed at slot "<<slot+1<<'\n';return EXIT_FAILURE;}if(juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTriggerId(8))->load())!=43||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTargetId(8))->load())!=47||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTriggerId(16))->load())!=45||juce::roundToInt(processor.parameters.getRawParameterValue(lr608::slotChokeTargetId(16))->load())!=47){std::cerr<<"Factory JSFX hi-hat choke migration failed\n";return EXIT_FAILURE;}}
    std::cout << "LR-608 factory presets: 14 at " << root.getFullPathName() << '\n';
    return EXIT_SUCCESS;
}
