#pragma once
#include "Interfaces.h"
#include "ProbeLog.h"
#include "Appearance.h"
#include "../ProductManual.h"
#include <windows.h>
#include <gdiplus.h>
#include <atomic>
#include <array>
#include <memory>
#include <cwchar>
#include <algorithm>
#pragma comment(lib,"gdiplus.lib")

struct NativeControls {
    std::array<std::atomic<int>,9> values{{1,1,1,59,40,35,160,2400,1}};
    std::atomic<int> theme{0};
    std::atomic<int> action{0},notes{0},sent{0},rejected{0},channel{-1},status{0},velocity{0},down{0};
    std::atomic<int> callbacks{0},lastTick{0},lastFrom{0},lastTo{0},result{0},outputErrors{0};
    std::atomic<int> liveNotes{0},sequenceNotes{0};
    static int clamp(int i,int v) {
        constexpr int lo[]={0,0,0,0,5,1,0,50,0},hi[]={1,1,1,127,200,250,2000,10000,1};
        return std::clamp(v,lo[i],hi[i]);
    }
};

// Scope DPI changes to our own calls. Cubase can call a legacy view with an
// unaware thread context even when its parent uses physical pixel coordinates.
struct EditorDpiScope {
    DPI_AWARENESS_CONTEXT previous=SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    ~EditorDpiScope(){if(previous)SetThreadDpiAwarenessContext(previous);}
};

