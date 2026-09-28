#include "MPackArray.h"
#include "MPackObject.hpp"
#include "MessagePayloadRegistry.h"
#include "MessageType.h"
#include "mpack/mpack-expect.h"
#include "mpack/mpack-reader.h"
#include "mpack/mpack-writer.h"
#include "objects/ArrayObjects.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>
#include <optional>
#include <vector>

namespace {

    mpack_error_t readObject(MPackObjectBase& object, const std::vector<uint8_t>& bytes) {
        mpack_reader_t reader;
        mpack_reader_init_data(&reader, reinterpret_cast<const char*>(bytes.data()), bytes.size());
        object.read(reader);
        return mpack_reader_destroy(&reader);
    }

    std::vector<uint8_t> writeObject(const MPackObjectBase& object) {
        mpack_writer_t writer;
        char* data = nullptr;
        size_t size = 0;
        mpack_writer_init_growable(&writer, &data, &size);
        object.write(writer);
        EXPECT_EQ(mpack_writer_destroy(&writer), mpack_ok);
        std::vector<uint8_t> bytes(data, data + size);
        MPACK_FREE(data);
        return bytes;
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

    class UIntObject : public MPackObject<UIntObject, 1> {
      public:
        uint32_t value{};

        static void registerMembers() {
            registerMember("value", CppType::U32, &UIntObject::value);
        }
    };

    class FloatObject : public MPackObject<FloatObject, 1> {
      public:
        float value{};

        static void registerMembers() {
            registerMember("value", CppType::F32, &FloatObject::value);
        }
    };

    class IntArrayObject : public MPackObject<IntArrayObject, 1> {
      public:
        MPackArray<int32_t> values;

        static void registerMembers() {
            registerMember("values", {CppType::Array, CppType::I32}, &IntArrayObject::values);
        }
    };

    class RegistryPayload : public MPackObject<RegistryPayload, 0> {
      public:
        static void registerMembers() {}
    };

    class OptionalObject : public MPackObject<OptionalObject, 2> {
      public:
        std::optional<int32_t> number;
        std::optional<bool> flag;

        static void registerMembers() {
            registerOptionalMember("number", CppType::I32, &OptionalObject::number);
            registerOptionalMember("flag", CppType::Bool, &OptionalObject::flag);
        }
    };

    class OmissionObject : public MPackObject<OmissionObject, 3> {
      public:
        const char* nullOrEmpty = nullptr;
        bool falseBool = false;
        const char* nullOnly = nullptr;

        static void registerMembers() {
            registerOmitNullOrEmptyCString("nullOrEmpty", &OmissionObject::nullOrEmpty);
            registerOmitFalseBool("falseBool", &OmissionObject::falseBool);
            registerOmitNullCString("nullOnly", &OmissionObject::nullOnly);
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

    TEST(MPackObjectBaseRegressionTests, RejectsIntegerOverflowWithoutChangingDestination) {
        const std::vector<uint8_t> bytes = {
            0x81, 0xA5, 'v', 'a', 'l', 'u', 'e', 0xCE, 0xFF, 0xFF, 0xFF, 0xFF,
        };
        IntObject object;
        object.value = 42;

        EXPECT_EQ(readObject(object, bytes), mpack_error_data);
        EXPECT_EQ(object.value, 42);
    }

    TEST(MPackObjectBaseRegressionTests, RejectsNegativeIntegerForUnsignedDestination) {
        const std::vector<uint8_t> bytes = {
            0x81, 0xA5, 'v', 'a', 'l', 'u', 'e', 0xFF,
        };
        UIntObject object;
        object.value = 42;

        EXPECT_EQ(readObject(object, bytes), mpack_error_data);
        EXPECT_EQ(object.value, 42U);
    }

    TEST(MPackObjectBaseRegressionTests, RejectsFractionalFloatForIntegerDestination) {
        const std::vector<uint8_t> bytes = {
            0x81, 0xA5, 'v', 'a', 'l', 'u', 'e', 0xCA, 0x3F, 0xC0, 0x00, 0x00,
        };
        IntObject object;
        object.value = 42;

        EXPECT_EQ(readObject(object, bytes), mpack_error_data);
        EXPECT_EQ(object.value, 42);
    }

    TEST(MPackObjectBaseRegressionTests, RejectsFloatingPointUnderflowWithoutChangingDestination) {
        mpack_writer_t writer;
        char* data = nullptr;
        size_t size = 0;
        mpack_writer_init_growable(&writer, &data, &size);
        mpack_start_map(&writer, 1);
        mpack_write_cstr(&writer, "value");
        mpack_write_double(&writer, std::numeric_limits<double>::denorm_min());
        mpack_finish_map(&writer);
        ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

        const std::vector<uint8_t> bytes(data, data + size);
        MPACK_FREE(data);
        FloatObject object;
        object.value = 42.0F;
        EXPECT_EQ(readObject(object, bytes), mpack_error_data);
        EXPECT_EQ(object.value, 42.0F);
    }

    TEST(MPackObjectBaseRegressionTests, NullArrayPointerSerializesAsNil) {
        IntArrayObject object;
        auto bytes = writeObject(object);
        mpack_reader_t reader;
        mpack_reader_init_data(&reader, reinterpret_cast<const char*>(bytes.data()), bytes.size());
        ASSERT_EQ(mpack_expect_map(&reader), 1U);
        mpack_expect_cstr_match(&reader, "values");
        mpack_expect_nil(&reader);
        mpack_done_map(&reader);
        ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);

    }

    TEST(MPackObjectBaseRegressionTests, ReusedObjectClearsOmittedDecodedArrays) {
        const std::vector<uint8_t> withArray = {
            0x81, 0xA6, 'v', 'a', 'l', 'u', 'e', 's', 0x91, 0x01,
        };
        const std::vector<uint8_t> withoutArray = {0x80};
        IntArrayObject object;

        ASSERT_EQ(readObject(object, withArray), mpack_ok);
        ASSERT_EQ(object.values.size, 1U);
        ASSERT_NE(object.values.p, nullptr);
        ASSERT_EQ(readObject(object, withoutArray), mpack_ok);
        EXPECT_EQ(object.values.size, 0U);
        EXPECT_EQ(object.values.p, nullptr);
    }

    TEST(MPackObjectBaseRegressionTests, PayloadRegistryOwnsOperationNames) {
        char opCode[] = "test.owned";
        ASSERT_TRUE(MessagePayloadRegistry::registerType<RegistryPayload>(MsgType::Event, opCode));
        opCode[0] = 'x';

        auto* payload = MessagePayloadRegistry::create(MsgType::Event, "test.owned");
        ASSERT_NE(payload, nullptr);
        delete payload;
    }

    TEST(MPackObjectBaseRegressionTests, MalformedOptionalValuesDoNotReplaceParsedValues) {
        const std::vector<uint8_t> malformedNumber = {
            0x82, 0xA6, 'n', 'u', 'm', 'b', 'e', 'r', 0x07,
            0xA6, 'n', 'u', 'm', 'b', 'e', 'r', 0xC3,
        };
        OptionalObject numberObject;
        EXPECT_EQ(readObject(numberObject, malformedNumber), mpack_error_type);
        ASSERT_TRUE(numberObject.number.has_value());
        EXPECT_EQ(*numberObject.number, 7);

        const std::vector<uint8_t> malformedBool = {
            0x82, 0xA4, 'f', 'l', 'a', 'g', 0xC3,
            0xA4, 'f', 'l', 'a', 'g', 0x01,
        };
        OptionalObject boolObject;
        EXPECT_EQ(readObject(boolObject, malformedBool), mpack_error_type);
        ASSERT_TRUE(boolObject.flag.has_value());
        EXPECT_TRUE(*boolObject.flag);
    }

    TEST(MPackObjectBaseRegressionTests, AppliesNonOptionalOmissionPoliciesOnWriteAndRead) {
        OmissionObject omitted;
        omitted.nullOrEmpty = "";
        EXPECT_EQ(writeObject(omitted), std::vector<uint8_t>({0x80}));

        OmissionObject included;
        included.nullOrEmpty = "value";
        included.falseBool = true;
        included.nullOnly = "";
        const auto bytes = writeObject(included);
        mpack_reader_t reader;
        mpack_reader_init_data(&reader, reinterpret_cast<const char*>(bytes.data()), bytes.size());
        ASSERT_EQ(mpack_expect_map(&reader), 3U);
        mpack_expect_cstr_match(&reader, "nullOrEmpty");
        mpack_expect_cstr_match(&reader, "value");
        mpack_expect_cstr_match(&reader, "falseBool");
        EXPECT_TRUE(mpack_expect_bool(&reader));
        mpack_expect_cstr_match(&reader, "nullOnly");
        mpack_expect_cstr_match(&reader, "");
        mpack_done_map(&reader);
        EXPECT_EQ(mpack_reader_destroy(&reader), mpack_ok);

        const std::vector<uint8_t> emptyMap = {0x80};
        EXPECT_EQ(readObject(included, emptyMap), mpack_ok);
        EXPECT_EQ(included.nullOrEmpty, nullptr);
        EXPECT_FALSE(included.falseBool);
        EXPECT_EQ(included.nullOnly, nullptr);
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
