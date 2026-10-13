#include "PluginProcessor.h"
#include "PluginEditor.h"

PianoAutoSustainProcessor::PianoAutoSustainProcessor()
    : AudioProcessor(
#if PAS_AU_MIDI
        BusesProperties()),
#else
        BusesProperties().withOutput("Silent Output", juce::AudioChannelSet::stereo(), true)),
#endif
      parameters(*this, nullptr, pas::plugin_state::treeType, createParameterLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout PianoAutoSustainProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"enabled",1},"Auto sustain",true));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Output mode",juce::StringArray{"Note Hold","CC64 pedal"},1));
    p.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"split",1},"Bass ceiling",24,84,59));
    p.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"group",1},"Chord window",5,200,40));
    p.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"gap",1},"Pedal lift",1,250,35));
    p.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"guard",1},"Change guard",0,2000,160));
    p.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"tail",1},"Hands-off tail",100,10000,2400));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"octave",1},"Ignore octave changes",true));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"chordProtection",1},"Chord protection",true));
    return p;
}

void PianoAutoSustainProcessor::prepareToPlay(double rate,int) {
    engine.prepare(rate); clock=0; output.ensureSize(65536); resetPending=true; wasPlaying=false; expectedHostSample=-1; uiPedalDown=false; uiNotes=0; uiCc64=0; uiAction=0; updateSettings();
}
void PianoAutoSustainProcessor::releaseResources() { resetPending=true; }
bool PianoAutoSustainProcessor::isBusesLayoutSupported(const BusesLayout& b) const {
#if PAS_AU_MIDI
    return b.inputBuses.isEmpty() && b.outputBuses.isEmpty();
#else
    return b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
#endif
}

void PianoAutoSustainProcessor::updateSettings() {
    engine.settings.enabled=value("enabled")>=0.5f; engine.settings.noteHold=value("mode")<0.5f;
    engine.settings.split=static_cast<int>(value("split")); engine.settings.groupMs=static_cast<int>(value("group"));
    engine.settings.gapMs=static_cast<int>(value("gap")); engine.settings.guardMs=static_cast<int>(value("guard"));
    engine.settings.tailMs=static_cast<int>(value("tail")); engine.settings.octaveSame=value("octave")>=0.5f;
    engine.settings.chordProtection=value("chordProtection")>=0.5f;
}

void PianoAutoSustainProcessor::process(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi,bool bypassed) {
    audio.clear(); output.clear(); const int samples=audio.getNumSamples(); if(samples<=0) return;
    auto emit=[&](pas::PedalEngine::Time time,pas::MidiEvent event) {
        const juce::uint8 bytes[]{static_cast<juce::uint8>(event.status),static_cast<juce::uint8>(event.data1),static_cast<juce::uint8>(event.data2)};
        output.addEvent(bytes,3,juce::jlimit(0,samples-1,static_cast<int>(time-clock)));
        if((event.status&0xf0)==0xb0&&event.data1==64){uiPedalDown=event.data2>=64;++uiCc64;}
    };
    bool discontinuity=bypassed;
    if(auto* head=getPlayHead()) if(auto position=head->getPosition()) {
        const bool playing=position->getIsPlaying(); discontinuity=discontinuity||(wasPlaying&&!playing);
        if(auto sample=position->getTimeInSamples()) {
            if(playing&&wasPlaying&&expectedHostSample>=0&&*sample!=expectedHostSample) discontinuity=true;
            expectedHostSample=playing?*sample+samples:-1;
        }
        wasPlaying=playing;
    }
    if(resetPending||discontinuity) {engine.panic(clock,emit);resetPending=false;}
    updateSettings();
    switch(uiAction.exchange(0)) {case 1: engine.repedal(clock,emit); break; case 2: engine.panic(clock,emit); break; default: break;}
    if(bypassed) {output.addEvents(midi,0,-1,0);midi.swapWith(output);clock+=samples;return;}
    for(const auto item:midi) {
        const auto message=item.getMessage(); const int offset=juce::jlimit(0,samples-1,item.samplePosition); const auto* raw=message.getRawData();
        if(message.isNoteOn()) ++uiNotes;
        if(message.getRawDataSize()==3&&raw[0]>=0x80&&raw[0]<0xf0) engine.input(clock+offset,{raw[0],raw[1],raw[2]},emit);
        else {engine.advance(clock+offset,emit);output.addEvent(message,offset);}
    }
    engine.advance(clock+samples-1,emit);midi.swapWith(output);clock+=samples;
}
void PianoAutoSustainProcessor::processBlock(juce::AudioBuffer<float>& a,juce::MidiBuffer& m){process(a,m,false);}
void PianoAutoSustainProcessor::processBlockBypassed(juce::AudioBuffer<float>& a,juce::MidiBuffer& m){process(a,m,true);}
juce::AudioProcessorEditor* PianoAutoSustainProcessor::createEditor(){return new PianoAutoSustainEditor(*this);}

void PianoAutoSustainProcessor::getStateInformation(juce::MemoryBlock& data) {
    auto xml=parameters.copyState().createXml();xml->setAttribute("schema",pas::plugin_state::schemaVersion);juce::AudioProcessor::copyXmlToBinary(*xml,data);
}
void PianoAutoSustainProcessor::setStateInformation(const void* data,int bytes) {
    if(auto xml=juce::AudioProcessor::getXmlFromBinary(data,bytes)) {
        if(!xml->hasTagName(parameters.state.getType())) return;
        xml->deleteAllChildElementsWithTagName("Instrument");
        const int schema=xml->getIntAttribute("schema",1);
        if(schema<=1) for(auto* child:xml->getChildIterator()) if(child->getStringAttribute("id")=="strategy") child->setAttribute("id","chordProtection");
        xml->setAttribute("schema",pas::plugin_state::schemaVersion);
        parameters.replaceState(juce::ValueTree::fromXml(*xml));resetPending=true;
    }
}
