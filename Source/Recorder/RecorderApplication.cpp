#include "JuceVirtualPortBackend.h"
#include "RecorderConfig.h"
#include "RecorderController.h"
#include "RecorderLaunchPolicy.h"
#include "RecorderPanel.h"
#include "WindowsMidiServicesBackend.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace pas {
class RecorderApplication final : public juce::JUCEApplication, private juce::MidiInputCallback, private juce::Timer {
public:
    const juce::String getApplicationName() override{return "Piano Auto Sustain Recorder";}
    const juce::String getApplicationVersion() override{return "1.0.0";}
    void initialise(const juce::String&)override
    {
        bridge.prepare(1000.0);midi2Api=createSystemWindowsMidiServicesApi();outputApi=createSystemJuceMidiOutputApi();midi2Backend=std::make_unique<WindowsMidiServicesBackend>(*midi2Api);compatibilityBackend=std::make_unique<JuceVirtualPortBackend>(*outputApi);controller=std::make_unique<RecorderController>(bridge,*midi2Backend,compatibilityBackend.get());
        const auto configPath=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("Amu music/Piano Auto Sustain/recorder.json");configStore=std::make_unique<RecorderConfigStore>(std::filesystem::path(configPath.getFullPathName().toWideCharPointer()));config=configStore->load();compatibilityBackend->select(config.outputIdentifier);
        refreshInputList();controller->refreshDevices(availableInputs);if(!config.inputIdentifier.empty())controller->selectInput(config.inputIdentifier);
        if(config.outputIdentifier.empty()){std::string exact;int count=0;for(const auto& d:outputApi->availableOutputs())if(d.isVirtual&&d.name==kRecorderEndpointName){exact=d.identifier;++count;}if(count==1){compatibilityBackend->select(exact);config.outputIdentifier=exact;}}
        openSelectedInput();controller->start();window=std::make_unique<Window>([this]{handleWindowClose();});wireActions();updatePanel();config.inputIdentifier=controller->selectedInput();configStore->save(config);applyStartupPreference();startTimerHz(5);
    }
    void shutdown()override{stopTimer();if(controller)controller->stop();midiInput.reset();tray.reset();window.reset();controller.reset();compatibilityBackend.reset();midi2Backend.reset();outputApi.reset();midi2Api.reset();}
    class Window final:public juce::DocumentWindow{
    public:explicit Window(std::function<void()> close):DocumentWindow("Piano Auto Sustain Recorder",juce::Colour(0xff101927),allButtons),closeRequested(std::move(close)){content=new RecorderPanel();setUsingNativeTitleBar(true);setContentOwned(content,true);centreWithSize(820,560);setResizable(true,true);setVisible(false);}RecorderPanel& panel(){return *content;}void closeButtonPressed()override{if(closeRequested)closeRequested();}private:RecorderPanel* content=nullptr;std::function<void()> closeRequested;};
    class TrayIcon final:public juce::SystemTrayIconComponent{public:std::function<void()> onRestore;void mouseDown(const juce::MouseEvent&)override{if(onRestore)onRestore();}};
private:
    void refreshInputList(){availableInputs.clear();for(const auto& d:juce::MidiInput::getAvailableDevices())availableInputs.push_back({d.identifier.toStdString(),d.name.toStdString(),true,false});}
    void wireActions(){auto& panel=window->panel();panel.onChooseKeyboard=[this]{showKeyboardMenu();};panel.onChooseOutput=[this]{showOutputMenu();};panel.onHostHelp=[] {juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Piano Auto Sustain",juce::String::fromUTF8(u8"快速模式：在 Cubase 的 MIDI 插入中选择 Piano Auto Sustain Cubase。\n\n录制 CC64：启动本录制器，并把乐器轨 MIDI 输入选择为 Piano Auto Sustain；该轨道不要再启用 MIDI 插入。"));};panel.panicButton().onClick=[this]{if(controller)controller->panic();};panel.setStartWithWindows(config.startWithWindows);panel.setCloseToTray(config.closeToTray);panel.onPreferencesChanged=[this]{config.startWithWindows=window->panel().startWithWindows();config.closeToTray=window->panel().closeToTray();configStore->save(config);applyStartupPreference();};}
    void applyStartupPreference(){
#if JUCE_WINDOWS
        const auto key="HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run\\Piano Auto Sustain";
        if(config.startWithWindows){const auto command="\""+juce::File::getSpecialLocation(juce::File::currentExecutableFile).getFullPathName()+"\"";juce::WindowsRegistry::setValue(key,command);}
        else juce::WindowsRegistry::deleteValue(key);
#endif
    }
    void ensureTray(){if(tray)return;tray=std::make_unique<TrayIcon>();juce::Image icon(juce::Image::ARGB,32,32,true);juce::Graphics g(icon);g.setColour(juce::Colour(0xff70b7ff));g.fillEllipse(5.0f,5.0f,22.0f,22.0f);tray->setIconImage(icon,icon);tray->setIconTooltip("Piano Auto Sustain Recorder");tray->onRestore=[this]{window->setVisible(true);window->toFront(true);};}
    void handleWindowClose(){config.closeToTray=window->panel().closeToTray();configStore->save(config);if(!config.closeToTray){systemRequestedQuit();return;}ensureTray();window->setVisible(false);}
    void applyWindowPolicy(const RecorderStatus& status){if(shouldShowRecorderWindow(config,status)){window->setVisible(true);window->toFront(true);}else{ensureTray();window->setVisible(false);}}
    void showKeyboardMenu(){refreshInputList();juce::PopupMenu menu;for(int i=0;i<(int)availableInputs.size();++i)menu.addItem(i+1,juce::String::fromUTF8(availableInputs[(size_t)i].name.c_str()));menu.showMenuAsync({},[this,items=availableInputs](int result){if(result<1||result>(int)items.size()||!controller)return;controller->stop();midiInput.reset();controller->refreshDevices(items);if(controller->selectInput(items[(size_t)(result-1)].identifier)){config.inputIdentifier=controller->selectedInput();openSelectedInput();controller->start();configStore->save(config);updatePanel();}});}
    void showOutputMenu(){const auto outputs=outputApi->availableOutputs();juce::PopupMenu menu;std::vector<DeviceInfo> virtualOutputs;for(const auto& item:outputs)if(item.isVirtual&&!item.isInput){virtualOutputs.push_back(item);menu.addItem((int)virtualOutputs.size(),juce::String::fromUTF8(item.name.c_str()));}menu.showMenuAsync({},[this,items=std::move(virtualOutputs)](int result){if(result<1||result>(int)items.size()||!controller)return;controller->stop();compatibilityBackend->select(items[(size_t)(result-1)].identifier);config.outputIdentifier=items[(size_t)(result-1)].identifier;controller->start();configStore->save(config);updatePanel();});}
    void updatePanel(){if(!window||!controller)return;const auto status=controller->status();window->panel().setStatus(status);window->panel().setCompatibilityRequired(status.error==RecorderError::BackendUnavailable);applyWindowPolicy(status);}
    void openSelectedInput(){if(!controller||controller->selectedInput().empty())return;midiInput=juce::MidiInput::openDevice(juce::String(controller->selectedInput()),this);if(midiInput)midiInput->start();}
    void handleIncomingMidiMessage(juce::MidiInput*,const juce::MidiMessage& message)override{const auto* data=message.getRawData();if(message.getRawDataSize()<1)return;const int d1=message.getRawDataSize()>1?data[1]:0;const int d2=message.getRawDataSize()>2?data[2]:0;controller->handleIncoming(static_cast<std::int64_t>(juce::Time::getMillisecondCounterHiRes()),{data[0],d1,d2});}
    void timerCallback()override{if(window&&controller){controller->poll();updatePanel();}}
    MidiBridgeEngine bridge;std::vector<DeviceInfo> availableInputs;std::unique_ptr<WindowsMidiServicesApi> midi2Api;std::unique_ptr<JuceMidiOutputApi> outputApi;std::unique_ptr<WindowsMidiServicesBackend> midi2Backend;std::unique_ptr<JuceVirtualPortBackend> compatibilityBackend;std::unique_ptr<RecorderController> controller;std::unique_ptr<juce::MidiInput> midiInput;std::unique_ptr<RecorderConfigStore> configStore;RecorderConfig config;std::unique_ptr<TrayIcon> tray;std::unique_ptr<Window> window;
};
}
START_JUCE_APPLICATION(pas::RecorderApplication)
