#include "objects/MixedObjects.h"
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <vector>

// --- harness ---------------------------------------------------------
static MixedObject parseMixed(const uint8_t* data, size_t len) {
    mpack_reader_t r;
    mpack_reader_init_data(&r, reinterpret_cast<const char*>(data), len);
    MixedObject obj;
    obj.read(r, 0);
    EXPECT_EQ(mpack_reader_destroy(&r), mpack_ok);
    return obj;
}

struct MixedCase {
    std::string name;
    std::vector<uint8_t> bytes;

    int8_t i8;
    uint8_t u8;
    int16_t i16;
    uint16_t u16;
    int32_t i32;
    uint32_t u32;
    int64_t i64;
    uint64_t u64;
    float f32;
    double f64;
    bool flag;
    std::string root_name;
    int32_t child_a;
    std::string child_label;
};

class MixedObjectTest : public ::testing::TestWithParam<MixedCase> {};

TEST_P(MixedObjectTest, DecodesAllScalarTypesAndObject) {
    const auto& tc = GetParam();
    auto obj = parseMixed(tc.bytes.data(), tc.bytes.size());

    EXPECT_EQ(obj.i8, tc.i8);
    EXPECT_EQ(obj.u8, tc.u8);
    EXPECT_EQ(obj.i16, tc.i16);
    EXPECT_EQ(obj.u16, tc.u16);
    EXPECT_EQ(obj.i32, tc.i32);
    EXPECT_EQ(obj.u32, tc.u32);
    EXPECT_EQ(obj.i64, tc.i64);
    EXPECT_EQ(obj.u64, tc.u64);
    EXPECT_FLOAT_EQ(obj.f32, tc.f32);
    EXPECT_DOUBLE_EQ(obj.f64, tc.f64);
    EXPECT_EQ(obj.flag, tc.flag);
    ASSERT_NE(obj.name, nullptr);
    EXPECT_STREQ(obj.name, tc.root_name.c_str());

    EXPECT_EQ(obj.child.a, tc.child_a);
    ASSERT_NE(obj.child.label, nullptr);
    EXPECT_STREQ(obj.child.label, tc.child_label.c_str());
}

struct MixedCaseName {
    template <class ParamType> std::string operator()(const ::testing::TestParamInfo<ParamType>& info) const {
        return info.param.name;
    }
};

static std::vector<uint8_t> bytes_mixed_case1 = {
    0x8D, 0xA2, 0x69, 0x38, 0xFF, 0xA2, 0x75, 0x38, 0xCC, 0xFF, 0xA3, 0x69, 0x31, 0x36, 0xD1, 0xFB, 0x2E, 0xA3, 0x75,
    0x31, 0x36, 0xCD, 0xFD, 0xE8, 0xA3, 0x69, 0x33, 0x32, 0xD2, 0xFF, 0xFE, 0x1D, 0xC0, 0xA3, 0x75, 0x33, 0x32, 0xCE,
    0xEE, 0x6B, 0x28, 0x00, 0xA3, 0x69, 0x36, 0x34, 0xD3, 0xFF, 0xFF, 0xFE, 0xE0, 0x8E, 0x04, 0xFB, 0x35, 0xA3, 0x75,
    0x36, 0x34, 0xCF, 0x11, 0x22, 0x10, 0xF4, 0x7D, 0xE9, 0x81, 0x15, 0xA3, 0x66, 0x33, 0x32, 0xCB, 0x40, 0x0C, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0xA3, 0x66, 0x36, 0x34, 0xCB, 0xD4, 0xB6, 0xDC, 0x18, 0x6E, 0xF9, 0xF4, 0x5C, 0xA4,
    0x66, 0x6C, 0x61, 0x67, 0xC3, 0xA4, 0x6E, 0x61, 0x6D, 0x65, 0xA4, 0x72, 0x6F, 0x6F, 0x74, 0xA5, 0x63, 0x68, 0x69,
    0x6C, 0x64, 0x82, 0xA1, 0x61, 0x2A, 0xA5, 0x6C, 0x61, 0x62, 0x65, 0x6C, 0xA5, 0x69, 0x6E, 0x6E, 0x65, 0x72};

static std::vector<uint8_t> bytes_mixed_case2 = {
    0x8D, 0xA2, 0x69, 0x38, 0x00, 0xA2, 0x75, 0x38, 0x00, 0xA3, 0x69, 0x31, 0x36, 0x00, 0xA3, 0x75, 0x31,
    0x36, 0x00, 0xA3, 0x69, 0x33, 0x32, 0x00, 0xA3, 0x75, 0x33, 0x32, 0x00, 0xA3, 0x69, 0x36, 0x34, 0x00,
    0xA3, 0x75, 0x36, 0x34, 0x00, 0xA3, 0x66, 0x33, 0x32, 0xCB, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xA3, 0x66, 0x36, 0x34, 0xCB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA4, 0x66, 0x6C,
    0x61, 0x67, 0xC2, 0xA4, 0x6E, 0x61, 0x6D, 0x65, 0xA0, 0xA5, 0x63, 0x68, 0x69, 0x6C, 0x64, 0x82, 0xA1,
    0x61, 0xD1, 0xFC, 0x18, 0xA5, 0x6C, 0x61, 0x62, 0x65, 0x6C, 0xA1, 0x78};

INSTANTIATE_TEST_SUITE_P(MPackMixedObjectAllTypes, MixedObjectTest,
                         ::testing::Values(MixedCase{"MixedAllTypes1", bytes_mixed_case1,
                                                     /* i8   */ -1,
                                                     /* u8   */ 255,
                                                     /* i16  */ -1234,
                                                     /* u16  */ 65000,
                                                     /* i32  */ -123456,
                                                     /* u32  */ 4000000000u,
                                                     /* i64  */ -1234567890123ll,
                                                     /* u64  */ 1234567890123456789ull,
                                                     /* f32  */ 3.5f,
                                                     /* f64  */ -1.25e100,
                                                     /* flag */ true,
                                                     /* name */ "root",
                                                     /* child_a     */ 42,
                                                     /* child_label */ "inner"},
                                           MixedCase{"MixedAllTypes2", bytes_mixed_case2,
                                                     /* i8   */ 0,
                                                     /* u8   */ 0,
                                                     /* i16  */ 0,
                                                     /* u16  */ 0,
                                                     /* i32  */ 0,
                                                     /* u32  */ 0,
                                                     /* i64  */ 0,
                                                     /* u64  */ 0,
                                                     /* f32  */ -0.0f,
                                                     /* f64  */ 0.0,
                                                     /* flag */ false,
                                                     /* name */ "",
                                                     /* child_a     */ -1000,
                                                     /* child_label */ "x"}),
                         MixedCaseName());
