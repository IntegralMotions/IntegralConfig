#pragma once
#include "MPackObjectType.h"
#include <cstddef>

struct MPackObjectMember {
  public:
    const char* name;
    MPackObjectType type;
    size_t offset;
};