#pragma once

#include "Configuration.h"
#include "MPackObject.hpp"
#include "MPackObjectBase.h"
#include <cstdint>
#include <cstring>
#include <mpack/mpack.h>

enum class MsgType : uint8_t { Request, Response, Event, Unknown };

struct ReadKeys {
    static constexpr const char *ReadDevice = "read.device";
    static constexpr const char *WriteDevice = "write.device";
};

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

  private:
    MPackObjectBase *createObject(const char * /*name*/) override {
        if (std::strcmp(opCode, ReadKeys::WriteDevice) == 0) {
            return reinterpret_cast<MPackObjectBase *>(new Device());
        }
        if (std::strcmp(opCode, ReadKeys::ReadDevice) == 0) {
            return nullptr;
        }
        return nullptr;
    }

    char *msgType{};
    char *opCode{};
    MPackObjectBase *payload{nullptr};
};
