#include "UI/MainPanel.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <cstdlib>
#include <iostream>
void require(bool ok,const char* text){if(!ok){std::cerr<<"FAIL: "<<text<<'\n';std::exit(1);}}
int main(){
 juce::ScopedJuceInitialiser_GUI init;pas::MainPanel panel;require(pas::themes().size()==5,"five themes");
 require(panel.productName()=="Piano Auto Sustain"&&panel.brandName()=="Amu music"&&panel.versionText()=="1.0","identity and version");
 panel.setLanguage(pas::Language::English);require(panel.language()==pas::Language::English,"English persists");
 panel.setLanguage(pas::Language::Chinese);require(panel.language()==pas::Language::Chinese,"Chinese persists");
 panel.setDeviceStatus(pas::DeviceStatus::Disconnected);require(panel.statusText().isNotEmpty(),"disconnected state visible");
 int scale=100;for(auto size:std::initializer_list<juce::Point<int>>{{760,500},{1140,750},{1520,1000}}){panel.setSize(size.x,size.y);auto image=panel.createComponentSnapshot(panel.getLocalBounds());require(image.isValid(),"DPI snapshot renders");juce::FileOutputStream stream(juce::File::getCurrentWorkingDirectory().getChildFile("ui-"+juce::String(scale)+".png"));stream.setPosition(0);stream.truncate();require(juce::PNGImageFormat{}.writeImageToStream(image,stream),"DPI snapshot writes");scale+=50;}
 std::cout<<"PASS: bilingual UI, five themes, identity, status, DPI snapshots\n";
}
