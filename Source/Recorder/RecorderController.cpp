#include "RecorderController.h"

namespace pas {
RecorderController::RecorderController(MidiBridgeEngine& bridge,VirtualMidiBackend& preferred,VirtualMidiBackend* fallback)
    :bridge(bridge),preferred(preferred),fallback(fallback){}

void RecorderController::refreshDevices(std::vector<DeviceInfo> value)
{
    devices=std::move(value);
    if(!selectedId.empty()&&!findDevice(devices,selectedId)){
        disconnect(true);currentStatus.connection=ConnectionState::WaitingForInput;currentStatus.error=RecorderError::InputUnavailable;return;
    }
    if(selectedId.empty()){
        if(const auto* only=uniquePhysicalInput(devices))selectedId=only->identifier;
        else if(!devices.empty()){currentStatus.connection=ConnectionState::WaitingForInput;currentStatus.error=RecorderError::AmbiguousInput;return;}
    }
    if(wantsRunning&&!bridge.running())start();
    else if(!selectedId.empty()){currentStatus.connection=ConnectionState::Stopped;currentStatus.error=RecorderError::None;}
}

bool RecorderController::selectInput(std::string id)
{
    const auto* device=findDevice(devices,id);if(!device||!device->isInput||device->isVirtual)return false;
    selectedId=std::move(id);currentStatus.connection=ConnectionState::Stopped;currentStatus.error=RecorderError::None;return true;
}

bool RecorderController::start()
{
    wantsRunning=true;const auto* device=findDevice(devices,selectedId);
    if(!device){currentStatus.connection=ConnectionState::WaitingForInput;currentStatus.error=RecorderError::InputUnavailable;return false;}
    activeBackend=nullptr;BackendKind selectedBackendKind=BackendKind::None;
    auto tryBackend=[&](VirtualMidiBackend& candidate){const auto a=candidate.probe();if(a.available&&candidate.start(kRecorderEndpointName)){selectedBackendKind=a.backend;return true;}return false;};
    if(tryBackend(preferred))activeBackend=&preferred;else if(fallback&&tryBackend(*fallback))activeBackend=fallback;
    if(!activeBackend){currentStatus.connection=ConnectionState::Error;currentStatus.error=RecorderError::BackendUnavailable;return false;}
    selected.id=selectedId;
    if(!bridge.start(selected,*activeBackend)){activeBackend->stop();activeBackend=nullptr;currentStatus.connection=ConnectionState::Error;currentStatus.error=RecorderError::SameEndpoint;return false;}
    currentStatus.backend=selectedBackendKind;currentStatus.inputConnected=true;currentStatus.endpointPublished=true;currentStatus.clientConnected=activeBackend->clientConnected();currentStatus.connection=currentStatus.clientConnected?ConnectionState::Ready:ConnectionState::WaitingForHost;currentStatus.error=RecorderError::None;return true;
}

void RecorderController::disconnect(bool preserveIntent)
{
    bridge.stop();if(activeBackend)activeBackend->stop();activeBackend=nullptr;currentStatus.inputConnected=false;currentStatus.endpointPublished=false;currentStatus.clientConnected=false;if(!preserveIntent)wantsRunning=false;
}
void RecorderController::stop(){disconnect(false);currentStatus.connection=ConnectionState::Stopped;currentStatus.error=RecorderError::None;}
void RecorderController::panic(){bridge.panic();}
void RecorderController::poll(){
    if(!activeBackend)return;
    if(!bridge.running()){currentStatus.connection=ConnectionState::Error;currentStatus.error=RecorderError::SendFailed;currentStatus.clientConnected=false;return;}
    currentStatus.clientConnected=activeBackend->clientConnected();currentStatus.connection=currentStatus.clientConnected?ConnectionState::Ready:ConnectionState::WaitingForHost;
}
void RecorderController::handleIncoming(std::int64_t time,MidiEvent event){bridge.handleIncoming(time,event);}
std::string RecorderController::selectedInputName()const{if(const auto* d=findDevice(devices,selectedId))return d->name;return {};}
}
