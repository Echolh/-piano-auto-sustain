#pragma once

#include "VirtualMidiBackend.h"
#include "WindowsMidiServicesApi.h"

namespace pas {

class WindowsMidiServicesBackend final : public VirtualMidiBackend {
public:
    explicit WindowsMidiServicesBackend(WindowsMidiServicesApi& api) : api(api) {}
    ~WindowsMidiServicesBackend() override { stop(); }

    Availability probe() override;
    bool start(std::string_view endpointName) override;
    void stop() override;
    bool send(MidiEvent event) override;
    bool clientConnected() const override;
    std::string endpointIdentifier() const override;

private:
    static std::uint32_t toUmp(MidiEvent event);
    WindowsMidiServicesApi& api;
    bool initialized = false;
    bool started = false;
};

} // namespace pas
