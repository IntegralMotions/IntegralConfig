#include "Configuration.h"
#include "DefaultMessagePayloads.h"
#include "DeviceResponseSerializer.h"
#include "Messages.h"
#include "SuccessResponseSerializer.h"
#include "WriteSettingsResponseSerializer.h"

#include <array>
#include <cstdint>
#include <string_view>
#include <gtest/gtest.h>
#include <mpack/mpack.h>

namespace IntegralMotions::Config {
    namespace {

        constexpr SettingKey EnableKey{.id = 0, .scope = SettingScope::Motor, .instance = 0};
        constexpr SettingKey SpeedKey{.id = 0, .scope = SettingScope::Motor, .instance = 1};

        TEST(DeviceResponseSerializer, WritesRegistrySettingsAsReadDeviceResponse) {
            ASSERT_TRUE(registerDefaultMessagePayloads());

            const SettingDefinition<bool> enableDefinition{
                .key = EnableKey,
                .moduleId = "motor",
                .groupId = "main",
                .id = "enable",
                .defaultValue = true,
            };
            const auto speedDefinition = [] {
                SettingDefinition<int32_t> value{
                    .key = SpeedKey,
                    .moduleId = "motor",
                    .groupId = "main",
                    .id = "speed",
                    .unit = "rpm",
                    .defaultValue = 500,
                    .limits = {.minimum = 0, .maximum = 2000, .step = 100, .optionCount = 2},
                };
                EXPECT_TRUE(value.limits.options[0].id.assign("low-speed"));
                value.limits.options[0].value = 500;
                EXPECT_TRUE(value.limits.options[1].id.assign("high-speed"));
                value.limits.options[1].value = 1500;
                return value;
            }();

            bool enable = false;
            int32_t speed = 0;
            SettingsRegistry<2> registry;
            ASSERT_EQ(registry.add(enableDefinition, enable), SettingResult::Ok);
            ASSERT_EQ(registry.add(speedDefinition, speed), SettingResult::Ok);
            ASSERT_EQ(registry.set(SpeedKey, int32_t{1500}), SettingResult::Ok);

            std::array<char, 1024> bytes{};
            mpack_writer_t writer;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            DeviceResponseSerializer<2>{registry, {.model = "Drive", .firmwareVersion = "1.0.0"}}.write(writer);
            const size_t size = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_t reader;
            mpack_reader_init_data(&reader, bytes.data(), size);
            Message response;
            response.read(reader);
            ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);

            EXPECT_EQ(response.getMsgType(), MsgType::Response);
            EXPECT_TRUE(response.isOpCode(DefaultReadKeys::readDevice));
            const auto* device = static_cast<const Device*>(response.payload);
            ASSERT_NE(device, nullptr);
            ASSERT_NE(device->deviceInfo, nullptr);
            EXPECT_STREQ(device->deviceInfo->model, "Drive");
            ASSERT_EQ(device->modules.size, 1);
            EXPECT_STREQ(device->modules[0]->id, "motor");
            ASSERT_EQ(device->modules[0]->groups.size, 1);
            const auto* group = device->modules[0]->groups[0];
            EXPECT_STREQ(group->id, "main");
            ASSERT_EQ(group->settings.size, 2);
            EXPECT_STREQ(group->settings[0]->type, "bool");
            EXPECT_STREQ(group->settings[1]->type, "i32");
            const auto* speedValue = static_cast<const NumberSetting<int32_t>*>(group->settings[1]->value);
            EXPECT_EQ(speedValue->address, 0x410000U);
            EXPECT_EQ(speedValue->value, 1500);
            ASSERT_TRUE(speedValue->limits.minimum.has_value());
            EXPECT_EQ(*speedValue->limits.minimum, 0);
            ASSERT_TRUE(speedValue->limits.maximum.has_value());
            EXPECT_EQ(*speedValue->limits.maximum, 2000);
            ASSERT_TRUE(speedValue->limits.step.has_value());
            EXPECT_EQ(*speedValue->limits.step, 100);
            EXPECT_FALSE(speedValue->limits.isRange);
            ASSERT_EQ(speedValue->limits.options.size, 2);
            EXPECT_EQ(speedValue->limits.options[0]->value, 500);
            EXPECT_STREQ(speedValue->limits.options[0]->id, "low-speed");
        }

