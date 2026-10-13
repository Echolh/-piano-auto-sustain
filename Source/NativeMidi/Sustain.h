#pragma once
#include "Interfaces.h"
#include "Timeline.h"
#include "Editor.h"
#include "ProbeLog.h"
#include "RuntimeTrace.h"
#include <atomic>
#include <cstring>
#include <chrono>

class MidiSustain final : public IMidiEffect, public IUIFactory2, public IGenericPlugController, public IViewFactory, public IPersistentChunk {
    std::atomic<uint32> refs{1};
    IMEAccessor* accessor=nullptr;
    IMasterTrackInfo* master=nullptr;
    NativeTimeline lanes[2]; // sequencer, MIDI Thru
    bool desired[2][16]{};
    bool output[16]{};
    bool recordedPedal[16]{};
    double ppq=480;
    bool failed=false;
    long positions[2]{};
    std::shared_ptr<NativeControls> controls=std::make_shared<NativeControls>();
    bool enabled=true;
    RuntimeTrace trace;
    unsigned traceSlices[2]{};
    static std::int64_t steadyMicros() {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    std::int64_t (*clockMicros)();
    std::int64_t liveOrigin=0;
    bool liveClockReady=false;
    long liveClockTick=0;
    std::int64_t liveClockMicros=0;
    double liveClockRate=0;
    long liveOutputTick() const {
        const double tick=liveClockTick+std::max<std::int64_t>(0,clockMicros()-liveClockMicros)*liveClockRate;
        return static_cast<long>(std::clamp(std::round(tick),
            static_cast<double>(std::numeric_limits<long>::min()),static_cast<double>(std::numeric_limits<long>::max())));
    }
    tresult forward(IMEObjectID event,bool live) {
        const long originalTick=accessor->getStart(event);
        const long target=live && liveClockReady?liveOutputTick():originalTick;
        // Modern Cubase records the input's absolute timestamp upstream, but
        // the monitored output runs on a transport-relative clock. Translate
        // only a clearly different clock origin, on a copy owned by us.
        if(live && liveClockReady && std::abs(static_cast<double>(originalTick)-target)>ppq) {
            auto copy=accessor->duplicateMidiEvent(event);
            if(copy) {
                const auto changed=accessor->setStart(copy,target);
                if(changed==kResultOk) {
                    const auto result=accessor->passToOutput(copy);
                    trace.push('M',originalTick,target,accessor->getStatus(copy),result);
                    accessor->destroyEvent(copy);return result;
                }
                accessor->destroyEvent(copy);
            }
        }
        return accessor->passToOutput(event);
    }
    std::int64_t liveNow() const {return std::max<std::int64_t>(0,clockMicros()-liveOrigin);}
    void advanceLive() {
        // MIDI Thru timing must not depend on a stopped transport, its PPQ
        // resolution, or host absolute timestamps. All live deadlines use us.
        lanes[1].advance(liveNow(),60,1000000,[&](auto,auto e){pedal(1,positions[1],e,true);});
    }
    void applyControls() {
        bool next=controls->values[1].load()!=0;
        if(next!=enabled) {reset(positions[1]);enabled=next;}
        for(auto& lane:lanes) {
            auto& s=lane.engine.settings;
            s.enabled=enabled;s.noteHold=false;s.chordProtection=controls->values[2].load()!=0;
            s.split=controls->values[3];s.groupMs=controls->values[4];s.gapMs=controls->values[5];
            s.guardMs=controls->values[6];s.tailMs=controls->values[7];s.octaveSame=controls->values[8].load()!=0;
        }
        int action=controls->action.exchange(0);
        if(action==2) reset(positions[1]);
        if(action==1) for(int i=0;i<2;++i) lanes[i].repedal([&](auto t,auto e){pedal(i,i==1?positions[1]:t,e,i==1);});
    }
    void send(long tick,int channel,bool down,bool immediate,bool direct) {
        if(!accessor) return;
        // Cubase input timestamps remain absolute while immediate playAction
        // switches to a transport-relative clock. Every generated Thru CC,
        // including one emitted from receiveEvent, must use the output clock.
        if(immediate && liveClockReady)tick=liveOutputTick();
        auto e=accessor->createMidiEvent(kController,64,down?127:0,0);
        if(!e) {trace.push('A',tick,channel,down);controls->result=kOutOfMemory;return;}
        accessor->setChannel(e,channel); accessor->setStart(e,tick);
        accessor->setImmediateEvent(e,immediate);
        trace.push('V',accessor->isImmediateEvent(e),accessor->getChannel(e),accessor->getStart(e),
                   accessor->getStatus(e),accessor->getData1(e),accessor->getData2(e));
        auto result=direct?accessor->passToOutput(e):accessor->passToOutputQueue(e);
        trace.push('C',immediate,direct,tick,channel,down,result);
        controls->result=result;
        if(result==kResultOk)++controls->sent;
        else ++controls->outputErrors;
        accessor->destroyEvent(e);
    }
    void pedal(int lane,std::int64_t tick,PedalEngine::Event e,bool direct) {
        int c=e.status&15;
        if(lane==0 && recordedPedal[c])return;
        desired[lane][c]=e.data2>=64;
        // Conservative arbitration: a live input cannot release the pedal that
        // the sequencer still needs on the same channel, or vice versa.
        bool down=desired[0][c] || desired[1][c];
        if(down!=output[c]) { output[c]=down; send(static_cast<long>(tick),c,down,lane==1,direct); }
        int mask=0;for(int ch=0;ch<16;++ch) if(output[ch]) mask|=1<<ch;controls->down=mask;
    }
    double tempo(long tick) {
        MidiContextInfo context{};
        return accessor && accessor->getContextInfo(tick,&context)==kResultOk && context.tempo>0
            // Cubase 15 trace reports whole BPM (70). The legacy SDK and
            // older hosts report BPM * 1000 (70000). Accept both conventions.
            ? (context.tempo>=1000 ? context.tempo/1000.0 : static_cast<double>(context.tempo)) : 120.0;
    }
    void reset(long position) {
        trace.push('R',position);
        for(int i=0;i<2;++i) {
            lanes[i].reset(i==1?0:position,[](auto,auto){});
            positions[i]=position;
        }
        std::memset(desired,0,sizeof(desired));
        std::memset(recordedPedal,0,sizeof(recordedPedal));
        for(int c=0;c<16;++c) if(output[c]) { send(position,c,false,true,true); output[c]=false; }
        controls->down=0;
        liveOrigin=clockMicros();
        liveClockReady=false;
        failed=false;
    }
    void resetSequencer(long position) {
        trace.push('Q',position);
        std::memset(recordedPedal,0,sizeof(recordedPedal));
        // Transport moves must not erase keys still held on the live keyboard.
        lanes[0].reset(position,[](auto,auto){});
        positions[0]=position;
        for(int c=0;c<16;++c) pedal(0,position,{0xb0|c,64,0},true);
        failed=false;
    }
public:
    explicit MidiSustain(std::int64_t (*clock)()=steadyMicros):clockMicros(clock),liveOrigin(clock()) {}
    ~MidiSustain() { terminate(); }
    tresult PLUGIN_API queryInterface(const TUID id,void** out) override {
        probeLog("sustain query ",id);
        if(!out) return kInvalidArgument;
        *out=nullptr;
        if(!std::memcmp(id,FUnknown_iid,16)||!std::memcmp(id,IPluginBase_iid,16)||!std::memcmp(id,IMidiEffect_iid,16)) {
            *out=static_cast<IMidiEffect*>(this); addRef(); return kResultOk;
        }
        if(!std::memcmp(id,IUIFactory_iid,16))*out=static_cast<IUIFactory*>(this);
        if(!std::memcmp(id,IUIFactory2_iid,16))*out=static_cast<IUIFactory2*>(this);
        if(!std::memcmp(id,IPlugController_iid,16))*out=static_cast<IPlugController*>(this);
        if(!std::memcmp(id,IGenericPlugController_iid,16))*out=static_cast<IGenericPlugController*>(this);
        if(!std::memcmp(id,IViewFactory_iid,16))*out=static_cast<IViewFactory*>(this);
        if(!std::memcmp(id,IPersistentChunk_iid,16))*out=static_cast<IPersistentChunk*>(this);
        if(*out) {addRef();return kResultOk;}
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override { auto n=--refs; if(!n) delete this; return n; }
    tresult PLUGIN_API initialize(FUnknown* host) override {
        if(!host || accessor) return kInvalidArgument;
        auto r=host->queryInterface(IMEAccessor_iid,reinterpret_cast<void**>(&accessor));
        if(r!=kResultOk || !accessor) return kNoInterface;
        host->queryInterface(IMasterTrackInfo_iid,reinterpret_cast<void**>(&master));
        if(master && master->getPpqBase()>0) ppq=master->getPpqBase();
        trace.push('I',static_cast<long long>(ppq),master!=nullptr);
        accessor->setCanPlayInStop(1); accessor->setInterval(1);
        return kResultOk;
    }
    tresult PLUGIN_API terminate() override {
        if(accessor) { reset(positions[1]); accessor->release(); accessor=nullptr; }
        if(master) { master->release(); master=nullptr; }
        return kResultOk;
    }
    tresult PLUGIN_API getCaps(MidiEffectCaps* caps) override {
        if(!caps || caps->_sizeof<sizeof(MidiEffectCaps)) return kInvalidArgument;
        caps->flags=kMEIsEditable; return kResultOk;
    }
    tresult PLUGIN_API configure(long usage) override { return usage==kMEUsageInsert?kResultOk:kNotImplemented; }
    tresult PLUGIN_API receiveEvent(IMEObjectID event) override {
        if(!accessor || !event) return kInvalidArgument;
        applyControls();
        int rawStatus=static_cast<int>(accessor->getStatus(event)),status=rawStatus&0xf0;
        int c=accessor->getChannel(event), d1=accessor->getData1(event), d2=accessor->getData2(event);
        controls->status=rawStatus;controls->channel=c;controls->velocity=d2;controls->lastTick=accessor->getStart(event);
        const bool live=accessor->isImmediateEvent(event);
        trace.push('N',live,rawStatus,c,accessor->getStart(event),accessor->getLength(event),d2>0);
        if(status==0xb0)trace.push('K',live,c,d1,d2,accessor->getStart(event));
        // Some host events inherit the track channel. Resolve inheritance
        // instead of silently disabling all processing for that event.
        if(c<0) {long track=accessor->getMidiChannel();c=track>=0&&track<16?static_cast<int>(track):rawStatus&15;}
        if(!enabled) return forward(event,live);
        if(failed || c<0 || c>15 || d1<0 || d1>127 || d2<0 || d2>127) {
            ++controls->rejected;
            return forward(event,live);
        }
        if(status==0xb0 && d1==64) {
            if(live)return kResultOk; // auto mode owns the keyboard pedal
            // MIDI inserts above Cubase's recording divider can print CC64.
            // Preserve those recorded decisions instead of generating a second
            // pedal performance over them. Detection resets on stop/seek.
            recordedPedal[c]=true;desired[0][c]=d2>=64;output[c]=d2>=64;
            int mask=0;for(int ch=0;ch<16;++ch)if(output[ch])mask|=1<<ch;controls->down=mask;
            return forward(event,false);
        }
        bool observed=status==0x80 || status==0x90 || (status==0xb0 && (d1==120||d1==121||d1==123));
        if(observed) {
            if(status==0x90 && d2>0) {
                ++controls->notes;
                if(live)++controls->liveNotes;else ++controls->sequenceNotes;
            }
            std::int64_t tick=accessor->getStart(event);
            const int lane=live?1:0;
            // Thru is timed by a monotonic clock; sequencer data uses PPQ.
            // Cubase playback notes carry a duration, not a separate note-off.
            // Observe that end internally, without modifying the host's note.
            if(live) {positions[1]=static_cast<long>(tick);tick=liveNow();}
            bool ok=lanes[lane].enqueue(tick,{status|c,d1,d2});
            const long length=accessor->getLength(event);
            if(!live && status==0x90 && d2>0 && length>=0)
                ok=lanes[0].enqueue(tick+std::max(1L,length),{0x80|c,d1,0}) && ok;
            if(!ok) {trace.push('X',lane,tick);reset(positions[0]);failed=true;}
            else if(live) advanceLive();
        }
        auto result=forward(event,live); // recording event stays untouched
        if(result!=kResultOk)trace.push('F',result,live,rawStatus);
        return result;
    }
    tresult PLUGIN_API playAction(long from,long to,bool immediate) override {
        if(!accessor) return kNotInitialized;
        if(immediate) {
            liveClockTick=to;liveClockMicros=clockMicros();
            liveClockRate=tempo(from)*ppq/60000000.0;liveClockReady=true;
        }
        ++controls->callbacks;controls->lastFrom=from;controls->lastTo=to;applyControls();
        const auto slice=++traceSlices[immediate?1:0];
        if(slice<=16 || slice%128==0) {
            MidiContextInfo ctx{};auto r=accessor->getContextInfo(from,&ctx);
            trace.push('P',immediate,from,to,ctx.tempo,lanes[0].queued(),lanes[1].queued());
            trace.push('H',r,ctx.tempo,ctx.ppqPosition,ctx.stopPosition,ctx.performedCyclesPpq,ctx.performedCyclesImmediatePpq);
        }
        if(immediate) {positions[1]=to;if(!failed)advanceLive();return kResultOk;}
        if(failed || to<from) return kResultOk;
        int i=immediate?1:0;
        double bpm=tempo(from);
        // Anchor a fresh lane at the beginning, not at the end of its first slice.
        lanes[i].advance(from,bpm,ppq,[&](auto t,auto e){pedal(i,t,e,false);});
        lanes[i].advance(to,bpm,ppq,[&](auto t,auto e){pedal(i,t,e,false);});
        positions[i]=to; return kResultOk;
    }
    tresult PLUGIN_API positAction(long p) override { resetSequencer(p); return kResultOk; }
    tresult PLUGIN_API swapAction(long p,long) override { resetSequencer(p); return kResultOk; }
    tresult PLUGIN_API stopAction(long p) override { reset(p); return kResultOk; }
    tresult PLUGIN_API startAction(long p) override {
        trace.push('S',p);
        // Like the SDK sample: start is a state notification, not a panic.
        // Clearing here loses lookahead notes and cuts a live recording chord.
        positions[0]=p; return kResultOk;
    }
    tresult PLUGIN_API createController(char* name,IPlugController** out) override {
        probeLog("createController");if(!out)return kInvalidArgument;*out=nullptr;
        if(!name || std::strcmp(name,"editor"))return kResultFalse;
        *out=static_cast<IPlugController*>(this);addRef();return kResultOk;
    }
    tresult PLUGIN_API getViewDescription(char* name,char* templateName,char* xml,long* size) override {
        char diagnostic[320]{};
        std::snprintf(diagnostic,sizeof(diagnostic),"editor request xml=%d size=%d notes=%d cc=%d skipped=%d status=%d ch=%d velocity=%d calls=%d tick=%d range=%d..%d result=%d",
            xml!=nullptr,size!=nullptr,controls->notes.load(),controls->sent.load(),controls->rejected.load(),controls->status.load(),controls->channel.load(),controls->velocity.load(),controls->callbacks.load(),controls->lastTick.load(),controls->lastFrom.load(),controls->lastTo.load(),controls->result.load());
        probeLog(diagnostic);
        if(!name||std::strcmp(name,"editor"))return kResultFalse;
        constexpr char description[]="<skin><template name=\"editor\"><view name=\"BassPedalEditor\" size=\"0,0,760,500\"/></template></skin>";
        if(templateName)std::strcpy(templateName,"editor");
        // SDK plugui.html explicitly permits a null size pointer on the second
        // call. size is output-only. Supply XML bytes, not the C-string NUL.
        if(xml)std::memcpy(xml,description,sizeof(description)-1);
        if(size)*size=sizeof(description)-1;
        return kResultOk;
    }
    tresult PLUGIN_API getResource(const char*,char*,long* size) override {
        if(size)*size=0;return kResultFalse; // Editor uses native controls, no image resources.
    }
    tresult PLUGIN_API getParameter(const char*,IParameter** out) override {if(out)*out=nullptr;return kResultFalse;}
    tresult PLUGIN_API parameterChanged(IParameter*,long) override {return kResultOk;}
    long PLUGIN_API countParameters() override {return 0;}
    tresult PLUGIN_API getParameterName(long,char str[128]) override {if(str)str[0]=0;return kResultFalse;}
    tresult PLUGIN_API getParameterIndexed(long,IParameter** out) override {if(out)*out=nullptr;return kResultFalse;}
    tresult PLUGIN_API createView(const char* name,ViewRect*,IPlugView** out) override {
        probeLog("createView");if(!out)return kInvalidArgument;*out=nullptr;
        if(!name||std::strcmp(name,"BassPedalEditor"))return kResultFalse;
        *out=new(std::nothrow) NativeEditor(controls);return *out?kResultOk:kOutOfMemory;
    }
    tresult PLUGIN_API testCondition(const char*) override {return kResultFalse;}
    tresult PLUGIN_API getUID(char* uid) override {
        if(!uid)return kInvalidArgument;
        static constexpr TUID storageID=INLINE_UID(0xCC7F240A,0x44114243,0xABFB530A,0xBA424A33);
        std::memcpy(uid,storageID,16);return kResultOk;
    }
    tresult PLUGIN_API getChunk(char* chunk,long* size) override {
        if(!size&&!chunk)return kInvalidArgument;constexpr long bytes=12*sizeof(int32);
        if(chunk){int32 data[12]={0x42504d31,2};for(int i=0;i<9;++i)data[i+2]=controls->values[i];data[11]=controls->theme;std::memcpy(chunk,data,bytes);}
        if(size)*size=bytes;return kResultOk;
    }
    tresult PLUGIN_API setChunk(char* chunk,long size) override {
        if(!chunk||(size!=44&&size!=48))return kInvalidArgument;int32 data[12]{};std::memcpy(data,chunk,size);
        if(data[0]!=0x42504d31||!((data[1]==1&&size==44)||(data[1]==2&&size==48)))return kInvalidArgument;
        for(int i=0;i<9;++i)controls->values[i]=NativeControls::clamp(i,data[i+2]);
        controls->theme=data[1]==2?std::clamp(data[11],0,pedalThemeCount-1):0;return kResultOk;
    }
};
