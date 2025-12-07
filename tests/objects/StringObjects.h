#pragma once

#include "MPackObject.hpp"

class ObjectWithStrings : public MPackObject<ObjectWithStrings, 3> { // NOLINT(readability-magic-numbers)
  public:
    const char *s1{};
    const char *s2{};
    const char *s3{};

    static void registerMembers() {
        registerMember("s1", CppType::String, &ObjectWithStrings::s1);
        registerMember("s2", CppType::String, &ObjectWithStrings::s2);
        registerMember("s3", CppType::String, &ObjectWithStrings::s3);
    }
};
