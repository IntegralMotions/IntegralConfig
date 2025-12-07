#include "MessagePayloadRegistry.h"

std::array<MessagePayloadRegistry::Entry, MAX_MESSAGE_PAYLOAD_ENTRIES> MessagePayloadRegistry::_entries{};

std::size_t MessagePayloadRegistry::_count = 0;

MPackObjectBase *MessagePayloadRegistry::create(const char *opCode) {
    for (std::size_t i = 0; i < _count; ++i) {
        if (std::strcmp(_entries[i].opCode, opCode) == 0) {
            return _entries[i].createFn();
        }
    }
    return nullptr;
}
