#include "MessagePayloadRegistry.h"

std::array<MessagePayloadRegistry::Entry, MAX_MESSAGE_PAYLOAD_ENTRIES> MessagePayloadRegistry::entries{};

std::size_t MessagePayloadRegistry::count = 0;

MPackObjectBase* MessagePayloadRegistry::create(MsgType messageType, const char* opCode) {
    for (std::size_t i = 0; i < count; ++i) {
        if (entries[i].messageType == messageType && std::strcmp(entries[i].opCode, opCode) == 0) {
            return entries[i].createFn();
        }
    }
    return nullptr;
}
