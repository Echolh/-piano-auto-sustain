# Piano Auto Sustain 2.2.1 验证记录

验证日期：2026-10-12

## 发布范围

- Windows x64
- Cubase 原生 MIDI 插入：`Piano Auto Sustain Cubase.dll`
- 通用 VST3 乐器：`Piano Auto Sustain.vst3`
- CC64 录制器：`Piano Auto Sustain Recorder.exe`

## 2.2.1 使用模式

1. **快速演奏**：Cubase 乐器轨的 MIDI 插入选择 `Piano Auto Sustain Cubase`，无需启动录制器。
2. **录制音符与 CC64**：录制器自动选择唯一键盘和 `Piano Auto Sustain` 虚拟输出；连接正常时隐藏到托盘，出现需要处理的连接错误时才显示窗口。

2.2.1 将 Cubase 播放时间线中的音符和已经录制的 CC64 完全透明直通，只在实时 MIDI Thru 输入上生成自动踏板，避免播放或录音运输状态干扰钢琴发声。

安装脚本创建开始菜单快捷方式、注册随 Windows 启动并立即启动录制器。旧版公开 DLL 名称和旧 Bridge 目录会在安装时清理。

## 自动验证

- Portable/VST3 配置：15/15 CTest 通过。
- Cubase MIDI 插入配置：14/14 CTest 通过。
- 覆盖版本号、踏板引擎、和弦保护、MIDI 转发、录制器配置迁移、CC64 录制文件、VST3 扫描、双语 UI 烟雾测试、原生 MIDI 插入 ABI 与编辑器。
- Windows 文件版本：DLL、VST3 二进制和 Recorder 均为 `2.2.1`。
- 发布包目录、说明、安装/卸载脚本及 SHA-256 清单由打包测试验证。

## 实机边界

自动测试验证插件和录制器的代码路径。Cubase 的实际键盘设备、音源监听和虚拟 MIDI 端口属于本机宿主状态，最终以 Cubase 中现场弹奏与录音回放为准。
