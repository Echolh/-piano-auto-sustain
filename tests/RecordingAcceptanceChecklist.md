# Cubase live-recording acceptance — Piano Auto Sustain 2.2.1

1. Start **Piano Auto Sustain Recorder** and confirm the status reads Ready.
2. In Cubase, set the instrument track input to `Piano Auto Sustain`. Do not add the Cubase MIDI insert to this track.
3. Enable Monitor and Record, play a phrase, then stop.
4. Confirm Kontakt was audible while playing and the part plays after Recorder is closed.
5. Export the part as Standard MIDI and run `recorded_midi_inspection <file.mid>`.
6. Confirm the inspector reports notes, CC64 down/up, and no duplicate Note On.
7. Replug the keyboard, reopen the project, and repeat. Use **Release all notes** once and confirm no stuck notes remain.

The release evidence must record the Cubase, Kontakt, keyboard and virtual-port versions used.
