#pragma once
#include <string>
namespace pas {
enum class HardwarePedalPolicy { Automatic, Physical, Merge };
struct BridgeState {
    std::string inputIdentifier,outputIdentifier;
    int channelFilter=0;
    bool autoConnect=true;
    HardwarePedalPolicy hardwarePedalPolicy=HardwarePedalPolicy::Automatic;
};
}
