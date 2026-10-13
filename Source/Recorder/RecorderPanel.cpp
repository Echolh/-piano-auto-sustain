#include "RecorderPanel.h"
#include <array>

namespace pas {
RecorderPanel::RecorderPanel()
{
    const std::array<juce::Component*,5> controls{{&chooseKeyboard,&openHostHelp,&panic,&startOnLogin,&trayOnClose}};
    for(auto* component:controls)addAndMakeVisible(component);
    chooseKeyboard.onClick=[this]{if(firstRunState()==FirstRunState::ChooseCompatibilityOutput){if(onChooseOutput)onChooseOutput();}else if(onChooseKeyboard)onChooseKeyboard();};
    openHostHelp.onClick=[this]{if(onHostHelp)onHostHelp();};
    startOnLogin.onClick=[this]{if(onPreferencesChanged)onPreferencesChanged();};
    trayOnClose.onClick=[this]{if(onPreferencesChanged)onPreferencesChanged();};
    setSize(820,560);updateText();
}

FirstRunState RecorderPanel::firstRunState()const
{
    if(compatibilityRequired)return FirstRunState::ChooseCompatibilityOutput;
    if(recorderStatus.error==RecorderError::AmbiguousInput||recorderStatus.error==RecorderError::InputUnavailable)return FirstRunState::ChooseKeyboard;
    if(recorderStatus.error==RecorderError::RuntimeMissing||recorderStatus.error==RecorderError::TransportUnavailable)return FirstRunState::InstallMidiRuntime;
    if(recorderStatus.connection==ConnectionState::Ready&&recorderStatus.inputConnected&&recorderStatus.endpointPublished&&recorderStatus.clientConnected)return FirstRunState::Ready;
    return FirstRunState::SelectHostInput;
}

juce::String RecorderPanel::titleText()const{return language==Language::Chinese?juce::String::fromUTF8(u8"演奏录制器"):juce::String("Performance Recorder");}
juce::String RecorderPanel::connectionText()const
{
    const auto state=firstRunState();if(language==Language::Chinese){
        if(state==FirstRunState::Ready)return juce::String::fromUTF8(u8"已就绪，可以弹奏和录制");
        if(state==FirstRunState::ChooseKeyboard)return juce::String::fromUTF8(u8"请选择你的 MIDI 键盘");
        if(state==FirstRunState::InstallMidiRuntime)return juce::String::fromUTF8(u8"需要 MIDI 运行环境或兼容输出");
        if(state==FirstRunState::ChooseCompatibilityOutput)return juce::String::fromUTF8(u8"请选择已有的虚拟 MIDI 输出");
        return juce::String::fromUTF8(u8"在宿主中选择 Piano Auto Sustain 输入");
    }
    if(state==FirstRunState::Ready)return "Ready to play and record";
    if(state==FirstRunState::ChooseKeyboard)return "Choose your MIDI keyboard";
    if(state==FirstRunState::InstallMidiRuntime)return "MIDI runtime or compatible output required";
    if(state==FirstRunState::ChooseCompatibilityOutput)return "Choose an existing virtual MIDI output";
    return "Select Piano Auto Sustain as the host input";
}

void RecorderPanel::setLanguage(Language value){language=value;updateText();repaint();}
void RecorderPanel::updateText()
{
    const bool zh=language==Language::Chinese;
    const bool output=firstRunState()==FirstRunState::ChooseCompatibilityOutput;
    chooseKeyboard.setButtonText(output?(zh?juce::String::fromUTF8(u8"选择虚拟输出"):"Choose virtual output"):(zh?juce::String::fromUTF8(u8"选择键盘"):"Choose keyboard"));
    openHostHelp.setButtonText(zh?juce::String::fromUTF8(u8"宿主连接说明"):"Host setup");
    panic.setButtonText(zh?juce::String::fromUTF8(u8"释放全部音符"):"Release all notes");
    startOnLogin.setButtonText(zh?juce::String::fromUTF8(u8"登录后启动"):"Start with Windows");
    trayOnClose.setButtonText(zh?juce::String::fromUTF8(u8"关闭到托盘"):"Close to tray");
}

void RecorderPanel::paint(juce::Graphics& g)
{
    const auto& t=themes()[theme];g.fillAll(t.background);const float s=getWidth()/820.0f;
    g.setColour(t.text);g.setFont(juce::FontOptions(30*s).withStyle("Bold"));g.drawText("Piano Auto Sustain",juce::Rectangle<int>(40*s,30*s,520*s,48*s),juce::Justification::left);
    g.setColour(t.muted);g.setFont(16*s);g.drawText(titleText(),juce::Rectangle<int>(40*s,78*s,500*s,30*s),juce::Justification::left);
    auto card=juce::Rectangle<float>(40*s,130*s,740*s,210*s);g.setColour(t.panel);g.fillRoundedRectangle(card,20*s);
    const bool ready=firstRunState()==FirstRunState::Ready;g.setColour(ready?t.accent:juce::Colour(0xffffc86b));g.fillEllipse(70*s,174*s,18*s,18*s);
    g.setColour(t.text);g.setFont(juce::FontOptions(24*s).withStyle("Bold"));g.drawText(connectionText(),juce::Rectangle<int>(105*s,158*s,625*s,54*s),juce::Justification::left);
    g.setColour(t.muted);g.setFont(15*s);g.drawFittedText(language==Language::Chinese?juce::String::fromUTF8(u8"录制后的 MIDI 将同时包含音符和自动生成的 CC64 延音数据。"):juce::String("Recorded MIDI contains both notes and generated CC64 sustain data."),juce::Rectangle<int>(70*s,225*s,660*s,60*s),juce::Justification::topLeft,2);
    g.setColour(t.accent);g.setFont(14*s);g.drawText("Amu music  |  1.0.0",juce::Rectangle<int>(550*s,510*s,230*s,28*s),juce::Justification::right);
}

void RecorderPanel::resized()
{
    const float s=getWidth()/820.0f;auto set=[s](juce::Component& c,int x,int y,int w,int h){c.setBounds(juce::roundToInt(x*s),juce::roundToInt(y*s),juce::roundToInt(w*s),juce::roundToInt(h*s));};
    set(chooseKeyboard,65,285,190,42);set(openHostHelp,270,285,190,42);set(panic,475,285,255,42);set(startOnLogin,145,370,240,46);set(trayOnClose,435,370,240,46);
}

bool RecorderPanel::layoutValid()const
{
    const std::array<const juce::Component*,5> controls{{&chooseKeyboard,&openHostHelp,&panic,&startOnLogin,&trayOnClose}};
    for(auto* component:controls)if(component->getBounds().isEmpty()||!getLocalBounds().contains(component->getBounds()))return false;
    return true;
}
}
