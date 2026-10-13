#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
namespace pas {
struct Theme { const char* id; juce::Colour background,panel,accent,text,muted; };
inline const std::array<Theme,5>& themes(){static const std::array<Theme,5> value{{
 {"ice",juce::Colour(0xff101927),juce::Colour(0xff1d2a3d),juce::Colour(0xff79bdff),juce::Colour(0xfff4f7fb),juce::Colour(0xff9bacc2)},
 {"obsidian",juce::Colour(0xff111111),juce::Colour(0xff222222),juce::Colour(0xffd5aa64),juce::Colour(0xfff4efe7),juce::Colour(0xffa49a8b)},
 {"forest",juce::Colour(0xff0f1d19),juce::Colour(0xff1d332b),juce::Colour(0xff6bc49a),juce::Colour(0xffedf7f1),juce::Colour(0xff9bb9aa)},
 {"dusk",juce::Colour(0xff1c1727),juce::Colour(0xff30253f),juce::Colour(0xffc18cff),juce::Colour(0xfff7f1ff),juce::Colour(0xffb6a3c8)},
 {"pearl",juce::Colour(0xffe9edf3),juce::Colour(0xfff8fafc),juce::Colour(0xff3979bd),juce::Colour(0xff182334),juce::Colour(0xff637188)}
}};return value;}
}
