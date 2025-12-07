#pragma once

#include "MPackObject.hpp"

struct Inner : public MPackObject<Inner, 2> { // NOLINT(readability-magic-numbers)
  public:
    int32_t a{};
    uint32_t b{};

    static void registerMembers() {
        registerMember("a", CppType::I32, &Inner::a);
        registerMember("b", CppType::U32, &Inner::b);
    }
};

class ObjectWithObjects : public MPackObject<ObjectWithObjects, 2> { // NOLINT(readability-magic-numbers)
  public:
    Inner left{};
    Inner right{};

    static void registerMembers() {
        registerMember("left", CppType::Object, &ObjectWithObjects::left);
        registerMember("right", CppType::Object, &ObjectWithObjects::right);
    }
};
