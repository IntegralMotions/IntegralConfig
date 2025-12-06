#pragma once

#include "MPackObject.hpp"

class ObjectWithInts : public MPackObject<ObjectWithInts, 10> { // NOLINT(readability-magic-numbers)
  public:
    uint8_t value1{};
    int8_t value2{};
    uint16_t value3{};
    int16_t value4{};
    uint32_t value5{};
    int32_t value6{};
    uint64_t value7{};
    int64_t value8{};
    unsigned int value9{};
    int value10{};

    static void registerMembers() {
        registerMember("value1", CppType::U8, &ObjectWithInts::value1);
        registerMember("value2", CppType::I8, &ObjectWithInts::value2);
        registerMember("value3", CppType::U16, &ObjectWithInts::value3);
        registerMember("value4", CppType::I16, &ObjectWithInts::value4);
        registerMember("value5", CppType::U32, &ObjectWithInts::value5);
        registerMember("value6", CppType::I32, &ObjectWithInts::value6);
        registerMember("value7", CppType::U64, &ObjectWithInts::value7);
        registerMember("value8", CppType::I64, &ObjectWithInts::value8);
        registerMember("value9", CppType::U32, &ObjectWithInts::value9);
        registerMember("value10", CppType::I32, &ObjectWithInts::value10);
    }
};
