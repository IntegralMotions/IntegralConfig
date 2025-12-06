#pragma once

#include "MPackObject.hpp"

struct MixedInner : public MPackObject<MixedInner, 2> { // NOLINT(readability-magic-numbers)
  public:
    int32_t a{};
    const char* label{};

    static void registerMembers() {
        registerMember("a", CppType::I32, &MixedInner::a);
        registerMember("label", CppType::String, &MixedInner::label);
    }
};

class MixedObject : public MPackObject<MixedObject, 13> { // NOLINT(readability-magic-numbers)
  public:
    int8_t i8{};
    uint8_t u8{};
    int16_t i16{};
    uint16_t u16{};
    int32_t i32{};
    uint32_t u32{};
    int64_t i64{};
    uint64_t u64{};
    float f32{};
    double f64{};
    bool flag{};
    const char* name{};
    MixedInner child{};

    static void registerMembers() {
        registerMember("i8", CppType::I8, &MixedObject::i8);
        registerMember("u8", CppType::U8, &MixedObject::u8);
        registerMember("i16", CppType::I16, &MixedObject::i16);
        registerMember("u16", CppType::U16, &MixedObject::u16);
        registerMember("i32", CppType::I32, &MixedObject::i32);
        registerMember("u32", CppType::U32, &MixedObject::u32);
        registerMember("i64", CppType::I64, &MixedObject::i64);
        registerMember("u64", CppType::U64, &MixedObject::u64);
        registerMember("f32", CppType::F32, &MixedObject::f32);
        registerMember("f64", CppType::F64, &MixedObject::f64);
        registerMember("flag", CppType::Bool, &MixedObject::flag);
        registerMember("name", CppType::String, &MixedObject::name);
        registerMember("child", CppType::Object, &MixedObject::child);
    }
};