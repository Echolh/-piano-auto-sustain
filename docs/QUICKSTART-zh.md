# Piano Auto Sustain 1.0 — Cubase 简洁版

只使用一个 MIDI 插入插件，不需要 Recorder 或虚拟 MIDI 端口。

## 安装
保存工程并退出 Cubase，双击发布包内「安装.cmd」。安装完成后重开 Cubase，插件界面底部应显示 1.0。

## 每个钢琴轨道设置一次
1. 加载 Kontakt / Keyscape 等钢琴音源和钢琴音色。
2. 轨道输入选择实体键盘，打开监听（小喇叭）和录音准备。
3. 在「MIDI 插入」加载一个 Piano Auto Sustain Cubase。
4. **要把延音一起录进去：把 MIDI 插入区的虚线分隔线拖到本插件下面。插件必须位于分隔线上方。**

示意顺序：Piano Auto Sustain Cubase → 虚线分隔线 → 空插槽。
保存工程保留设置。以后直接弹奏、播放或录音。不需要额外启动程序。

## 录制和回放
分隔线上方的 MIDI 插入输出参与录制，音符和生成的 CC64 可一起写入片段。这是 Cubase 的轨道设置，插件 SDK 没有提供自动修改它的接口。

新录制后，在钢琴卷帘控制器通道选择 CC64 Sustain Pedal 检查。松键后等待延音结束再停止录制，可保留尾部释放。既有片段不会自动补写 CC64。

插件在一个通道上收到回放 CC64 后，保留该通道的踏板数据，停止叠加自动回放换踏，直到停止或跳转；只有音符的回放仍生成自动延音。实时键盘在自动模式下由插件接管踏板。混合人工踏板片段、无踏板片段或同通道叠录仍需实机验收。

不要使用旧的 Piano Auto Sustain 虚拟输入。安装程序会停止旧 Recorder 并移除它的同名启动项。

## 一次验收
- 停止状态弹奏：有声音，松键后延音。
- 录制弹奏：实时有声音、有延音；松键等尾音结束后停止，CC64 通道有数据。
- 回放新录音：有声音、有延音；检查无踏板的旧片段仍能自动延音。

用户已验证此前同一处理逻辑的实时声音、录制延音、回放及 CC64 录入；1.0 保留该逻辑，仅整理版本、说明书和联系信息。新 UI 和发布产物另经自动检查。

官方说明：[Recording MIDI Insert Effects](https://www.steinberg.help/r/cubase-pro/15.0/en/cubase_nuendo/topics/midi_realtime_parameters_and_effects/midi_realtime_parameters_midi_effects_midi_insert_effect_recording_t.html)。插件「高级设置」内也有中英文录制说明。

界面顶部「说明书」提供参数说明和录制步骤。合作咨询微信：Amumusic1005。
