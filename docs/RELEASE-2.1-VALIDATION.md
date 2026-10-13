# Piano Auto Sustain 2.1 发布验收

验收日期：2026-10-11

## 已验证

- Windows x64 Recorder、VST3 与 Cubase MIDI Insert 均成功生成，文件版本和产品版本为 `2.1.0`。
- Recorder 会把键盘音符与自动生成的 CC64 一起输出；同一时间点先发送 CC64，再发送 Note On。
- 单键盘自动选择；多键盘可从界面选择；虚拟输出缺失或存在多个候选时可从界面选择。
- MIDI 输出断开后会关闭失效端口并进入明确错误状态。
- “随 Windows 启动”和“关闭到托盘”会保存并实际生效。
- 中英文界面、五套配色及 100%、150%、200% 缩放快照通过测试。
- 发布目录与解压后的 ZIP 均通过文件清单和 SHA256 校验。

## 自动测试

- Portable/JUCE：15/15 通过。
- Cubase Native MIDI：14/14 通过。
- MIDI 录制夹具包含 Note On/Off、CC64 踩下/释放，且没有重复 Note On。

## 当前平台范围

正式二进制支持 Windows x64。Cubase 可继续使用原生 MIDI Insert DLL；其他支持外部 MIDI 输入的宿主使用 Recorder 输出。macOS 与 Linux 保留源码级后端规划，尚未作为 2.1 正式支持平台发布。

当前机器没有可用于正式发布的 Windows MIDI Services 虚拟端点运行环境，因此 Recorder 会使用已有虚拟 MIDI 端口兼容模式。用户需要先提供一个名为 `Piano Auto Sustain` 的虚拟 MIDI 端口，快速入门中给出了最短设置步骤。

## 仍需实机确认

自动测试不能代替 Cubase 中的人工录音验收。发布前人工检查应确认：录音监听有声、录下的事件包含音符和 CC64、回放延音正确、长时间演奏没有挂音或回路。
