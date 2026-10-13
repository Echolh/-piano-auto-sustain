#include "MainPanel.h"
namespace pas {
MainPanel::MainPanel(){addAndMakeVisible(pedal);setSize(760,500);}
juce::String MainPanel::statusText()const{if(lang==Language::Chinese)return status==DeviceStatus::Connected?juce::String::fromUTF8(u8"设备已连接"):status==DeviceStatus::FeedbackStopped?juce::String::fromUTF8(u8"已阻止 MIDI 回环"):juce::String::fromUTF8(u8"等待 MIDI 设备");return status==DeviceStatus::Connected?"Devices connected":status==DeviceStatus::FeedbackStopped?"MIDI feedback stopped":"Waiting for MIDI devices";}
void MainPanel::paint(juce::Graphics& g){const auto&t=themes()[theme];g.fillAll(t.background);auto r=getLocalBounds().reduced(28);g.setColour(t.text);g.setFont(juce::FontOptions(34).withStyle("Bold"));g.drawText(productName(),r.removeFromTop(48),juce::Justification::left);g.setColour(t.muted);g.setFont(18);g.drawText(lang==Language::Chinese?juce::String::fromUTF8(u8"钢琴自动延音"):juce::String("Automatic piano sustain"),r.removeFromTop(30),juce::Justification::left);g.setColour(t.panel);g.fillRoundedRectangle(r.removeFromTop(250).toFloat(),20);g.setColour(t.text);g.setFont(20);g.drawText(statusText(),28,getHeight()-92,getWidth()-56,30,juce::Justification::left);g.setColour(t.accent);g.drawText("Amu music  |  1.0",28,getHeight()-55,getWidth()-56,28,juce::Justification::right);}
void MainPanel::resized(){pedal.setBounds(70,145,180,180);}
}
