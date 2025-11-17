#include "mpack-object.hpp"
#include <gtest/gtest.h>

class ObjectWithFloats : public MPackObject<ObjectWithFloats> {
public:
  float f32{};
  double f64{};

public:
  void registerMembers() {
    registerMember("f32", CppType::F32, &f32);
    registerMember("f64", CppType::F64, &f64);
  }
};

static ObjectWithFloats parseFloats(const uint8_t *data, size_t len) {
  mpack_reader_t r;
  mpack_reader_init_data(&r, reinterpret_cast<const char *>(data), len);
  ObjectWithFloats obj;
  obj.read(r, 0);
  EXPECT_EQ(mpack_reader_destroy(&r), mpack_ok);
  return obj;
}

struct FloatCase {
  std::vector<uint8_t> bytes;
  float f32;
  double f64;
};

class ObjectWithFloatsTest : public ::testing::TestWithParam<FloatCase> {};

TEST_P(ObjectWithFloatsTest, DecodesFloatDouble) {
  const auto &tc = GetParam();
  auto obj = parseFloats(tc.bytes.data(), tc.bytes.size());
  EXPECT_FLOAT_EQ(obj.f32, tc.f32);
  EXPECT_DOUBLE_EQ(obj.f64, tc.f64);
}

static std::vector<uint8_t> bytes_float_case1 = {
    0x82, 0xA3, 0x66, 0x33, 0x32, 0xCA, 0x40, 0x49, 0x0F, 0xD0, 0xA3, 0x66,
    0x36, 0x34, 0xCB, 0xC1, 0xD2, 0x54, 0x13, 0xE0, 0x00, 0x00, 0x00};

static std::vector<uint8_t> bytes_float_case2 = {
    0x82, 0xA3, 0x66, 0x33, 0x32, 0xCB, 0xBF, 0xE0, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x66, 0x36, 0x34,
    0xCB, 0x44, 0xDF, 0xE1, 0x85, 0xCA, 0x57, 0xC5, 0x17};

INSTANTIATE_TEST_SUITE_P(
    MPackFloats, ObjectWithFloatsTest,
    ::testing::Values(FloatCase{bytes_float_case1, /* f32 */ 3.14159f,
                                /* f64 */ -1.23e9},
                      FloatCase{bytes_float_case2, /* f32 */ -0.5f,
                                /* f64 */ 6.02214076e23}));
