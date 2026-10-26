#pragma once

#include "MPackObject.hpp"
#include "MPackObjectBase.h"
#include "MessagePayloadRegistry.h"
#include <cstring>
#include <mpack/mpack.h>

class Message : public MPackObject<Message, 3> {
  public:
    ~Message() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("msgType", CppType::String, &Message::msgType);
        registerMember("opCode", CppType::String, &Message::opCode);
        registerMember("payload", CppType::ObjectPtr, &Message::payload);
    }

    [[nodiscard]] MsgType getMsgType() const {
        if (msgType == nullptr) {
            return MsgType::Unknown;
        }
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

    bool isOpCode(const char* value) const {
        return opCode != nullptr && value != nullptr && std::strcmp(opCode, value) == 0;
    }

  private:
    MPackObjectBase* createObject(const char* /*name*/) override {
        if (opCode == nullptr || getMsgType() == MsgType::Unknown) {
            return nullptr;
        }
        return MessagePayloadRegistry::create(getMsgType(), opCode);
    }

  public:
    const char* msgType{};
    const char* opCode{};
    MPackObjectBase* payload{nullptr};
};
