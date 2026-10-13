#pragma once

#include "Core/MidiEvent.h"
#include "Bridge/MidiPort.h"
#include "RecorderStatus.h"

#include <string>
#include <string_view>

namespace pas {

class VirtualMidiBackend : public IMidiOutput {
public:
    virtual ~VirtualMidiBackend() = default;

    virtual Availability probe() = 0;
    virtual bool start(std::string_view endpointName) = 0;
    virtual void stop() = 0;
    virtual bool send(MidiEvent event) = 0;
    virtual bool clientConnected() const = 0;
    virtual std::string endpointIdentifier() const = 0;
    std::string identifier() const final { return endpointIdentifier(); }
};

} // namespace pas
