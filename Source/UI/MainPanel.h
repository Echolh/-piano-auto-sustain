#pragma once
#include "Theme.h"
#include "PedalView.h"
namespace pas {
enum class Language{Chinese,English};enum class DeviceStatus{Connected,Disconnected,FeedbackStopped};
class MainPanel:public juce::Component{
public:MainPanel();void paint(juce::Graphics&)override;void resized()override;
 void setLanguage(Language value){lang=value;repaint();}Language language()const{return lang;}
 void setTheme(int value){theme=juce::jlimit(0,4,value);repaint();}int themeIndex()const{return theme;}
 void setDeviceStatus(DeviceStatus value){status=value;repaint();}
 juce::String statusText()const;juce::String productName()const{return "Piano Auto Sustain";}juce::String brandName()const{return "Amu music";}juce::String versionText()const{return "1.0";}
 PedalView pedal;
private:Language lang=Language::Chinese;DeviceStatus status=DeviceStatus::Disconnected;int theme=0;
};}
