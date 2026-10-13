#pragma once
#include <string>
namespace pas {
inline std::wstring manual(bool chinese, bool cubase, bool au=false) {
    std::wstring result = chinese ? L"Piano Auto Sustain 1.0 · 使用说明\n\n" : L"Piano Auto Sustain 1.0 · User guide\n\n";
    if(cubase) result += chinese
        ? L"连接与录制\n轨道输入选实体键盘，打开监听与录音准备；在 MIDI 插入加载本插件。\n要录入 CC64：把虚线分隔线拖到插件下面，再开始新录音。\n松键后等尾音结束再停止；在钢琴卷帘 CC64 通道查看踏板数据。\n保存工程保留设置。不需要 Recorder 或虚拟 MIDI 端口。\n\n"
        : L"Connection and recording\nSelect your keyboard input; enable Monitor and Record Enable. Add this MIDI insert.\nTo capture CC64, drag Cubase's dotted divider BELOW the plugin before recording.\nLet the tail finish before stopping; inspect CC64 in the Key Editor.\nSave your project. No Recorder or virtual MIDI port is needed.\n\n";
    else if(au) result += chinese ? L"AU MIDI FX 连接\n在 Logic 软件乐器轨的 MIDI FX 插槽加载本插件，Instrument 插槽加载钢琴。\n本插件不发声；录制生成的 CC64 由宿主的 MIDI FX 录制路径决定。\n已有踏板数据时可旁路插件，保留原演奏。\n\n" : L"AU MIDI FX routing\nInsert this plugin in Logic MIDI FX, before the piano in the Instrument slot.\nThis plugin makes no sound. Capturing generated CC64 depends on the host recording path.\nBypass when replaying an existing pedal performance.\n\n";
    else result += chinese
        ? L"VST3 连接与录制\n建立两条乐器轨：处理轨加载本插件，输入选实体键盘；钢琴轨加载钢琴音源，输入选本插件的 MIDI 输出。\n两轨监听都打开；只打开钢琴轨的录音准备，处理轨关闭录音准备。这样只录出一份音符和 CC64。\nStudio One / Fender Studio Pro：钢琴轨键盘图标行选钢琴音源，箭头图标行选 Piano Auto Sustain / MIDI Output 1。处理轨键盘图标行选本插件，箭头图标行选键盘。\nCubase VST3：钢琴轨顶部输入选 Piano Auto Sustain - MIDI Output，乐器选 Kontakt 等钢琴；处理轨输入选键盘，乐器选本插件。钢琴轨不要再加载专用 MIDI 插入。\n录完在钢琴轨查看 CC64；处理轨保留但不要留重复录音。保存为模板，下次直接使用。\n回放已录入 CC64 的钢琴片段，直接送钢琴音源；不要把片段移回处理轨。其他宿主详见随包使用指南。\n\n"
        : L"VST3 connection and recording\nCreate two instrument tracks: processor track receives the hardware keyboard; piano track receives this plugin's MIDI output and hosts the piano instrument.\nMonitor both tracks. Arm only the piano track; leave the processor track unarmed. This records one clip with notes and CC64.\nStudio One / Fender Studio Pro: on the piano track, select the piano in the keyboard-icon row and Piano Auto Sustain / MIDI Output 1 in the arrow-icon row. On the processor track, select this plugin and the keyboard respectively.\nCubase VST3: select Piano Auto Sustain - MIDI Output as the piano track input, and load a piano instrument. The processor track receives the keyboard and hosts this plugin. Do not also add the Cubase MIDI insert.\nInspect CC64 on the recorded piano track. Keep the processor track without duplicate clips. Save as a template.\nReplay recorded CC64 clips directly through the piano; do not move them back to the processor track. See the bundled host guide for other hosts.\n\n";
    result += chinese
        ? L"参数\n低音识别上限：低于此音高的音符参与换踏判断。\n松键后的延音：全部松键后保留踏板的时间，不是延长采样音色本身。\n和弦收集：将短时间内落下的音一起判断。抬踩间隔：换踏时释放多久。\n换踏保护：限制连续换踏；和弦保护、忽略八度可减少分解和弦误换踏。\n\n排查\n无声先检查音源、轨道监听和 MIDI 路由。\nCC64 未录入先检查录制分隔线；旧录音不会自动补写。\n\n合作咨询微信：Amumusic1005"
        : L"Controls\nBass ceiling: notes below this pitch participate in pedal-change detection.\nRelease tail: pedal hold after all keys are released; it does not extend the sample itself.\nChord window: group nearby attacks. Pedal gap: release time when changing pedal.\nChange guard: limit rapid repedaling. Chord protection and Ignore octaves reduce false changes.\n\nTroubleshooting\nNo sound: check the instrument, monitoring and MIDI routing.\nMissing recorded CC64: check the recording path. Existing clips are not rewritten.\n\nCooperation / WeChat: Amumusic1005";
    return result;
}
}
