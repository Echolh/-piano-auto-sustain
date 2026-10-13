#include "Recorder/RecorderStatus.h"
#include "Recorder/VirtualMidiBackend.h"

#include <cassert>
#include <string_view>
#include <type_traits>

int main()
{
    static_assert(std::is_abstract_v<pas::VirtualMidiBackend>);
    static_assert(std::string_view(pas::kRecorderEndpointName) == "Piano Auto Sustain");

    const pas::Availability availability;
    assert(!availability.available);
    assert(availability.backend == pas::BackendKind::None);
    assert(availability.error == pas::RecorderError::BackendUnavailable);

    const pas::RecorderStatus status;
    assert(status.connection == pas::ConnectionState::Unavailable);
    assert(status.error == pas::RecorderError::BackendUnavailable);
    assert(!status.inputConnected);
    assert(!status.endpointPublished);
    assert(!status.clientConnected);
}
