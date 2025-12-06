#pragma once

#include "MPackArray.h"
#include "MPackObject.hpp"
#include <cstdint>

struct Elem : public MPackObject<Elem, 2> { // NOLINT(readability-magic-numbers) if needed
  public:
    int32_t a{};
    uint32_t b{};

    static void registerMembers() {
        registerMember("a", CppType::I32, &Elem::a);
        registerMember("b", CppType::U32, &Elem::b);
    }
};

class ObjectWithArrays : public MPackObject<ObjectWithArrays, 12> { // NOLINT(readability-magic-numbers) if needed
  public:
    MPackArray<int64_t> i64;
    MPackArray<uint64_t> u64;
    MPackArray<float> f32;
    MPackArray<double> f64;
    MPackArray<const char*> ss;
    MPackArray<Elem*> objs;
    MPackArray<MPackArray<int32_t>> aa;

    MPackArray<MPackArray<double>> aa_f64;             // nested of double
    MPackArray<MPackArray<bool>> aa_bool;              // nested of bool
    MPackArray<MPackArray<Elem*>> aa_objs;             // nested of object
    MPackArray<MPackArray<const char*>> aa_ss;         // nested of string
    MPackArray<MPackArray<MPackArray<float>>> aaa_f32; // nested-of-nested float

    ~ObjectWithArrays() override {
        delete[] i64.begin();
        delete[] u64.begin();
        delete[] f32.begin();
        delete[] f64.begin();

        if (ss != nullptr) {
            for (size_t i = 0; i < ss.size; ++i) {
                delete[] ss[i];
            }
            delete[] ss.begin();
        }

        if (objs != nullptr) {
            for (size_t i = 0; i < objs.size; ++i) {
                delete objs[i];
            }
            delete[] objs.begin();
        }

        if (aa != nullptr) {
            for (size_t i = 0; i < aa.size; ++i) {
                delete[] aa[i].begin();
            }
            delete[] aa.begin();
        }

        if (aa_f64 != nullptr) {
            for (size_t i = 0; i < aa_f64.size; ++i) {
                delete[] aa_f64[i].begin();
            }
            delete[] aa_f64.begin();
        }

        if (aa_bool != nullptr) {
            for (size_t i = 0; i < aa_bool.size; ++i) {
                delete[] aa_bool[i].begin();
            }
            delete[] aa_bool.begin();
        }

        if (aa_objs != nullptr) {
            for (size_t i = 0; i < aa_objs.size; ++i) {
                auto& inner = aa_objs[i];
                for (size_t j = 0; j < inner.size; ++j) {
                    delete inner[j];
                }
                delete[] inner.begin();
            }
            delete[] aa_objs.begin();
        }

        if (aa_ss != nullptr) {
            for (size_t i = 0; i < aa_ss.size; ++i) {
                auto& inner = aa_ss[i];
                for (size_t j = 0; j < inner.size; ++j) {
                    delete[] inner[j];
                }
                delete[] inner.begin();
            }
            delete[] aa_ss.begin();
        }

        if (aaa_f32 != nullptr) {
            for (size_t i = 0; i < aaa_f32.size; ++i) {
                auto& mid = aaa_f32[i];
                for (size_t j = 0; j < mid.size; ++j) {
                    delete[] mid[j].begin();
                }
                delete[] mid.begin();
            }
            delete[] aaa_f32.begin();
        }
    }

    static void registerMembers() {
        registerMember("i64", {CppType::Array, CppType::I64}, &ObjectWithArrays::i64);
        registerMember("u64", {CppType::Array, CppType::U64}, &ObjectWithArrays::u64);
        registerMember("f32", {CppType::Array, CppType::F32}, &ObjectWithArrays::f32);
        registerMember("f64", {CppType::Array, CppType::F64}, &ObjectWithArrays::f64);
        registerMember("ss", {CppType::Array, CppType::String}, &ObjectWithArrays::ss);
        registerMember("objs", {CppType::Array, CppType::ObjectPtr}, &ObjectWithArrays::objs);

        registerMember("aa", {CppType::Array, {CppType::Array, CppType::I32}}, &ObjectWithArrays::aa);
        registerMember("aa_f64", {CppType::Array, {CppType::Array, CppType::F64}}, &ObjectWithArrays::aa_f64);
        registerMember("aa_bool", {CppType::Array, {CppType::Array, CppType::Bool}}, &ObjectWithArrays::aa_bool);
        registerMember("aa_objs", {CppType::Array, {CppType::Array, CppType::ObjectPtr}}, &ObjectWithArrays::aa_objs);
        registerMember("aa_ss", {CppType::Array, {CppType::Array, CppType::String}}, &ObjectWithArrays::aa_ss);
        registerMember("aaa_f32", {CppType::Array, {CppType::Array, {CppType::Array, CppType::F32}}},
                       &ObjectWithArrays::aaa_f32);
    }

  private:
    MPackObjectBase* createObject(const char* /*name*/) override {
        return new Elem();
    }
};
