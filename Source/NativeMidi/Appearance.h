#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

struct PedalTheme {
    const wchar_t* chinese;
    const wchar_t* english;
    std::uint32_t top,bottom,surface,control,active,pressed,ink,muted,accent,border;
};
inline constexpr PedalTheme pedalThemes[]={
    {L"冰蓝",L"Ice blue",0x1C2639,0x0D131D,0x1D2533,0x1E293A,0x26425E,0x40648C,0xECF3FC,0xA4B4C9,0x88C6FF,0x3C506A},
    {L"曜石",L"Obsidian",0x292B30,0x141519,0x303238,0x363941,0x454D59,0x5C6777,0xF1F3F7,0xB5BBC6,0xC5D5EB,0x555E6B},
    {L"香槟",L"Champagne",0x302B27,0x191613,0x37302A,0x40362D,0x5C4830,0x785D3B,0xF7F0E5,0xC4B7A5,0xE7C38B,0x665744},
    {L"松绿",L"Forest",0x20332F,0x101C19,0x283D35,0x2C443B,0x315E4B,0x3D745B,0xEEF7F0,0xACC9B9,0x9CDEB7,0x486C59},
    {L"暮紫",L"Dusk",0x302940,0x191523,0x392F49,0x443555,0x594477,0x75599A,0xF5F0FF,0xC4B5DB,0xCEB4FF,0x68557F},
    {L"月白",L"Pearl",0xF6F8FC,0xE2E8F0,0xFFFFFF,0xDAE3EF,0xC3D7EC,0xADC8E4,0x172C44,0x4C6179,0x275D94,0xAFBED1}
};
inline constexpr int pedalThemeCount=sizeof(pedalThemes)/sizeof(pedalThemes[0]);

// UI-only motion. It never drives MIDI timing or delays CC64 delivery.
class PedalMotion {
public:
    float position=0;
    bool step(bool pressed,float elapsedMs,bool animate=true) {
        float before=position,target=pressed?1.f:0.f;
        if(!animate)position=target;
        else position+=(target-position)*(1.f-std::exp(-std::clamp(elapsedMs,0.f,100.f)/(pressed?42.f:65.f)));
        if(std::abs(target-position)<0.001f)position=target;
        return position!=before;
    }
};
