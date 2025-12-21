#pragma once

#include "MPackObject.hpp"
#include "MPackObjectBase.h"
#include "MessagePayloadRegistry.h"
#include <cstdint>
#include <cstring>
#include <mpack/mpack.h>

enum class MsgType : uint8_t { Request, Response, Event, Unknown };

class Message : public MPackObject<Message, 3> {
  public:
    static void registerMembers() {
        registerMember("msgType", CppType::String, &Message::msgType);
        registerMember("opCode", CppType::String, &Message::opCode);
        registerMember("payload", CppType::ObjectPtr, &Message::payload);
    }

    [[nodiscard]] MsgType getMsgType() const {
        if (std::strcmp(msgType, "request") == 0) {
            return MsgType::Request;
        }
        if (std::strcmp(msgType, "response") == 0) {
            return MsgType::Response;
        }
        if (std::strcmp(msgType, "event") == 0) {
            return MsgType::Event;
        }
        return MsgType::Unknown;
    }

    bool isOpCode(const char *value) const {
        return std::strcmp(opCode, value) == 0;
    }

  private:
    MPackObjectBase *createObject(const char * /*name*/) override {
        return MessagePayloadRegistry::create(opCode);
    }

  public:
    const char *msgType{};
    const char *opCode{};
    MPackObjectBase *payload{nullptr};
};
