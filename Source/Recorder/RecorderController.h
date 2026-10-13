#pragma once

#include "DeviceSelection.h"
#include "RecorderStatus.h"
#include "VirtualMidiBackend.h"
#include "Bridge/MidiBridgeEngine.h"

#include <vector>

namespace pas {
class RecorderController {
public:
    RecorderController(MidiBridgeEngine&,VirtualMidiBackend& preferred,VirtualMidiBackend* fallback=nullptr);
    void refreshDevices(std::vector<DeviceInfo>);
    bool selectInput(std::string id);
    bool start();
    void stop();
    void panic();
    void poll();
    void handleIncoming(std::int64_t time,MidiEvent event);
    RecorderStatus status() const { return currentStatus; }
    BridgeMetrics metrics() const { return bridge.metrics(); }
    const std::string& selectedInput() const { return selectedId; }
    std::string selectedInputName() const;
private:
    struct SelectedInput final : IMidiInput { std::string id;std::string identifier()const override{return id;} } selected;
    void disconnect(bool preserveIntent);
    MidiBridgeEngine& bridge;
    VirtualMidiBackend& preferred;
    VirtualMidiBackend* fallback;
    VirtualMidiBackend* activeBackend=nullptr;
    std::vector<DeviceInfo> devices;
    std::string selectedId;
    bool wantsRunning=false;
    RecorderStatus currentStatus;
};
}
