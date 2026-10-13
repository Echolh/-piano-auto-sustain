#pragma once
#include "MidiPort.h"
#include <juce_audio_devices/juce_audio_devices.h>
namespace pas {
struct JuceInputRef final:IMidiInput{juce::String id;std::string identifier()const override{return id.toStdString();}};
struct JuceOutputRef final:IMidiOutput{juce::String id;std::unique_ptr<juce::MidiOutput> device;std::string identifier()const override{return id.toStdString();}bool send(MidiEvent e)override{if(!device)return false;const juce::uint8 b[]{(juce::uint8)e.status,(juce::uint8)e.data1,(juce::uint8)e.data2};device->sendMessageNow(juce::MidiMessage(b,3));return true;}};
}