class NativeEditor final : public IPlugView {
    std::atomic<uint32> refs{1};
    std::shared_ptr<NativeControls> model;
    HWND window=nullptr;
    HWND coveredProxy=nullptr;
    bool proxyWasVisible=false;
    HINSTANCE instance=nullptr;
    HFONT font=nullptr,valueFont=nullptr;
    HBRUSH editBrush=CreateSolidBrush(RGB(29,37,51));
    ULONG_PTR gdip=0;
    int page=0,lastLanguage=-1,lastDown=-1,lastNotes=-1,lastSent=-1;
    double sx=1,sy=1;
    PedalMotion motion;
    ULONGLONG lastFrame=0,lastRefresh=0;
    int lastTheme=-1;
    bool animate=true;
    const PedalTheme& palette() const {return pedalThemes[std::clamp(model->theme.load(),0,pedalThemeCount-1)];}
    static Gdiplus::Color tone(std::uint32_t c){return Gdiplus::Color(0xff000000u|c);}
    static COLORREF nativeColor(std::uint32_t c){return RGB((c>>16)&255,(c>>8)&255,c&255);}
    struct Item {HWND handle;int id,x,y,w,h,page;};
    std::array<Item,32> items{};
    size_t count=0;
    static constexpr int width=760,height=500;
    bool zh() const{return model->values[0]!=0;}
    const wchar_t* tr(const wchar_t* cn,const wchar_t* en) const{return zh()?cn:en;}
    HWND item(int id) const{return GetDlgItem(window,id);}
    void add(const wchar_t* type,int id,int x,int y,int w,int h,int onPage=-1,DWORD style=BS_OWNERDRAW) {
        HWND c=CreateWindowExW(0,type,L"",WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_TABSTOP|style,x,y,w,h,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),instance,nullptr);
        items[count++]={c,id,x,y,w,h,onPage};
    }
    void label(int id,const wchar_t* text){SetWindowTextW(item(id),text);}
    void syncValues() {
        for(int i=3;i<=7;++i)if(GetFocus()!=item(200+i)) {
            wchar_t text[24]{},current[24]{};swprintf_s(text,L"%d",model->values[i].load());
            GetWindowTextW(item(200+i),current,24);
            if(wcscmp(text,current))SetWindowTextW(item(200+i),text);
        }
    }
    void refresh() {
        lastLanguage=model->values[0];
        lastTheme=model->theme;
        if(editBrush)DeleteObject(editBrush);editBrush=CreateSolidBrush(nativeColor(palette().surface));
        wchar_t themeTitle[96]{};swprintf_s(themeTitle,L"%s: %s",tr(L"配色",L"Theme"),zh()?palette().chinese:palette().english);label(130,themeTitle);
        label(100,tr(L"中文 / EN",L"EN / 中文"));
        label(101,tr(L"启用自动踏板",L"Automatic pedal"));
        label(102,tr(L"和弦保护",L"Chord protection"));
        label(108,tr(L"忽略八度变化",L"Ignore octaves"));
        label(110,tr(L"换踏一次",L"Repedal"));
        label(111,tr(L"释放踏板",L"Release pedal"));
        label(112,tr(L"诊断",L"Diagnostics"));
        label(113,tr(L"恢复默认",L"Reset defaults"));
        label(114,tr(L"演奏",L"Play"));label(115,tr(L"高级设置",L"Advanced"));
        label(117,tr(L"说明书",L"Guide"));
        label(116,tr(L"如何把延音一起录进去",L"How to record sustain"));
        label(120,L"−");label(121,L"+");label(122,L"−");label(123,L"+");
        syncValues();
        for(size_t i=0;i<count;++i){auto& c=items[i];ShowWindow(c.handle,c.page<0||c.page==page?SW_SHOW:SW_HIDE);InvalidateRect(c.handle,nullptr,FALSE);}
        InvalidateRect(window,nullptr,FALSE);
    }
    void layout() {
        if(!window||!count)return;
        EditorDpiScope dpi;
        RECT r{};GetClientRect(window,&r);sx=r.right/double(width);sy=r.bottom/double(height);
        double scale=std::min(sx,sy);
        HFONT next=CreateFontW(-std::max(11,int(14*scale)),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");
        HFONT nextValue=CreateFontW(-std::max(14,int(29*scale)),0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        for(size_t i=0;i<count;++i) {
            auto& c=items[i];MoveWindow(c.handle,int(c.x*sx),int(c.y*sy),int(c.w*sx),int(c.h*sy),TRUE);
            SendMessageW(c.handle,WM_SETFONT,reinterpret_cast<WPARAM>((c.id==203||c.id==207)?nextValue:next),TRUE);
        }
        if(font)DeleteObject(font);if(valueFont)DeleteObject(valueFont);font=next;valueFont=nextValue;
        InvalidateRect(window,nullptr,FALSE);
    }
    void fitParent() {
        EditorDpiScope dpi;
        RECT p{},r{};GetClientRect(GetParent(window),&p);GetClientRect(window,&r);
        if(p.right>0&&p.bottom>0&&(p.right!=r.right||p.bottom!=r.bottom))
            SetWindowPos(window,nullptr,0,0,p.right,p.bottom,SWP_NOZORDER|SWP_NOACTIVATE);
    }
    void createControls() {
        count=0;
        add(L"BUTTON",130,452,24,164,32);
        add(L"BUTTON",100,630,24,104,32);
        add(L"BUTTON",114,284,88,96,34);add(L"BUTTON",115,388,88,138,34);add(L"BUTTON",112,534,88,94,34);add(L"BUTTON",117,636,88,98,34);
        add(L"BUTTON",101,28,338,218,38);
        add(L"EDIT",203,306,184,126,42,0,ES_NUMBER|ES_CENTER);
        add(L"EDIT",207,538,184,148,42,0,ES_NUMBER|ES_CENTER);
        add(L"BUTTON",120,306,244,54,30,0);add(L"BUTTON",121,390,244,54,30,0);
        add(L"BUTTON",122,538,244,54,30,0);add(L"BUTTON",123,654,244,54,30,0);
        add(L"BUTTON",102,284,306,216,38,0);add(L"BUTTON",108,516,306,218,38,0);
        for(int i=0;i<3;++i)add(L"EDIT",204+i,594,151+i*70,116,32,1,ES_NUMBER|ES_CENTER);
        add(L"BUTTON",116,284,346,450,36,1);
        add(L"BUTTON",110,28,410,218,40);add(L"BUTTON",111,264,410,218,40);add(L"BUTTON",113,562,410,172,40);
        BOOL clientAnimation=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&clientAnimation,0);animate=clientAnimation!=FALSE;
        lastFrame=lastRefresh=GetTickCount64();motion.position=model->down?1.f:0.f;
        layout();refresh();SetTimer(window,1,16,nullptr);
    }
    static Gdiplus::Color color(int r,int g,int b){return Gdiplus::Color(255,r,g,b);}
    void text(Gdiplus::Graphics& g,const wchar_t* value,float x,float y,float w,float h,float size,Gdiplus::Color c,bool bold=false) {
        Gdiplus::FontFamily family(L"Microsoft YaHei UI");Gdiplus::Font f(&family,size,bold?Gdiplus::FontStyleBold:Gdiplus::FontStyleRegular,Gdiplus::UnitPixel);
        Gdiplus::SolidBrush b(c);Gdiplus::StringFormat format;format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        g.DrawString(value,-1,&f,Gdiplus::RectF(x,y,w,h),&format,&b);
    }
    void rounded(Gdiplus::Graphics& g,float x,float y,float w,float h,float radius,Gdiplus::Color fill) {
        Gdiplus::GraphicsPath p;float d=radius*2;
        p.AddArc(x,y,d,d,180,90);p.AddArc(x+w-d,y,d,d,270,90);p.AddArc(x+w-d,y+h-d,d,d,0,90);p.AddArc(x,y+h-d,d,d,90,90);p.CloseFigure();
        Gdiplus::SolidBrush b(fill);g.FillPath(&b,&p);
    }
    void drawPedal(Gdiplus::Graphics& g,float travel) {
        const auto& t=palette();float drop=19*travel;
        // A fixed heel hinge and a moving toe create the perspective of an
        // actual lever, with thickness, reflected light and a ground shadow.
        Gdiplus::SolidBrush shadow(Gdiplus::Color(70,0,0,0));
        g.FillEllipse(&shadow,68.f,238.f,151.f,24.f);
        Gdiplus::PointF base[]={{68,211},{116,183},{222,237},{168,269}};
        Gdiplus::SolidBrush rubber(color(20,23,28));g.FillPolygon(&rubber,base,4);
        Gdiplus::Pen baseEdge(color(61,67,77),2);g.DrawPolygon(&baseEdge,base,4);
        rounded(g,78,183,45,28,10,color(45,49,56));
        Gdiplus::SolidBrush hinge(color(105,114,124));g.FillEllipse(&hinge,80.f,183.f,34.f,16.f);
        auto outline=[&](Gdiplus::GraphicsPath& path,float depth) {
            path.StartFigure();path.AddLine(92.f,177.f+depth,117.f,165.f+depth);
            path.AddBezier(117.f,165.f+depth,126.f,161.f+depth,192.f,201.f+drop+depth,206.f,215.f+drop+depth);
            path.AddBezier(206.f,215.f+drop+depth,215.f,224.f+drop+depth,188.f,241.f+drop+depth,174.f,243.f+drop+depth);
            path.AddBezier(174.f,243.f+drop+depth,160.f,245.f+drop+depth,97.f,199.f+depth,90.f,188.f+depth);
            path.AddBezier(90.f,188.f+depth,86.f,182.f+depth,88.f,179.f+depth,92.f,177.f+depth);path.CloseFigure();
        };
        Gdiplus::GraphicsPath side;outline(side,8);
        Gdiplus::SolidBrush edge(color(72,82,95));g.FillPath(&edge,&side);
        Gdiplus::GraphicsPath top;outline(top,0);
        Gdiplus::LinearGradientBrush metal(Gdiplus::PointF(95,165),Gdiplus::PointF(179,245+drop),color(216,224,233),color(119,135,151));
        g.FillPath(&metal,&top);Gdiplus::Pen rim(color(230,237,246),1.3f);g.DrawPath(&rim,&top);
        Gdiplus::Pen reflection(Gdiplus::Color(175,255,255,255),3);
        g.DrawBezier(&reflection,112.f,176.f,138.f,189.f+drop*.3f,174.f,209.f+drop*.8f,191.f,224.f+drop);
        Gdiplus::Pen grooves(color(88,104,122),1.4f);
        for(int i=0;i<4;++i){float x=139.f+i*7.f,y=213.f+i*4.f+drop;
            g.DrawLine(&grooves,x,y,x+21,y-11);}
        Gdiplus::SolidBrush led(tone(travel>.5f?t.accent:t.border));g.FillEllipse(&led,78.f,218.f,7.f,4.f);
    }
    void paint(HDC dc) {
        RECT r{};GetClientRect(window,&r);
        HDC buffer=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,std::max(1L,r.right),std::max(1L,r.bottom));auto old=SelectObject(buffer,bitmap);
        {
            const auto& t=palette();
            Gdiplus::Graphics g(buffer);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);g.Clear(tone(t.bottom));g.ScaleTransform(float(sx),float(sy));
            Gdiplus::LinearGradientBrush backdrop(Gdiplus::Point(0,0),Gdiplus::Point(760,500),tone(t.top),tone(t.bottom));g.FillRectangle(&backdrop,0,0,760,500);
            auto ink=tone(t.ink),muted=tone(t.muted),gold=tone(t.accent);
            text(g,L"Piano Auto Sustain",28,17,416,38,26,ink,true);
            text(g,tr(L"钢琴自动延音",L"Automatic piano sustain"),29,55,450,23,13,muted);
            Gdiplus::Pen line(tone(t.border),1);g.DrawLine(&line,28.f,393.f,734.f,393.f);
            text(g,tr(L"实时踏板",L"Live pedal"),46,102,160,24,14,muted);
            bool down=model->down.load()!=0;
            drawPedal(g,motion.position);
            text(g,down?tr(L"延音中",L"Sustaining"):tr(L"已释放",L"Released"),91,276,135,29,18,down?gold:ink,true);
            if(page==0) {
                rounded(g,284,138,216,148,18,tone(t.surface));rounded(g,516,138,218,148,18,tone(t.surface));
                text(g,tr(L"低音识别上限",L"Bass upper limit"),306,146,184,27,14,muted);
                text(g,tr(L"松键后的延音",L"Release tail"),538,146,180,27,14,muted);
                wchar_t note[32]{};const wchar_t* names[]={L"C",L"C#",L"D",L"D#",L"E",L"F",L"F#",L"G",L"G#",L"A",L"A#",L"B"};int n=model->values[3];swprintf_s(note,L"%s%d",names[n%12],n/12-1);
                text(g,note,444,188,48,31,14,gold);text(g,L"ms",692,188,34,31,13,muted);
                text(g,tr(L"同和弦分解伴奏保持延音，和声变化时换踏。",L"Keep chord tones ringing; repedal on harmony changes."),284,351,448,35,12,muted);
            } else if(page==1) {
                const wchar_t* cn[]={L"和弦收集",L"抬踩间隔",L"换踏保护"};const wchar_t* en[]={L"Chord window",L"Pedal gap",L"Change guard"};
                const wchar_t* hintsCn[]={L"把先后落下的和弦音一起判断",L"换踏时短暂释放，让旧和声消退",L"避免连续低音触发过密的换踏"};
                const wchar_t* hintsEn[]={L"Group notes played close together",L"Release briefly between pedal changes",L"Minimum time between pedal changes"};
                for(int i=0;i<3;++i){float y=145.f+i*70;text(g,zh()?cn[i]:en[i],284,y,280,24,16,ink);text(g,zh()?hintsCn[i]:hintsEn[i],284,y+27,300,24,12,muted);text(g,L"ms",715,y+5,22,30,12,muted);}
            } else {
                text(g,tr(L"MIDI 信号诊断",L"MIDI signal diagnostics"),284,145,430,30,19,ink,true);
                wchar_t info[600]{};swprintf_s(info,tr(L"输入音符  %d     已提交 CC64  %d\n输出失败  %d     返回值  %d\nMIDI 通道  %d     未处理  %d\n实时音符  %d     回放音符  %d\n宿主位置  %d     回调  %d",L"Notes  %d     CC64 submitted  %d\nOutput errors  %d     Result  %d\nMIDI channel  %d     Skipped  %d\nLive notes  %d     Playback notes  %d\nHost tick  %d     Callbacks  %d"),model->notes.load(),model->sent.load(),model->outputErrors.load(),model->result.load(),model->channel.load()+1,model->rejected.load(),model->liveNotes.load(),model->sequenceNotes.load(),model->lastTick.load(),model->callbacks.load());
                text(g,info,284,184,450,151,14,muted);
                text(g,tr(L"“已提交”表示宿主接受事件，不代表音源已响应。",L"Submitted means accepted by host, not confirmed by instrument."),284,345,450,38,12,gold);
            }
            wchar_t state[160]{};swprintf_s(state,tr(L"音符 %d   /   CC64 %d",L"Notes %d   /   CC64 %d"),model->notes.load(),model->sent.load());
            text(g,state,28,463,420,23,12,muted);
            text(g,L"MIDI Insert / 1.0",393,463,160,23,11,muted);
            Gdiplus::Pen brand(tone(t.accent),2.2f);
            for(int i=0;i<4;++i){float h=6.f+(i==1?12.f:i==2?7.f:0.f);g.DrawLine(&brand,589.f+i*5,475.f-h/2,589.f+i*5,475.f+h/2);}
            text(g,L"Amu music",617,460,125,29,17,ink,true);
        }
        BitBlt(dc,0,0,r.right,r.bottom,buffer,0,0,SRCCOPY);SelectObject(buffer,old);DeleteObject(bitmap);DeleteDC(buffer);
    }
    void drawButton(DRAWITEMSTRUCT* d) {
        const auto& t=palette();
        int id=d->CtlID;bool toggle=id==101||id==102||id==108;
        bool active=toggle&&model->values[id-100]!=0;
        bool tab=(id==114&&page==0)||(id==115&&page==1)||(id==112&&page==2);
        Gdiplus::Graphics g(d->hDC);g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        float w=float(d->rcItem.right),h=float(d->rcItem.bottom),scale=float(std::min(sx,sy));
        if(id>=120&&id<=123)g.Clear(tone(t.surface));
        else {
            g.Clear(tone(t.bottom));
            POINT origin{};MapWindowPoints(d->hwndItem,window,&origin,1);
            Gdiplus::LinearGradientBrush backdrop(Gdiplus::PointF(float(-origin.x),float(-origin.y)),
                Gdiplus::PointF(float(width*sx-origin.x),float(height*sy-origin.y)),tone(t.top),tone(t.bottom));
            g.FillRectangle(&backdrop,0.f,0.f,w,h);
        }
        auto fill=tone(active?t.active:t.control);
        if(tab)fill=tone(t.active);if(d->itemState&ODS_SELECTED)fill=tone(t.pressed);
        rounded(g,1,1,w-2,h-2,8*scale,fill);
        if(toggle) {
            rounded(g,12*scale,h/2-7*scale,28*scale,14*scale,7*scale,tone(active?t.accent:t.border));
            Gdiplus::SolidBrush dot(color(242,248,255));g.FillEllipse(&dot,(active?28.f:14.f)*scale,h/2-5*scale,10*scale,10*scale);
        }
        auto old=SelectObject(d->hDC,font);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,nativeColor(t.ink));
        wchar_t title[160]{},value[180]{};GetWindowTextW(d->hwndItem,title,160);
        swprintf_s(value,L"%s",title);
        RECT r=d->rcItem;if(toggle)r.left+=int(42*std::min(sx,sy));DrawTextW(d->hDC,value,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        if(d->itemState&ODS_FOCUS){InflateRect(&r,-3,-3);DrawFocusRect(d->hDC,&r);}SelectObject(d->hDC,old);
    }
    static LRESULT CALLBACK procedure(HWND hwnd,UINT msg,WPARAM w,LPARAM l) {
        EditorDpiScope dpi;
        auto* s=reinterpret_cast<NativeEditor*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
        if(msg==WM_NCCREATE){s=static_cast<NativeEditor*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);s->window=hwnd;SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(s));}
        if(!s)return DefWindowProcW(hwnd,msg,w,l);
        if(msg==WM_CREATE){s->createControls();return 0;}
        if(msg==WM_SIZE){s->layout();return 0;}
        if(msg==WM_ERASEBKGND)return 1;
        if(msg==WM_PAINT){PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);s->paint(dc);EndPaint(hwnd,&ps);return 0;}
        if(msg==WM_TIMER){
            auto now=GetTickCount64();int down=s->model->down;
            bool moved=s->motion.step(down!=0,float(now-s->lastFrame),s->animate);s->lastFrame=now;
            if(moved){RECT area{int(35*s->sx),int(131*s->sy),int(242*s->sx),int(312*s->sy)};InvalidateRect(hwnd,&area,FALSE);}
            if(now-s->lastRefresh>=100){s->lastRefresh=now;s->fitParent();s->syncValues();
                if(s->lastLanguage!=s->model->values[0]||s->lastTheme!=s->model->theme)s->refresh();
                int notes=s->model->notes,sent=s->model->sent;
                if(down!=s->lastDown||notes!=s->lastNotes||sent!=s->lastSent||s->page==2){s->lastDown=down;s->lastNotes=notes;s->lastSent=sent;InvalidateRect(hwnd,nullptr,FALSE);}
            }return 0;
        }
        if(msg==WM_CTLCOLOREDIT){auto dc=reinterpret_cast<HDC>(w);SetTextColor(dc,nativeColor(s->palette().ink));SetBkColor(dc,nativeColor(s->palette().surface));return reinterpret_cast<LRESULT>(s->editBrush);}
        if(msg==WM_DRAWITEM){s->drawButton(reinterpret_cast<DRAWITEMSTRUCT*>(l));return TRUE;}
        if(msg==WM_COMMAND){int id=LOWORD(w),code=HIWORD(w);
            if(code==BN_CLICKED) {
                if(id==130){HMENU menu=CreatePopupMenu();for(int i=0;i<pedalThemeCount;++i)AppendMenuW(menu,MF_STRING|(s->model->theme==i?MF_CHECKED:0),500+i,s->zh()?pedalThemes[i].chinese:pedalThemes[i].english);RECT r{};GetWindowRect(s->item(130),&r);int selected=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,r.left,r.bottom,0,hwnd,nullptr);DestroyMenu(menu);if(selected>=500&&selected<500+pedalThemeCount)s->model->theme=selected-500;}
                if(id>=500&&id<500+pedalThemeCount)s->model->theme=id-500;
                if(id==100)s->model->values[0]=!s->model->values[0].load();
                if(id==101||id==102||id==108)s->model->values[id-100]=!s->model->values[id-100].load();
                if(id==110)s->model->action=1;if(id==111)s->model->action=2;
                if(id==114)s->page=0;if(id==115)s->page=1;if(id==112)s->page=2;
                if(id==116||id==117)MessageBoxW(s->window,pas::manual(s->zh(),true).c_str(),L"Piano Auto Sustain 1.0",MB_OK|MB_ICONINFORMATION);
                if(id==113){constexpr int initial[]={1,1,1,59,40,35,160,2400,1};for(int i=1;i<9;++i)s->model->values[i]=initial[i];}
                if(id>=120&&id<=123){int index=id<122?3:7,step=id<122?1:100;int delta=id%2?step:-step;s->model->values[index]=NativeControls::clamp(index,s->model->values[index]+delta);}
                s->refresh();
            }
            if(id>=203&&id<=207&&(code==EN_CHANGE||code==EN_KILLFOCUS)){
                wchar_t value[32]{};GetWindowTextW(reinterpret_cast<HWND>(l),value,32);int raw=_wtoi(value),v=NativeControls::clamp(id-200,raw);
                if(code==EN_KILLFOCUS||(value[0]&&raw==v))s->model->values[id-200]=v;
                if(code==EN_KILLFOCUS){swprintf_s(value,L"%d",v);SetWindowTextW(reinterpret_cast<HWND>(l),value);}InvalidateRect(hwnd,nullptr,FALSE);
            }
            return 0;
        }
        if(msg==WM_NCDESTROY){s->window=nullptr;SetWindowLongPtrW(hwnd,GWLP_USERDATA,0);}
        return DefWindowProcW(hwnd,msg,w,l);
    }
