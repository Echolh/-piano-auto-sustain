#pragma once
#include "PluginProcessor.h"
#include "NativeMidi/Appearance.h"
#include "ProductManual.h"
#include <array>
#include <cmath>

class PianoAutoSustainEditor final : public juce::AudioProcessorEditor, private juce::Timer {
    using State=juce::AudioProcessorValueTreeState;
    PianoAutoSustainProcessor& processor;
    std::array<juce::TextButton,6> buttons; // theme, language, play, advanced, diagnostics, guide
    std::array<juce::TextButton,7> actions; // repedal, release, reset, bass -/+, tail -/+
    std::array<juce::ToggleButton,3> toggles;
    std::array<juce::Label,5> labels,values;
    std::array<juce::Slider,5> linkedSliders;
    juce::ComboBox mode;
    std::array<std::unique_ptr<State::ButtonAttachment>,3> toggleLinks;
    std::array<std::unique_ptr<State::SliderAttachment>,5> valueLinks;
    std::unique_ptr<State::ComboBoxAttachment> modeLink;
    bool chinese=true;int theme=0,page=0;float pedalPosition=0;
    juce::String tr(const wchar_t* zh,const char* en)const{return chinese?juce::String(zh):juce::String(en);}
    const PedalTheme& palette()const{return pedalThemes[juce::jlimit(0,pedalThemeCount-1,theme)];}
    static juce::Colour c(std::uint32_t rgb){return juce::Colour(0xff000000u|rgb);}
    void text(juce::Graphics& g,const juce::String& s,int x,int y,int w,int h,float size,juce::Colour ink,bool bold=false)const{
        g.setColour(ink);g.setFont(juce::FontOptions(size).withStyle(bold?"Bold":"Regular"));g.drawText(s,x,y,w,h,juce::Justification::centredLeft,false);
    }
    void setNumber(const char* id,int step){if(auto* p=processor.parameters.getParameter(id)){auto n=p->convertFrom0to1(p->getValue());p->setValueNotifyingHost(p->convertTo0to1(n+static_cast<float>(step)));}}
    void refresh(){
        const auto& p=palette();buttons[0].setButtonText(tr(L"配色: ","Theme: ")+juce::String(chinese?p.chinese:p.english));
        buttons[1].setButtonText(chinese?L"中文 / EN":L"EN / 中文");
        const wchar_t* bc[]={L"演奏",L"高级设置",L"诊断",L"说明书"};const char* be[]={"Play","Advanced","Diagnostics","Guide"};
        for(int i=0;i<4;++i)buttons[i+2].setButtonText(tr(bc[i],be[i]));
        const wchar_t* ac[]={L"换踏一次",L"释放踏板",L"恢复默认"};const char* ae[]={"Repedal","Release pedal","Reset defaults"};
        for(int i=0;i<3;++i)actions[i].setButtonText(tr(ac[i],ae[i]));
        actions[3].setButtonText("-");actions[4].setButtonText("+");actions[5].setButtonText("-");actions[6].setButtonText("+");
        const wchar_t* tc[]={L"启用自动踏板",L"和弦保护",L"忽略八度变化"};const char* te[]={"Automatic pedal","Chord protection","Ignore octaves"};
        for(int i=0;i<3;++i){toggles[i].setButtonText(tr(tc[i],te[i]));toggles[i].setColour(juce::ToggleButton::textColourId,c(p.ink));toggles[i].setColour(juce::ToggleButton::tickColourId,c(p.accent));}
        const wchar_t* lc[]={L"低音识别上限",L"和弦收集",L"抬踩间隔",L"换踏保护",L"松键后的延音"};
        const char* le[]={"Bass upper limit","Chord window","Pedal gap","Change guard","Release tail"};
        for(int i=0;i<5;++i){labels[i].setText(tr(lc[i],le[i]),juce::dontSendNotification);labels[i].setColour(juce::Label::textColourId,c(p.muted));values[i].setColour(juce::Label::textColourId,c(p.ink));values[i].setColour(juce::Label::backgroundColourId,c(p.surface));values[i].setColour(juce::Label::outlineColourId,c(p.border));bool visible=page==(i==0||i==4?0:1);labels[i].setVisible(visible);values[i].setVisible(visible);}
        for(auto& b:buttons){b.setColour(juce::TextButton::buttonColourId,c(p.control));b.setColour(juce::TextButton::textColourOffId,c(p.ink));}
        for(auto& b:actions){b.setColour(juce::TextButton::buttonColourId,c(p.control));b.setColour(juce::TextButton::textColourOffId,c(p.ink));}
        for(int i=0;i<3;++i)buttons[i+2].setColour(juce::TextButton::buttonColourId,c(page==i?p.active:p.control));
        for(int i=3;i<7;++i)actions[i].setVisible(page==0);
        const int selected=mode.getSelectedId();mode.clear(juce::dontSendNotification);mode.addItem(tr(L"延长音符","Note Hold"),1);mode.addItem(tr(L"CC64 踏板（推荐）","CC64 pedal (recommended)"),2);mode.setSelectedId(selected,juce::dontSendNotification);mode.setColour(juce::ComboBox::backgroundColourId,c(p.surface));mode.setColour(juce::ComboBox::textColourId,c(p.ink));mode.setVisible(page==1);
        toggles[1].setVisible(page==0);toggles[2].setVisible(page==0);repaint();
    }
    void timerCallback()override{float target=processor.uiPedalDown.load()?1.0f:0.0f,before=pedalPosition;pedalPosition+=(target-pedalPosition)*.28f;if(std::abs(target-pedalPosition)<.002f)pedalPosition=target;if(before!=pedalPosition||page==2)repaint();}
    void drawPedal(juce::Graphics& g)const{
        float drop=19*pedalPosition;g.setColour(juce::Colours::black.withAlpha(.28f));g.fillEllipse(68.f,238.f,151.f,24.f);
        juce::Path base;base.startNewSubPath(68,211);base.lineTo(116,183);base.lineTo(222,237);base.lineTo(168,269);base.closeSubPath();g.setColour(juce::Colour(0xff15181d));g.fillPath(base);g.setColour(juce::Colour(0xff414955));g.strokePath(base,juce::PathStrokeType(2));
        g.setColour(juce::Colour(0xff464d56));g.fillRoundedRectangle(78.f,183.f,45.f,28.f,10.f);
        auto outline=[&](float d){juce::Path path;path.startNewSubPath(92,177+d);path.lineTo(117,165+d);path.cubicTo(126,161+d,192,201+drop+d,206,215+drop+d);path.cubicTo(215,224+drop+d,188,241+drop+d,174,243+drop+d);path.cubicTo(160,245+drop+d,97,199+d,90,188+d);path.cubicTo(86,182+d,88,179+d,92,177+d);path.closeSubPath();return path;};
        g.setColour(juce::Colour(0xff48525f));g.fillPath(outline(8));auto top=outline(0);g.setGradientFill(juce::ColourGradient(juce::Colour(0xffd8e0e9),95,165,juce::Colour(0xff778797),179,245+drop,false));g.fillPath(top);g.setColour(juce::Colour(0xffe6edf6));g.strokePath(top,juce::PathStrokeType(1.3f));
        juce::Path shine;shine.startNewSubPath(112,176);shine.cubicTo(138,189+drop*.3f,174,209+drop*.8f,191,224+drop);g.setColour(juce::Colours::white.withAlpha(.7f));g.strokePath(shine,juce::PathStrokeType(3));g.setColour(juce::Colour(0xff58687a));for(int i=0;i<4;++i){float x=139.f+i*7.f,y=213.f+i*4.f+drop;g.drawLine(x,y,x+21,y-11,1.4f);}g.setColour(c(pedalPosition>.5f?palette().accent:palette().border));g.fillEllipse(78.f,218.f,7.f,4.f);
    }
public:
    explicit PianoAutoSustainEditor(PianoAutoSustainProcessor& p):AudioProcessorEditor(p),processor(p){
        const juce::Rectangle<int> br[]={{452,24,164,32},{630,24,104,32},{284,88,96,34},{388,88,138,34},{534,88,94,34},{636,88,98,34}};
        for(int i=0;i<6;++i){addAndMakeVisible(buttons[i]);buttons[i].setBounds(br[i]);}
        const juce::Rectangle<int> ar[]={{28,410,218,40},{264,410,218,40},{562,410,172,40},{306,244,54,30},{390,244,54,30},{538,244,54,30},{654,244,54,30}};
        for(int i=0;i<7;++i){addAndMakeVisible(actions[i]);actions[i].setBounds(ar[i]);}
        const char* tids[]={"enabled","chordProtection","octave"};for(int i=0;i<3;++i){addAndMakeVisible(toggles[i]);toggleLinks[i]=std::make_unique<State::ButtonAttachment>(p.parameters,tids[i],toggles[i]);}
        addAndMakeVisible(mode);mode.addItem("Note Hold",1);mode.addItem("CC64 pedal",2);modeLink=std::make_unique<State::ComboBoxAttachment>(p.parameters,"mode",mode);mode.setBounds(284,351,450,32);
        const char* ids[]={"split","group","gap","guard","tail"};for(int i=0;i<5;++i){addAndMakeVisible(labels[i]);addAndMakeVisible(values[i]);addChildComponent(linkedSliders[i]);linkedSliders[i].setVisible(false);values[i].setEditable(true,true,false);values[i].setJustificationType(juce::Justification::centred);valueLinks[i]=std::make_unique<State::SliderAttachment>(p.parameters,ids[i],linkedSliders[i]);linkedSliders[i].onValueChange=[this,i]{values[i].setText(juce::String(static_cast<int>(linkedSliders[i].getValue())),juce::dontSendNotification);};values[i].onTextChange=[this,i]{linkedSliders[i].setValue(values[i].getText().getIntValue(),juce::sendNotificationSync);};values[i].setText(juce::String(static_cast<int>(linkedSliders[i].getValue())),juce::dontSendNotification);}
        buttons[0].onClick=[this]{theme=(theme+1)%pedalThemeCount;refresh();};buttons[1].onClick=[this]{chinese=!chinese;refresh();};for(int i=0;i<3;++i)buttons[i+2].onClick=[this,i]{page=i;refresh();};
        buttons[5].onClick=[this]{juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Piano Auto Sustain 1.0",juce::String(pas::manual(chinese,false,PAS_AU_MIDI!=0).c_str()),"OK",this);};
        actions[0].onClick=[this]{processor.uiAction=1;};actions[1].onClick=[this]{processor.uiAction=2;};actions[2].onClick=[this]{const char* ids[]={"enabled","chordProtection","octave","split","group","gap","guard","tail","mode"};const float defaults[]={1,1,1,59,40,35,160,2400,1};for(int i=0;i<9;++i)if(auto* q=processor.parameters.getParameter(ids[i]))q->setValueNotifyingHost(q->convertTo0to1(defaults[i]));};
        actions[3].onClick=[this]{setNumber("split",-1);};actions[4].onClick=[this]{setNumber("split",1);};actions[5].onClick=[this]{setNumber("tail",-100);};actions[6].onClick=[this]{setNumber("tail",100);};
        setSize(760,500);refresh();startTimerHz(30);
    }
    void paint(juce::Graphics& g)override{
        const auto& p=palette();g.setGradientFill(juce::ColourGradient(c(p.top),0,0,c(p.bottom),760,500,false));g.fillAll();
        text(g,"Piano Auto Sustain",28,17,416,38,26,c(p.ink),true);text(g,tr(L"钢琴自动延音","Automatic piano sustain"),29,55,450,23,13,c(p.muted));text(g,tr(L"实时踏板","Live pedal"),46,102,160,24,14,c(p.muted));drawPedal(g);
        const bool down=processor.uiPedalDown.load();text(g,down?tr(L"延音中","Sustaining"):tr(L"已释放","Released"),91,276,135,29,18,c(down?p.accent:p.ink),true);
        if(page==0){g.setColour(c(p.surface));g.fillRoundedRectangle(284.f,138.f,216.f,148.f,18.f);g.fillRoundedRectangle(516.f,138.f,218.f,148.f,18.f);int n=static_cast<int>(processor.parameters.getRawParameterValue("split")->load());const char* names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};text(g,juce::String(names[n%12])+juce::String(n/12-1),444,188,48,31,14,c(p.accent));text(g,"ms",692,188,34,31,13,c(p.muted));text(g,tr(L"同和弦分解伴奏保持延音，和声变化时换踏。","Keep chord tones ringing; repedal on harmony changes."),284,351,448,35,12,c(p.muted));}
        else if(page==1){const wchar_t* zh[]={L"把先后落下的和弦音一起判断",L"换踏时短暂释放，让旧和声消退",L"避免连续低音触发过密的换踏"};const char* en[]={"Group notes played close together","Release briefly between pedal changes","Minimum time between pedal changes"};for(int i=0;i<3;++i){text(g,tr(zh[i],en[i]),284,172+i*70,300,24,12,c(p.muted));text(g,"ms",715,150+i*70,22,30,12,c(p.muted));}}
        else {text(g,tr(L"MIDI 信号诊断","MIDI signal diagnostics"),284,145,430,30,19,c(p.ink),true);text(g,tr(L"输入音符","Notes")+"  "+juce::String(processor.uiNotes.load()),284,195,420,28,15,c(p.muted));text(g,"CC64  "+juce::String(processor.uiCc64.load()),284,233,420,28,15,c(p.muted));text(g,tr(L"已输出事件不代表音源已响应。","Output does not confirm instrument response."),284,345,450,38,12,c(p.accent));}
        g.setColour(c(p.border));g.drawLine(28.f,393.f,734.f,393.f);text(g,tr(L"音符 ","Notes ")+juce::String(processor.uiNotes.load())+"   /   CC64 "+juce::String(processor.uiCc64.load()),28,463,350,23,12,c(p.muted));text(g,PAS_AU_MIDI?"AU MIDI FX / 1.0":"VST3 / 1.0",393,463,160,23,11,c(p.muted));g.setColour(c(p.accent));for(int i=0;i<4;++i){float h=6.f+(i==1?12.f:i==2?7.f:0.f);g.drawLine(589.f+i*5,475-h/2,589.f+i*5,475+h/2,2.2f);}text(g,"Amu music",617,460,125,29,17,c(p.ink),true);
    }
    void resized()override{
        toggles[0].setBounds(28,338,218,38);toggles[1].setBounds(284,306,216,38);toggles[2].setBounds(516,306,218,38);
        labels[0].setBounds(306,146,184,27);values[0].setBounds(306,184,126,42);labels[4].setBounds(538,146,180,27);values[4].setBounds(538,184,148,42);
        for(int i=1;i<=3;++i){labels[i].setBounds(284,145+(i-1)*70,280,24);values[i].setBounds(594,151+(i-1)*70,116,32);}
    }
};
