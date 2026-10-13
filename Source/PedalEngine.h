#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include "ChordMemory.h"
#include "Core/MidiEvent.h"
#include "Core/SustainSettings.h"

// No allocation, locks or framework dependencies. Time is an absolute sample clock.
namespace pas {
class PedalEngine {
public:
    SustainSettings settings;
    using Event = MidiEvent;
    using Time = std::int64_t;
    void prepare(double rate) { sampleRate = rate; channels = {}; }
    int bass(int ch) const { return channels[ch].bass; }
    bool pedal(int ch) const { return channels[ch].down; }

    template<class Emit> void repedal(Time t, Emit emit) {
        for(int c=0;c<16;++c) {
            auto& s=channels[c];
            if(!s.down && s.downAt<0) continue;
            lift(c,t,emit); s.downAt=t+ms(settings.gapMs);
            s.groupAt=s.changeAt=-1; s.groupMask=0; s.harmony.reset();
            s.bass=s.candidate=-1;
        }
    }

    template<class Emit> void panic(Time t, Emit emit) {
        for (int c=0; c<16; ++c) {
            auto& s = channels[c];
            if (s.used) {
                for (int n=0; n<128; ++n)
                    if (s.held[n] || s.deferred[n]) emit(t, Event{0x80|c,n,0});
                emit(t, Event{0xb0|c,64,0});
                emit(t, Event{0xb0|c,123,0});
            }
        }
        channels = {};
    }
    template<class Emit> void advance(Time to, Emit emit) {
        for (int c=0; c<16; ++c) {
            auto& s=channels[c];
            for (;;) {
                Time next = -1;
                for (auto t : {s.groupAt,s.changeAt,s.downAt,s.tailAt})
                    if (t >= 0 && (next < 0 || t < next)) next=t;
                if (next < 0 || next > to) break;
                if (s.groupAt == next) {
                    s.groupAt=-1;
                    const int playedBass=s.candidate;
                    int candidate=playedBass;
                    // Observe the hand at the end of the grouping window:
                    // this permits upward legato if the old bass is released
                    // shortly after the new one lands, but preserves held basses.
                    for(int n=0;n<candidate;++n) if(s.held[n]) {candidate=n; break;}
                    s.candidate=-1;
                    const bool different=settings.chordProtection
                        ? s.harmony.needsChange(s.groupMask,playedBass,settings.octaveSame)
                        : s.harmony.lastBass>=0 && (settings.octaveSame ? s.harmony.lastBass%12!=candidate%12 : s.harmony.lastBass!=candidate);
                    s.bass=candidate;
                    if(different) {
                        s.pendingMask=s.groupMask; s.pendingBass=settings.chordProtection?playedBass:candidate;
                        s.changeAt=std::max(next,s.guardUntil);
                    } else {
                        s.changeAt=-1;
                        if(!settings.chordProtection) s.harmony.reset(s.groupMask,candidate);
                    }
                    s.groupMask=0;
                }
                if(s.changeAt==next) {
                    s.changeAt=-1;
                    lift(c,next,emit);
                    s.harmony.reset(s.pendingMask,s.pendingBass);
                    s.downAt=next+ms(settings.gapMs);
                    s.guardUntil=next+ms(settings.guardMs);
                }
                if (s.downAt == next) { s.downAt=-1; press(c,next,emit); }
                if (s.tailAt == next) {
                    lift(c,next,emit);
                    s.tailAt=s.downAt=s.groupAt=s.changeAt=-1;
                    s.bass=s.candidate=-1;
                    s.harmony.reset(); s.groupMask=0;
                }
            }
        }
    }
    template<class Emit> void input(Time t, Event e, Emit emit) {
        advance(t,emit);
        if (!settings.enabled) { emit(t,e); return; }
        const int c=e.status&15, type=e.status&0xf0;
        auto& s=channels[c]; s.used=true;
        if (type==0xb0 && (e.data1==120 || e.data1==123 || e.data1==121)) {
            lift(c,t,emit);
            for(int n=0;n<128;++n) if(s.held[n]) emit(t,Event{0x80|c,n,0});
            s=Channel{}; emit(t,e); return;
        }
        // Auto mode owns CC64; bypass restores original hardware pedal messages.
        if(type==0xb0 && e.data1==64) return;
        if(type==0x90 && e.data2>0) {
            s.tailAt=-1;
            if(settings.noteHold && s.deferred[e.data1]) {
                emit(t,Event{0x80|c,e.data1,0}); s.deferred[e.data1]=false;
            }
            ++s.held[e.data1];
            emit(t,e);
            if(!s.down && s.downAt<0) press(c,t,emit);
            if(e.data1<=settings.split) {
                s.groupMask=static_cast<ChordMemory::Mask>(s.groupMask|ChordMemory::bit(e.data1));
                if(s.groupAt>=0) s.candidate=std::min(s.candidate,e.data1);
                else {
                    s.candidate=e.data1; s.groupAt=t+ms(settings.groupMs);
                }
            }
            return;
        }
        if(type==0x80 || (type==0x90 && e.data2==0)) {
            if(s.held[e.data1]>0) --s.held[e.data1];
            if(settings.noteHold && s.down && s.held[e.data1]==0)
                s.deferred[e.data1]=true;
            else emit(t,e);
            bool any=false;
            for(auto count:s.held) if(count) any=true;
            if(!any) s.tailAt=t+ms(settings.tailMs);
            return;
        }
        emit(t,e);
    }
private:
    struct Channel {
        std::array<unsigned int,128> held{};
        std::array<bool,128> deferred{};
        bool down=false, used=false;
        int bass=-1,candidate=-1;
        ChordMemory harmony;
        ChordMemory::Mask groupMask=0,pendingMask=0;
        int pendingBass=-1;
        Time groupAt=-1,downAt=-1,tailAt=-1,guardUntil=0,changeAt=-1;
    };
    std::array<Channel,16> channels{};
    double sampleRate=44100;
    Time ms(int value) const { return std::max<Time>(1,static_cast<Time>(sampleRate*value/1000.0)); }
    template<class Emit> void press(int c,Time t,Emit emit) {
        auto& s=channels[c];
        if(!s.down && !settings.noteHold) emit(t,Event{0xb0|c,64,127});
        s.down=true;
    }
    template<class Emit> void lift(int c,Time t,Emit emit) {
        auto& s=channels[c];
        if(!settings.noteHold && s.down) emit(t,Event{0xb0|c,64,0});
        for(int n=0;n<128;++n) if(s.deferred[n]) {
            emit(t,Event{0x80|c,n,0}); s.deferred[n]=false;
        }
        s.down=false;
    }
};
}

using PedalEngine = pas::PedalEngine;
