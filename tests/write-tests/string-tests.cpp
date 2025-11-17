#include "mpack-object.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

class ObjectWithStrings : public MPackObject<ObjectWithStrings> {
public:
  const char *s1{};
  const char *s2{};
  const char *s3{};

  void registerMembers() {
    registerMember("s1", CppType::String, &s1);
    registerMember("s2", CppType::String, &s2);
    registerMember("s3", CppType::String, &s3);
  }
};

static std::vector<uint8_t> writeStrings(const ObjectWithStrings &inObj) {
  mpack_writer_t w;
  char *buf = nullptr;
  size_t size = 0;

  mpack_writer_init_growable(&w, &buf, &size);

  ObjectWithStrings obj = inObj;
  obj.write(w, 0);

  EXPECT_EQ(mpack_writer_destroy(&w), mpack_ok);

  std::vector<uint8_t> out(reinterpret_cast<uint8_t *>(buf),
                           reinterpret_cast<uint8_t *>(buf) + size);
  MPACK_FREE(buf);
  return out;
}

static std::vector<uint8_t> bytes_string_case1 = {
    0x83, 0xA2, 0x73, 0x31, 0xA5, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0xA2, 0x73,
    0x32, 0xA5, 0x77, 0x6F, 0x72, 0x6C, 0x64, 0xA2, 0x73, 0x33, 0xA0};

static std::vector<uint8_t> bytes_string_case2 = {
    0x83, 0xA2, 0x73, 0x31, 0xA9, 0x55, 0x54, 0x46, 0x2D, 0x38, 0x20, 0xE2,
    0x9C, 0x93, 0xA2, 0x73, 0x32, 0xD9, 0x20, 0x73, 0x79, 0x6D, 0x62, 0x6F,
    0x6C, 0x73, 0x20, 0x21, 0x40, 0x23, 0x24, 0x20, 0x22, 0x71, 0x75, 0x6F,
    0x74, 0x65, 0x22, 0x20, 0x5C, 0x20, 0x62, 0x61, 0x63, 0x6B, 0x73, 0x6C,
    0x61, 0x73, 0x68, 0xA2, 0x73, 0x33, 0xAE, 0x6D, 0x75, 0x6C, 0x74, 0x69,
    0x6C, 0x69, 0x6E, 0x65, 0x0A, 0x74, 0x65, 0x78, 0x74};

struct StringWriteCase {
  std::string name;
  std::vector<uint8_t> expected;
  std::string s1;
  std::string s2;
  std::string s3;
};

class ObjectWithStringsWriteTest
    : public ::testing::TestWithParam<StringWriteCase> {};

TEST_P(ObjectWithStringsWriteTest, EncodesVariousStrings) {
  const auto &tc = GetParam();

  ObjectWithStrings obj;
  obj.s1 = tc.s1.c_str();
  obj.s2 = tc.s2.c_str();
  obj.s3 = tc.s3.c_str();

  auto bytes = writeStrings(obj);
  EXPECT_EQ(bytes, tc.expected);
}

struct StringWriteCaseName {
  template <class ParamType>
  std::string
  operator()(const ::testing::TestParamInfo<ParamType> &info) const {
    return info.param.name;
  }
};

INSTANTIATE_TEST_SUITE_P(
    MPackStringsWrite, ObjectWithStringsWriteTest,
    ::testing::Values(StringWriteCase{"StringCase1", bytes_string_case1,
                                      "hello", "world", ""},
                      StringWriteCase{"StringCase2", bytes_string_case2,
                                      "UTF-8 ✓",
                                      "symbols !@#$ \"quote\" \\ backslash",
                                      "multiline\ntext"}),
    StringWriteCaseName());
