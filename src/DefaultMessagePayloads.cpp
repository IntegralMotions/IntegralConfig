#include "DefaultMessagePayloads.h"

#include "Configuration.h"
#include "MessagePayloadRegistry.h"

bool registerDefaultMessagePayloads() {
    bool success = true;
    success &= MessagePayloadRegistry::registerType<Device>(MsgType::Response, DefaultReadKeys::readDevice);
    success &= MessagePayloadRegistry::registerType<WriteSettings>(MsgType::Request, DefaultWriteKeys::writeSettings);
    success &=
        MessagePayloadRegistry::registerType<WriteSettingsResult>(MsgType::Response, DefaultWriteKeys::writeSettings);
    return success;
}
