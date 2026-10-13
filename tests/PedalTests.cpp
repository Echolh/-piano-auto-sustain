#include "PedalEngine.h"
#include <vector>
#include <iostream>
#include <cstdlib>
struct Item { PedalEngine::Time t; PedalEngine::Event e; };
void check(bool ok,const char* name) { if(!ok) { std::cerr<<"FAIL: "<<name<<'\n'; std::exit(1); } }
int main() {
    PedalEngine p; p.prepare(1000);
    p.settings.noteHold=true; p.settings.tailMs=1200;
    p.settings.chordProtection=false; // Keep the original mode's regression suite.
    std::vector<Item> out;
    auto emit=[&](auto t,auto e){out.push_back({t,e});};
    auto count=[&](int type,int note){int n=0; for(auto x:out) if((x.e.status&0xf0)==type&&x.e.data1==note)++n; return n;};
    p.input(0,{0x90,48,100},emit); p.advance(40,emit);
    p.input(100,{0x80,48,0},emit);
    check(count(0x80,48)==0,"release deferred");
    p.input(200,{0x90,67,100},emit); p.advance(250,emit);
    check(count(0x80,48)==0,"melody cannot lift");
    p.input(300,{0x90,50,100},emit); p.advance(340,emit);
    check(count(0x80,48)==1,"new bass releases old harmony");
    check(!p.pedal(0),"pedal gap"); p.advance(375,emit); check(p.pedal(0),"repedal on time");
    p.input(400,{0x80,50,0},emit); p.input(410,{0x80,67,0},emit);
    p.advance(1610,emit); check(!p.pedal(0),"idle tail cleanup");
    p.prepare(1000); out.clear(); p.settings.noteHold=false;
    p.input(0,{0x90,48,100},emit); p.input(5,{0x90,55,100},emit); p.advance(40,emit);
    check(p.bass(0)==48,"chord window chooses minimum");
    p.input(100,{0x80,48,0},emit); p.input(200,{0x90,36,100},emit); p.advance(240,emit);
    check(count(0xb0,64)==1,"same pitch class does not repedal");
    p.input(300,{0x91,43,100},emit); p.advance(400,emit);
    check(p.bass(1)==43&&p.bass(0)==36,"channel isolation");
    p.panic(500,emit); check(!p.pedal(0)&&!p.pedal(1),"panic resets");
    p.prepare(1000); p.settings.noteHold=true; out.clear();
    p.input(0,{0x90,48,100},emit); p.input(50,{0x90,48,0},emit);
    p.input(100,{0x90,48,110},emit);
    check(out.size()==3&&out[1].e.status==0x80&&out[2].e.status==0x90,"retrigger closes deferred voice first");
    p.input(110,{0xb0,123,0},emit); p.advance(2000,emit); check(!p.pedal(0),"all notes off cancels timers");
    p.prepare(1000); out.clear();
    p.input(0,{0x90,48,100},emit); p.advance(40,emit);
    p.input(200,{0x90,50,100},emit); p.input(220,{0x80,48,0},emit); p.advance(240,emit);
    check(p.bass(0)==50&&count(0x80,48)==1,"upward legato bass handover");
    p.prepare(1000); out.clear(); p.settings.noteHold=false;
    p.input(0,{0x90,48,100},emit); p.advance(40,emit); p.input(100,{0x80,48,0},emit);
    p.input(255,{0x90,50,100},emit);
    for(int t=256;t<=330;++t) p.advance(t,emit);
    check(out.size()==6&&out[4].t==295&&out[4].e.data2==0&&out[5].t==330&&out[5].e.data2==127,"sample-accurate CC64 across blocks");
    p.prepare(48000); out.clear();
    p.input(0,{0x90,48,100},emit); p.advance(1920,emit);
    p.input(4800,{0x80,48,0},emit); p.advance(62399,emit);
    check(p.pedal(0),"tail sample rate scaling before deadline"); p.advance(62400,emit);
    check(!p.pedal(0),"tail sample rate scaling deadline");
    std::cout<<"PASS: sustain, bass changes, melody, grouping, octave, channels, panic, retrigger, idle\n";
}
