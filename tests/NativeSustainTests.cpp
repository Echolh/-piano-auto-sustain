#include "NativeMidi/Sustain.h"
#include <vector>
#include <iostream>
#include <stdexcept>
struct Midi { int status=0x90,d1=48,d2=100,channel=0; long tick=0,length=120; bool live=true; };
class Host final : public IMEAccessor {
public:
    unsigned refs=1,created=0,destroyed=0; int passed=0; long bpm=120000;
    std::vector<Midi> cc, originals;
    int queued=0;
    tresult PLUGIN_API queryInterface(const TUID id,void** out) override {
        *out=nullptr;
        if(std::memcmp(id,IMEAccessor_iid,16)) return kNoInterface;
        *out=static_cast<IMEAccessor*>(this); addRef(); return kResultOk;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override { return --refs; }
    IMEObjectID PLUGIN_API createMidiEvent(MidiStatus s,long a,long b,long l) override { ++created; auto* e=new Midi; e->status=s;e->d1=a;e->d2=b;e->length=l;return e; }
    IMEObjectID PLUGIN_API createSysexEvent(unsigned char*,long) override { return nullptr; }
    IMEObjectID PLUGIN_API duplicateMidiEvent(IMEObjectID e) override { ++created;return new Midi(*static_cast<Midi*>(e)); }
    tresult PLUGIN_API destroyEvent(IMEObjectID e) override { ++destroyed;delete static_cast<Midi*>(e);return kResultOk; }
    tresult PLUGIN_API passToOutputQueue(IMEObjectID e) override { ++queued;cc.push_back(*static_cast<Midi*>(e)); return kResultOk; }
    tresult PLUGIN_API passToOutput(IMEObjectID e) override {
        auto& m=*static_cast<Midi*>(e);
        if(m.status==0xb0 && m.d1==64) cc.push_back(m); else {++passed;originals.push_back(m);}
        return kResultOk;
    }
#define PROPERTY(Name,Type,Field) \
    Type PLUGIN_API get##Name(IMEObjectID e) override {return static_cast<Midi*>(e)->Field;} \
    tresult PLUGIN_API set##Name(IMEObjectID e,Type v) override {static_cast<Midi*>(e)->Field=v;return kResultOk;}
    PROPERTY(Start,long,tick)
    PROPERTY(Length,long,length)
    PROPERTY(Channel,int,channel)
    PROPERTY(Data1,int,d1)
    PROPERTY(Data2,int,d2)
#undef PROPERTY
    tresult PLUGIN_API setImmediateEvent(IMEObjectID e,bool v) override {static_cast<Midi*>(e)->live=v;return kResultOk;}
    bool PLUGIN_API isImmediateEvent(IMEObjectID e) override {return static_cast<Midi*>(e)->live;}
    MidiStatus PLUGIN_API getStatus(IMEObjectID e) override {return static_cast<MidiStatus>(static_cast<Midi*>(e)->status);}
    tresult PLUGIN_API setStatus(IMEObjectID e,MidiStatus s) override {static_cast<Midi*>(e)->status=s;return kResultOk;}
    int PLUGIN_API getData3(IMEObjectID) override {return 0;}
    tresult PLUGIN_API setData3(IMEObjectID,int) override {return kResultOk;}
    int PLUGIN_API getMicroTuning(IMEObjectID) override {return 0;}
    tresult PLUGIN_API setMicroTuning(IMEObjectID,int) override {return kResultOk;}
    long PLUGIN_API getMidiChannel() override {return 0;}
    long PLUGIN_API getInterval() override {return 1;}
    tresult PLUGIN_API setInterval(long) override {return kResultOk;}
    tresult PLUGIN_API getContextInfo(long,MidiContextInfo* c) override {*c={};c->tempo=bpm;return kResultOk;}
    tresult PLUGIN_API setCanPlayInStop(int) override {return kResultOk;}
};
void check(bool b,const char* s) {if(!b) throw std::runtime_error(s);}
std::int64_t fakeMicros=0;
std::int64_t testClock(){return fakeMicros;}
int main() {
 try {
    {
        Host actual;actual.bpm=70;MidiSustain e(testClock);e.initialize(&actual);
        e.positAction(320);Midi n;n.live=false;n.tick=320;n.length=120;
        e.receiveEvent(&n);e.playAction(320,1784,false);
        check(actual.cc.size()==2 && actual.cc[1].tick==1784,
              "Cubase trace: raw tempo 70 means 70 BPM; 2400ms tail equals 1344 ticks");
    }
    {
        Host actual;actual.bpm=70;MidiSustain e(testClock);e.initialize(&actual);
        e.playAction(0,1663,true);fakeMicros+=110000;
        Midi n;n.tick=5023355;n.length=0;e.receiveEvent(&n);
        check(actual.cc.size()==1 && std::abs(actual.cc[0].tick-1725)<=1,
              "Cubase trace: CC64 must use callback clock, never absolute input timestamp 5023355");
        check(n.tick==5023355,"clock conversion must not modify the host recording event");
        check(actual.originals[0].tick==actual.cc[0].tick,
              "live monitoring note and pedal must use the same output clock");
        n.status=0x80;n.d2=0;n.tick=5023391;fakeMicros+=35000;e.receiveEvent(&n);
        check(std::abs(actual.originals.back().tick-1744)<=1 && n.tick==5023391,
              "monitor note-off must be translated too without modifying recording");
    }
    {
        Host actual;MidiSustain e(testClock);e.initialize(&actual);
        Midi cc;cc.live=false;cc.status=0xb0;cc.d1=64;cc.d2=127;
        e.receiveEvent(&cc);Midi n;n.live=false;e.receiveEvent(&n);
        cc.tick=480;cc.d2=0;e.receiveEvent(&cc);e.playAction(0,3000,false);
        check(actual.cc.size()==2 && actual.cc[0].tick==0 && actual.cc[1].tick==480,
              "recorded CC64 timing is preserved with no regenerated pedal changes");
        e.positAction(0);actual.cc.clear();e.receiveEvent(&n);e.playAction(0,2424,false);
        check(actual.cc.size()==2 && actual.cc[1].tick==2424,
              "seek resets recorded-pedal detection for a new notes-only passage");
    }
    Host h; auto* effect=new MidiSustain(testClock);
    check(effect->initialize(&h)==kResultOk,"initialize");
    Midi note; note.channel=3;
    effect->receiveEvent(&note);
    check(h.passed==1 && h.cc.size()==1 && h.cc[0].d2==127 && h.cc[0].channel==3,"live sustain and original note");
    note.status=0x80; note.d2=0; note.tick=96; effect->receiveEvent(&note);
    fakeMicros+=200000;effect->playAction(0,26389062,true);
    check(h.cc.size()==1,"large host live timestamps cannot prematurely release sustain");
    fakeMicros+=2200000;effect->playAction(0,26389062,true);
    check(h.cc.size()==2 && h.cc.back().d2==0,"live tail release");
    effect->stopAction(0); h.cc.clear();
    note={}; note.live=false; const auto passedBeforePlayback=h.passed; effect->receiveEvent(&note);
    check(h.passed==passedBeforePlayback+1 && h.cc.empty(),"sequencer notes remain transparent");
    effect->playAction(0,2424,false);
    check(h.cc.size()==2 && h.cc[0].d2==127 && h.cc[0].tick==0 && !h.cc[0].live &&
          h.cc[1].d2==0 && h.cc[1].tick==2424 && h.queued==2,
          "playback must schedule sustain and release after note length plus 2400ms");
    check(h.originals.back().length==120 && h.originals.back().d2==100 && !h.originals.back().live,
          "original playback note duration velocity and lane are untouched");
    h.cc.clear();
    note.status=0xb0;note.d1=64;note.d2=127;effect->receiveEvent(&note);
    check(h.cc.size()==1 && h.cc.back().d2==127,"recorded pedal must pass through instead of being swallowed");
    effect->stopAction(0);h.cc.clear();
    note={};note.length=-1;effect->receiveEvent(&note);
    effect->startAction(0);
    check(h.cc.size()==1 && h.cc.back().d2==127,"starting recording does not release a held live chord");
    note.status=0x80;note.d2=0;effect->receiveEvent(&note);
    effect->playAction(0,960,false);
    fakeMicros+=2400000;effect->playAction(26389062,26391366,true);
    check(h.cc.size()==2 && h.cc.back().d2==0,"live recording tail follows real time while transport moves");
    effect->stopAction(0);h.cc.clear();
    // A sequencer can provide lookahead events before startAction.
    note={};note.live=false;note.tick=480;note.length=960;effect->receiveEvent(&note);
    effect->startAction(480);effect->playAction(480,600,false);
    check(h.cc.size()==1 && h.cc[0].tick==480 && h.cc[0].d2==127,"start preserves sequencer lookahead");
    effect->swapAction(0,1920);
    check(h.cc.back().d2==0,"loop jump releases the old sequencer pedal");
    h.cc.clear();note.tick=0;effect->receiveEvent(&note);effect->playAction(0,120,false);
    check(h.cc.size()==1 && h.cc[0].d2==127,"loop restarts sustain at the new position");
    effect->stopAction(0);h.cc.clear();
    // Long notes must not be mistaken for short realtime key presses.
    note={};note.live=false;note.tick=120;note.length=9600;note.channel=5;
    effect->receiveEvent(&note);effect->playAction(0,9600,false);
    check(h.cc.size()==1 && h.cc[0].tick==120 && h.cc[0].channel==5,"sustain stays down throughout a long recorded note");
    effect->playAction(9600,12024,false);
    check(h.cc.size()==2 && h.cc.back().tick==12024 && h.cc.back().d2==0,"long recorded note releases at its own end plus tail");
    effect->stopAction(0);h.cc.clear();
    note={};note.length=-1;note.channel=2;effect->receiveEvent(&note);
    effect->swapAction(0,1920);effect->positAction(960);
    check(h.cc.size()==1 && h.cc[0].d2==127,"transport jumps do not interrupt held live keys");
    note.status=0x80;note.d2=0;effect->receiveEvent(&note);
    fakeMicros+=2400000;effect->playAction(0,99999999,true);
    check(h.cc.size()==2 && h.cc.back().d2==0,"live keys still release after a transport jump");
    effect->stopAction(0);h.cc.clear();
    // Realtime and sequencer lanes use different clocks and MIDI channels.
    note={};note.channel=1;note.length=-1;effect->receiveEvent(&note);
    Midi recorded;recorded.live=false;recorded.channel=4;
    effect->receiveEvent(&recorded);effect->playAction(0,2424,false);
    check(h.cc.size()==3 && h.cc[0].channel==1 && h.cc[1].channel==4 && h.cc[2].channel==4,
          "playback release cannot release the live keyboard on another channel");
    note.status=0x80;note.d2=0;effect->receiveEvent(&note);
    fakeMicros+=2400000;effect->playAction(0,99999999,true);
    check(h.cc.size()==4 && h.cc.back().channel==1 && h.cc.back().d2==0,"mixed lanes release independently");
    effect->stopAction(0); h.cc.clear();
    check(h.created==h.destroyed,"host-owned event lifetime balanced");
    h.cc.clear();note={};note.channel=-1;effect->receiveEvent(&note);
    check(h.cc.size()==1 && h.cc[0].channel==0,"inherit unspecified event channel from track");
    effect->stopAction(0);
    long bytes=0;check(effect->getChunk(nullptr,&bytes)==kResultOk && bytes==48,"state length");
    std::array<char,48> state{};long available=48;
    check(effect->getChunk(state.data(),&available)==kResultOk,"save state");
    check(effect->setChunk(state.data(),43)==kInvalidArgument,"reject truncated state");
    int32 off=0;std::memcpy(state.data()+3*sizeof(int32),&off,sizeof(off));
    check(effect->setChunk(state.data(),48)==kResultOk,"restore disabled state");
    h.cc.clear();note={};effect->receiveEvent(&note);check(h.cc.empty(),"disabled state bypasses automatic pedal");
    int32 theme=4;std::memcpy(state.data()+44,&theme,4);
    check(effect->setChunk(state.data(),48)==kResultOk,"restore selected appearance");
    std::array<char,48> saved{};effect->getChunk(saved.data(),nullptr);
    int32 restoredTheme=0;std::memcpy(&restoredTheme,saved.data()+44,4);
    check(restoredTheme==4,"theme persists in project state");
    int32 legacyVersion=1;std::memcpy(state.data()+4,&legacyVersion,4);
    check(effect->setChunk(state.data(),44)==kResultOk,"load original version-one projects");
    effect->getChunk(saved.data(),nullptr);std::memcpy(&restoredTheme,saved.data()+44,4);
    check(restoredTheme==0,"legacy state defaults to ice blue");
    IUIFactory* ui=nullptr;
    check(effect->queryInterface(IUIFactory_iid,reinterpret_cast<void**>(&ui))==kResultOk,"UI factory interface");
    char editorName[]="editor",templateName[128]{};long xmlSize=0;
    check(ui->getViewDescription(editorName,templateName,nullptr,&xmlSize)==kResultOk && xmlSize>0,"editor XML size");
    std::vector<char> xml(xmlSize+1);check(ui->getViewDescription(editorName,nullptr,xml.data(),nullptr)==kResultOk,"documented host second XML call has null size");
    check(xml[xmlSize-1]=='>',"XML byte count excludes C string terminator");
    check(std::strstr(xml.data(),"BassPedalEditor")!=nullptr,"custom editor declared");
    IUIFactory2* ui2=nullptr;
    check(ui->queryInterface(IUIFactory2_iid,reinterpret_cast<void**>(&ui2))==kResultOk,"host resource factory interface");
    ui2->release();
    IGenericPlugController* generic=nullptr;
    check(effect->queryInterface(IGenericPlugController_iid,reinterpret_cast<void**>(&generic))==kResultOk,"host generic controller interface");
    check(generic->countParameters()==0,"custom editor state is chunk based");generic->release();
    std::array<unsigned char,24> uid;uid.fill(0xA5);
    check(effect->getUID(reinterpret_cast<char*>(uid.data()+4))==kResultOk,"binary state identifier");
    for(int i=0;i<4;++i)check(uid[i]==0xA5 && uid[i+20]==0xA5,"state UID writes exactly 16 bytes");
    ui->release();
    effect->terminate();effect->release();check(h.refs==1,"host interface released");
    std::cout<<"Native sustain host simulation passed\n";return 0;
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
