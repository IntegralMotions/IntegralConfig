#pragma once
#include "MPackObjectType.h"
#include <cstddef>

enum class MPackOmitPolicy {
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
