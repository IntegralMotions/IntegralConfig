#include "DefaultMessagePayloads.h"

#include "Configuration.h"
#include "MessagePayloadRegistry.h"

bool registerDefaultMessagePayloads() {
    bool success = true;
    success &= MessagePayloadRegistry::registerType<Device>(MsgType::Response, DefaultReadKeys::readDevice);
    success &= MessagePayloadRegistry::registerType<Device>(MsgType::Request, DefaultReadKeys::writeDevice);
    success &= MessagePayloadRegistry::registerType<Device>(MsgType::Event, DefaultReadKeys::writeDevice);
    success &= MessagePayloadRegistry::registerType<SuccessResult>(MsgType::Response, DefaultReadKeys::writeDevice);
    return success;
}
