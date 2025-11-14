// tests/array-tests.cpp
#include "mpack-object.h"
#include <gtest/gtest.h>
#include <string>

struct Elem : MPackObject
{
    int32_t a{};
    uint32_t b{};
    bool setMPackValue(const char *n, void *v, const mpack_type_t &t) override
    {
        if (!n)
            return false;
        if (strcmp(n, "a") == 0)
        {
            if (t == mpack_type_nil)
            {
                a = 0;
                return true;
            }
            if (t == mpack_type_int)
            {
                a = (int32_t)*reinterpret_cast<int64_t *>(v);
                return true;
            }
            if (t == mpack_type_uint)
            {
                a = (int32_t)*reinterpret_cast<uint64_t *>(v);
                return true;
            }
            if (t == mpack_type_float)
            {
                a = (int32_t)*reinterpret_cast<float *>(v);
                return true;
            }
            if (t == mpack_type_double)
            {
                a = (int32_t)*reinterpret_cast<double *>(v);
                return true;
            }
            return false;
        }
        if (strcmp(n, "b") == 0)
        {
            if (t == mpack_type_nil)
            {
                b = 0;
                return true;
            }
            if (t == mpack_type_uint)
            {
                b = (uint32_t)*reinterpret_cast<uint64_t *>(v);
                return true;
            }
            if (t == mpack_type_int)
            {
                b = (uint32_t)*reinterpret_cast<int64_t *>(v);
                return true;
            }
            if (t == mpack_type_float)
            {
                b = (uint32_t)*reinterpret_cast<float *>(v);
                return true;
            }
            if (t == mpack_type_double)
            {
                b = (uint32_t)*reinterpret_cast<double *>(v);
                return true;
            }
            return false;
        }
        return false;
    }
    void write(mpack_writer_t &, int) override {}
};

class ObjectWithArrays : public MPackObject
{
public:
    int64_t *i64 = nullptr;
    size_t i64n = 0;
    uint64_t *u64 = nullptr;
    size_t u64n = 0;
    float *f32 = nullptr;
    size_t f32n = 0;
    double *f64 = nullptr;
    size_t f64n = 0;
    std::string *ss = nullptr;
    size_t ssn = 0;
    Elem **objs = nullptr;
    size_t objn = 0;
    int32_t **aa = nullptr;
    size_t aan = 0;
    size_t *aalen = nullptr;

    ~ObjectWithArrays() override
    {
        delete[] i64;
        delete[] u64;
        delete[] f32;
        delete[] f64;
        delete[] ss;
        if (objs)
        {
            for (size_t i = 0; i < objn; ++i)
                delete objs[i];
            delete[] objs;
        }
        if (aa)
        {
            for (size_t i = 0; i < aan; ++i)
                delete[] aa[i];
            delete[] aa;
        }
        delete[] aalen;
    }

private:
    bool setMPackValue(const char *, void *, const mpack_type_t &) override
    {
        return false;
    }
    void write(mpack_writer_t &, int) override {}
    bool isArray(const char *n) override
    {
        return !strcmp(n, "i64") || !strcmp(n, "u64") || !strcmp(n, "f32") ||
               !strcmp(n, "f64") || !strcmp(n, "ss") || !strcmp(n, "objs") ||
               !strcmp(n, "aa");
    }
    void *getArray(const char *n, size_t count, CppType &outType) override
    {
        if (!strcmp(n, "i64"))
        {
            delete[] i64;
            i64n = count;
            i64 = new int64_t[count]{};
            outType = CppType::I64;
            return i64;
        }
        if (!strcmp(n, "u64"))
        {
            delete[] u64;
            u64n = count;
            u64 = new uint64_t[count]{};
            outType = CppType::U64;
            return u64;
        }
        if (!strcmp(n, "f32"))
        {
            delete[] f32;
            f32n = count;
            f32 = new float[count]{};
            outType = CppType::F32;
            return f32;
        }
        if (!strcmp(n, "f64"))
        {
            delete[] f64;
            f64n = count;
            f64 = new double[count]{};
            outType = CppType::F64;
            return f64;
        }
        if (!strcmp(n, "ss"))
        {
            delete[] ss;
            ssn = count;
            ss = new std::string[count];
            outType = CppType::String;
            return ss;
        }
        if (!strcmp(n, "objs"))
        {
            if (objs)
            {
                for (size_t i = 0; i < objn; ++i)
                    delete objs[i];
                delete[] objs;
            }
            objn = count;
            objs = new Elem *[count]{};
            outType = CppType::Object;
            return objs;
        }
        if (!strcmp(n, "aa"))
        {
            delete[] aalen;
            if (aa)
            {
                for (size_t i = 0; i < aan; ++i)
                    delete[] aa[i];
                delete[] aa;
            }
            aan = count;
            aa = new int32_t *[count]{};
            aalen = new size_t[count]{};
            outType = CppType::Array;
            return aa;
        }
        return nullptr;
    }
    MPackObject *getObject(const char *) override { return new Elem(); }
};

