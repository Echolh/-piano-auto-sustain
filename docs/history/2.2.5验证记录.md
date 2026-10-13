# 2.2.5 验证范围（2026-10-12）

## 已完成
- MSVC Release 构建 Windows x64 Cubase MIDI 插入 DLL。
- CTest 15/15 通过；涵盖版本、延音引擎、和弦保护、时间线、接口、编辑器及历史核心模块。
- 用实际 Cubase 15 日志中的直接 BPM 和实时/播放时间戳差异构造回归测试。实时监听转发使用事件副本，不修改原始录音事件；保留队列/直接输出的所有权约定。
- 新增已录 CC64 原值和时间保留测试，停止/跳转后恢复音符自动踏板测试。
- 编辑器中英文录制帮助标签、既有 DPI/配色检查通过。

## 尚未验证
- 本版 Cubase 15 + Kontakt 的录制实时声音、录入 CC64 和回放整套实机验收。模拟接口通过不等于可听结果通过。
- 混合 CC64/无 CC64 片段和同时同通道弹奏回放、循环录音、离线导出。
- 其他系统或宿主。本次不发布 Recorder、虚拟 MIDI 驱动或其他格式。

## 使用边界
Cubase MIDI 插入录制分隔线是宿主设置：插件位于分隔线上方才录制其输出。当前 SDK 未提供修改接口，因此保留一次性手动设置，不再要求 Recorder。
插件从每通道收到第一条回放 CC64 起保留踏板数据，直到停止或跳转；无法预先扫描整条 MIDI 轨道。旧录音不会自动补写 CC64。

官方依据：https://www.steinberg.help/r/cubase-pro/15.0/en/cubase_nuendo/topics/midi_realtime_parameters_and_effects/midi_realtime_parameters_midi_effects_midi_insert_effect_recording_t.html
