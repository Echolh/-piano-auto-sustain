#include "NativeMidi/Interfaces.h"
#include <windows.h>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
std::wstring versionString(const wchar_t* path,const wchar_t* field) {
    DWORD ignored=0; const auto size=GetFileVersionInfoSizeW(path,&ignored); check(size>0,"version resource size");
    std::vector<unsigned char> data(size); check(GetFileVersionInfoW(path,0,size,data.data())!=FALSE,"version resource read");
    struct Translation { WORD language; WORD codepage; }* translations=nullptr; UINT translationSize=0;
    check(VerQueryValueW(data.data(),L"\\VarFileInfo\\Translation",reinterpret_cast<void**>(&translations),&translationSize)!=FALSE,"version translation");
    wchar_t query[128]{}; swprintf_s(query,L"\\StringFileInfo\\%04x%04x\\%s",translations[0].language,translations[0].codepage,field);
    wchar_t* value=nullptr; UINT valueSize=0; check(VerQueryValueW(data.data(),query,reinterpret_cast<void**>(&value),&valueSize)!=FALSE,"version string");
    return value;
}
int wmain(int argc,wchar_t** argv) {
    try {
        check(argc==2,"DLL path required");
        const std::wstring path=argv[1];
        const bool probe=path.find(L"Probe")!=std::wstring::npos;
        if(!probe) {
            const std::wstring expected=L"Piano Auto Sustain Cubase.dll";
            check(path.size()>=expected.size() && path.compare(path.size()-expected.size(),expected.size(),expected)==0,"Cubase DLL filename");
            check(versionString(argv[1],L"ProductVersion")==L"1.0.0","Cubase DLL ProductVersion");
            check(versionString(argv[1],L"OriginalFilename")==L"Piano Auto Sustain Cubase.dll","Cubase DLL resource filename");
        }
        auto dll=LoadLibraryW(argv[1]); check(dll!=nullptr,"LoadLibrary");
        using GetFactory=IPluginFactory* (PLUGIN_API*)();
        auto entry=reinterpret_cast<GetFactory>(GetProcAddress(dll,"GetPluginFactory"));
        check(entry!=nullptr,"factory export");
        auto* factory=entry(); check(factory!=nullptr,"factory creation");
        PFactoryInfo info{}; check(factory->getFactoryInfo(&info)==kResultOk,"factory info");
        check((info.flags&PFactoryInfo::kUnicode)!=0,"Unicode flag");
        check(factory->countClasses()==1,"class count");
        PClassInfo cls{}; check(factory->getClassInfo(0,&cls)==kResultOk,"class info");
        check(!std::strcmp(cls.category,kMidiModuleClass),"MIDI insert category");
        IMidiEffect* effect=nullptr;
        check(factory->createInstance(cls.cid,IMidiEffect_iid,reinterpret_cast<void**>(&effect))==kResultOk,"create MIDI interface");
        MidiEffectCaps caps{}; caps._sizeof=sizeof(caps);
        const bool probeClass=std::strstr(cls.name,"Probe")!=nullptr;
        check(effect->getCaps(&caps)==kResultOk && caps.flags==(probeClass?0:kMEIsEditable),"editor capability matches module");
        check(effect->initialize(nullptr)==kInvalidArgument,"reject null host");
        check(effect->configure(kMEUsageInsert)==kResultOk,"insert usage");
        check(effect->configure(kMEUsageOffline)==kNotImplemented,"reject unsupported usage");
        check(effect->receiveEvent(nullptr)==kInvalidArgument,"uninitialized event rejected");
        void* unknown=reinterpret_cast<void*>(1); TUID bogus{}; bogus[1]=42;
        check(effect->queryInterface(bogus,&unknown)==kNoInterface && unknown==nullptr,"unsupported interface");
        effect->terminate(); check(effect->release()==0,"instance lifetime");
        check(factory->release()==0,"factory lifetime"); FreeLibrary(dll);
        std::cout<<"Native DLL factory and ABI smoke checks passed. Cubase recognition is NOT tested.\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
