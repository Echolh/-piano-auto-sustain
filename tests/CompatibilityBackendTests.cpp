#include "Recorder/JuceVirtualPortBackend.h"
#include <cstdlib>
#include <iostream>
#include <memory>

void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
struct FakePortApi final:pas::JuceMidiOutputApi{
    std::vector<pas::DeviceInfo> outputs;std::string opened;bool sendOk=true;int closes=0;
    std::vector<pas::DeviceInfo> availableOutputs()const override{return outputs;}
    bool open(const std::string& id)override{for(const auto& d:outputs)if(d.identifier==id){opened=id;return true;}return false;}
    bool send(pas::MidiEvent)override{return !opened.empty()&&sendOk;}
    void close()override{opened.clear();++closes;}
};
int main(){
    FakePortApi none;pas::JuceVirtualPortBackend empty{none};require(!empty.probe().available,"no external port is unavailable");
    FakePortApi api;api.outputs={{"physical","USB Synth",false,false},{"loop-1","Piano Auto Sustain",false,true}};
    pas::JuceVirtualPortBackend backend{api};backend.select("loop-1");
    require(backend.probe().available&&backend.start("ignored"),"saved virtual port opens by stable identifier");
    require(backend.endpointIdentifier()=="loop-1"&&backend.clientConnected(),"opened external port reports stable identity");
    require(backend.send({0x90,60,100}),"opened external port sends MIDI");
    api.sendOk=false;require(!backend.send({0x80,60,0}),"output loss is reported");
    require(api.closes==1&&api.opened.empty(),"output loss closes the failed MIDI port");
    backend.stop();api.sendOk=true;require(backend.start("ignored"),"disconnected port can reconnect");
    backend.select("physical");require(!backend.probe().available,"physical MIDI outputs are excluded from recorder fallback");
    api.outputs={{"a","Virtual Cable",false,true},{"b","Virtual Cable",false,true}};backend.selectByName("Virtual Cable");
    require(!backend.probe().available,"ambiguous display names never guess a port");
    backend.select("a");require(backend.probe().available,"explicit stable id resolves ambiguous names");
    std::cout<<"PASS: compatible port selection, exclusion, disconnect and reconnect\n";
}
