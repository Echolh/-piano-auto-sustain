#pragma once
#include "../PedalEngine.h"
#include <array>
#include <cmath>
#include <limits>

// A bounded, allocation-free observation queue. Original host events are never
// reconstructed: the adapter emits only added pedal messages.
class NativeTimeline {
public:
    using Event=PedalEngine::Event;
    using Tick=std::int64_t;
    static constexpr size_t capacity=4096;
    PedalEngine engine;
    NativeTimeline() { engine.prepare(1000000); engine.settings.noteHold=false; }
    bool enqueue(Tick tick,Event event) {
        if(size==capacity) return false;
        size_t i=size++;
        while(i && pending[i-1].tick>tick) { pending[i]=pending[i-1]; --i; }
        pending[i]={tick,event}; return true;
    }
    size_t queued() const { return size; }
    template<class Emit> void repedal(Emit emit) {
        engine.repedal(static_cast<PedalEngine::Time>(std::llround(elapsed)),[&](auto,Event e){
            if((e.status&0xf0)==0xb0 && e.data1==64)emit(last,e);
        });
    }
    // Tempo supplied at each host slice. Existing deadlines remain in elapsed
    // microseconds, so a tempo change does not rescale the pedal gap itself.
    template<class Emit> void advance(Tick tick,double bpm,double ppq,Emit emit) {
        if(!initialized) { last=tick; initialized=true; }
        if(tick<last) return; // transport resets are explicit
        const double rate=60000000.0/(std::clamp(bpm,1.0,1000.0)*std::max(1.0,ppq));
        const auto baseTick=last; const double baseUs=elapsed;
        auto output=[&](PedalEngine::Time us,Event e) {
            if((e.status&0xf0)!=0xb0 || e.data1!=64) return;
            auto t=baseTick+static_cast<Tick>(std::llround((us-baseUs)/rate));
            emit(std::clamp(t,baseTick,tick),e);
        };
        size_t consumed=0;
        while(consumed<size && pending[consumed].tick<=tick) {
            const auto& p=pending[consumed++];
            double at=baseUs+static_cast<double>(std::max(baseTick,p.tick)-baseTick)*rate;
            engine.input(static_cast<PedalEngine::Time>(std::llround(at)),p.event,output);
        }
        if(consumed) { for(size_t i=consumed;i<size;++i) pending[i-consumed]=pending[i]; size-=consumed; }
        elapsed=baseUs+static_cast<double>(tick-baseTick)*rate;
        engine.advance(static_cast<PedalEngine::Time>(std::llround(elapsed)),output);
        last=tick;
    }
    template<class Emit> void reset(Tick tick,Emit emit) {
        engine.panic(0,[&](auto,Event e) {
            if((e.status&0xf0)==0xb0 && e.data1==64) emit(tick,e);
        });
        size=0; elapsed=0; last=tick; initialized=true;
    }
private:
    struct Pending { Tick tick; Event event; };
    std::array<Pending,capacity> pending{};
    size_t size=0;
    Tick last=0;
    double elapsed=0;
    bool initialized=false;
};
