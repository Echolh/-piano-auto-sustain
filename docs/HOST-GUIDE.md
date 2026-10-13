# Piano Auto Sustain 1.0 — Windows 宿主使用指南

本文件随 Windows x64 VST3 **测试包**提供。插件只处理 MIDI 并生成 CC64，不自带钢琴音色；信号必须是「键盘或 MIDI 片段 → Piano Auto Sustain → 钢琴音源 → 声卡」。界面选用默认的「CC64 pedal / 延音踏板」模式。VST3 和 Cubase 专用 MIDI 插入 DLL 是两种不同格式，不能互相改后缀或放入对方的插槽。

## 安装

解压测试包，退出所有宿主，运行 `Install.cmd`，然后重开宿主并扫描 VST3。默认安装到 `C:\Program Files\Common Files\VST3\Piano Auto Sustain.vst3`。需 Windows x64 与支持 VST3 MIDI 输出路由的宿主。`Uninstall.cmd` 只卸载本插件。测试包不含商业授权，也尚未完成各宿主实机验收；请勿作为付费正式版交付。

## REAPER：一条轨弹奏和录制（推荐先试）

1. 在 REAPER 首选项中启用 MIDI 键盘输入。建立一条轨道，选择该键盘作为 MIDI 输入，打开录音准备和输入监听。
2. 右键轨道录音准备按钮，打开 **Track input FX chain / 轨道输入 FX 链**，将 `Piano Auto Sustain` 加入这里；不要先把它放在钢琴的音频效果器后面。
3. 在同一轨道的普通 FX 链加入 Kontakt、Keyscape 等钢琴音源。弹奏时应有声音及延音。
4. 选择 **Record: input (MIDI)** 并录制。输入 FX 在录制前处理输入；录完在 MIDI 编辑器的 CC64/Sustain Pedal 控制器通道核对踏板事件。播放录好的片段时，输入 FX 不会再次处理它。
5. 如果本机 REAPER 不接受该 VST3 作为输入 FX：建「处理轨」加载本插件，建「钢琴轨」加载钢琴；处理轨将 MIDI 发送至钢琴轨并关闭音频发送。处理轨选 **Record: output (MIDI)** 录下处理后的 MIDI；回放这段已处理录音前旁路处理轨上的插件，以免重复生成 CC64。

REAPER 官方手册说明输入 FX 可在录制前处理 MIDI，并提供 Record: output (MIDI)。本插件在 REAPER 的实际录音、回放和 CC64 尚待实机确认：<https://www.reaper.fm/userguide.php>。

## Cakewalk Sonar / Cakewalk by BandLab：两条 MIDI 轨

1. 把插件作为 VST3 软音源插入工程；在插入选项中勾选 **Enable MIDI Output**。若插件能打开却没有 MIDI 端口，在插件窗口的 VST 选项中启用 MIDI Input / Output，重新实例化插件。
2. 新建「键盘输入」MIDI 轨：输入选真实键盘，输出选 `Piano Auto Sustain`，打开 **Input Echo**。这轨无需录制。
3. 新建「钢琴」MIDI 轨：输入选 `Piano Auto Sustain` 的 MIDI 输出，输出选 Kontakt / Keyscape 等钢琴音源，打开 **Input Echo**。为这轨录音准备，然后弹奏录制。
4. 在钢琴轨的 MIDI 编辑器打开 CC64/Sustain Pedal 通道核对数据。回放录音时，停止键盘输入轨的实时输入回送，避免双重音符；钢琴轨直接送音源即可。

Cakewalk 官方说明了软音源 MIDI 输出、接收轨和录制方法，以及旧版的 MIDI 端口开关。具体菜单名称可能随 Sonar/BandLab 版本变化：<https://legacy.cakewalk.com/Documentation?help=SoftSynths.19.html&language=3&product=CakewalkSonar>、<https://legacy.cakewalk.com/Documentation?help=Mixing.40.html&language=3&product=SONAR>。本插件尚待 Cakewalk 实机验证。

## Cubase 15 与较旧 Cubase：使用专用 DLL

Cubase 的 **MIDI 插入**槽使用专用 DLL，不是上面的 VST3。Cubase 15 Windows 已由用户确认实时演奏、录制、播放和 CC64。安装 `01_发布包/01_未授权版` 的 Cubase 包后，在钢琴乐器轨的 MIDI 插入槽选择 Piano Auto Sustain。要把 CC64 录进音符片段，把插件放在录制分隔线**上方**，然后在键盘编辑器打开 CC64 通道检查。

