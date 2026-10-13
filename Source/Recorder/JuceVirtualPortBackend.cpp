#include "JuceVirtualPortBackend.h"
#include <juce_audio_devices/juce_audio_devices.h>

namespace pas {
void JuceVirtualPortBackend::select(std::string identifier){if(started)stop();selectedIdentifier=std::move(identifier);}
bool JuceVirtualPortBackend::eligible(const DeviceInfo& device)const{return !device.isInput&&device.isVirtual;}
void JuceVirtualPortBackend::selectByName(std::string_view name)
{
    std::string match;int count=0;for(const auto& device:api.availableOutputs())if(eligible(device)&&device.name==name){match=device.identifier;++count;}
    select(count==1?match:std::string{});
}
Availability JuceVirtualPortBackend::probe()
{
    for(const auto& device:api.availableOutputs())if(device.identifier==selectedIdentifier&&eligible(device))return {true,BackendKind::ExternalVirtualPort,RecorderError::None};
    return {false,BackendKind::ExternalVirtualPort,RecorderError::OutputUnavailable};
}
bool JuceVirtualPortBackend::start(std::string_view){stop();started=!selectedIdentifier.empty()&&probe().available&&api.open(selectedIdentifier);return started;}
void JuceVirtualPortBackend::stop(){if(started)api.close();started=false;}
bool JuceVirtualPortBackend::send(MidiEvent event){if(!started)return false;if(!api.send(event)){api.close();started=false;return false;}return true;}

namespace {
bool looksVirtual(const juce::String& name)
{
    const auto value=name.toLowerCase();return value.contains("loopmidi")||value.contains("virtual")||value.contains("midi through")||value.contains("loopbe")||value.contains("piano auto sustain");
}
class SystemJuceMidiOutputApi final:public JuceMidiOutputApi{
public:
    std::vector<DeviceInfo> availableOutputs()const override{
        std::vector<DeviceInfo> result;for(const auto& d:juce::MidiOutput::getAvailableDevices())result.push_back({d.identifier.toStdString(),d.name.toStdString(),false,looksVirtual(d.name)});return result;
    }
    bool open(const std::string& identifier)override{output=juce::MidiOutput::openDevice(juce::String(identifier));return output!=nullptr;}
    bool send(MidiEvent event)override{if(!output)return false;const auto bytes=juce::MidiMessage(static_cast<juce::uint8>(event.status),static_cast<juce::uint8>(event.data1),static_cast<juce::uint8>(event.data2));output->sendMessageNow(bytes);return true;}
    void close()override{output.reset();}
private:std::unique_ptr<juce::MidiOutput> output;
};
}
std::unique_ptr<JuceMidiOutputApi> createSystemJuceMidiOutputApi(){return std::make_unique<SystemJuceMidiOutputApi>();}
}
