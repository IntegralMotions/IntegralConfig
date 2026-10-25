#pragma once

#include "MPackObjectType.h"
#include <cstddef>
#include <cstdint>

enum class MPackOmitPolicy : uint8_t {
    None,
    NullOrEmptyCString,
    FalseBool,
    NullCString,
};

struct MPackObjectMember {
  public:
    const char* name;
    MPackObjectType type;
    size_t offset;
    bool optional = false;
    MPackOmitPolicy omit = MPackOmitPolicy::None;
};
