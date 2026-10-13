#include "NativeMidi/Editor.h"
#include <cstring>
#include <iostream>
int main(int argc,char** argv) {
    auto luminance=[](std::uint32_t color){double value=0;int shifts[]={16,8,0};double weights[]={.2126,.7152,.0722};for(int i=0;i<3;++i){double v=((color>>shifts[i])&255)/255.;value+=weights[i]*(v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4));}return value;};
    auto contrast=[&](auto a,auto b){double x=luminance(a),y=luminance(b);return (std::max(x,y)+.05)/(std::min(x,y)+.05);};
    for(const auto& t:pedalThemes){
        for(auto bg:{t.top,t.bottom,t.surface,t.control,t.active,t.pressed})if(contrast(t.ink,bg)<4.5)return 24;
        for(auto bg:{t.top,t.bottom,t.surface})if(contrast(t.muted,bg)<4.5)return 25;
    }
    EditorDpiScope dpi;
    bool show=argc>1 && !std::strcmp(argv[1],"--show");
    wchar_t executable[MAX_PATH]{};GetModuleFileNameW(nullptr,executable,MAX_PATH);
    auto filename=std::wcsrchr(executable,L'\\');
    show=show || _wcsicmp(filename?filename+1:executable,L"BassPedalPreview.exe")==0;
    PedalMotion motion;
    for(int i=0;i<20;++i)motion.step(true,16);
    if(motion.position<.99f||motion.position>1.f)return 19;
    motion.step(false,16);if(motion.position<=0.f||motion.position>=1.f)return 20;
    motion.step(true,16,false);if(motion.position!=1.f)return 21;
    auto model=std::make_shared<NativeControls>();
    auto parent=CreateWindowExW(0,L"STATIC",L"Piano Auto Sustain - Visual preview (demo)",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,710,590,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!parent)return 1;
    auto* editor=new NativeEditor(model);
    if(editor->attached(parent)!=kResultOk)return 2;
    auto child=GetWindow(parent,GW_CHILD);
    auto language=GetDlgItem(child,100);auto enabled=GetDlgItem(child,101);
    wchar_t text[128]{};GetWindowTextW(enabled,text,128);
    if(std::wcscmp(text,L"启用自动踏板"))return 3;
    GetWindowTextW(GetDlgItem(child,116),text,128);
    if(std::wcscmp(text,L"如何把延音一起录进去"))return 30;
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(100,BN_CLICKED),reinterpret_cast<LPARAM>(language));
    GetWindowTextW(enabled,text,128);
    if(std::wcscmp(text,L"Automatic pedal") || model->values[0]!=0)return 4;
    GetWindowTextW(GetDlgItem(child,116),text,128);
    if(std::wcscmp(text,L"How to record sustain"))return 31;
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(100,BN_CLICKED),reinterpret_cast<LPARAM>(language));
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(101,BN_CLICKED),reinterpret_cast<LPARAM>(enabled));
    if(model->values[1]!=0)return 7;
    auto split=GetDlgItem(child,203);SetWindowTextW(split,L"66");
    if(model->values[3]!=66)return 8;
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(110,BN_CLICKED),0);
    if(model->action!=1)return 9;
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(111,BN_CLICKED),0);
    if(model->action!=2)return 10;
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(113,BN_CLICKED),0);
    if(model->values[3]!=59 || model->values[1]!=1 || model->values[0]!=1)return 11;
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(115,BN_CLICKED),0);
    if((GetWindowLongW(GetDlgItem(child,204),GWL_STYLE)&WS_VISIBLE)==0 || (GetWindowLongW(split,GWL_STYLE)&WS_VISIBLE)!=0)return 12;
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(114,BN_CLICKED),0);
    if((GetWindowLongW(split,GWL_STYLE)&WS_VISIBLE)==0)return 13;
    for(int theme=0;theme<pedalThemeCount;++theme){
        SendMessageW(child,WM_COMMAND,MAKEWPARAM(500+theme,BN_CLICKED),0);
        if(model->theme!=theme)return 22;
    }
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(113,BN_CLICKED),0);
    if(model->theme!=pedalThemeCount-1)return 23;
    SendMessageW(child,WM_COMMAND,MAKEWPARAM(500,BN_CLICKED),0);
    for(int percent:{100,150,200,100}) {
        int width=760*percent/100,height=500*percent/100;
        RECT desired{0,0,width,height};AdjustWindowRect(&desired,WS_OVERLAPPEDWINDOW,FALSE);
        SetWindowPos(parent,nullptr,0,0,desired.right-desired.left,desired.bottom-desired.top,SWP_NOMOVE|SWP_NOZORDER);
        ViewRect logical(0,0,760,500);editor->onSize(&logical);
        RECT actual{};GetClientRect(child,&actual);
        RECT parentClient{};GetClientRect(parent,&parentClient);
        if(actual.right!=parentClient.right || actual.bottom!=parentClient.bottom){std::cerr<<"client mismatch "<<actual.right<<","<<actual.bottom<<" vs "<<parentClient.right<<","<<parentClient.bottom;return 5;}
        RECT editRect{};GetWindowRect(GetDlgItem(child,207),&editRect);MapWindowPoints(nullptr,child,reinterpret_cast<POINT*>(&editRect),2);
        if(editRect.right>parentClient.right || editRect.bottom>parentClient.bottom || editRect.left<0){std::cerr<<"edit clipped "<<percent;return 6;}
    }
    // Reproduce legacy host callbacks running in an unaware DPI context.
    editor->removed();
    auto oldDpi=SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_UNAWARE);
    auto attached=editor->attached(parent);
    SetThreadDpiAwarenessContext(oldDpi);
    if(attached!=kResultOk)return 14;
    child=GetWindow(parent,GW_CHILD);
    RECT parentRect{},childRect{};GetClientRect(parent,&parentRect);GetClientRect(child,&childRect);
    if(parentRect.right!=childRect.right||parentRect.bottom!=childRect.bottom)return 15;
    editor->removed();
    RECT large{0,0,1140,750};AdjustWindowRect(&large,WS_OVERLAPPEDWINDOW,FALSE);
    SetWindowPos(parent,nullptr,0,0,large.right-large.left,large.bottom-large.top,SWP_NOMOVE|SWP_NOZORDER);
    auto holder=CreateWindowExW(0,L"STATIC",L"",WS_CHILD|WS_VISIBLE,0,0,760,500,parent,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(editor->attached(holder)!=kResultOk)return 16;
    if(GetWindowLongPtrW(holder,GWL_STYLE)&WS_VISIBLE)return 28;
    ViewRect nestedSize(0,0,760,500);editor->onSize(&nestedSize);
    GetClientRect(parent,&parentRect);GetClientRect(GetWindow(parent,GW_CHILD),&childRect);
    RECT holderRect{};GetClientRect(holder,&holderRect);
    if(holderRect.right!=760||holderRect.bottom!=500)return 26;
    for(int repeat=0;repeat<100;++repeat)editor->onSize(&nestedSize);
    RECT stable{};GetClientRect(parent,&stable);
    if(stable.right!=parentRect.right||stable.bottom!=parentRect.bottom)return 27;
    if(parentRect.right!=childRect.right||parentRect.bottom!=childRect.bottom)return 17;
    editor->removed();
    if(!(GetWindowLongPtrW(holder,GWL_STYLE)&WS_VISIBLE))return 29;
    DestroyWindow(holder);
    if(editor->attached(parent)!=kResultOk)return 18;
    if(show) {
        RECT desired{0,0,1140,750};AdjustWindowRect(&desired,WS_OVERLAPPEDWINDOW,FALSE);
        SetWindowPos(parent,nullptr,0,0,desired.right-desired.left,desired.bottom-desired.top,SWP_NOMOVE|SWP_NOZORDER);
        ViewRect logical(0,0,760,500);editor->onSize(&logical);
        ShowWindow(parent,SW_SHOW);UpdateWindow(parent);
        auto demoStart=GetTickCount64();
        MSG msg{};while(IsWindow(parent)) {
            model->down=((GetTickCount64()-demoStart)/1800)%2;
            while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
            Sleep(20);
        }
    }
    editor->removed();editor->release();if(IsWindow(parent))DestroyWindow(parent);
    std::cout<<"Native bilingual editor attachment and language tests passed\n";return 0;
}
