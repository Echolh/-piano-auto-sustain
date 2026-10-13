# Piano Auto Sustain 1.0 — Cubase

Use one MIDI insert. No Recorder or virtual MIDI port is required.

Save your project, quit Cubase, run 安装.cmd from the package, then reopen Cubase. The editor footer should show 1.0.

Set up each piano track once:
1. Load a piano instrument and patch.
2. Select your physical keyboard input; enable Monitor and Record Enable.
3. Add Piano Auto Sustain Cubase to MIDI Inserts.
4. To record generated sustain, drag the dotted recording divider BELOW this plugin. Inserts ABOVE the divider are recorded.

Save the project. Play and record normally. Allow the sustain tail to end before stopping to capture its release. In the Key Editor, select CC64 Sustain Pedal to inspect a new recording. Existing clips are not rewritten.

Recorded CC64 is preserved: after the first playback CC64 on a channel, automatic sequencer pedal generation is suppressed on that channel until stop or seek. Note-only playback still generates sustain. Live pedal input remains controlled by automatic mode. Mixed pedal/no-pedal clips and simultaneous playback/live input on one channel require host acceptance testing.

Do not select the old virtual Piano Auto Sustain input. The installer stops the old Recorder and removes its matching startup entry.

Acceptance: verify stopped playing, live recording with sound and sustain, recorded CC64, and playback. The user confirmed live sound, recording sustain, playback and recorded CC64 with the unchanged processing logic. Version 1.0 adds documentation and release labeling.

The divider is a host setting with no setter in the available plugin SDK. See [Steinberg recording instructions](https://www.steinberg.help/r/cubase-pro/15.0/en/cubase_nuendo/topics/midi_realtime_parameters_and_effects/midi_realtime_parameters_midi_effects_midi_insert_effect_recording_t.html). Bilingual help is available in Advanced.

Open Guide in the editor for instructions. Cooperation / WeChat: Amumusic1005.
