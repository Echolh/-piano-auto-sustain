#pragma once

namespace pas {

inline constexpr char kRecorderEndpointName[] = "Piano Auto Sustain";

enum class BackendKind {
    None,
    WindowsMidiServices,
    ExternalVirtualPort
};

enum class ConnectionState {
    Unavailable,
    Stopped,
    WaitingForInput,
    WaitingForHost,
    Ready,
    Error
};

enum class RecorderError {
    BackendUnavailable,
    None,
    RuntimeMissing,
    TransportUnavailable,
    EndpointCreationFailed,
    InputUnavailable,
    OutputUnavailable,
    AmbiguousInput,
    SameEndpoint,
    SendFailed,
    MessageStorm
};

struct Availability {
    bool available = false;
    BackendKind backend = BackendKind::None;
    RecorderError error = RecorderError::BackendUnavailable;
};

struct RecorderStatus {
    BackendKind backend = BackendKind::None;
    ConnectionState connection = ConnectionState::Unavailable;
    RecorderError error = RecorderError::BackendUnavailable;
    bool inputConnected = false;
    bool endpointPublished = false;
    bool clientConnected = false;
};

} // namespace pas
