#include "PedalEngine.h"
#include <vector>
#include <iostream>
#include <cstdlib>
struct Item {PedalEngine::Time time; PedalEngine::Event event;};
void require(bool ok,const char* name) {if(!ok) {std::cerr<<"FAIL: "<<name<<'\n'; std::exit(1);}}
struct Rig {
    PedalEngine p;
    std::vector<Item> events;
    Rig() {p.prepare(1000); p.settings.noteHold=false; p.settings.tailMs=1200;}
    auto emit() {return [this](auto t,auto e){events.push_back({t,e});};}
    void on(int t,int n,int ch=0) {p.input(t,{0x90|ch,n,100},emit());}
    void off(int t,int n,int ch=0) {p.input(t,{0x80|ch,n,0},emit());}
    void advance(int t) {p.advance(t,emit());}
    int lifts(int ch=0) {int n=0; for(auto x:events) if(x.event.status==(0xb0|ch)&&x.event.data1==64&&x.event.data2==0) ++n; return n;}
    void note(int t,int n,int ch=0) {on(t,n,ch); off(t+80,n,ch);}
    void chord(int t,std::initializer_list<int> ns) {for(auto n:ns) on(t,n); advance(t+40); for(auto n:ns) off(t+80,n);}
};
int main() {
    {
        Rig r;
        for(int i=0;i<3;++i) {r.note(i*900,36); r.note(i*900+300,43); r.note(i*900+600,48);}
        require(r.lifts()==0,"C-G-C broken accompaniment has no false lifts");
        r.note(3000,41); r.advance(3040); require(r.lifts()==1,"C-G-C to F detects change");
    }
    {
        Rig r; r.note(0,36); r.note(250,40); r.note(500,43); r.note(750,40); r.note(1000,36);
        require(r.lifts()==0,"C-E-G-E-C major arpeggio");
        r.chord(1300,{38,41,45}); require(r.lifts()==1,"major to D minor full voicing");
    }
    {
        Rig r; r.note(0,45); r.note(300,48); r.note(600,52); r.note(900,48);
        require(r.lifts()==0,"A-C-E minor arpeggio");
    }
    {
        Rig r; r.chord(0,{40,43,48}); r.note(300,43); r.note(600,40); r.note(900,48);
        require(r.lifts()==0,"learned C/E inversion members protected");
        r.chord(1200,{40,43,47}); require(r.lifts()==1,"E minor not absorbed as Cmaj7");
    }
    {
        Rig r; r.on(0,36); r.off(30,36); r.on(55,40); r.off(85,40); r.on(110,43); r.off(150,43); r.advance(200);
        require(r.lifts()==0,"rolled chord wider than detection window");
    }
    {
        Rig r; r.chord(0,{36,40,43}); r.chord(300,{36,41,45});
        require(r.lifts()==1,"C major to F/C with unchanged bass");
    }
    {
        Rig r; r.note(0,36); r.note(200,40); r.note(400,43); r.note(600,46);
        require(r.lifts()==0,"C dominant seventh extension");
        r.on(800,78); r.off(900,78); require(r.lifts()==0,"upper melody cannot change harmonic memory");
    }
    {
        Rig r; r.note(0,36); r.note(200,37); r.on(300,38); r.advance(399);
        require(r.lifts()==1,"guard delays second change"); r.advance(400);
        require(r.lifts()==2,"guard does not silently discard real change");
    }
    {
        Rig r; r.p.settings.guardMs=500; r.note(0,36); r.note(200,37); r.note(300,38); r.on(450,37); r.advance(750);
        require(r.lifts()==1,"return to previous harmony cancels pending change");
    }
    {
        Rig r; r.note(0,36); r.note(200,43); r.on(300,41,1); r.off(400,41,1); r.advance(500);
        require(r.lifts(0)==0&&r.lifts(1)==0,"channel memories isolated");
        r.p.repedal(600,r.emit()); r.advance(635); require(r.lifts(0)==1&&r.p.pedal(0),"manual repedal");
        r.advance(1600); require(!r.p.pedal(0)&&!r.p.pedal(1),"idle still clears protected arpeggio");
    }
    {
        Rig r; r.p.settings.noteHold=true; r.note(0,36); r.note(250,43);
        int offs=0; for(auto x:r.events) if((x.event.status&0xf0)==0x80) ++offs;
        require(offs==0,"note-hold mode protects arpeggio too"); r.note(500,38);
        offs=0; for(auto x:r.events) if((x.event.status&0xf0)==0x80) ++offs;
        require(offs>=2,"note-hold change releases old harmony");
    }
    {
        Rig r; r.p.settings.octaveSame=false; r.note(0,36); r.note(200,48);
        require(r.lifts()==1,"octave option respected in protected mode");
    }
    {
        Rig r; r.on(0,36); r.advance(40); r.note(300,41); r.off(400,36); r.note(600,45); r.note(900,48);
        require(r.lifts()==1,"old held bass does not corrupt new harmonic root");
    }
    std::cout<<"PASS: chord protection, major/minor/inversion, rolled chords, harmony changes, guard scheduling, manual refresh, output modes\n";
}
