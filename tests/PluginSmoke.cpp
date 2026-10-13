#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
void snapshot(juce::Component& component,const char* name) {
    auto file=juce::File::getCurrentWorkingDirectory().getChildFile(name);
    juce::FileOutputStream stream(file); stream.setPosition(0); stream.truncate();
    juce::PNGImageFormat png;
    if(!stream.openedOk() || !png.writeImageToStream(component.createComponentSnapshot(component.getLocalBounds()),stream)) std::exit(5);
}
int main(int argc,char** argv) {
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> plugin(createPluginFilter());
    plugin->prepareToPlay(48000,256);
    juce::AudioBuffer<float> audio(2,256);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1,48,static_cast<juce::uint8>(100)),0);
    plugin->processBlock(audio,midi);
    if(midi.getNumEvents()!=1 || audio.getMagnitude(0,256)!=0) return 1;
    midi.clear(); midi.addEvent(juce::MidiMessage::noteOff(1,48),0);
    plugin->processBlock(audio,midi);
    if(!midi.isEmpty()) return 2;
    juce::MemoryBlock state; plugin->getStateInformation(state);
    if(state.getSize()==0) return 3;
    plugin->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    midi.clear(); plugin->processBlock(audio,midi);
    if(midi.isEmpty()) return 4;
    std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditor());
    snapshot(*editor,"plugin-preview-zh.png");
    auto* language=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("language"));
    if(!language) return 6;
    language->setSelectedId(2,juce::sendNotificationSync);
    snapshot(*editor,"plugin-preview-en.png");
    plugin->getStateInformation(state);
    auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));
    if(!xml || xml->getStringAttribute("language")!="en") return 7;
    editor.reset();
    plugin->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    editor.reset(plugin->createEditor());
    language=dynamic_cast<juce::ComboBox*>(editor->findChildWithID("language"));
    if(!language || language->getSelectedId()!=2) return 8;
    editor.reset();
    juce::AudioProcessorParameter* strategy=nullptr;
    for(auto* parameter:plugin->getParameters()) if(auto* id=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter)) if(id->paramID=="strategy") strategy=parameter;
    if(!strategy) return 13;
    strategy->setValueNotifyingHost(1.0f);
    plugin->getStateInformation(state);
    auto legacy=juce::AudioProcessor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));
    for(auto* child:legacy->getChildIterator()) if(child->getStringAttribute("id")=="strategy") {legacy->removeChildElement(child,true); break;}
    juce::AudioProcessor::copyXmlToBinary(*legacy,state); plugin->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    if(strategy->getValue()!=0.0f) return 14;
    plugin->getStateInformation(state);
    xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));
    if(argc>1) {
        juce::VST3PluginFormat format; juce::OwnedArray<juce::PluginDescription> types;
        format.findAllTypesForFile(types,juce::String::fromUTF8(argv[1]));
        if(types.isEmpty()) {std::cerr<<"No instrument found\n"; return 9;}
        auto* instrument=xml->createNewChildElement("Instrument");
        instrument->addChildElement(types[0]->createXml().release());
        instrument->setAttribute("state",juce::MemoryBlock{}.toBase64Encoding());
        juce::AudioProcessor::copyXmlToBinary(*xml,state);
        plugin->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        for(auto* p:plugin->getParameters()) if(auto* id=dynamic_cast<juce::AudioProcessorParameterWithID*>(p)) if(id->paramID=="mode") p->setValueNotifyingHost(1.0f);
        auto render=[&](juce::AudioProcessor& synth) {
            float peak=0;
            for(int i=0;i<100;++i) {
                midi.clear();
                if(i==0) midi.addEvent(juce::MidiMessage::noteOn(1,60,static_cast<juce::uint8>(110)),0);
                if(i==20) midi.addEvent(juce::MidiMessage::noteOff(1,60),0);
                synth.processBlock(audio,midi); peak=std::max(peak,audio.getMagnitude(0,256));
                if(!midi.isEmpty()) return -1.0f;
            }
            return peak;
        };
        const auto peak=render(*plugin);
        if(peak<=0.0f) {std::cerr<<"Hosted rendering failed, peak="<<peak<<'\n'; return 10;}
        plugin->getStateInformation(state);
        std::unique_ptr<juce::AudioProcessor> restored(createPluginFilter());
        restored->prepareToPlay(48000,256); restored->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
        restored->setNonRealtime(true);
        if(render(*restored)<=0.0f) return 11;
        editor.reset(restored->createEditor()); snapshot(*editor,"plugin-preview-hosted.png"); editor.reset();
        restored->releaseResources();
        std::cout<<"PASS: hosted "<<types[0]->name<<", audio peak="<<peak<<", saved state, new-instance restore, offline audio, no duplicate MIDI\n";
    }
    // Failed restoration retains the missing instrument's identity and state.
    xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));
    xml->deleteAllChildElementsWithTagName("Instrument");
    juce::PluginDescription missing; missing.name="Missing Piano"; missing.pluginFormatName="VST3"; missing.fileOrIdentifier="Z:/missing-piano.vst3";
    auto* child=xml->createNewChildElement("Instrument"); child->addChildElement(missing.createXml().release()); child->setAttribute("state",juce::MemoryBlock{}.toBase64Encoding());
    juce::AudioProcessor::copyXmlToBinary(*xml,state); plugin->setStateInformation(state.getData(),static_cast<int>(state.getSize()));
    plugin->getStateInformation(state); xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),static_cast<int>(state.getSize()));
    if(!xml->getChildByName("Instrument") || !xml->toString().contains("missing-piano")) return 12;
    plugin->releaseResources();
    std::cout<<"PASS: MIDI, silence, restore cleanup, bilingual rendering, language persistence, missing instrument preservation\n";
}
