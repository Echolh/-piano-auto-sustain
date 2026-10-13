#pragma once

#include "DeviceSelection.h"
#include "VirtualMidiBackend.h"

#include <memory>
#include <vector>

namespace pas {
class JuceMidiOutputApi {
public:
    virtual ~JuceMidiOutputApi()=default;
    virtual std::vector<DeviceInfo> availableOutputs()const=0;
    virtual bool open(const std::string& identifier)=0;
    virtual bool send(MidiEvent)=0;
    virtual void close()=0;
};

std::unique_ptr<JuceMidiOutputApi> createSystemJuceMidiOutputApi();

class JuceVirtualPortBackend final:public VirtualMidiBackend {
public:
    explicit JuceVirtualPortBackend(JuceMidiOutputApi& api):api(api){}
    ~JuceVirtualPortBackend()override{stop();}
    void select(std::string identifier);
    void selectByName(std::string_view name);
    Availability probe()override;
    bool start(std::string_view endpointName)override;
    void stop()override;
    bool send(MidiEvent event)override;
    bool clientConnected()const override{return started;}
    std::string endpointIdentifier()const override{return started?selectedIdentifier:std::string{};}
private:
    bool eligible(const DeviceInfo&)const;
    JuceMidiOutputApi& api;std::string selectedIdentifier;bool started=false;
};
}
