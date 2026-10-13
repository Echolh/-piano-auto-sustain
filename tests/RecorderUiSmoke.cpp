#include "Recorder/RecorderPanel.h"
#include <juce_audio_utils/juce_audio_utils.h>
#include <cstdlib>
#include <iostream>

void require(bool ok,const char* text){if(!ok){std::cerr<<"FAIL: "<<text<<'\n';std::exit(1);}}
int main(){
    juce::ScopedJuceInitialiser_GUI init;pas::RecorderPanel panel;
    require(pas::themes().size()==5,"five product themes remain available");
    panel.setLanguage(pas::Language::Chinese);require(panel.titleText().isNotEmpty()&&panel.connectionText().isNotEmpty(),"Chinese recorder strings exist");
    panel.setLanguage(pas::Language::English);require(panel.titleText()=="Performance Recorder","English recorder strings exist");
    pas::RecorderStatus ready;ready.connection=pas::ConnectionState::Ready;ready.inputConnected=ready.endpointPublished=ready.clientConnected=true;panel.setStatus(ready);
    require(panel.firstRunState()==pas::FirstRunState::Ready&&panel.connectionText()=="Ready to play and record","ready requires input and output");
    pas::RecorderStatus choose;choose.connection=pas::ConnectionState::WaitingForInput;choose.error=pas::RecorderError::AmbiguousInput;panel.setStatus(choose);
    require(panel.firstRunState()==pas::FirstRunState::ChooseKeyboard,"ambiguous keyboards request one choice");
    pas::RecorderStatus runtime;runtime.connection=pas::ConnectionState::Unavailable;runtime.error=pas::RecorderError::RuntimeMissing;panel.setStatus(runtime);
    require(panel.firstRunState()==pas::FirstRunState::InstallMidiRuntime,"missing runtime gets guidance");
    panel.setCompatibilityRequired(true);require(panel.firstRunState()==pas::FirstRunState::ChooseCompatibilityOutput,"compatibility output is explicit");
    panel.setStatus(runtime);require(panel.firstRunState()==pas::FirstRunState::ChooseCompatibilityOutput,"status refresh preserves compatibility guidance");
    panel.setCompatibilityRequired(false);panel.setStatus(choose);bool chose=false;panel.onChooseKeyboard=[&]{chose=true;};panel.onChooseKeyboard();require(chose,"keyboard chooser action is exposed");
    panel.setStartWithWindows(true);panel.setCloseToTray(true);require(panel.startWithWindows()&&panel.closeToTray(),"launch and tray choices are explicit and retained");
    bool preferences=false;panel.onPreferencesChanged=[&]{preferences=true;};panel.onPreferencesChanged();require(preferences,"preference changes are exposed to the application");
    require(panel.panicButton().getButtonText()=="Release all notes","panic action is visible");
    int scale=100;for(auto size:std::initializer_list<juce::Point<int>>{{820,560},{1230,840},{1640,1120}}){panel.setSize(size.x,size.y);require(panel.layoutValid(),"scaled layout keeps controls visible");auto image=panel.createComponentSnapshot(panel.getLocalBounds());require(image.isValid(),"recorder DPI snapshot renders");juce::FileOutputStream stream(juce::File::getCurrentWorkingDirectory().getChildFile("recorder-ui-"+juce::String(scale)+".png"));stream.setPosition(0);stream.truncate();require(juce::PNGImageFormat{}.writeImageToStream(image,stream),"recorder DPI snapshot writes");scale+=50;}
    std::cout<<"PASS: recorder first-run, bilingual controls, preferences and DPI layouts\n";
}
