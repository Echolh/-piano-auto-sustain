#include "Core/MidiEvent.h"
#include "Core/SustainSettings.h"
#include "Core/StateMigration.h"
#include "PedalEngine.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
struct TimedEvent { pas::PedalEngine::Time time; pas::MidiEvent event; };

void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

struct Rig {
    pas::PedalEngine engine;
    std::vector<TimedEvent> output;
    Rig() { engine.prepare(1000.0); engine.settings.noteHold = false; }
    auto emit() { return [this](auto time, auto event) { output.push_back({time, event}); }; }
    void on(int time, int note, int channel = 0) { engine.input(time, {0x90 | channel, note, 100}, emit()); }
    void off(int time, int note, int channel = 0) { engine.input(time, {0x80 | channel, note, 0}, emit()); }
    void advance(int time) { engine.advance(time, emit()); }
    int lifts(int channel = 0) const {
        int result = 0;
        for (const auto& item : output)
            if (item.event.status == (0xb0 | channel) && item.event.data1 == 64 && item.event.data2 == 0) ++result;
        return result;
    }
};
}

int main() {
    {
        Rig rig;
        rig.on(0, 36); rig.on(8, 43); rig.on(16, 48); rig.advance(56);
        rig.off(100, 36); rig.off(108, 43); rig.off(116, 48);
        require(rig.lifts() == 0, "one left-hand chord with several bass notes must not repedal");
        rig.on(300, 41); rig.on(308, 45); rig.on(316, 48); rig.advance(356);
        require(rig.lifts() == 1, "a real harmony change must repedal once");
    }
    {
        Rig rig;
        rig.on(0, 36); rig.advance(40); rig.off(80, 36);
        rig.on(200, 48); rig.advance(240);
        require(rig.lifts() == 0, "octave-equivalent bass is protected by default");
        Rig exact;
        exact.engine.settings.octaveSame = false;
        exact.on(0, 36); exact.advance(40); exact.off(80, 36);
        exact.on(200, 48); exact.advance(240);
        require(exact.lifts() == 1, "octave changes repedal when octave protection is disabled");
    }
    {
        Rig rig;
        rig.on(0, 36, 0); rig.on(0, 41, 1); rig.advance(40);
        rig.off(60, 36, 0); rig.on(100, 38, 0); rig.advance(140);
        require(rig.lifts(0) == 1 && rig.lifts(1) == 0, "channels keep independent sustain state");
    }
    {
        pas::PedalEngine engine; engine.prepare(1000.0); engine.settings.noteHold = true;
        std::vector<TimedEvent> output;
        auto emit = [&](auto time, auto event) { output.push_back({time, event}); };
        engine.input(0, {0x90, 48, 100}, emit);
        engine.input(10, {0x90, 48, 110}, emit);
        engine.input(20, {0x80, 48, 0}, emit);
        engine.input(30, {0x80, 48, 0}, emit);
        engine.panic(40, emit);
        int noteOffs = 0, panicNoteOffs = 0, pedalUps = 0;
        for (const auto& item : output) {
            if ((item.event.status & 0xf0) == 0x80 && item.event.data1 == 48) {
                ++noteOffs;
                if (item.time == 40) ++panicNoteOffs;
            }
            if (item.event.status == 0xb0 && item.event.data1 == 64 && item.event.data2 == 0) ++pedalUps;
        }
        require(noteOffs == 2 && panicNoteOffs == 1,
                "two repeated Note On events receive one immediate and one panic Note Off");
        require(pedalUps == 1 && !engine.pedal(0), "panic releases CC64 and resets channel state");
    }
    {
        const std::map<std::string, double> legacy{{"split", 55.0}, {"tail", 3200.0}};
        const auto settings = pas::StateMigration::fromVersionedMap(1, legacy);
        require(settings.split == 55 && settings.tailMs == 3200, "1.0 values survive migration");
        require(settings.groupMs == 40 && settings.gapMs == 35 && settings.guardMs == 160,
                "missing 2.0 fields receive documented defaults");
        require(settings.chordProtection && settings.octaveSame && !settings.noteHold,
                "migration uses safe CC64 chord-protection defaults");
        const std::map<std::string, double> invalid{{"split", 200.0}, {"tail", -1.0}};
        const auto clamped = pas::StateMigration::fromVersionedMap(2, invalid);
        require(clamped.split == 84 && clamped.tailMs == 100, "migrated numeric values are clamped");
    }
    std::cout << "PASS: shared sustain core contract and state migration\n";
}
