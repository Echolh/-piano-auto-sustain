#include "NativeMidi/Timeline.h"
#include <vector>
#include <stdexcept>
#include <iostream>
void require(bool b,const char* s) { if(!b) throw std::runtime_error(s); }
int main() {
 try {
    NativeTimeline t;
    t.engine.settings.tailMs=1200;
    std::vector<std::pair<long long,int>> cc;
    auto emit=[&](auto tick,auto e) { require(e.data1==64,"only pedal output"); cc.push_back({tick,e.data2}); };
    t.advance(0,120,480,emit);
    t.enqueue(0,{0x90,48,100}); t.advance(0,120,480,emit);
    require(cc.size()==1 && cc[0].second==127,"pedal press");
    t.enqueue(96,{0x80,48,0}); t.enqueue(120,{0x90,55,100});
    t.enqueue(216,{0x80,55,0}); t.enqueue(240,{0x90,60,100});
    t.enqueue(336,{0x80,60,0}); t.advance(500,120,480,emit);
    require(cc.size()==1,"C-G-C arpeggio protected");
    t.advance(1488,120,480,emit);
    require(cc.size()==2 && cc.back().second==0,"duration note release and tail");
    t.reset(2000,emit); cc.clear();
    t.enqueue(2000,{0x90,48,100}); t.enqueue(2001,{0x80,48,0});
    t.advance(2001,120,480,emit);
    t.advance(2481,60,480,emit); // one second
    require(cc.size()==1,"tempo change preserves elapsed-time deadline");
    t.advance(2577,60,480,emit); // another 200 ms
    require(cc.size()==2 && cc.back().second==0,"tail remains 1200 ms across tempo change");
    t.reset(-100,emit); cc.clear();
    t.enqueue(-100,{0x90,48,100}); t.advance(-100,120,480,emit);
    t.reset(0,emit); require(cc.back().second==0 && t.queued()==0,"seek cleanup at negative pre-roll");
    NativeTimeline bounded;
    for(size_t i=0;i<NativeTimeline::capacity;++i) require(bounded.enqueue(i,{0x90,60,100}),"queue capacity");
    require(!bounded.enqueue(9999,{0x90,60,100}),"bounded queue overflow signalled");
    std::cout<<"Native timeline tests passed\n"; return 0;
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
