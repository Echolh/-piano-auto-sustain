#pragma once
#include "BridgeState.h"
#include "MidiPort.h"
#include "PedalEngine.h"
namespace pas {
struct BridgeMetrics {
    std::uint64_t inputMessages=0,outputMessages=0,cc64Messages=0,droppedMessages=0,reconnects=0,errors=0;
};
class MidiBridgeEngine {
public:
    void prepare(double sampleRate);
    bool start(IMidiInput&,IMidiOutput&);
    void stop();
    void handleIncoming(std::int64_t time,MidiEvent);
    void advance(std::int64_t time);
    void refreshDevices(IMidiInput*,IMidiOutput*);
    void panic();
    bool running()const{return active;}
    bool safetyReleasePending()const{return pendingRelease;}
    bool loopDetected()const{return feedbackDetected;}
    BridgeMetrics metrics()const{return counters;}
    BridgeState state;
private:
    void sendSafetyRelease();
    bool send(MidiEvent);
    PedalEngine engine; IMidiInput* input=nullptr; IMidiOutput* output=nullptr;
    bool active=false,pendingRelease=false,feedbackDetected=false;std::int64_t rateWindow=0;int rateCount=0;
    BridgeMetrics counters;
};
}
