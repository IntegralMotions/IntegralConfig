#pragma once

#include "MPackObjectBase.h"
#include <cstddef>

template <typename Derived> class MPackObject : public MPackObjectBase {
  public:
    MPackObject() {
        static_cast<Derived*>(this)->registerMembers();
    }
};
