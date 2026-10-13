#include "Recorder/RecorderController.h"
#include "Recorder/RecorderConfig.h"
#include "Recorder/RecorderLaunchPolicy.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}

struct Backend final:pas::VirtualMidiBackend{
    pas::Availability availability{true,pas::BackendKind::WindowsMidiServices,pas::RecorderError::None};
    bool startOk=true,connected=true,started=false,probeWhileStarted=false;std::vector<pas::MidiEvent> sent;
    pas::Availability probe()override{if(started)probeWhileStarted=true;return availability;}
    bool start(std::string_view)override{return started=startOk;}
    void stop()override{started=false;}
    bool send(pas::MidiEvent event)override{if(!started)return false;sent.push_back(event);return true;}
    bool clientConnected()const override{return connected;}
    std::string endpointIdentifier()const override{return "virtual:piano";}
};

int main(){
    Backend preferred,fallback;fallback.availability.backend=pas::BackendKind::ExternalVirtualPort;
    pas::MidiBridgeEngine bridge;bridge.prepare(1000);
    pas::RecorderController controller{bridge,preferred,&fallback};
    controller.refreshDevices({{"kbd-1","Keyboard",true,false}});
    require(controller.selectedInput()=="kbd-1","one physical keyboard is selected automatically");
    require(controller.start()&&controller.status().connection==pas::ConnectionState::Ready,"single-device path starts without extra choice");
    require(!preferred.probeWhileStarted,"controller does not probe and tear down a running backend");
    controller.handleIncoming(0,{0x90,48,100});require(preferred.sent.size()==2,"controller forwards processed record stream");
    controller.stop();
    preferred.connected=false;pas::RecorderController waitingController{bridge,preferred,&fallback};waitingController.refreshDevices({{"kbd-1","Keyboard",true,false}});require(waitingController.start()&&waitingController.status().connection==pas::ConnectionState::WaitingForHost,"published endpoint waits for host");preferred.connected=true;waitingController.poll();require(waitingController.status().connection==pas::ConnectionState::Ready,"poll observes host connection");waitingController.stop();

    pas::RecorderController ambiguous{bridge,preferred,&fallback};
    ambiguous.refreshDevices({{"kbd-a","Keyboard",true,false},{"kbd-b","Keyboard",true,false}});
    require(ambiguous.selectedInput().empty()&&ambiguous.status().error==pas::RecorderError::AmbiguousInput,"duplicate display names require stable-id selection");
    require(ambiguous.selectInput("kbd-b"),"explicit stable identifier selects keyboard");
    ambiguous.refreshDevices({{"kbd-b","Renamed Keyboard",true,false}});
    require(ambiguous.selectedInput()=="kbd-b"&&ambiguous.selectedInputName()=="Renamed Keyboard","rename preserves stable-id selection");
    ambiguous.refreshDevices({});require(ambiguous.status().connection==pas::ConnectionState::WaitingForInput,"missing keyboard waits instead of guessing");

    Backend unavailable;unavailable.availability={false,pas::BackendKind::WindowsMidiServices,pas::RecorderError::RuntimeMissing};
    pas::RecorderController fallbackController{bridge,unavailable,&fallback};
    fallbackController.refreshDevices({{"kbd-1","Keyboard",true,false}});
    require(fallbackController.start()&&fallbackController.status().backend==pas::BackendKind::ExternalVirtualPort,"unavailable native backend falls back safely");

    const auto root=std::filesystem::temp_directory_path()/"pas-recorder-config-test";
    std::filesystem::remove_all(root);std::filesystem::create_directories(root);
    pas::RecorderConfig defaults;require(defaults.schemaVersion==5&&defaults.startWithWindows&&defaults.closeToTray&&defaults.quietWhenOperational,"new installs default to zero-interruption background operation");
    pas::RecorderStatus operational;operational.connection=pas::ConnectionState::WaitingForHost;operational.error=pas::RecorderError::None;operational.inputConnected=true;operational.endpointPublished=true;
    require(!pas::shouldShowRecorderWindow(defaults,operational),"operational recorder stays quietly in the tray");
    operational.error=pas::RecorderError::OutputUnavailable;require(pas::shouldShowRecorderWindow(defaults,operational),"actionable output errors reveal setup");
    pas::RecorderConfigStore store{root/"recorder.json"};pas::RecorderConfig config;config.inputIdentifier="kbd-1";config.outputIdentifier="virtual:piano";config.theme=4;config.startWithWindows=true;config.closeToTray=true;config.quietWhenOperational=true;
    require(store.save(config)&&!std::filesystem::exists(root/"recorder.json.tmp"),"config save atomically replaces temp file");
    const auto loaded=store.load();require(loaded.inputIdentifier=="kbd-1"&&loaded.theme==4&&loaded.startWithWindows&&loaded.closeToTray&&loaded.quietWhenOperational&&loaded.schemaVersion==5,"schema 5 config round-trips");
    {std::ofstream bad(root/"recorder.json",std::ios::trunc);bad<<"{broken";}
    const auto recovered=store.load();require(recovered.schemaVersion==5&&std::filesystem::exists(root/"recorder.json.corrupt"),"corrupt config is backed up and reset");
    {std::ofstream old(root/"recorder.json",std::ios::trunc);old<<R"({"inputIdentifier":"old-kbd","outputIdentifier":"old-port"})";}
    const auto migrated=store.load();require(migrated.schemaVersion==5&&migrated.inputIdentifier=="old-kbd"&&migrated.outputIdentifier=="old-port"&&migrated.quietWhenOperational,"older identifiers migrate to quiet schema 5 defaults");
    std::filesystem::remove_all(root);
    std::cout<<"PASS: recorder selection, fallback, forwarding, config recovery and migration\n";
}
