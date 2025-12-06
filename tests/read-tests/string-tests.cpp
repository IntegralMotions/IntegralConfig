#include "objects/StringObjects.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

static ObjectWithStrings parseStrings(const uint8_t* data, size_t len) {
    mpack_reader_t r;
    mpack_reader_init_data(&r, reinterpret_cast<const char*>(data), len);
    ObjectWithStrings obj;
    obj.read(r, 0);
    EXPECT_EQ(mpack_reader_destroy(&r), mpack_ok);
    return obj;
}

struct StringCase {
    std::vector<uint8_t> bytes;
    std::string s1, s2, s3;
};

class ObjectWithStringsTest : public ::testing::TestWithParam<StringCase> {};

TEST_P(ObjectWithStringsTest, DecodesVariousStrings) {
    const auto& tc = GetParam();
    auto obj = parseStrings(tc.bytes.data(), tc.bytes.size());
    EXPECT_EQ(obj.s1, tc.s1);
    EXPECT_EQ(obj.s2, tc.s2);
    EXPECT_EQ(obj.s3, tc.s3);
}

static std::vector<uint8_t> bytes_string_case1 = {0x83, 0xA2, 0x73, 0x31, 0xA5, 0x68, 0x65, 0x6C,
                                                  0x6C, 0x6F, 0xA2, 0x73, 0x32, 0xA5, 0x77, 0x6F,
                                                  0x72, 0x6C, 0x64, 0xA2, 0x73, 0x33, 0xA0};

static std::vector<uint8_t> bytes_string_case2 = {
    0x83, 0xA2, 0x73, 0x31, 0xA9, 0x55, 0x54, 0x46, 0x2D, 0x38, 0x20, 0xE2, 0x9C, 0x93, 0xA2, 0x73, 0x32, 0xD9,
    0x20, 0x73, 0x79, 0x6D, 0x62, 0x6F, 0x6C, 0x73, 0x20, 0x21, 0x40, 0x23, 0x24, 0x20, 0x22, 0x71, 0x75, 0x6F,
    0x74, 0x65, 0x22, 0x20, 0x5C, 0x20, 0x62, 0x61, 0x63, 0x6B, 0x73, 0x6C, 0x61, 0x73, 0x68, 0xA2, 0x73, 0x33,
    0xAE, 0x6D, 0x75, 0x6C, 0x74, 0x69, 0x6C, 0x69, 0x6E, 0x65, 0x0A, 0x74, 0x65, 0x78, 0x74};

INSTANTIATE_TEST_SUITE_P(MPackStrings, ObjectWithStringsTest,
                         ::testing::Values(StringCase{bytes_string_case1, "hello", "world", ""},
                                           StringCase{bytes_string_case2, "UTF-8 ✓",
                                                      "symbols !@#$ \"quote\" \\ backslash", "multiline\ntext"}));
