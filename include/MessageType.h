#pragma once

#include <cstdint>

enum class MsgType : uint8_t { Unknown, Request, Response, Event };
