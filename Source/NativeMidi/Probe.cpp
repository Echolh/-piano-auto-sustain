#include "Interfaces.h"
#include "ProbeLog.h"
#include <atomic>
#include <cstring>
#include <new>
#ifdef BASS_PEDAL_NATIVE_SUSTAIN
#include "Sustain.h"
#endif

// Deliberately transparent: this module establishes the host ABI before any
// experimental processing or GUI code is allowed into a user's project.
class MidiInsertProbe final : public IMidiEffect {
    std::atomic<uint32> refs{1};
    IMEAccessor* accessor = nullptr;
public:
    ~MidiInsertProbe() { terminate(); }
    tresult PLUGIN_API queryInterface(const TUID id, void** out) override {
        probeLog("effect query ",id);
        if (!out) return kInvalidArgument;
        *out = nullptr;
        if (!std::memcmp(id,FUnknown_iid,16) || !std::memcmp(id,IPluginBase_iid,16)
            || !std::memcmp(id,IMidiEffect_iid,16)) {
            *out=static_cast<IMidiEffect*>(this); addRef(); return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override { auto n=--refs; if(!n) delete this; return n; }
    tresult PLUGIN_API initialize(FUnknown* host) override {
        if (!host || accessor) return kInvalidArgument;
        auto result=host->queryInterface(IMEAccessor_iid,reinterpret_cast<void**>(&accessor));
        probeLog(result==kResultOk ? "initialized with MIDI accessor" : "MIDI accessor unavailable");
        return result;
    }
    tresult PLUGIN_API terminate() override {
        if(accessor) { accessor->release(); accessor=nullptr; }
        return kResultOk;
    }
    tresult PLUGIN_API getCaps(MidiEffectCaps* caps) override {
        probeLog("getCaps");
        if(!caps || caps->_sizeof < sizeof(MidiEffectCaps)) return kInvalidArgument;
        caps->flags=0; return kResultOk;
    }
    tresult PLUGIN_API configure(long usage) override {
        probeLog("configure");
        return usage==kMEUsageInsert ? kResultOk : kNotImplemented;
    }
    tresult PLUGIN_API receiveEvent(IMEObjectID event) override {
        if(!accessor || !event) return kInvalidArgument;
        return accessor->passToOutput(event);
    }
    tresult PLUGIN_API playAction(long,long,bool) override { return kResultOk; }
    tresult PLUGIN_API positAction(long) override { return kResultOk; }
    tresult PLUGIN_API swapAction(long,long) override { return kResultOk; }
    tresult PLUGIN_API stopAction(long) override { return kResultOk; }
    tresult PLUGIN_API startAction(long) override { return kResultOk; }
};

class ProbeFactory final : public IPluginFactory {
    std::atomic<uint32> refs{1};
public:
    tresult PLUGIN_API queryInterface(const TUID id,void** out) override {
        probeLog("factory query ",id);
        if(!out) return kInvalidArgument;
        *out=nullptr;
        if(!std::memcmp(id,FUnknown_iid,16) || !std::memcmp(id,IPluginFactory_iid,16)) {
            *out=static_cast<IPluginFactory*>(this); addRef(); return kResultOk;
        }
        return kNoInterface;
    }
    uint32 PLUGIN_API addRef() override { return ++refs; }
    uint32 PLUGIN_API release() override { auto n=--refs; if(!n) delete this; return n; }
    tresult PLUGIN_API getFactoryInfo(PFactoryInfo* info) override {
        if(!info) return kInvalidArgument;
        *info={}; std::strcpy(info->vendor,"Amu music");
        // The only text-bearing interface exposed is the current factory ABI.
        // No obsolete ASCII GUI or parameter interfaces are advertised.
        info->flags=PFactoryInfo::kUnicode;
        return kResultOk;
    }
    int32 PLUGIN_API countClasses() override { return 1; }
    tresult PLUGIN_API getClassInfo(int32 index,PClassInfo* info) override {
        if(index!=0 || !info) return kInvalidArgument;
        *info={}; std::memcpy(info->cid,BassPedalProbeID,16);
        info->cardinality=PClassInfo::kManyInstances;
        std::strcpy(info->category,kMidiModuleClass);
#ifdef BASS_PEDAL_NATIVE_SUSTAIN
        std::strcpy(info->name,"Piano Auto Sustain");
#else
        std::strcpy(info->name,"Bass Pedal MIDI Probe");
#endif
        return kResultOk;
    }
    tresult PLUGIN_API createInstance(FIDString cid,FIDString iid,void** out) override {
        if(!out) return kInvalidArgument;
        *out=nullptr;
        if(!cid || !iid || std::memcmp(cid,BassPedalProbeID,16)) return kNoInterface;
#ifdef BASS_PEDAL_NATIVE_SUSTAIN
        auto* instance=new(std::nothrow) MidiSustain;
#else
        auto* instance=new(std::nothrow) MidiInsertProbe;
#endif
        if(!instance) return kOutOfMemory;
        auto result=instance->queryInterface(iid,out); instance->release(); return result;
    }
};

extern "C" __declspec(dllexport) IPluginFactory* PLUGIN_API GetPluginFactory() {
    probeLog("GetPluginFactory");
    return new(std::nothrow) ProbeFactory;
}
