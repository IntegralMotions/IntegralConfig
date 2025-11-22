#pragma once

#include "mpack/mpack-common.h"
#include <cstddef>

struct MPackHeader {
  mpack_tag_t tag;
  mpack_type_t type;
  size_t countOrLength;
  int8_t extType;
};