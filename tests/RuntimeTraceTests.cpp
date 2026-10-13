#include "NativeMidi/RuntimeTrace.h"
#include <iostream>
int main(){
    RuntimeTrace trace(false);
    for(size_t i=0;i<RuntimeTrace::capacity;++i)trace.push('N',i,1,2,3,4,5);
    trace.push('X');
    std::array<RuntimeTrace::Row,RuntimeTrace::capacity> rows{};
    if(trace.take(rows)!=RuntimeTrace::capacity || trace.dropped!=1)return 1;
    for(size_t i=0;i<rows.size();++i)if(rows[i].a!=i || rows[i].kind!='N' || rows[i].f!=5)return 2;
    if(trace.take(rows)!=0)return 3;
    trace.push('C',1);if(trace.take(rows)!=1 || rows[0].kind!='C')return 4;
    {
        RuntimeTrace writer;writer.push('I',480);
        wchar_t temp[MAX_PATH]{},path[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);
        swprintf_s(path,L"%sPianoAutoSustain-trace-%lu-1.log",temp,GetCurrentProcessId());
        bool readable=false;
        for(int i=0;i<100 && !readable;++i){
            auto file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                  nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(file!=INVALID_HANDLE_VALUE){char buffer[64]{};DWORD got=0;
                readable=ReadFile(file,buffer,sizeof(buffer),&got,nullptr) && got>0;CloseHandle(file);}
            if(!readable)Sleep(10);
        }
        if(!readable){std::cerr<<"Trace must be readable while plugin is running\n";return 5;}
    }
    std::cout<<"Bounded trace order, overflow and reuse passed\n";
}
