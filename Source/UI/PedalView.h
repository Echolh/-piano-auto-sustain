#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
namespace pas {class PedalView:public juce::Component{public:void setDown(bool v){down=v;repaint();}void paint(juce::Graphics& g)override{auto r=getLocalBounds().toFloat().reduced(12);g.setColour(juce::Colour(down?0xff79bdff:0xff8995a5));g.fillRoundedRectangle(r,18);g.setColour(juce::Colours::white.withAlpha(.45f));for(int i=1;i<5;++i)g.fillRect(r.getX()+r.getWidth()*.25f,r.getY()+r.getHeight()*(.2f+i*.11f),r.getWidth()*.5f,2.f);}private:bool down=false;};}
