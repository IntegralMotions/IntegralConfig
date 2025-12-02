#pragma once

#include "mpack-object.hpp"
#include <cstdint>
#include <mpack/mpack.h>

enum class MsgType : uint8_t { Request, Response, Event, Unknown };

class Payload : public MPackObject<Payload> {};

class Message : public MPackObject<Message> {
  public:
    [[nodiscard]] MsgType getMsgType() const {
        if (strcmp(msgType, "request") == 0) {
            return MsgType::Request;
        }
        if (strcmp(msgType, "response") == 0) {
            return MsgType::Response;
        }
        if (strcmp(msgType, "event") == 0) {
            return MsgType::Event;
        }
        return MsgType::Unknown;
    }

  private:
    void registerMembers() {}

    char* msgType;
    char* opCode;
    Payload* payload = nullptr;
};
