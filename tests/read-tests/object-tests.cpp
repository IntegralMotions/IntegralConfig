// tests/object-tests.cpp
#include "objects/ObjectObjects.h"
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <vector>

// --- harness ---------------------------------------------------------
static ObjectWithObjects parseObjects(const uint8_t *data, size_t len) {
    mpack_reader_t r;
    mpack_reader_init_data(&r, reinterpret_cast<const char *>(data), len);
    ObjectWithObjects obj;
    obj.read(r, 0);
    EXPECT_EQ(mpack_reader_destroy(&r), mpack_ok);
    return obj;
}

struct ObjectCase {
    std::string name;
    std::vector<uint8_t> bytes;
    int32_t left_a;
    uint32_t left_b;
    int32_t right_a;
    uint32_t right_b;
};

class ObjectWithObjectsTest : public ::testing::TestWithParam<ObjectCase> {};

TEST_P(ObjectWithObjectsTest, DecodesNestedObjects) {
    const auto &tc = GetParam();
    auto obj = parseObjects(tc.bytes.data(), tc.bytes.size());

    EXPECT_EQ(obj.left.a, tc.left_a);
    EXPECT_EQ(obj.left.b, tc.left_b);
    EXPECT_EQ(obj.right.a, tc.right_a);
    EXPECT_EQ(obj.right.b, tc.right_b);
}

struct ObjectCaseName {
    template <class ParamType> std::string operator()(const ::testing::TestParamInfo<ParamType> &info) const {
        return info.param.name;
    }
};

// Fill these byte arrays from the JSON below
static std::vector<uint8_t> bytes_objects_case1 = {0x82, 0xA4, 0x6C, 0x65, 0x66, 0x74, 0x82, 0xA1, 0x61,
                                                   0x01, 0xA1, 0x62, 0x02, 0xA5, 0x72, 0x69, 0x67, 0x68,
                                                   0x74, 0x82, 0xA1, 0x61, 0xFD, 0xA1, 0x62, 0x04}; // SimpleNested1

static std::vector<uint8_t> bytes_objects_case2 = {0x82, 0xA4, 0x6C, 0x65, 0x66, 0x74, 0x82, 0xA1, 0x61,
                                                   0x00, 0xA1, 0x62, 0x00, 0xA5, 0x72, 0x69, 0x67, 0x68,
                                                   0x74, 0x82, 0xA1, 0x61, 0x0A, 0xA1, 0x62, 0x14}; // SimpleNested2

static std::vector<uint8_t> bytes_objects_case3 = {0x82, 0xA4, 0x6C, 0x65, 0x66, 0x74, 0x82, 0xA1, 0x61, 0xD0, 0x9C,
                                                   0xA1, 0x62, 0xCE, 0x00, 0x01, 0xE2, 0x40, 0xA5, 0x72, 0x69, 0x67,
                                                   0x68, 0x74, 0x82, 0xA1, 0x61, 0x2A, 0xA1, 0x62, 0x00}; // MixedSigns

INSTANTIATE_TEST_SUITE_P(MPackObjects, ObjectWithObjectsTest,
                         ::testing::Values(ObjectCase{"SimpleNested1", bytes_objects_case1,
                                                      /*left_a*/ 1,
                                                      /*left_b*/ 2u,
                                                      /*right_a*/ -3,
                                                      /*right_b*/ 4u},
                                           ObjectCase{"SimpleNested2", bytes_objects_case2,
                                                      /*left_a*/ 0,
                                                      /*left_b*/ 0u,
                                                      /*right_a*/ 10,
                                                      /*right_b*/ 20u},
                                           ObjectCase{"MixedSigns", bytes_objects_case3,
                                                      /*left_a*/ -100,
                                                      /*left_b*/ 123456u,
                                                      /*right_a*/ 42,
                                                      /*right_b*/ 0u}),
                         ObjectCaseName());
