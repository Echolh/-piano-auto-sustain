# VST3 测试版 1.0 — 使用说明

REAPER、Cakewalk Sonar、Cubase 等 Windows 宿主的逐步操作请先看 `HOST-GUIDE.md`。此 VST3 包尚未逐宿主实测，不代表所有 VST 宿主都可用。

本插件是 MIDI 处理器，不包含钢琴采样或音源，不输出钢琴声音。
将整个 Piano Auto Sustain.vst3 文件夹复制到宿主扫描的 VST3 目录。Windows 常用目录：C:\Program Files\Common Files\VST3（复制需要管理员权限）。重启宿主并重新扫描。

信号方向：键盘/音符 → Piano Auto Sustain → 钢琴音源 → 音频输出。
不要将它放在钢琴的音频效果器后面。VST3 以 Instrument / Tools 分类，宿主需支持插件 MIDI 输出。

## Ableton Live 参考步骤（待此产品实测）
第一条 MIDI 轨加载本插件并接键盘。第二条 MIDI 轨加载钢琴，MIDI From 选择第一轨，并在下一级选择插件输出，打开第二轨监听。在接收轨录制处理后的 MIDI。
官方：https://help.ableton.com/hc/en-us/articles/209070189-Accessing-the-MIDI-output-of-a-VST-plug-in

## 其他宿主
REAPER、Cakewalk、FL Studio 等需分别验证 MIDI 路由和录制处理后 MIDI 的选项，不保证仅扫描成功就能正确录制。不要照搬 Cubase 的虚线操作到其他宿主。
Cubase 用户直接用专用 MIDI 插入版更简便。

## 参数与限制
VST3 界面采用与 Cubase 专用版相同的踏板主页、演奏/高级设置/诊断分页和六款配色。启用自动踏板、低音上限、和弦收集、抬踩间隔、换踏保护、松键尾音均可在界面调节。中文/英文可切换，Guide/说明书提供帮助。
默认使用 CC64 模式。Note Hold 模式延迟音符释放，不等同钢琴踏板共鸣。
已有 CC64 片段建议旁路 VST3，避免重新处理；此测试版的输入无法可靠区分实时键盘和回放，不承诺与 Cubase 专用版完全相同。
语言与配色仅作用于本次编辑器会话；处理参数由宿主保存。

本包为本机开发测试产物，尚未逐一完成其他 DAW 的实机验证。JUCE 的许可说明随包附带；正式对外分发应选择适用的 JUCE 授权方式。
合作咨询微信：Amumusic1005