public:
    explicit NativeEditor(std::shared_ptr<NativeControls> m):model(std::move(m)){Gdiplus::GdiplusStartupInput input;Gdiplus::GdiplusStartup(&gdip,&input,nullptr);}
    ~NativeEditor(){removed();DeleteObject(editBrush);if(gdip)Gdiplus::GdiplusShutdown(gdip);}
    tresult PLUGIN_API queryInterface(const TUID id,void** out) override{if(!out)return kInvalidArgument;*out=nullptr;if(!std::memcmp(id,FUnknown_iid,16)||!std::memcmp(id,IPlugView_iid,16)){*out=static_cast<IPlugView*>(this);addRef();return kResultOk;}return kNoInterface;}
    uint32 PLUGIN_API addRef() override{return ++refs;}
    uint32 PLUGIN_API release() override{auto n=--refs;if(!n)delete this;return n;}
    tresult PLUGIN_API attached(void* parent) override {
        if(window||!IsWindow(static_cast<HWND>(parent)))return kInvalidArgument;
        EditorDpiScope dpi;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&procedure),&instance);
        WNDCLASSW wc{};wc.lpfnWndProc=procedure;wc.hInstance=instance;wc.lpszClassName=L"AmuNativeMidiEditor04";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);RegisterClassW(&wc);
        // Mount directly in a dedicated editor window, avoiding the legacy
        // XML proxy whose resize notification can make Cubase grow its window.
        HWND renderParent=static_cast<HWND>(parent),root=GetAncestor(renderParent,GA_ROOT);
        wchar_t title[256]{};GetWindowTextW(root,title,256);
        POINT origin{};MapWindowPoints(renderParent,root,&origin,1);
        if((wcsstr(title,L"Piano Auto Sustain")||wcsstr(title,L"Bass Pedal"))&&origin.x==0&&origin.y==0){
            renderParent=root;
            // Recover an oversized/off-screen position left by the old build,
            // once at attachment, never from the refresh timer.
            MONITORINFO monitor{sizeof(MONITORINFO)};RECT frame{},inside{};
            if(GetMonitorInfoW(MonitorFromWindow(root,MONITOR_DEFAULTTONEAREST),&monitor)&&GetWindowRect(root,&frame)){
                GetClientRect(root,&inside);auto work=monitor.rcWork;
                int w=frame.right-frame.left,h=frame.bottom-frame.top;
                if(w>work.right-work.left||h>work.bottom-work.top){
                    double factor=GetDpiForWindow(root)/96.;
                    w=std::min(int(width*factor)+w-int(inside.right),int(work.right-work.left));
                    h=std::min(int(height*factor)+h-int(inside.bottom),int(work.bottom-work.top));
                }
                int x=std::clamp(int(frame.left),int(work.left),int(work.right)-w);
                int y=std::clamp(int(frame.top),int(work.top),int(work.bottom)-h);
                if(x!=frame.left||y!=frame.top||w!=frame.right-frame.left||h!=frame.bottom-frame.top)
                    SetWindowPos(root,nullptr,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE);
            }
        }
        RECT client{};GetClientRect(renderParent,&client);
        char info[160]{};std::snprintf(info,sizeof(info),"editor061 parent=%ldx%ld dpi=%u",client.right,client.bottom,GetDpiForWindow(static_cast<HWND>(parent)));probeLog(info);
        count=0;
        HWND child=CreateWindowExW(WS_EX_CONTROLPARENT,wc.lpszClassName,L"Piano Auto Sustain",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,0,0,client.right,client.bottom,renderParent,nullptr,instance,this);
        if(child&&renderParent!=static_cast<HWND>(parent)){
            // The old XML proxy overlaps the new full-size view. Its repaint
            // can erase portions of child controls inside its 760x500 bounds.
            // Suppress only that proxy while our direct view is mounted; do
            // not resize it or hide any ancestor of our view.
            coveredProxy=static_cast<HWND>(parent);
            proxyWasVisible=(GetWindowLongPtrW(coveredProxy,GWL_STYLE)&WS_VISIBLE)!=0;
            ShowWindow(coveredProxy,SW_HIDE);
        }
        if(child)SetWindowPos(child,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        return child?kResultOk:kResultFalse;
    }
    tresult PLUGIN_API removed() override{EditorDpiScope dpi;if(window){KillTimer(window,1);DestroyWindow(window);window=nullptr;}if(coveredProxy&&IsWindow(coveredProxy)&&proxyWasVisible)ShowWindow(coveredProxy,SW_SHOWNA);coveredProxy=nullptr;proxyWasVisible=false;count=0;if(font){DeleteObject(font);font=nullptr;}if(valueFont){DeleteObject(valueFont);valueFont=nullptr;}return kResultOk;}
    tresult PLUGIN_API idle() override{return kResultOk;}
    tresult PLUGIN_API onWheel(float) override{return kResultFalse;}
    tresult PLUGIN_API onKey(char,long,long) override{return kResultFalse;}
    tresult PLUGIN_API onSize(ViewRect* r) override{if(!r)return kInvalidArgument;if(window)fitParent();return kResultOk;}
};

