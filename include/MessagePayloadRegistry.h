#pragma once

#include "Containers/FixedString.h"
#include "MPackObjectBase.h"
#include "MessageType.h"
#include <array>
#include <cstddef>
#include <cstring>

inline constexpr size_t MaxMessagePayloadEntries = 256;
inline constexpr size_t MaxMessageOpcodeLength = 64;

class MessagePayloadRegistry {
  public:
    struct Entry {
        MsgType messageType = MsgType::Unknown;
        IntegralMotions::Containers::FixedString<MaxMessageOpcodeLength> opCode;
        MPackObjectBase* (*createFn)(){};
    };

    template <typename T>
    static bool registerType(MsgType messageType, const char* opCode);

    static MPackObjectBase* create(MsgType messageType, const char* opCode);

  private:
    template <typename T>
    static MPackObjectBase* createImpl();

    static std::array<Entry, MaxMessagePayloadEntries> entries;
    static std::size_t count;
};

// ---------- template definitions (must stay in header) ----------

template <typename T>
inline MPackObjectBase* MessagePayloadRegistry::createImpl() {
    return new T();
}

template <typename T>
inline bool MessagePayloadRegistry::registerType(MsgType messageType, const char* opCode) {
    if (messageType == MsgType::Unknown || opCode == nullptr || opCode[0] == '\0') {
        return false;
    }
    for (std::size_t i = 0; i < count; ++i) {
        if (entries[i].messageType == messageType && entries[i].opCode.view() == opCode) {
            return entries[i].createFn == &createImpl<T>;
        }
    }

    const bool canAdd = count < MaxMessagePayloadEntries;
    if (canAdd) {
        Entry entry{.messageType = messageType, .createFn = &createImpl<T>};
        if (!entry.opCode.assign(opCode)) {
            return false;
        }
        entries[count++] = entry;
    }
    return canAdd;
}
