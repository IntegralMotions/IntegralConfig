#include "mpack-array.h"
#include "mpack-object-base.h"
#include "mpack-object.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

struct Elem : MPackObject<Elem> {
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

  ~ObjectWithArrays() override {
    delete[] i64.p;
    delete[] u64.p;
    delete[] f32.p;
    delete[] f64.p;
    delete[] ss.p;
    if (objs) {
      for (size_t i = 0; i < objs.size; ++i)
        delete objs[i];
      delete[] objs.p;
    }
    if (aa) {
      for (size_t i = 0; i < aa.size; ++i)
        delete[] aa[i].p;
      delete[] aa.p;
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
  }

private:
  MPackObjectBase *createObject(const char *) override { return new Elem(); }
};

static std::vector<uint8_t> writeArrays(const ObjectWithArrays &inObj) {
  mpack_writer_t w;
  char *buf = nullptr;
  size_t size = 0;

  mpack_writer_init_growable(&w, &buf, &size);

  ObjectWithArrays obj = inObj;
  obj.write(w, 0);

  EXPECT_EQ(mpack_writer_destroy(&w), mpack_ok);

  std::vector<uint8_t> out(reinterpret_cast<uint8_t *>(buf),
                           reinterpret_cast<uint8_t *>(buf) + size);
  MPACK_FREE(buf);
  return out;
}

static std::vector<uint8_t> bytes_arrays_case1 = {
    0x87, 0xA3, 0x69, 0x36, 0x34, 0x93, 0x01, 0xFE, 0xCD, 0x01, 0x2C, 0xA3,
    0x75, 0x36, 0x34, 0x93, 0x00, 0xCE, 0x00, 0x01, 0x5F, 0x90, 0xCB, 0x41,
    0xFA, 0x13, 0xB8, 0x60, 0x00, 0x00, 0x00, 0xA3, 0x66, 0x33, 0x32, 0x92,
    0xCB, 0x40, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xCB, 0xBF, 0xE0,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x66, 0x36, 0x34, 0x92, 0xD2,
    0xB6, 0xAF, 0xB0, 0x80, 0xCB, 0x44, 0xDF, 0xE1, 0x85, 0xCA, 0x57, 0xC5,
    0x17, 0xA2, 0x73, 0x73, 0x93, 0xA5, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0xA9,
    0x55, 0x54, 0x46, 0x2D, 0x38, 0x20, 0xE2, 0x9C, 0x93, 0xA0, 0xA4, 0x6F,
    0x62, 0x6A, 0x73, 0x92, 0x82, 0xA1, 0x61, 0xF6, 0xA1, 0x62, 0x14, 0x82,
    0xA1, 0x61, 0x00, 0xA1, 0x62, 0x2A, 0xA2, 0x61, 0x61, 0x92, 0x93, 0x01,
    0x02, 0x03, 0x92, 0xFC, 0xFB};

static std::vector<uint8_t> bytes_arrays_case2 = {
    0x87, 0xA3, 0x69, 0x36, 0x34, 0x95, 0xFF, 0x02, 0xFD, 0x04, 0xFB, 0xA3,
    0x75, 0x36, 0x34, 0x95, 0x01, 0x02, 0x03, 0x04, 0x05, 0xA3, 0x66, 0x33,
    0x32, 0x94, 0x01, 0xFF, 0xCB, 0x3F, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xCB, 0xC0, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x66,
    0x36, 0x34, 0x94, 0xCB, 0x40, 0x09, 0x21, 0xFB, 0x54, 0x44, 0x2D, 0x18,
    0x00, 0xCB, 0x01, 0xA5, 0x6E, 0x1F, 0xC2, 0xF8, 0xF3, 0x59, 0xCB, 0x7E,
    0x37, 0xE4, 0x3C, 0x88, 0x00, 0x75, 0x9C, 0xA2, 0x73, 0x73, 0x92, 0xD9,
    0x20, 0x73, 0x79, 0x6D, 0x62, 0x6F, 0x6C, 0x73, 0x20, 0x21, 0x40, 0x23,
    0x24, 0x20, 0x22, 0x71, 0x75, 0x6F, 0x74, 0x65, 0x22, 0x20, 0x5C, 0x20,
    0x62, 0x61, 0x63, 0x6B, 0x73, 0x6C, 0x61, 0x73, 0x68, 0xAE, 0x6D, 0x75,
    0x6C, 0x74, 0x69, 0x6C, 0x69, 0x6E, 0x65, 0x0A, 0x74, 0x65, 0x78, 0x74,
    0xA4, 0x6F, 0x62, 0x6A, 0x73, 0x91, 0x82, 0xA1, 0x61, 0x7B, 0xA1, 0x62,
    0xCD, 0x01, 0xC8, 0xA2, 0x61, 0x61, 0x93, 0x90, 0x91, 0x0A, 0x92, 0x14,
    0x1E};

static std::vector<uint8_t> bytes_arrays_case3 = {
    0x87, 0xA3, 0x69, 0x36, 0x34, 0x93, 0x01, 0xFE, 0x03, 0xA3,
    0x75, 0x36, 0x34, 0x90, 0xA3, 0x66, 0x33, 0x32, 0x90, 0xA3,
    0x66, 0x36, 0x34, 0x90, 0xA2, 0x73, 0x73, 0x90, 0xA4, 0x6F,
    0x62, 0x6A, 0x73, 0x90, 0xA2, 0x61, 0x61, 0x90};

static std::vector<uint8_t> bytes_arrays_case4 = {
    0x87, 0xA3, 0x69, 0x36, 0x34, 0x90, 0xA3, 0x75, 0x36, 0x34, 0x92, 0x00,
    0xCF, 0x11, 0x22, 0x10, 0xF4, 0x7D, 0xE9, 0x81, 0x15, 0xA3, 0x66, 0x33,
    0x32, 0x90, 0xA3, 0x66, 0x36, 0x34, 0x90, 0xA2, 0x73, 0x73, 0x90, 0xA4,
    0x6F, 0x62, 0x6A, 0x73, 0x90, 0xA2, 0x61, 0x61, 0x90};

static std::vector<uint8_t> bytes_arrays_case5 = {
    0x87, 0xA3, 0x69, 0x36, 0x34, 0x90, 0xA3, 0x75, 0x36, 0x34, 0x90,
    0xA3, 0x66, 0x33, 0x32, 0x93, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xCB, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xCB, 0xC0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA3,
    0x66, 0x36, 0x34, 0x90, 0xA2, 0x73, 0x73, 0x90, 0xA4, 0x6F, 0x62,
    0x6A, 0x73, 0x90, 0xA2, 0x61, 0x61, 0x90};

static std::vector<uint8_t> bytes_arrays_case6 = {
    0x87, 0xA3, 0x69, 0x36, 0x34, 0x90, 0xA3, 0x75, 0x36, 0x34, 0x90,
    0xA3, 0x66, 0x33, 0x32, 0x90, 0xA3, 0x66, 0x36, 0x34, 0x92, 0xCB,
    0x2B, 0x2B, 0xFF, 0x2E, 0xE4, 0x8E, 0x05, 0x30, 0xCB, 0xD4, 0xB2,
    0x49, 0xAD, 0x25, 0x94, 0xC3, 0x7D, 0xA2, 0x73, 0x73, 0x90, 0xA4,
    0x6F, 0x62, 0x6A, 0x73, 0x90, 0xA2, 0x61, 0x61, 0x90};

static std::vector<uint8_t> bytes_arrays_case7 = {
    0x87, 0xA3, 0x69, 0x36, 0x34, 0x90, 0xA3, 0x75, 0x36, 0x34, 0x90, 0xA3,
    0x66, 0x33, 0x32, 0x90, 0xA3, 0x66, 0x36, 0x34, 0x90, 0xA2, 0x73, 0x73,
    0x92, 0xA8, 0x6F, 0x6E, 0x6C, 0x79, 0x2D, 0x6F, 0x6E, 0x65, 0xA0, 0xA4,
    0x6F, 0x62, 0x6A, 0x73, 0x90, 0xA2, 0x61, 0x61, 0x90};

static std::vector<uint8_t> bytes_arrays_case8 = {
    0x87, 0xA3, 0x69, 0x36, 0x34, 0x90, 0xA3, 0x75, 0x36, 0x34,
    0x90, 0xA3, 0x66, 0x33, 0x32, 0x90, 0xA3, 0x66, 0x36, 0x34,
    0x90, 0xA2, 0x73, 0x73, 0x90, 0xA4, 0x6F, 0x62, 0x6A, 0x73,
    0x92, 0x82, 0xA1, 0x61, 0x01, 0xA1, 0x62, 0x02, 0x82, 0xA1,
    0x61, 0xFD, 0xA1, 0x62, 0x04, 0xA2, 0x61, 0x61, 0x90};

static std::vector<uint8_t> bytes_arrays_case9 = {
    0x87, 0xA3, 0x69, 0x36, 0x34, 0x90, 0xA3, 0x75, 0x36, 0x34, 0x90,
    0xA3, 0x66, 0x33, 0x32, 0x90, 0xA3, 0x66, 0x36, 0x34, 0x90, 0xA2,
    0x73, 0x73, 0x90, 0xA4, 0x6F, 0x62, 0x6A, 0x73, 0x90, 0xA2, 0x61,
    0x61, 0x93, 0x93, 0x01, 0x02, 0x03, 0x90, 0x91, 0xFF};

struct ArrayWriteCase {
  std::string name;
  std::vector<uint8_t> expected;
  std::vector<int64_t> i64;
  std::vector<uint64_t> u64;
  std::vector<float> f32;
  std::vector<double> f64;
  std::vector<std::string> ss;
  std::vector<std::pair<int32_t, uint32_t>> objs;
  std::vector<std::vector<int32_t>> aa;
};

class ObjectWithArraysWriteTest
    : public ::testing::TestWithParam<ArrayWriteCase> {};

TEST_P(ObjectWithArraysWriteTest, EncodesAllArrays) {
  const auto &tc = GetParam();

  ObjectWithArrays obj;

  obj.i64.size = tc.i64.size();
  obj.i64.p = obj.i64.size ? new int64_t[obj.i64.size] : new int64_t[0];
  for (size_t i = 0; i < tc.i64.size(); ++i)
    obj.i64.p[i] = tc.i64[i];

  obj.u64.size = tc.u64.size();
  obj.u64.p = obj.u64.size ? new uint64_t[obj.u64.size] : new uint64_t[0];
  for (size_t i = 0; i < tc.u64.size(); ++i)
    obj.u64.p[i] = tc.u64[i];

  obj.f32.size = tc.f32.size();
  obj.f32.p = obj.f32.size ? new float[obj.f32.size] : new float[0];
  for (size_t i = 0; i < tc.f32.size(); ++i)
    obj.f32.p[i] = tc.f32[i];

  obj.f64.size = tc.f64.size();
  obj.f64.p = obj.f64.size ? new double[obj.f64.size] : new double[0];
  for (size_t i = 0; i < tc.f64.size(); ++i)
    obj.f64.p[i] = tc.f64[i];

  obj.ss.size = tc.ss.size();
  obj.ss.p = obj.ss.size ? new const char *[obj.ss.size] : new const char *[0];
  for (size_t i = 0; i < tc.ss.size(); ++i)
    obj.ss.p[i] = tc.ss[i].c_str();

  obj.objs.size = tc.objs.size();
  obj.objs.p = obj.objs.size ? new Elem *[obj.objs.size] : new Elem *[0];
  for (size_t i = 0; i < tc.objs.size(); ++i) {
    auto *e = new Elem();
    e->a = tc.objs[i].first;
    e->b = tc.objs[i].second;
    obj.objs.p[i] = e;
  }

  obj.aa.size = tc.aa.size();
  obj.aa.p = obj.aa.size ? new MPackArray<int32_t>[obj.aa.size]
                         : new MPackArray<int32_t>[0];
  for (size_t i = 0; i < tc.aa.size(); ++i) {
    auto &inner = obj.aa.p[i];
    inner.size = tc.aa[i].size();
    inner.p = inner.size ? new int32_t[inner.size] : new int32_t[0];
    for (size_t j = 0; j < tc.aa[i].size(); ++j)
      inner.p[j] = tc.aa[i][j];
  }

  auto bytes = writeArrays(obj);
  EXPECT_EQ(bytes, tc.expected);
}

struct ArrayWriteCaseName {
  template <class ParamType>
  std::string
  operator()(const ::testing::TestParamInfo<ParamType> &info) const {
    return info.param.name;
  }
};

INSTANTIATE_TEST_SUITE_P(
    MPackAllArraysWrite, ObjectWithArraysWriteTest,
    ::testing::Values(ArrayWriteCase{"MixOfAllArrays1",
                                     bytes_arrays_case1,
                                     {1, -2, 300},
                                     {0ULL, 90000ULL, 7000000000ULL},
                                     {3.25f, -0.5f},
                                     {-1230000000.0, 6.02214076e23},
                                     {"hello", "UTF-8 ✓", ""},
                                     {{-10, 20u}, {0, 42u}},
                                     {{1, 2, 3}, {-4, -5}}},
                      ArrayWriteCase{"MixOfAllArrays2",
                                     bytes_arrays_case2,
                                     {-1, 2, -3, 4, -5},
                                     {1ULL, 2ULL, 3ULL, 4ULL, 5ULL},
                                     {1.0f, -1.0f, 1.5f, -2.75f},
                                     {3.141592653589793, 0.0, 1e-300, 1e300},
                                     {"symbols !@#$ \"quote\" \\ backslash",
                                      "multiline\ntext"},
                                     {{123, 456u}},
                                     {{}, {10}, {20, 30}}},
                      ArrayWriteCase{"ArrayOfI64",
                                     bytes_arrays_case3,
                                     {1, -2, 3},
                                     {},
                                     {},
                                     {},
                                     {},
                                     {},
                                     {}},
                      ArrayWriteCase{"ArrayOfU64",
                                     bytes_arrays_case4,
                                     {},
                                     {0ULL, 1234567890123456789ULL},
                                     {},
                                     {},
                                     {},
                                     {},
                                     {}},
                      ArrayWriteCase{"ArrayOfFloats",
                                     bytes_arrays_case5,
                                     {},
                                     {},
                                     {0.0f, 1.0f, -2.5f},
                                     {},
                                     {},
                                     {},
                                     {}},
                      ArrayWriteCase{"ArrayOfDoubles",
                                     bytes_arrays_case6,
                                     {},
                                     {},
                                     {},
                                     {1.0e-100, -1.0e100},
                                     {},
                                     {},
                                     {}},
                      ArrayWriteCase{"ArrayOfStrings",
                                     bytes_arrays_case7,
                                     {},
                                     {},
                                     {},
                                     {},
                                     {"only-one", ""},
                                     {},
                                     {}},
                      ArrayWriteCase{"ArrayOfObjects",
                                     bytes_arrays_case8,
                                     {},
                                     {},
                                     {},
                                     {},
                                     {},
                                     {{1, 2u}, {-3, 4u}},
                                     {}},
                      ArrayWriteCase{"ArrayOfArrays",
                                     bytes_arrays_case9,
                                     {},
                                     {},
                                     {},
                                     {},
                                     {},
                                     {},
                                     {{1, 2, 3}, {}, {-1}}}),
    ArrayWriteCaseName());
