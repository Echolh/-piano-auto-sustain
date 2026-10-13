#include "DeviceSelection.h"

namespace pas {
const DeviceInfo* findDevice(const std::vector<DeviceInfo>& devices, const std::string& identifier)
{
    for (const auto& device : devices) if (device.identifier == identifier) return &device;
    return nullptr;
}

const DeviceInfo* uniquePhysicalInput(const std::vector<DeviceInfo>& devices)
{
    const DeviceInfo* result = nullptr;
    for (const auto& device : devices) {
        if (!device.isInput || device.isVirtual) continue;
        if (result) return nullptr;
        result = &device;
    }
    return result;
}
}