Cubase Pro / Artist 10.5 的官方手册同样有 MIDI 插入与 **Record Output to Track**，但当前 DLL 尚未在 10.5、9.5 或更早版本实测。若插件能被扫描并出现在 MIDI 插入列表，可在该版本启用 Record Output to Track，再逐项测试弹奏、播放和录制。若列表中没有插件，不要把 VST3 放进 MIDI 插入，也不要将专用 DLL 放到 VST3 文件夹。Cubase AI / Elements 的功能集可能没有 MIDI 插入，应先检查本机版本的菜单。

Cubase 15 官方：<https://www.steinberg.help/r/cubase-artist/15.0/en/cubase_nuendo/topics/midi_realtime_parameters_and_effects/midi_realtime_parameters_midi_effects_midi_insert_effect_recording_t.html>。Cubase 10.5 官方手册：<https://archive.steinberg.help/cubase_artist/v10.5/en/Cubase_Artist_10_5_Operation_Manual_en.pdf>。

## 其他 Windows VST3 宿主

### Studio One / Fender Studio Pro 8：两轨连接，一轨录制

2026-10-13：用户在 Fender Studio Pro 8 确认该路由可发声、产生延音，录音截图显示钢琴轨已有 CC64。其他 Studio One 版本仍需实测。

| 设置 | 钢琴轨 | 自动延音处理轨 |
|---|---|---|
| 键盘图标行：乐器输出 | Amu Piano、Kontakt 等钢琴音源 | Piano Auto Sustain / MIDI Input 1 |
| 箭头图标行：MIDI 输入 | Piano Auto Sustain / MIDI Output 1 | 实体键盘，例如 iRig |
| 监听 | 打开 | 打开 |
| 录音准备 | 打开 | 关闭 |

只在钢琴轨录制，生成一份包含音符和 CC64 的片段。处理轨保留供实时弹奏使用，不录制它。若之前两轨都录了，检查钢琴轨 CC64 后删除处理轨的重复片段，保留处理轨及监听。保存为工程模板，之后可直接录音。钢琴轨回放已记录的 CC64 片段时，片段直接进入钢琴音源，不再经过处理插件。

### Cubase 15 通用 VST3：两轨连接

2026-10-13：用户确认两轨接法可用；当前截图未展示 CC64 编辑通道，录制 CC64 仍应按以下步骤核对。

1. 建立「Piano Auto Sustain」乐器轨，乐器选本 VST3，输入选实体键盘，例如 iRig KEYS Pro。监听打开，录音准备关闭。
2. 建立「piano」钢琴乐器轨，乐器选 Kontakt 等钢琴音源并加载音色。左侧顶部的 MIDI 输入从 All MIDI Inputs 改选插件分组下的 `Piano Auto Sustain - MIDI Output`。监听和录音准备都打开。
3. 钢琴轨的 MIDI 插入槽保持空着，不再叠加专用 MIDI 插入版。弹奏确认有声及延音，录制只产生钢琴轨片段。
4. 录完在键盘编辑器查看 CC64 / Sustain，确认有 0 与 127 事件。回放保留片段在钢琴轨，处理轨不要留下重复录音。

两种宿主的信号方向都是「实体键盘 → 自动延音处理轨 → 钢琴轨 → 声卡」。输入选反、两轨都加载处理插件、钢琴没有音色或监听关闭，都会导致异常。

- Ableton Live：处理轨加载本插件；钢琴轨 `MIDI From` 选处理轨及插件输出，监听设为 In。接收轨录制处理后的 MIDI。官方：<https://help.ableton.com/hc/en-us/articles/209070189-Accessing-the-MIDI-output-of-a-VST-plug-in>。
- FL Studio：在插件 Wrapper 设 MIDI Output Port，在钢琴音源 Wrapper 设相同 MIDI Input Port；录制 CC64 的步骤还需该宿主实测。官方：<https://www.image-line.com/fl-studio-learning-content/fl-studio-online-manual-zh/html/plugins/wrapper.htm>。
- 其他宿主：只有同时接收本 VST3 的 MIDI 输入、暴露它的 MIDI 输出并能把输出送到钢琴音源时才可用。仅能扫描或打开界面不等于完成音频和 CC64 验收。

## 三步验收与排查

先用没有其他插件的空工程：①弹一个短音，确认钢琴正常发声；②松键，确认延音；③录四个和弦，确认实时有声、播放有声且录音的 CC64 通道有高低事件。没有声音时，先确认钢琴音源已加载并接收本插件的 MIDI 输出；没有 CC64 时，检查是否录到了处理**之后**的 MIDI；踏板重复或卡住时，检查录好的片段是否又通过自动踏板插件处理了一遍。

本测试包的自动验证只覆盖 VST3 装载、MIDI 处理、静音音频输出、旁路清理和状态迁移，不代替上述宿主实测。
