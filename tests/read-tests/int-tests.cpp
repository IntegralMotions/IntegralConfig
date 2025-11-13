#include "integral_config.h"
#include "mpack-object.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <sys/_types/_u_int16_t.h>
#include <sys/types.h>

class ObjectWithInts : public MPackObject {
public:
  uint8_t value1;
  int8_t value2;
  uint16_t value3;
  int16_t value4;
  uint32_t value5;
  int32_t value6;
  uint64_t value7;
  int64_t value8;
  uint value9;
  int value10;

private:
  bool setMPackValue(const char *name, void *value,
                     const mpack_type_t &type) override {
    if (!name || !value)
      return false;

    switch (type) {
    case mpack_type_uint: {
      uint64_t v = *reinterpret_cast<uint64_t *>(value);
      if (strcmp(name, "value1") == 0)
        value1 = static_cast<uint8_t>(v);
      else if (strcmp(name, "value3") == 0)
        value3 = static_cast<uint16_t>(v);
      else if (strcmp(name, "value5") == 0)
        value5 = static_cast<uint32_t>(v);
      else if (strcmp(name, "value7") == 0)
        value7 = v;
      else if (strcmp(name, "value9") == 0)
        value9 = static_cast<unsigned int>(v);
      else
        return false;
      return true;
    }

    case mpack_type_int: {
      int64_t v = *reinterpret_cast<int64_t *>(value);
      if (strcmp(name, "value2") == 0)
        value2 = static_cast<int8_t>(v);
      else if (strcmp(name, "value4") == 0)
        value4 = static_cast<int16_t>(v);
      else if (strcmp(name, "value6") == 0)
        value6 = static_cast<int32_t>(v);
      else if (strcmp(name, "value8") == 0)
        value8 = v;
      else if (strcmp(name, "value10") == 0)
        value10 = static_cast<int>(v);
      else
        return false;
      return true;
    }

    default:
      // ignore non-integer types
      return false;
    }
  }

  void write(mpack_writer_t &writer, int depth = 0) override {}
};

static ObjectWithInts parseObject(const uint8_t *data, size_t len) {
  mpack_reader_t reader;
  mpack_reader_init_data(&reader, reinterpret_cast<const char *>(data), len);
  ObjectWithInts obj;
  obj.read(reader, 0);
  EXPECT_EQ(mpack_reader_destroy(&reader), mpack_ok);
  return obj;
}

struct MPackTestCase {
  std::vector<uint8_t> bytes;
  uint8_t v1;
  int8_t v2;
  uint16_t v3;
  int16_t v4;
  uint32_t v5;
  int32_t v6;
  uint64_t v7;
  int64_t v8;
  unsigned v9;
  int v10;
};

class ObjectWithIntsTest : public ::testing::TestWithParam<MPackTestCase> {};

TEST_P(ObjectWithIntsTest, DecodesValues) {
  const auto &tc = GetParam();
  auto obj = parseObject(tc.bytes.data(), tc.bytes.size());
  EXPECT_EQ(obj.value1, tc.v1);
  EXPECT_EQ(obj.value2, tc.v2);
  EXPECT_EQ(obj.value3, tc.v3);
  EXPECT_EQ(obj.value4, tc.v4);
  EXPECT_EQ(obj.value5, tc.v5);
  EXPECT_EQ(obj.value6, tc.v6);
  EXPECT_EQ(obj.value7, tc.v7);
  EXPECT_EQ(obj.value8, tc.v8);
  EXPECT_EQ(obj.value9, tc.v9);
  EXPECT_EQ(obj.value10, tc.v10);
}

static std::vector<uint8_t> bytes_case1 = {
    0x8A, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x31, 0x01, 0xA6, 0x76, 0x61,
    0x6C, 0x75, 0x65, 0x32, 0xFE, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x33,
    0xCD, 0x01, 0x2C, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x34, 0xD1, 0xFE,
    0x70, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x35, 0xCD, 0xC3, 0x50, 0xA6,
    0x76, 0x61, 0x6C, 0x75, 0x65, 0x36, 0xD2, 0xFF, 0xFF, 0x15, 0xA0, 0xA6,
    0x76, 0x61, 0x6C, 0x75, 0x65, 0x37, 0xCF, 0x00, 0x00, 0x00, 0x01, 0xA1,
    0x3B, 0x86, 0x00, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x38, 0xD3, 0xFF,
    0xFF, 0xFF, 0xFE, 0x23, 0x29, 0xB0, 0x00, 0xA6, 0x76, 0x61, 0x6C, 0x75,
    0x65, 0x39, 0xCE, 0x00, 0x01, 0x5F, 0x90, 0xA7, 0x76, 0x61, 0x6C, 0x75,
    0x65, 0x31, 0x30, 0xD2, 0xFF, 0xFE, 0x79, 0x60};

static std::vector<uint8_t> bytes_case2 = {
    0x8A, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x31, 0x03, 0xA6, 0x76, 0x61,
    0x6C, 0x75, 0x65, 0x32, 0xFB, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x33,
    0xCD, 0x18, 0x5A, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x34, 0xD1, 0xFF,
    0x16, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x35, 0xCE, 0x00, 0x01, 0x26,
    0x4F, 0xA6, 0x76, 0x61, 0x6C, 0x75, 0x65, 0x36, 0xD1, 0xE6, 0xE0, 0xA6,
    0x76, 0x61, 0x6C, 0x75, 0x65, 0x37, 0xCE, 0x00, 0xBC, 0x84, 0x3F, 0xA6,
    0x76, 0x61, 0x6C, 0x75, 0x65, 0x38, 0xD2, 0xCC, 0x6A, 0xAF, 0x4B, 0xA6,
    0x76, 0x61, 0x6C, 0x75, 0x65, 0x39, 0xCD, 0x09, 0x83, 0xA7, 0x76, 0x61,
    0x6C, 0x75, 0x65, 0x31, 0x30, 0xD2, 0xFF, 0xF6, 0x59, 0x7B};

INSTANTIATE_TEST_SUITE_P(
    MPackObjectVectors, ObjectWithIntsTest,
    ::testing::Values(MPackTestCase{bytes_case1,
                                    /* v1  */ 1,
                                    /* v2  */ -2,
                                    /* v3  */ 300,
                                    /* v4  */ -400,
                                    /* v5  */ 50000u,
                                    /* v6  */ -60000,
                                    /* v7  */ 7000000000ULL,
                                    /* v8  */ -8000000000LL,
                                    /* v9  */ 90000u,
                                    /* v10 */ -100000},
                      MPackTestCase{bytes_case2,
                                    /* v1  */ 3,
                                    /* v2  */ -5,
                                    /* v3  */ 6234,
                                    /* v4  */ -234,
                                    /* v5  */ 75343u,
                                    /* v6  */ -6432,
                                    /* v7  */ 12354623ULL,
                                    /* v8  */ -865423541LL,
                                    /* v9  */ 2435u,
                                    /* v10 */ -632453}));