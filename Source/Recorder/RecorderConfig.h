#pragma once

#include "Bridge/BridgeState.h"

#include <filesystem>
#include <string>

namespace pas {
struct RecorderConfig {
    int schemaVersion = 5;
    std::string inputIdentifier;
    std::string outputIdentifier;
    HardwarePedalPolicy pedalPolicy = HardwarePedalPolicy::Automatic;
    int theme = 0;
    bool startWithWindows = true;
    bool closeToTray = true;
    bool quietWhenOperational = true;
    std::string language = "zh";
};

class RecorderConfigStore {
public:
    explicit RecorderConfigStore(std::filesystem::path path) : path(std::move(path)) {}
    RecorderConfig load() const;
    bool save(const RecorderConfig& config) const;
private:
    std::filesystem::path path;
};
}
