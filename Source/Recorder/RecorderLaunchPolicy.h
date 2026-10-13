#pragma once

#include "RecorderConfig.h"
#include "RecorderStatus.h"

namespace pas {
inline bool shouldShowRecorderWindow(const RecorderConfig& config, const RecorderStatus& status)
{
    if (!config.quietWhenOperational) return true;
    if (status.error != RecorderError::None) return true;
    return !status.inputConnected || !status.endpointPublished;
}
}
