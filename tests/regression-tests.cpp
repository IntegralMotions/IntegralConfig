#include "MPackArray.h"
#include "MPackObject.hpp"
#include "mpack/mpack-reader.h"
#include "mpack/mpack-writer.h"
#include "objects/ArrayObjects.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

namespace {

    mpack_error_t readObject(MPackObjectBase& object, const std::vector<uint8_t>& bytes) {
        mpack_reader_t reader;
        mpack_reader_init_data(&reader, reinterpret_cast<const char*>(bytes.data()), bytes.size());
        object.read(reader);
        return mpack_reader_destroy(&reader);
    }

    class IntObject : public MPackObject<IntObject, 1> {
      public:
        int32_t value{};

        static void registerMembers() {
            registerMember("value", CppType::I32, &IntObject::value);
        }
    };

    class NullableStringObject : public MPackObject<NullableStringObject, 1> {
      public:
        const char* value{};

        ~NullableStringObject() override {
            delete[] value;
        }

        static void registerMembers() {
            registerMember("value", CppType::String, &NullableStringObject::value);
        }
    };

    class ChildObject : public MPackObject<ChildObject, 0> {
      public:
        static void registerMembers() {}
    };

    class ParentObject : public MPackObject<ParentObject, 1> {
      public:
        ChildObject child;

        static void registerMembers() {
            registerMember("child", CppType::Object, &ParentObject::child);
        }
    };

    class ObjectArrayWithoutFactory : public MPackObject<ObjectArrayWithoutFactory, 1> {
      public:
        MPackArray<MPackObjectBase*> values;

        ~ObjectArrayWithoutFactory() override {
            if (values.p != nullptr) {
                for (size_t i = 0; i < values.size; ++i) {
                    delete values[i];
                }
                delete[] values.begin();
            }
        }

        static void registerMembers() {
            registerMember("values", {CppType::Array, CppType::ObjectPtr}, &ObjectArrayWithoutFactory::values);
        }
    };

    TEST(MPackObjectBaseRegressionTests, DiscardsUnknownMemberAndReadsFollowingKnownMember) {
        const std::vector<uint8_t> bytes = {
            0x82, 0xA7, 'u', 'n', 'k', 'n', 'o', 'w', 'n', 0x01, 0xA5, 'v', 'a', 'l', 'u', 'e', 0x2A,
        };
        IntObject object;

        EXPECT_EQ(readObject(object, bytes), mpack_ok);
        EXPECT_EQ(object.value, 42);
    }

    TEST(MPackObjectBaseRegressionTests, RejectsMapKeysLargerThanConfiguredMaximum) {
        std::vector<uint8_t> bytes = {0x81, 0xDA, 0x04, 0x00};
        bytes.insert(bytes.end(), MPACK_MAX_STRING, static_cast<uint8_t>('x'));
        bytes.push_back(0x01);
        IntObject object;

        EXPECT_EQ(readObject(object, bytes), mpack_error_too_big);
    }

    TEST(MPackObjectBaseRegressionTests, ReadsNilStringAsNullptr) {
        const std::vector<uint8_t> bytes = {
            0x81, 0xA5, 'v', 'a', 'l', 'u', 'e', 0xC0,
        };
        NullableStringObject object;

        EXPECT_EQ(readObject(object, bytes), mpack_ok);
        EXPECT_EQ(object.value, nullptr);
    }

    TEST(MPackObjectBaseRegressionTests, ReadsNilElementsInStringAndObjectArrays) {
        const std::vector<uint8_t> bytes = {
            0x82, 0xA2, 's', 's', 0x93, 0xA1, 'a', 0xC0, 0xA1, 'b', 0xA4, 'o', 'b', 'j', 's', 0x91, 0xC0,
        };
        ObjectWithArrays object;

        ASSERT_EQ(readObject(object, bytes), mpack_ok);
        ASSERT_EQ(object.ss.size, 3U);
        EXPECT_STREQ(object.ss[0], "a");
        EXPECT_EQ(object.ss[1], nullptr);
        EXPECT_STREQ(object.ss[2], "b");
        ASSERT_EQ(object.objs.size, 1U);
        EXPECT_EQ(object.objs[0], nullptr);
    }

    TEST(MPackObjectBaseRegressionTests, MissingObjectFactoryFlagsDataErrorInsteadOfCrashing) {
        const std::vector<uint8_t> bytes = {
            0x81, 0xA6, 'v', 'a', 'l', 'u', 'e', 's', 0x91, 0x80,
        };
        ObjectArrayWithoutFactory object;

        EXPECT_EQ(readObject(object, bytes), mpack_error_data);
    }

    TEST(MPackObjectBaseRegressionTests, PropagatesWriteDepthToNestedObjects) {
        ParentObject object;
        mpack_writer_t writer;
        char* data = nullptr;
        size_t size = 0;
        mpack_writer_init_growable(&writer, &data, &size);

        object.write(writer, MPACK_MAX_DEPTH);

        EXPECT_EQ(mpack_writer_destroy(&writer), mpack_error_too_big);
        MPACK_FREE(data);
    }
} // namespace
