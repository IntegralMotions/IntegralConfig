#include "MessagePayloadRegistry.h"

std::array<MessagePayloadRegistry::Entry, MAX_MESSAGE_PAYLOAD_ENTRIES> MessagePayloadRegistry::entries{};

std::size_t MessagePayloadRegistry::count = 0;

MPackObjectBase* MessagePayloadRegistry::create(const char* opCode) {
    for (std::size_t i = 0; i < count; ++i) {
        if (std::strcmp(entries[i].opCode, opCode) == 0) {
            return entries[i].createFn();
        }
    }
    return nullptr;
}
