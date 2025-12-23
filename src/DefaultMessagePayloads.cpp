#include "DefaultMessagePayloads.h"

#include "Configuration.h"
#include "MessagePayloadRegistry.h"

bool registerDefaultMessagePayloads() {
    bool success = true;
    success &= MessagePayloadRegistry::registerType<Device>(DefaultReadKeys::WriteDevice);
    return success;
}
