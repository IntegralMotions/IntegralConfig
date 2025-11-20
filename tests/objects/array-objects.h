#pragma once

#include "mpack-object.hpp"
#include <cstdint>

struct Elem : public MPackObject<Elem> {
public:
  int32_t a{};
  uint32_t b{};

  void registerMembers() {
    registerMember("a", CppType::I32, &a);
    registerMember("b", CppType::U32, &b);
  }
};

class ObjectWithArrays : public MPackObject<ObjectWithArrays> {
public:
  MPackArray<int64_t> i64;
  MPackArray<uint64_t> u64;
  MPackArray<float> f32;
  MPackArray<double> f64;
  MPackArray<const char *> ss;
  MPackArray<Elem *> objs;
  MPackArray<MPackArray<int32_t>> aa;

  MPackArray<MPackArray<double>> aa_f64;             // nested of double
  MPackArray<MPackArray<bool>> aa_bool;              // nested of bool
  MPackArray<MPackArray<Elem *>> aa_objs;            // nested of object
  MPackArray<MPackArray<const char *>> aa_ss;        // nested of string
  MPackArray<MPackArray<MPackArray<float>>> aaa_f32; // nested-of-nested float

  ~ObjectWithArrays() override {
    delete[] i64.begin();
    delete[] u64.begin();
    delete[] f32.begin();
    delete[] f64.begin();
    if (ss.p) {
      for (size_t i = 0; i < ss.size; ++i)
        delete[] ss[i];
      delete[] ss.begin();
    }
    if (objs) {
      for (size_t i = 0; i < objs.size; ++i)
        delete objs[i];
      delete[] objs.begin();
    }
    if (aa) {
      for (size_t i = 0; i < aa.size; ++i)
        delete[] aa[i].begin();
      delete[] aa.begin();
    }
    if (aa_f64) {
      for (size_t i = 0; i < aa_f64.size; ++i)
        delete[] aa_f64[i].begin();
      delete[] aa_f64.begin();
    }
    if (aa_bool) {
      for (size_t i = 0; i < aa_bool.size; ++i)
        delete[] aa_bool[i].begin();
      delete[] aa_bool.begin();
    }
    if (aa_objs) {
      for (size_t i = 0; i < aa_objs.size; ++i) {
        auto &inner = aa_objs[i];
        for (size_t j = 0; j < inner.size; ++j)
          delete inner[j];
        delete[] inner.begin();
      }
      delete[] aa_objs.begin();
    }
    if (aa_ss) {
      for (size_t i = 0; i < aa_ss.size; ++i) {
        auto &inner = aa_ss[i];
        for (size_t j = 0; j < inner.size; ++j)
          delete[] inner[j];
        delete[] inner.begin();
      }
      delete[] aa_ss.begin();
    }
    if (aaa_f32) {
      for (size_t i = 0; i < aaa_f32.size; ++i) {
        auto &mid = aaa_f32[i];
        for (size_t j = 0; j < mid.size; ++j)
          delete[] mid[j].begin();
        delete[] mid.begin();
      }
      delete[] aaa_f32.begin();
    }
  }

  void registerMembers() {
    registerMember("i64", {CppType::Array, CppType::I64}, &i64);
    registerMember("u64", {CppType::Array, CppType::U64}, &u64);
    registerMember("f32", {CppType::Array, CppType::F32}, &f32);
    registerMember("f64", {CppType::Array, CppType::F64}, &f64);
    registerMember("ss", {CppType::Array, CppType::String}, &ss);
    registerMember("objs", {CppType::Array, CppType::ObjectPtr}, &objs);
    registerMember("aa", {CppType::Array, {CppType::Array, CppType::I32}}, &aa);
    registerMember("aa_f64", {CppType::Array, {CppType::Array, CppType::F64}},
                   &aa_f64);
    registerMember("aa_bool", {CppType::Array, {CppType::Array, CppType::Bool}},
                   &aa_bool);
    registerMember("aa_objs",
                   {CppType::Array, {CppType::Array, CppType::ObjectPtr}},
                   &aa_objs);
    registerMember("aa_ss", {CppType::Array, {CppType::Array, CppType::String}},
                   &aa_ss);
    registerMember(
        "aaa_f32",
        {CppType::Array, {CppType::Array, {CppType::Array, CppType::F32}}},
        &aaa_f32);
  }

private:
  MPackObjectBase *createObject(const char *) override { return new Elem(); }
};