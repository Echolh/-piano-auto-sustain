#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace pas {

class WindowsMidiServicesApi {
public:
    virtual ~WindowsMidiServicesApi() = default;
    virtual bool initialize() = 0;
    virtual bool transportAvailable() const = 0;
    virtual bool createVirtualDevice(std::string_view endpointName) = 0;
    virtual bool sendUmp(std::uint32_t packet) = 0;
    virtual bool clientConnected() const = 0;
    virtual std::string endpointIdentifier() const = 0;
    virtual void closeClientEndpoint() = 0;
    virtual void closeVirtualDevice() = 0;
    virtual void shutdownRuntime() = 0;
};

std::unique_ptr<WindowsMidiServicesApi> createSystemWindowsMidiServicesApi();

} // namespace pas
