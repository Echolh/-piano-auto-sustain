#include "MidiBridgeEngine.h"
namespace pas {
void MidiBridgeEngine::prepare(double rate){engine.prepare(rate);engine.settings.noteHold=false;active=false;pendingRelease=false;feedbackDetected=false;rateWindow=0;rateCount=0;counters={};}
bool MidiBridgeEngine::start(IMidiInput& in,IMidiOutput& out){
    if(in.identifier()==out.identifier()){active=false;++counters.errors;return false;}
    input=&in;output=&out;state.inputIdentifier=in.identifier();state.outputIdentifier=out.identifier();active=true;feedbackDetected=false;rateWindow=0;rateCount=0;
    if(pendingRelease)sendSafetyRelease();return active;
}
bool MidiBridgeEngine::send(MidiEvent e){
    if(!output||!output->send(e)){active=false;pendingRelease=true;++counters.errors;return false;}
    ++counters.outputMessages;if((e.status&0xf0)==0xb0&&e.data1==64)++counters.cc64Messages;return true;
}
void MidiBridgeEngine::sendSafetyRelease(){
    if(!output){pendingRelease=true;return;} bool ok=true;
    for(int channel=0;channel<16;++channel){
        const bool pedalOk=output->send({0xb0|channel,64,0});if(pedalOk){++counters.outputMessages;++counters.cc64Messages;}else ++counters.errors;ok=pedalOk&&ok;
        const bool notesOk=output->send({0xb0|channel,123,0});if(notesOk)++counters.outputMessages;else ++counters.errors;ok=notesOk&&ok;
    }
    pendingRelease=!ok;if(!ok)active=false;engine.prepare(1000.0);engine.settings.noteHold=false;
}
void MidiBridgeEngine::stop(){if(output)sendSafetyRelease();active=false;input=nullptr;output=nullptr;}
void MidiBridgeEngine::panic(){sendSafetyRelease();}
void MidiBridgeEngine::handleIncoming(std::int64_t time,MidiEvent event){
    if(!active)return;++counters.inputMessages;const int channel=(event.status&15)+1;if(state.channelFilter&&state.channelFilter!=channel){++counters.droppedMessages;return;}
    if(time-rateWindow>=1000){rateWindow=time;rateCount=0;}if(++rateCount>2048){feedbackDetected=true;sendSafetyRelease();active=false;return;}
    const bool physicalCc64=(event.status&0xf0)==0xb0&&event.data1==64;
    if(state.hardwarePedalPolicy==HardwarePedalPolicy::Physical){send(event);return;}
    if(physicalCc64){if(state.hardwarePedalPolicy==HardwarePedalPolicy::Merge)send(event);else ++counters.droppedMessages;return;}
    std::array<MidiEvent,260> emitted{};std::size_t count=0;
    engine.input(time,event,[&](auto,MidiEvent generated){if(count<emitted.size())emitted[count++]=generated;else{++counters.droppedMessages;}});
    const bool noteOn=(event.status&0xf0)==0x90&&event.data2>0;
    if(noteOn){for(std::size_t i=0;i<count;++i)if((emitted[i].status&0xf0)==0xb0&&emitted[i].data1==64)send(emitted[i]);}
    for(std::size_t i=0;i<count;++i){const bool cc64=(emitted[i].status&0xf0)==0xb0&&emitted[i].data1==64;if(!noteOn||!cc64)send(emitted[i]);}
}
void MidiBridgeEngine::advance(std::int64_t time){if(active)engine.advance(time,[&](auto,MidiEvent generated){send(generated);});}
void MidiBridgeEngine::refreshDevices(IMidiInput* in,IMidiOutput* out){
    if(!in||!out){if(active)sendSafetyRelease();active=false;input=nullptr;output=nullptr;return;}
    if(in->identifier()==state.inputIdentifier&&out->identifier()==state.outputIdentifier){input=in;output=out;active=true;++counters.reconnects;if(pendingRelease)sendSafetyRelease();}
}
}
