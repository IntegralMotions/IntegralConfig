#pragma once

#include "mpack-object.h"
#include <mpack/mpack.h>

enum class MsgType { Request, Response, Event, Unknown };

inline MsgType toType(const char *s) {
  if (s == "request")
    return MsgType::Request;
  if (s == "response")
    return MsgType::Response;
  if (s == "event")
    return MsgType::Event;
  return MsgType::Unknown;
}

inline const char *fromType(MsgType t) {
  switch (t) {
  case MsgType::Request:
    return "request";
  case MsgType::Response:
    return "response";
  case MsgType::Event:
    return "event";
  default:
    return "unknown";
  }
}

class Payload : public MPackObject {};

class Message : public MPackObject {
private:
  // Message read(mpack_reader_t &reader)
  // {
  // 	MPackHeader header;
  // 	readHeader(reader, header);

  // }

public:
  MsgType msgType;
  char *opCode;
  Payload *payload = nullptr;
};
