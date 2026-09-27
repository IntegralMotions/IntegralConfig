#pragma once

#include "MPackObjectBase.h"
#include "MessageType.h"
#include <array>
#include <cstddef>
#include <cstring>

#ifndef MAX_MESSAGE_PAYLOAD_ENTRIES
#define MAX_MESSAGE_PAYLOAD_ENTRIES 256
#endif

class MessagePayloadRegistry {
  public:
    struct Entry {
        MsgType messageType;
        const char* opCode;
        MPackObjectBase* (*createFn)();
    };

    template <typename T>
    static bool registerType(MsgType messageType, const char* opCode);

    static MPackObjectBase* create(MsgType messageType, const char* opCode);

  private:
    template <typename T>
    static MPackObjectBase* createImpl();

    static std::array<Entry, MAX_MESSAGE_PAYLOAD_ENTRIES> entries;
    static std::size_t count;
};

// ---------- template definitions (must stay in header) ----------

template <typename T>
inline MPackObjectBase* MessagePayloadRegistry::createImpl() {
    return new T();
}

template <typename T>
inline bool MessagePayloadRegistry::registerType(MsgType messageType, const char* opCode) {
    for (std::size_t i = 0; i < count; ++i) {
        if (entries[i].messageType == messageType && std::strcmp(entries[i].opCode, opCode) == 0) {
            return entries[i].createFn == &createImpl<T>;
        }
    }

    const bool canAdd = count < MAX_MESSAGE_PAYLOAD_ENTRIES;
    if (canAdd) {
        entries[count++] = Entry{messageType, opCode, &createImpl<T>};
    }
    return canAdd;
}
