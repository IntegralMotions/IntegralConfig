#include "MPackObject.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

class ObjectWithFloats : public MPackObject<ObjectWithFloats> {
public:
  float f32{};
  double f64{};

  void registerMembers() {
    registerMember("f32", CppType::F32, &f32);
    registerMember("f64", CppType::F64, &f64);
  }
};

static std::vector<uint8_t> writeFloats(const ObjectWithFloats &inObj) {
  mpack_writer_t w;
  char *buf = nullptr;
  size_t size = 0;

  mpack_writer_init_growable(&w, &buf, &size);

  ObjectWithFloats obj = inObj;
  obj.write(w, 0);

  EXPECT_EQ(mpack_writer_destroy(&w), mpack_ok);

  std::vector<uint8_t> out(reinterpret_cast<uint8_t *>(buf),
                           reinterpret_cast<uint8_t *>(buf) + size);
  MPACK_FREE(buf);
  return out;
}

static std::vector<uint8_t> bytes_float_case1 = {
    0x82, 0xA3, 0x66, 0x33, 0x32, 0xCA, 0x40, 0x49, 0x0F, 0xD0, 0xA3, 0x66,
    0x36, 0x34, 0xCB, 0xC1, 0xD2, 0x54, 0x13, 0xE0, 0x00, 0x00, 0x00};

static std::vector<uint8_t> bytes_float_case2 = {
    0x82, 0xA3, 0x66, 0x33, 0x32, 0xCA, 0xBF, 0xE0, 0x00, 0x00, 0xA3, 0x66,
    0x36, 0x34, 0xCB, 0x44, 0xDF, 0xE1, 0x85, 0xCA, 0x57, 0xC5, 0x17};

struct FloatWriteCase {
  std::string name;
  std::vector<uint8_t> expected;
  float f32;
  double f64;
};

class ObjectWithFloatsWriteTest
    : public ::testing::TestWithParam<FloatWriteCase> {};

TEST_P(ObjectWithFloatsWriteTest, EncodesFloatDouble) {
  const auto &tc = GetParam();

  ObjectWithFloats obj;
  obj.f32 = tc.f32;
  obj.f64 = tc.f64;

  auto bytes = writeFloats(obj);
  EXPECT_EQ(bytes, tc.expected);
}

struct FloatWriteCaseName {
  template <class ParamType>
  std::string
  operator()(const ::testing::TestParamInfo<ParamType> &info) const {
    return info.param.name;
  }
};

INSTANTIATE_TEST_SUITE_P(
    MPackFloatsWrite, ObjectWithFloatsWriteTest,
    ::testing::Values(
        FloatWriteCase{"FloatCase1", bytes_float_case1, 3.14159f, -1.23e9},
        FloatWriteCase{"FloatCase2", bytes_float_case2, -1.75f, 6.02214076e23}),
    FloatWriteCaseName());
