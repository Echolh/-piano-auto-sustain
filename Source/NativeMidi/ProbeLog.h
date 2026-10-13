#pragma once
#include <windows.h>
#include <cstdio>
#include <cwchar>
// Interface diagnostics only: no notes, project names or audio are logged.
// Never called from receiveEvent/playAction.
inline void probeLog(const char* operation,const char* iid=nullptr) {
    wchar_t path[MAX_PATH]{};
    auto n=GetTempPathW(MAX_PATH,path);
    if(!n || n+32>=MAX_PATH) return;
    std::wcscat(path,L"BassPedalMidiProbe.log");
    FILE* file=nullptr;
    if(_wfopen_s(&file,path,L"ab") || !file) return;
    std::fprintf(file,"%lu %s",GetCurrentProcessId(),operation);
    if(iid) for(int i=0;i<16;++i) std::fprintf(file,"%02X",static_cast<unsigned char>(iid[i]));
    std::fprintf(file,"\r\n"); std::fclose(file);
}
