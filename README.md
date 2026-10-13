# Piano Auto Sustain 1.0

Amu music · 钢琴自动延音
合作咨询微信：Amumusic1005

## 选择版本
- Windows + Cubase：优先 Cubase MIDI 插入版。已由用户验证实时声音、录制延音、回放以及 CC64 录入。
- Windows + 其他支持 MIDI 输出路由的宿主：VST3 测试版。本插件不发声，MIDI 输出必须连接钢琴音源；不在本次声明已通过所有宿主实测。
- REAPER、Cakewalk、Cubase 旧版及其他 Windows 宿主的具体路由与录制步骤见 docs/HOST-GUIDE.md；通用 VST3 测试包位于 01_发布包/03_Windows通用VST3测试版。
- macOS / Linux：移植源码和构建方案留在源码中，发布暂缓；目前没有可安装的跨平台成品。
- Logic Pro：AU MIDI FX 构建目标仍待 auval 和 Logic 实机验证。

界面提供中英文说明书，合作咨询微信仅显示在说明书内。显示版本为 1.0；文件资源及 VST3 标准三段版本为 1.0.0。由开发版 2.2.5 重新编号为首个正式版本，保留插件 ID、参数和工程兼容。

参见 docs/QUICKSTART-zh.md、docs/VST3-GUIDE.md、docs/COMPATIBILITY.md、docs/BUILD-OTHER-SYSTEMS.md。
旧 Recorder/Bridge 源码仅供维护追溯，不作为用户安装内容。源码、测试、SDK 依赖和历史记录保留，旧二进制发布包清理。
现有授权版 ZIP 的交付脚本原样归档在 scripts/commercial-reference；授权版 DLL 的 C++ 验证实现与线上服务端源码不在本工程中，不能由本工程重建授权版。
