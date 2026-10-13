#include "Bridge/MidiBridgeEngine.h"
#include <cstdlib>
#include <iostream>
#include <vector>

void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
struct Input final:pas::IMidiInput{std::string id;explicit Input(std::string value):id(std::move(value)){}std::string identifier()const override{return id;}};
struct Output final:pas::IMidiOutput{
    std::string id;bool available=true;std::vector<pas::MidiEvent> sent;explicit Output(std::string value):id(std::move(value)){}
    std::string identifier()const override{return id;}
    bool send(pas::MidiEvent event)override{if(!available)return false;sent.push_back(event);return true;}
    int cc64(int value)const{int n=0;for(auto e:sent)if((e.status&0xf0)==0xb0&&e.data1==64&&e.data2==value)++n;return n;}
};
bool isCc64(const pas::MidiEvent& e,int value){return (e.status&0xf0)==0xb0&&e.data1==64&&e.data2==value;}
int main(){
    Input in{"keyboard"};Output out{"virtual-out"};pas::MidiBridgeEngine bridge;bridge.prepare(1000);
    require(bridge.start(in,out),"different input and output start");
    bridge.handleIncoming(0,{0x90,48,100});
    require(out.sent.size()==2&&isCc64(out.sent[0],127)&&out.sent[1].status==0x90,"pedal transition precedes note-on at the same timestamp");
    bridge.handleIncoming(1,{0xe0,1,65});require(out.sent.back().status==0xe0,"non-CC64 MIDI is preserved");

    const auto automaticBefore=out.sent.size();
    bridge.handleIncoming(2,{0xb0,64,80});
    require(out.sent.size()==automaticBefore,"automatic policy owns and suppresses physical CC64");
    Output physicalOut{"physical-out"};pas::MidiBridgeEngine physicalBridge;physicalBridge.prepare(1000);
    physicalBridge.state.hardwarePedalPolicy=pas::HardwarePedalPolicy::Physical;
    require(physicalBridge.start(in,physicalOut),"physical-policy bridge starts");
    physicalBridge.handleIncoming(3,{0xb0,64,80});
    require(physicalOut.sent.size()==1&&physicalOut.sent.back().data2==80,"physical policy forwards hardware CC64");
    physicalBridge.handleIncoming(4,{0x90,52,90});
    require(physicalOut.sent.size()==2&&physicalOut.cc64(127)==0,"physical policy disables automatic transitions");
    Output mergeOut{"merge-out"};pas::MidiBridgeEngine mergeBridge;mergeBridge.prepare(1000);
    mergeBridge.state.hardwarePedalPolicy=pas::HardwarePedalPolicy::Merge;
    require(mergeBridge.start(in,mergeOut),"merge-policy bridge starts");
    mergeBridge.handleIncoming(5,{0xb0,64,45});
    require(mergeOut.sent.back().data2==45,"merge policy preserves hardware CC64");
    mergeBridge.handleIncoming(6,{0x90,55,90});
    require(mergeOut.cc64(127)==1,"merge policy retains automatic pedal generation");

    const auto metrics=bridge.metrics();
    require(metrics.inputMessages>=3&&metrics.outputMessages==out.sent.size()&&metrics.cc64Messages>=1,"bridge reports record-stream metrics");
    out.available=false;bridge.handleIncoming(10,{0x80,48,0});bridge.refreshDevices(nullptr,nullptr);require(!bridge.running()&&bridge.safetyReleasePending(),"device loss stops and queues release");
    out.available=true;bridge.refreshDevices(&in,&out);require(bridge.running()&&out.cc64(0)>=1,"identifier reconnect sends safety release");
    bridge.panic();require(out.cc64(0)>=16,"panic releases all channels");
    bridge.stop();require(out.cc64(0)>=32,"stop sends safety release on all channels");
    Input loopIn{"same"};Output loopOut{"same"};require(!bridge.start(loopIn,loopOut),"same endpoint identifier is rejected");
    bridge.state.channelFilter=2;require(bridge.start(in,out),"bridge restarts after validation error");const auto before=out.sent.size();
    bridge.handleIncoming(100,{0x90,60,100});require(out.sent.size()==before,"channel filter drops non-selected channel");
    bridge.handleIncoming(101,{0x91,60,100});require(out.sent.size()>before,"channel filter forwards selected channel");
    for(int i=0;i<2100&&bridge.running();++i)bridge.handleIncoming(200,{0x91,61,100});
    require(!bridge.running()&&bridge.loopDetected(),"message storm disables output");
    require(bridge.safetyReleasePending()==false,"successful storm shutdown completes safety release");
    std::cout<<"PASS: bridge forwarding, loss, reconnect, loop protection, channel filter, shutdown\n";
}
