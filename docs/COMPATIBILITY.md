# 1.0 系统与宿主兼容性

| 系统 / 宿主 | 交付 | 验证状态 |
|---|---|---|
| Windows x64 / Cubase 15 | 专用 MIDI 插入 DLL | 用户确认实时声音、延音、录制、回放与 CC64 录入；1.0 保留处理逻辑 |
| Windows x64 / REAPER | VST3 测试包 | 编译、插件扫描和处理自动测试；输入 FX 录制路径有官方文档依据，本插件实机待验收 |
| Windows x64 / Cakewalk Sonar、Cakewalk by BandLab | VST3 测试包 | 编译、插件扫描和处理自动测试；软音源 MIDI 输出与接收轨有官方文档依据，本插件实机待验收 |
| Windows x64 / Fender Studio Pro 8 | VST3 测试包 | 2026-10-13 用户确认两轨路由有声及延音，录制截图可见钢琴轨 CC64；其他 Studio One 版本待实测 |
| Windows x64 / Cubase 15 通用 VST3 | VST3 测试包 | 2026-10-13 用户确认两轨路由可用；当前截图未展示 CC64 编辑通道，需进一步核对录制结果 |
| Windows x64 / Ableton Live、FL Studio | VST3 测试包 | 有官方 MIDI 路由说明，本插件实机与 CC64 录制待验收 |
| Windows x64 / 其他支持 MIDI 输出的 VST3 宿主 | VST3 测试包 | 不可仅凭 VST3 扫描成功承诺兼容；逐宿主验证 |
| Windows x64 / Cubase Pro、Artist 10.5 等较旧版本 | 专用 MIDI 插入 DLL | 官方手册确认 MIDI 插入功能，当前插件在这些版本尚未实测；AI/Elements 需核对功能集 |
| macOS Intel / Apple Silicon | Universal VST3 + AU MIDI FX 源码与构建方案 | 发布暂缓；尚无安装包，未完成平台验收 |
| Linux x64 | VST3 x64 源码与构建方案 | 尚无安装包，未完成平台验收 |
| Logic Pro | AU MIDI FX 构建目标 | 已编译，auval 尚未执行；未交付安装包 |
| Nuendo / Windows ARM / 32位 | 不发布专用包 | 未验证 |

VST3 与 Cubase 的 MIDI 插入 DLL 是不同格式，不能互相改后缀代替。多个系统需要各自编译、测试和打包。macOS 对外发布还需签名/公证；不能将 Windows DLL 压缩后当作 Mac 安装包。

技术依据：
- JUCE 支持 VST3，AU 仅适用于 macOS：https://juce.com/tutorials/tutorial_create_projucer_basic_plugin/
- Ableton VST MIDI 输出路由：https://help.ableton.com/hc/en-us/articles/209070189-Accessing-the-MIDI-output-of-a-VST-plug-in
- Logic MIDI 插件说明：https://help.apple.com/pdf/logicpromac-effects/en_US/logic-pro-mac-effects-user-guide.pdf
