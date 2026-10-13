#pragma once

#include <string>
#include <vector>

namespace pas {
struct DeviceInfo {
    std::string identifier;
    std::string name;
    bool isInput = false;
    bool isVirtual = false;
};

const DeviceInfo* findDevice(const std::vector<DeviceInfo>& devices, const std::string& identifier);
const DeviceInfo* uniquePhysicalInput(const std::vector<DeviceInfo>& devices);
}
