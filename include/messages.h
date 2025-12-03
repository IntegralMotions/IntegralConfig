#pragma once

#include "configuration.h"
#include "mpack-object-base.h"
#include "mpack-object.hpp"
#include <cstdint>
#include <cstring>
#include <mpack/mpack.h>

enum class MsgType : uint8_t { Request, Response, Event, Unknown };

struct ReadKeys {
    static constexpr const char* ReadDevice = "read.device";
    static constexpr const char* WriteDevice = "write.device";
};
class Payload : public MPackObject<Payload> {};

class Message : public MPackObject<Message> {
  public:
    void registerMembers() {
        this->registerMember("msgType", CppType::String, &msgType);
        this->registerMember("opCode", CppType::String, &opCode);
        this->registerMember("payload", CppType::ObjectPtr, &payload);
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
    MPackObjectBase* createObject(const char* name) override {
        if (std::strcmp(opCode, "read.device") == 0) {
            return new Device();
        }

        return nullptr;
    }

    char* msgType;
    char* opCode;
    MPackObjectBase* payload = nullptr;
};
