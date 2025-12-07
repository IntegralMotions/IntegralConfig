#pragma once

#include "MPackObject.hpp"

class ObjectWithFloats : public MPackObject<ObjectWithFloats, 2> { // NOLINT(readability-magic-numbers)
  public:
    float f32{};
    double f64{};

    static void registerMembers() {
        registerMember("f32", CppType::F32, &ObjectWithFloats::f32);
        registerMember("f64", CppType::F64, &ObjectWithFloats::f64);
    }
};
