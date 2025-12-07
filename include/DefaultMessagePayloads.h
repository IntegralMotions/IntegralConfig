#pragma once

#include "Configuration.h"
#include "MessagePayloadRegistry.h"

struct DefaultReadKeys {
    static constexpr const char *ReadDevice = "read.device";
    static constexpr const char *WriteDevice = "write.device";
};

bool registerDefaultMessagePayloads() {
    bool success = true;
    success &= MessagePayloadRegistry::registerType<Device>(DefaultReadKeys::WriteDevice);
    return success;
}