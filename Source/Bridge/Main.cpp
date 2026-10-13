#include "JuceMidiPorts.h"
#include "MidiBridgeEngine.h"
#include "UI/MainPanel.h"
#include <juce_gui_extra/juce_gui_extra.h>

class BridgeComponent final:public pas::MainPanel,private juce::MidiInputCallback,private juce::Timer{
public:
 BridgeComponent(){addAndMakeVisible(inputBox);addAndMakeVisible(outputBox);inputBox.onChange=[this]{connect();};outputBox.onChange=[this]{connect();};engine.prepare(1000);refresh();startTimer(1000);}
 void resized()override{pas::MainPanel::resized();inputBox.setBounds(310,160,360,38);outputBox.setBounds(310,215,360,38);}
private:
 void timerCallback()override{refresh();engine.advance((std::int64_t)juce::Time::getMillisecondCounterHiRes());setDeviceStatus(engine.loopDetected()?pas::DeviceStatus::FeedbackStopped:engine.running()?pas::DeviceStatus::Connected:pas::DeviceStatus::Disconnected);}
 void refresh(){auto oldIn=inputBox.getText(),oldOut=outputBox.getText();inputBox.clear(juce::dontSendNotification);outputBox.clear(juce::dontSendNotification);int n=1;for(auto d:juce::MidiInput::getAvailableDevices())inputBox.addItem(d.name,n++);n=1;for(auto d:juce::MidiOutput::getAvailableDevices())outputBox.addItem(d.name,n++);inputBox.setText(oldIn,juce::dontSendNotification);outputBox.setText(oldOut,juce::dontSendNotification);}
 void connect(){input.reset();output.device.reset();auto ins=juce::MidiInput::getAvailableDevices(),outs=juce::MidiOutput::getAvailableDevices();int i=inputBox.getSelectedItemIndex(),o=outputBox.getSelectedItemIndex();if(i<0||o<0||i>=ins.size()||o>=outs.size())return;inputRef.id=ins[i].identifier;output.id=outs[o].identifier;output.device=juce::MidiOutput::openDevice(output.id);input=juce::MidiInput::openDevice(inputRef.id,this);if(input&&output.device&&engine.start(inputRef,output))input->start();}
 void handleIncomingMidiMessage(juce::MidiInput*,const juce::MidiMessage& m)override{auto*r=m.getRawData();if(m.getRawDataSize()==3)engine.handleIncoming((std::int64_t)juce::Time::getMillisecondCounterHiRes(),{r[0],r[1],r[2]});}
 juce::ComboBox inputBox,outputBox;std::unique_ptr<juce::MidiInput> input;pas::JuceInputRef inputRef;pas::JuceOutputRef output;pas::MidiBridgeEngine engine;
};
class BridgeApplication final:public juce::JUCEApplication{public:const juce::String getApplicationName()override{return "Piano Auto Sustain Bridge";}const juce::String getApplicationVersion()override{return "1.0.0";}void initialise(const juce::String&)override{window=std::make_unique<Window>();}void shutdown()override{window.reset();}class Window:public juce::DocumentWindow{public:Window():DocumentWindow("Piano Auto Sustain Bridge",juce::Colour(0xff101927),allButtons){setUsingNativeTitleBar(true);setContentOwned(new BridgeComponent(),true);centreWithSize(760,500);setResizable(true,true);setVisible(true);}void closeButtonPressed()override{juce::JUCEApplication::getInstance()->systemRequestedQuit();}};std::unique_ptr<Window>window;};
START_JUCE_APPLICATION(BridgeApplication)
