#pragma once

#include "mpack/mpack.h"

struct MPackHeader {
  mpack_type_t type;
  uint32_t countOrLength;
  int8_t extType;
};