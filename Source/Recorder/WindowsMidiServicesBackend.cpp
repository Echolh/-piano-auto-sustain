#include "WindowsMidiServicesBackend.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace pas {

Availability WindowsMidiServicesBackend::probe()
{
    if (!api.initialize()) return {false, BackendKind::WindowsMidiServices, RecorderError::RuntimeMissing};
    const bool available = api.transportAvailable();
    api.shutdownRuntime();
    return {available, BackendKind::WindowsMidiServices,
            available ? RecorderError::None : RecorderError::TransportUnavailable};
}

bool WindowsMidiServicesBackend::start(std::string_view endpointName)
{
    stop();
    if (!api.initialize()) return false;
    initialized = true;
    if (!api.transportAvailable() || !api.createVirtualDevice(endpointName)) {
        stop();
        return false;
    }
    started = true;
    return true;
}

void WindowsMidiServicesBackend::stop()
{
    if (started) {
        api.closeClientEndpoint();
        api.closeVirtualDevice();
        started = false;
    }
    if (initialized) {
        api.shutdownRuntime();
        initialized = false;
    }
}

bool WindowsMidiServicesBackend::send(MidiEvent event)
{
    return started && api.sendUmp(toUmp(event));
}

bool WindowsMidiServicesBackend::clientConnected() const
{
    return started && api.clientConnected();
}

std::string WindowsMidiServicesBackend::endpointIdentifier() const
{
    return started ? api.endpointIdentifier() : std::string{};
}

std::uint32_t WindowsMidiServicesBackend::toUmp(MidiEvent event)
{
    const auto status = static_cast<std::uint32_t>(event.status & 0xff);
    const auto data1 = static_cast<std::uint32_t>(event.data1 & 0x7f);
    const auto data2 = static_cast<std::uint32_t>(event.data2 & 0x7f);
    const auto messageType = status < 0xf0 ? 0x2u : 0x1u;
    return (messageType << 28) | (status << 16) | (data1 << 8) | data2;
}

namespace {
class SystemWindowsMidiServicesApi final : public WindowsMidiServicesApi {
public:
    bool initialize() override
    {
#ifdef _WIN32
        // The production MIDI 2 runtime is optional. Loading by name keeps this
        // executable usable on Windows versions where it is not installed.
        module = LoadLibraryExW(L"Microsoft.Windows.Devices.Midi2.dll", nullptr,
                                LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#endif
        return module != nullptr;
    }
    bool transportAvailable() const override { return false; }
    bool createVirtualDevice(std::string_view) override { return false; }
    bool sendUmp(std::uint32_t) override { return false; }
    bool clientConnected() const override { return false; }
    std::string endpointIdentifier() const override { return {}; }
    void closeClientEndpoint() override {}
    void closeVirtualDevice() override {}
    void shutdownRuntime() override
    {
#ifdef _WIN32
        if (module) FreeLibrary(static_cast<HMODULE>(module));
#endif
        module = nullptr;
    }
private:
    void* module = nullptr;
};
}

std::unique_ptr<WindowsMidiServicesApi> createSystemWindowsMidiServicesApi()
{
    return std::make_unique<SystemWindowsMidiServicesApi>();
}

} // namespace pas