// --- harness
static ObjectWithArrays parseArrays(const uint8_t *data, size_t len)
{
    mpack_reader_t r;
    mpack_reader_init_data(&r, (const char *)data, len);
    ObjectWithArrays obj;
    obj.read(r, 0);
    EXPECT_EQ(mpack_reader_destroy(&r), mpack_ok);
    return obj;
}

struct ArrayCase
{
    std::vector<uint8_t> bytes;
    std::vector<int64_t> i64;
    std::vector<uint64_t> u64;
    std::vector<float> f32;
    std::vector<double> f64;
    std::vector<std::string> ss;
    std::vector<std::pair<int32_t, uint32_t>> objs;
    std::vector<std::vector<int32_t>> aa;
};

class ArraysTest : public ::testing::TestWithParam<ArrayCase>
{
};
TEST_P(ArraysTest, DecodesAllArrays)
{
    const auto &tc = GetParam();
    auto obj = parseArrays(tc.bytes.data(), tc.bytes.size());
    ASSERT_EQ(obj.i64n, tc.i64.size());
    ASSERT_EQ(obj.u64n, tc.u64.size());
    ASSERT_EQ(obj.f32n, tc.f32.size());
    ASSERT_EQ(obj.f64n, tc.f64.size());
    ASSERT_EQ(obj.ssn, tc.ss.size());
    ASSERT_EQ(obj.objn, tc.objs.size());
    ASSERT_EQ(obj.aan, tc.aa.size());
    for (size_t i = 0; i < tc.i64.size(); ++i)
        EXPECT_EQ(obj.i64[i], tc.i64[i]);
    for (size_t i = 0; i < tc.u64.size(); ++i)
        EXPECT_EQ(obj.u64[i], tc.u64[i]);
    for (size_t i = 0; i < tc.f32.size(); ++i)
        EXPECT_FLOAT_EQ(obj.f32[i], tc.f32[i]);
    for (size_t i = 0; i < tc.f64.size(); ++i)
        EXPECT_DOUBLE_EQ(obj.f64[i], tc.f64[i]);
    for (size_t i = 0; i < tc.ss.size(); ++i)
        EXPECT_EQ(obj.ss[i], tc.ss[i]);
    for (size_t i = 0; i < tc.objs.size(); ++i)
    {
        ASSERT_NE(obj.objs[i], nullptr);
        EXPECT_EQ(obj.objs[i]->a, tc.objs[i].first);
        EXPECT_EQ(obj.objs[i]->b, (uint32_t)tc.objs[i].second);
    }
    for (size_t i = 0; i < tc.aa.size(); ++i)
    {
        ASSERT_EQ(obj.aalen[i], tc.aa[i].size());
        for (size_t j = 0; j < tc.aa[i].size(); ++j)
            EXPECT_EQ(obj.aa[i][j], tc.aa[i][j]);
    }
}

// --- your exact hex ---
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

INSTANTIATE_TEST_SUITE_P(
    MPackAllArrays, ArraysTest,
    ::testing::Values(ArrayCase{bytes_arrays_case1,
                                {1, -2, 300},
                                {0ULL, 90000ULL, 7000000000ULL},
                                {3.25f, -0.5f},
                                {-1230000000.0, 6.02214076e23},
                                {"hello", "UTF-8 ✓", ""},
                                {{0, 20u}, {0, 42u}},
                                {{1, 2, 3}, {-4, -5}}},
                      ArrayCase{bytes_arrays_case2,
                                {-1, 2, -3, 4, -5},
                                {1ULL, 2ULL, 3ULL, 4ULL, 5ULL},
                                {1.0f, -1.0f, 1.5f, -2.75f},
                                {3.141592653589793, 0.0, 1e-300, 1e300},
                                {"symbols !@#$ \"quote\" \\ backslash",
                                 "multiline\ntext"},
                                {{123, 456u}},
                                {{}, {10}, {20, 30}}}));
