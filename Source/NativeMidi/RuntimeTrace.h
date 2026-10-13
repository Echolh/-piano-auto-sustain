#pragma once
#include <array>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
#include <cstdio>
#include <share.h>
#include <windows.h>

// Bounded diagnostic buffer. The MIDI callback never waits, allocates or writes
// a file. If the consumer owns the buffer we drop the diagnostic, not the MIDI.
class RuntimeTrace {
public:
    struct Row { long long time; char kind; long long a,b,c,d,e,f; };
    static constexpr size_t capacity=512;
    explicit RuntimeTrace(bool writeFile=true) {
        if(writeFile) {
            try { worker=std::thread([this]{run();}); } catch(...) {}
        }
    }
    ~RuntimeTrace(){stop=true;if(worker.joinable())worker.join();}
    void push(char kind,long long a=0,long long b=0,long long c=0,long long d=0,long long e=0,long long f=0) {
        std::unique_lock<std::mutex> guard(mutex,std::try_to_lock);
        if(!guard.owns_lock() || count==capacity){++dropped;return;}
        rows[count++]={static_cast<long long>(GetTickCount64()),kind,a,b,c,d,e,f};
    }
    size_t take(std::array<Row,capacity>& out) {
        std::lock_guard<std::mutex> guard(mutex);
        const auto n=count;std::copy_n(rows.begin(),n,out.begin());count=0;return n;
    }
    std::atomic<unsigned> dropped{0};
private:
    std::mutex mutex;
    std::array<Row,capacity> rows{};
    size_t count=0;
    std::atomic<bool> stop{false};
    std::thread worker;
    void run() {
        wchar_t temp[MAX_PATH]{},path[MAX_PATH]{};
        if(!GetTempPathW(MAX_PATH,temp))return;
        static std::atomic<unsigned> next{0};
        swprintf_s(path,L"%sPianoAutoSustain-trace-%lu-%u.log",temp,GetCurrentProcessId(),++next);
        FILE* file=_wfsopen(path,L"wb",_SH_DENYNO);if(!file)return;
        std::fprintf(file,"Piano Auto Sustain 1.0.0; host timing diagnostic; no note pitches or project names\n"
            "I: ppq; N: live,status,channel,start,length,velocityPositive; P: live,from,to,tempo,queue0,queue1\n"
            "C: immediate,direct,tick,channel,down,result; V: outputReadbackLive,channel,tick,status,controller,value\n"
            "K: inputControllerLive,channel,controller,value,tick; R: resetPosition; Q: seekPosition; S: startPosition\n"
            "A: creationFailure; F: forwardResult; X: inputQueueOverflow; H: contextResult,tempo,ppqPosition,stopPosition,cyclesPPQ,cyclesImmediate\n");
        std::array<Row,capacity> batch{};size_t written=0;
        do {
            auto n=take(batch);
            for(size_t i=0;i<n && written<65536;++i,++written){const auto&r=batch[i];
                std::fprintf(file,"%lld %c %lld %lld %lld %lld %lld %lld\n",r.time,r.kind,r.a,r.b,r.c,r.d,r.e,r.f);
            }
            std::fflush(file);
            if(stop.load())break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }while(written<65536);
        std::fprintf(file,"END dropped=%u rows=%zu\n",dropped.load(),written);std::fclose(file);
    }
};
