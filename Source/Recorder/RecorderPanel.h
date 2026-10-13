#pragma once

#include "RecorderStatus.h"
#include "UI/MainPanel.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace pas {
enum class FirstRunState { ChooseKeyboard, InstallMidiRuntime, ChooseCompatibilityOutput, SelectHostInput, Ready };

class RecorderPanel : public juce::Component {
public:
    RecorderPanel();
    void paint(juce::Graphics&) override;
    void resized() override;
    void setLanguage(Language);
    void setTheme(int value){theme=juce::jlimit(0,4,value);repaint();}
    void setStatus(RecorderStatus value){recorderStatus=value;updateText();repaint();}
    void setCompatibilityRequired(bool value){compatibilityRequired=value;updateText();repaint();}
    FirstRunState firstRunState() const;
    juce::String titleText() const;
    juce::String connectionText() const;
    void setStartWithWindows(bool value){startOnLogin.setToggleState(value,juce::dontSendNotification);}
    bool startWithWindows() const{return startOnLogin.getToggleState();}
    void setCloseToTray(bool value){trayOnClose.setToggleState(value,juce::dontSendNotification);}
    bool closeToTray() const{return trayOnClose.getToggleState();}
    juce::TextButton& panicButton(){return panic;}
    juce::TextButton& chooseKeyboardButton(){return chooseKeyboard;}
    std::function<void()> onChooseKeyboard;
    std::function<void()> onChooseOutput;
    std::function<void()> onHostHelp;
    std::function<void()> onPreferencesChanged;
    bool layoutValid() const;
private:
    void updateText();
    Language language=Language::Chinese;int theme=0;RecorderStatus recorderStatus;bool compatibilityRequired=false;
    juce::TextButton chooseKeyboard,openHostHelp,panic;
    juce::ToggleButton startOnLogin,trayOnClose;
};
}