        TEST(SuccessResponseSerializer, WritesFailureResultWithErrorId) {
            ASSERT_TRUE(registerDefaultMessagePayloads());

            std::array<char, 256> bytes{};
            mpack_writer_t writer;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            SuccessResponseSerializer{DefaultReadKeys::writeDevice, SettingResult::ReadOnly}.write(writer);
            const size_t size = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_t reader;
            mpack_reader_init_data(&reader, bytes.data(), size);
            Message response;
            response.read(reader);
            ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);

            EXPECT_EQ(response.getMsgType(), MsgType::Response);
            EXPECT_TRUE(response.isOpCode(DefaultReadKeys::writeDevice));
            const auto* result = static_cast<const SuccessResult*>(response.payload);
            ASSERT_NE(result, nullptr);
            EXPECT_FALSE(result->success);
            EXPECT_STREQ(result->errorMessage, "readonly");
        }

        TEST(SuccessResponseSerializer, OmitsErrorMessageForSuccess) {
            ASSERT_TRUE(registerDefaultMessagePayloads());

            std::array<char, 256> bytes{};
            mpack_writer_t writer;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            SuccessResponseSerializer{DefaultReadKeys::writeDevice, SettingResult::Ok}.write(writer);
            const size_t size = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_t reader;
            mpack_reader_init_data(&reader, bytes.data(), size);
            Message response;
            response.read(reader);
            ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);

            const auto* result = static_cast<const SuccessResult*>(response.payload);
            ASSERT_NE(result, nullptr);
            EXPECT_TRUE(result->success);
            EXPECT_EQ(result->errorMessage, nullptr);
        }

        TEST(WriteSettings, DecodesCompactTypedValues) {
            ASSERT_TRUE(registerDefaultMessagePayloads());

            std::array<char, 256> bytes{};
            mpack_writer_t writer;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            mpack_start_map(&writer, 3);
            mpack_write_cstr(&writer, "msgType");
            mpack_write_cstr(&writer, "request");
            mpack_write_cstr(&writer, "opCode");
            mpack_write_cstr(&writer, DefaultWriteKeys::writeSettings);
            mpack_write_cstr(&writer, "payload");
            mpack_start_map(&writer, 1);
            mpack_write_cstr(&writer, "values");
            mpack_start_array(&writer, 2);
            mpack_start_map(&writer, 3);
            mpack_write_cstr(&writer, "address");
            mpack_write_u32(&writer, 0x410000U);
            mpack_write_cstr(&writer, "type");
            mpack_write_cstr(&writer, "i32");
            mpack_write_cstr(&writer, "value");
            mpack_start_map(&writer, 1);
            mpack_write_cstr(&writer, "value");
            mpack_write_i32(&writer, 1200);
            mpack_finish_map(&writer);
            mpack_finish_map(&writer);
            mpack_start_map(&writer, 3);
            mpack_write_cstr(&writer, "address");
            mpack_write_u32(&writer, 0x400000U);
            mpack_write_cstr(&writer, "type");
            mpack_write_cstr(&writer, "bool");
            mpack_write_cstr(&writer, "value");
            mpack_start_map(&writer, 1);
            mpack_write_cstr(&writer, "value");
            mpack_write_bool(&writer, true);
            mpack_finish_map(&writer);
            mpack_finish_map(&writer);
            mpack_finish_array(&writer);
            mpack_finish_map(&writer);
            mpack_finish_map(&writer);
            const size_t size = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_t reader;
            mpack_reader_init_data(&reader, bytes.data(), size);
            Message message;
            message.read(reader);
            ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);

            ASSERT_EQ(message.getMsgType(), MsgType::Request);
            ASSERT_TRUE(message.isOpCode(DefaultWriteKeys::writeSettings));
            const auto* writes = static_cast<const WriteSettings*>(message.payload);
            ASSERT_NE(writes, nullptr);
            ASSERT_EQ(writes->values.size, 2);
            EXPECT_EQ(writes->values[0]->address, 0x410000U);
            EXPECT_STREQ(writes->values[0]->type, "i32");
            EXPECT_EQ(static_cast<const NumberSetting<int32_t>*>(writes->values[0]->value)->value, 1200);
            EXPECT_EQ(writes->values[1]->address, 0x400000U);
            EXPECT_STREQ(writes->values[1]->type, "bool");
            EXPECT_TRUE(static_cast<const BoolSetting*>(writes->values[1]->value)->value);
        }

        TEST(WriteSettingsResponseSerializer, WritesPerValueResults) {
            ASSERT_TRUE(registerDefaultMessagePayloads());
            const std::array values{
                WriteSettingsResultEntry{.address = 0x410000U, .result = SettingResult::Ok},
                WriteSettingsResultEntry{.address = 0x400000U, .result = SettingResult::ReadOnly},
            };
            std::array<char, 256> bytes{};
            mpack_writer_t writer;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            WriteSettingsResponseSerializer{values}.write(writer);
            const size_t size = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_t reader;
            mpack_reader_init_data(&reader, bytes.data(), size);
            Message message;
            message.read(reader);
            ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);

            const auto* result = static_cast<const WriteSettingsResult*>(message.payload);
            ASSERT_NE(result, nullptr);
            ASSERT_EQ(result->values.size, 2);
            EXPECT_TRUE(result->values[0]->success);
            EXPECT_EQ(result->values[0]->errorMessage, nullptr);
            EXPECT_FALSE(result->values[1]->success);
            EXPECT_STREQ(result->values[1]->errorMessage, "readonly");
        }

        TEST(ProtocolModels, GenericSerializationOmitsAbsentFields) {
            NumberSetting<int32_t> setting;
            setting.id = "speed";
            setting.unit = "";
            setting.value = 100;
            setting.readonly = false;

            std::array<char, 256> bytes{};
            mpack_writer_t writer;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            setting.write(writer);
            const size_t size = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_t reader;
            mpack_reader_init_data(&reader, bytes.data(), size);
            ASSERT_EQ(mpack_expect_map(&reader), 4U);
            mpack_expect_cstr_match(&reader, "address");
            EXPECT_EQ(mpack_expect_u32(&reader), 0U);
            ASSERT_EQ(mpack_reader_error(&reader), mpack_ok);
            mpack_expect_cstr_match(&reader, "id");
            mpack_expect_cstr_match(&reader, "speed");
            ASSERT_EQ(mpack_reader_error(&reader), mpack_ok);
            mpack_expect_cstr_match(&reader, "value");
            mpack_discard(&reader);
            ASSERT_EQ(mpack_reader_error(&reader), mpack_ok);
            mpack_expect_cstr_match(&reader, "limits");
            ASSERT_EQ(mpack_expect_map(&reader), 2U);
            mpack_expect_cstr_match(&reader, "isRange");
            EXPECT_FALSE(mpack_expect_bool(&reader));
            ASSERT_EQ(mpack_reader_error(&reader), mpack_ok);
            mpack_expect_cstr_match(&reader, "options");
            mpack_expect_nil(&reader);
            ASSERT_EQ(mpack_reader_error(&reader), mpack_ok);
            mpack_done_map(&reader);
            ASSERT_EQ(mpack_reader_error(&reader), mpack_ok);
            mpack_done_map(&reader);
            EXPECT_EQ(mpack_reader_destroy(&reader), mpack_ok);

            SuccessResult result;
            result.success = true;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            result.write(writer);
            const size_t resultSize = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);
            mpack_reader_init_data(&reader, bytes.data(), resultSize);
            ASSERT_EQ(mpack_expect_map(&reader), 1U);
            mpack_expect_cstr_match(&reader, "success");
            EXPECT_TRUE(mpack_expect_bool(&reader));
            mpack_done_map(&reader);
            EXPECT_EQ(mpack_reader_destroy(&reader), mpack_ok);
        }

        TEST(ProtocolModels, RejectsPayloadBeforeDiscriminatorsWithoutCrashing) {
            std::array<char, 128> bytes{};
            mpack_writer_t writer;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            mpack_start_map(&writer, 3);
            mpack_write_cstr(&writer, "payload");
            mpack_start_map(&writer, 0);
            mpack_finish_map(&writer);
            mpack_write_cstr(&writer, "msgType");
            mpack_write_cstr(&writer, "response");
            mpack_write_cstr(&writer, "opCode");
            mpack_write_cstr(&writer, DefaultReadKeys::readDevice);
            mpack_finish_map(&writer);
            const size_t messageSize = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_t reader;
            mpack_reader_init_data(&reader, bytes.data(), messageSize);
            Message message;
            message.read(reader);
            EXPECT_EQ(mpack_reader_destroy(&reader), mpack_error_data);

            mpack_writer_init(&writer, bytes.data(), bytes.size());
            mpack_start_map(&writer, 2);
            mpack_write_cstr(&writer, "value");
            mpack_start_map(&writer, 0);
            mpack_finish_map(&writer);
            mpack_write_cstr(&writer, "type");
            mpack_write_cstr(&writer, "i32");
            mpack_finish_map(&writer);
            const size_t settingSize = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_init_data(&reader, bytes.data(), settingSize);
            Setting settingValue;
            settingValue.read(reader);
            EXPECT_EQ(mpack_reader_destroy(&reader), mpack_error_data);
        }

        TEST(ProtocolModels, ReusedObjectsDoNotRetainDiscriminatorsOrPayloadTypes) {
            ASSERT_TRUE(registerDefaultMessagePayloads());
            std::array<char, 256> bytes{};
            mpack_writer_t writer;
            mpack_reader_t reader;

            mpack_writer_init(&writer, bytes.data(), bytes.size());
            mpack_start_map(&writer, 3);
            mpack_write_cstr(&writer, "msgType");
            mpack_write_cstr(&writer, "response");
            mpack_write_cstr(&writer, "opCode");
            mpack_write_cstr(&writer, DefaultReadKeys::readDevice);
            mpack_write_cstr(&writer, "payload");
            mpack_start_map(&writer, 0);
            mpack_finish_map(&writer);
            mpack_finish_map(&writer);
            const size_t validMessageSize = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            Message message;
            mpack_reader_init_data(&reader, bytes.data(), validMessageSize);
            message.read(reader);
            ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);
            ASSERT_NE(message.payload, nullptr);

            mpack_writer_init(&writer, bytes.data(), bytes.size());
            mpack_start_map(&writer, 3);
            mpack_write_cstr(&writer, "payload");
            mpack_start_map(&writer, 0);
            mpack_finish_map(&writer);
            mpack_write_cstr(&writer, "msgType");
            mpack_write_cstr(&writer, "response");
            mpack_write_cstr(&writer, "opCode");
            mpack_write_cstr(&writer, DefaultReadKeys::readDevice);
            mpack_finish_map(&writer);
            const size_t invalidMessageSize = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_init_data(&reader, bytes.data(), invalidMessageSize);
            message.read(reader);
            EXPECT_EQ(mpack_reader_destroy(&reader), mpack_error_data);

            const auto writeSetting = [&bytes](const char* type) {
                mpack_writer_t settingWriter;
                mpack_writer_init(&settingWriter, bytes.data(), bytes.size());
                mpack_start_map(&settingWriter, 2);
                mpack_write_cstr(&settingWriter, "type");
                mpack_write_cstr(&settingWriter, type);
                mpack_write_cstr(&settingWriter, "value");
                mpack_start_map(&settingWriter, 0);
                mpack_finish_map(&settingWriter);
                mpack_finish_map(&settingWriter);
                const size_t used = mpack_writer_buffer_used(&settingWriter);
                EXPECT_EQ(mpack_writer_destroy(&settingWriter), mpack_ok);
                return used;
            };

            Setting setting;
            size_t settingSize = writeSetting("i32");
            mpack_reader_init_data(&reader, bytes.data(), settingSize);
            setting.read(reader);
            ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);
            ASSERT_NE(dynamic_cast<NumberSetting<int32_t>*>(setting.value), nullptr);

            settingSize = writeSetting("f64");
            mpack_reader_init_data(&reader, bytes.data(), settingSize);
            setting.read(reader);
            ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);
            EXPECT_NE(dynamic_cast<NumberSetting<double>*>(setting.value), nullptr);
        }

        TEST(ProtocolModels, RejectsRemovedStringSettingType) {
            std::array<char, 128> bytes{};
            mpack_writer_t writer;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            mpack_start_map(&writer, 2);
            mpack_write_cstr(&writer, "type");
            mpack_write_cstr(&writer, "string");
            mpack_write_cstr(&writer, "value");
            mpack_start_map(&writer, 0);
            mpack_finish_map(&writer);
            mpack_finish_map(&writer);
            const size_t size = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            mpack_reader_t reader;
            mpack_reader_init_data(&reader, bytes.data(), size);
            Setting setting;
            setting.read(reader);
            EXPECT_EQ(mpack_reader_destroy(&reader), mpack_error_data);
        }

        TEST(DeviceResponseSerializer, UsesExplicitWidthTypeNames) {
            SettingsRegistry<10> registry;
            int8_t i8{};
            uint8_t u8{};
            int16_t i16{};
            uint16_t u16{};
            int32_t i32{};
            uint32_t u32{};
            int64_t i64{};
            uint64_t u64{};
            float f32{};
            double f64{};

            const SettingDefinition<int8_t> i8Definition{.key = {.instance = 0}, .moduleId = "types", .groupId = "all", .id = "i8"};
            const SettingDefinition<uint8_t> u8Definition{.key = {.instance = 1}, .moduleId = "types", .groupId = "all", .id = "u8"};
            const SettingDefinition<int16_t> i16Definition{.key = {.instance = 2}, .moduleId = "types", .groupId = "all", .id = "i16"};
            const SettingDefinition<uint16_t> u16Definition{.key = {.instance = 3}, .moduleId = "types", .groupId = "all", .id = "u16"};
            const SettingDefinition<int32_t> i32Definition{.key = {.instance = 4}, .moduleId = "types", .groupId = "all", .id = "i32"};
            const SettingDefinition<uint32_t> u32Definition{.key = {.instance = 5}, .moduleId = "types", .groupId = "all", .id = "u32"};
            const SettingDefinition<int64_t> i64Definition{.key = {.instance = 6}, .moduleId = "types", .groupId = "all", .id = "i64"};
            const SettingDefinition<uint64_t> u64Definition{.key = {.instance = 7}, .moduleId = "types", .groupId = "all", .id = "u64"};
            const SettingDefinition<float> f32Definition{.key = {.instance = 8}, .moduleId = "types", .groupId = "all", .id = "f32"};
            const SettingDefinition<double> f64Definition{.key = {.instance = 9}, .moduleId = "types", .groupId = "all", .id = "f64"};

            ASSERT_EQ(registry.add(i8Definition, i8), SettingResult::Ok);
            ASSERT_EQ(registry.add(u8Definition, u8), SettingResult::Ok);
            ASSERT_EQ(registry.add(i16Definition, i16), SettingResult::Ok);
            ASSERT_EQ(registry.add(u16Definition, u16), SettingResult::Ok);
            ASSERT_EQ(registry.add(i32Definition, i32), SettingResult::Ok);
            ASSERT_EQ(registry.add(u32Definition, u32), SettingResult::Ok);
            ASSERT_EQ(registry.add(i64Definition, i64), SettingResult::Ok);
            ASSERT_EQ(registry.add(u64Definition, u64), SettingResult::Ok);
            ASSERT_EQ(registry.add(f32Definition, f32), SettingResult::Ok);
            ASSERT_EQ(registry.add(f64Definition, f64), SettingResult::Ok);

            std::array<char, 4096> bytes{};
            mpack_writer_t writer;
            mpack_writer_init(&writer, bytes.data(), bytes.size());
            DeviceResponseSerializer<10>{registry, {.model = "Drive", .firmwareVersion = "1.0.0"}}.write(writer);
            const size_t size = mpack_writer_buffer_used(&writer);
            ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

            ASSERT_TRUE(registerDefaultMessagePayloads());
            mpack_reader_t reader;
            mpack_reader_init_data(&reader, bytes.data(), size);
            Message response;
            response.read(reader);
            ASSERT_EQ(mpack_reader_destroy(&reader), mpack_ok);

            const auto* device = static_cast<const Device*>(response.payload);
            ASSERT_NE(device, nullptr);
            ASSERT_EQ(device->modules.size, 1U);
            ASSERT_EQ(device->modules[0]->groups.size, 1U);
            const auto& settings = device->modules[0]->groups[0]->settings;
            ASSERT_EQ(settings.size, 10U);
            constexpr std::array<std::string_view, 10> expected{"i8", "u8", "i16", "u16", "i32",
                                                                 "u32", "i64", "u64", "f32", "f64"};
            for (size_t index = 0; index < expected.size(); ++index) {
                EXPECT_EQ(settings[index]->type, expected[index]);
            }
        }

    } // namespace
} // namespace IntegralMotions::Config
