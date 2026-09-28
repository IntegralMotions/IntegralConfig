#include "MessagePayloadRegistry.h"

std::array<MessagePayloadRegistry::Entry, MAX_MESSAGE_PAYLOAD_ENTRIES> MessagePayloadRegistry::entries{};

std::size_t MessagePayloadRegistry::count = 0;

MPackObjectBase* MessagePayloadRegistry::create(MsgType messageType, const char* opCode) {
    if (messageType == MsgType::Unknown || opCode == nullptr) {
        return nullptr;
    }
    for (std::size_t i = 0; i < count; ++i) {
        if (entries[i].messageType == messageType && entries[i].opCode.view() == opCode) {
            return entries[i].createFn();
        }
    }
    return nullptr;
}
