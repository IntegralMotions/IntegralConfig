#pragma once

#include "MPackObjectBase.h"
#include <array>
#include <cstddef>
#include <cstring>

#ifndef MAX_MESSAGE_PAYLOAD_ENTRIES
#define MAX_MESSAGE_PAYLOAD_ENTRIES 256
#endif

class MessagePayloadRegistry {
  public:
    struct Entry {
        const char *opCode;
        MPackObjectBase *(*createFn)();
    };

    template <typename T> static bool registerType(const char *opCode);

    static MPackObjectBase *create(const char *opCode);

  private:
    template <typename T> static MPackObjectBase *createImpl();

    static std::array<Entry, MAX_MESSAGE_PAYLOAD_ENTRIES> _entries;
    static std::size_t _count;
};

// ---------- template definitions (must stay in header) ----------

template <typename T> inline MPackObjectBase *MessagePayloadRegistry::createImpl() {
    return new T();
}

template <typename T> inline bool MessagePayloadRegistry::registerType(const char *opCode) {
    const bool canAdd = _count < MAX_MESSAGE_PAYLOAD_ENTRIES;
    if (canAdd) {
        _entries[_count++] = Entry{opCode, &createImpl<T>};
    }
    return canAdd;
}
