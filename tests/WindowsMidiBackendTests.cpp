#include "Recorder/WindowsMidiServicesBackend.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}

struct FakeApi final:pas::WindowsMidiServicesApi{
    bool runtime=true,transport=true,created=true,connected=false,sendOk=true;
    std::vector<std::string> calls;std::vector<std::uint32_t> packets;
    bool initialize()override{calls.push_back("initialize");return runtime;}
    bool transportAvailable()const override{return transport;}
    bool createVirtualDevice(std::string_view name)override{calls.push_back("create:"+std::string(name));return created;}
    bool sendUmp(std::uint32_t packet)override{calls.push_back("send");if(sendOk)packets.push_back(packet);return sendOk;}
    bool clientConnected()const override{return connected;}
    std::string endpointIdentifier()const override{return created?"wms:piano-auto-sustain":"";}
    void closeClientEndpoint()override{calls.push_back("close-client");}
    void closeVirtualDevice()override{calls.push_back("close-device");}
    void shutdownRuntime()override{calls.push_back("shutdown-runtime");}
};

int main(){
    FakeApi missing;missing.runtime=false;pas::WindowsMidiServicesBackend missingBackend{missing};
    auto availability=missingBackend.probe();require(!availability.available&&availability.error==pas::RecorderError::RuntimeMissing,"missing runtime is explicit");
    FakeApi disabled;disabled.transport=false;pas::WindowsMidiServicesBackend disabledBackend{disabled};
    availability=disabledBackend.probe();require(!availability.available&&availability.error==pas::RecorderError::TransportUnavailable,"disabled transport is explicit");
    FakeApi failed;failed.created=false;pas::WindowsMidiServicesBackend failedBackend{failed};
    require(!failedBackend.start("Piano Auto Sustain"),"creation failure stops startup");

    FakeApi api;pas::WindowsMidiServicesBackend backend{api};
    require(backend.start("Piano Auto Sustain"),"available service creates endpoint");
    require(!backend.clientConnected(),"published endpoint may wait for host client");
    api.connected=true;require(backend.clientConnected(),"client connection is observed without endpoint guessing");
    require(backend.send({0x90,60,100}),"MIDI 1 channel voice event sends as UMP");
    require(api.packets.size()==1&&api.packets[0]==0x20903c64u,"MIDI 1 channel voice converts to a UMP MIDI 1 packet");
    require(backend.endpointIdentifier()=="wms:piano-auto-sustain","service endpoint identifier is stable");
    backend.stop();
    const auto n=api.calls.size();require(n>=3&&api.calls[n-3]=="close-client"&&api.calls[n-2]=="close-device"&&api.calls[n-1]=="shutdown-runtime","shutdown order is client, virtual device, runtime");
    require(!backend.clientConnected(),"stopped backend reports no client");
    std::cout<<"PASS: Windows MIDI Services capability, conversion, client state, shutdown\n";
}
