#include "Configuration.h"
#include "DefaultMessagePayloads.h"
#include "DeviceResponseSerializer.h"
#include "Messages.h"
#include "SuccessResponseSerializer.h"

#include <array>
#include <cstdint>
#include <gtest/gtest.h>
#include <mpack/mpack.h>

namespace IntegralMotions::Config {
    namespace {

        constexpr SettingKey EnableKey{.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 0};
        constexpr SettingKey SpeedKey{.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 1};

        TEST(DeviceResponseSerializer, WritesRegistrySettingsAsReadDeviceResponse) {
            ASSERT_TRUE(registerDefaultMessagePayloads());

            const SettingDefinition<bool> enableDefinition{
                .key = EnableKey,
                .moduleId = "motor",
                .groupId = "main",
                .id = "enable",
                .defaultValue = true,
            };
            SettingDefinition<int32_t> speedDefinition{
                .key = SpeedKey,
                .moduleId = "motor",
                .groupId = "main",
                .id = "speed",
                .unit = "rpm",
                .defaultValue = 500,
                .limits = {.minimum = 0, .maximum = 2000, .step = 100, .optionCount = 2},
            };
            ASSERT_TRUE(speedDefinition.limits.options[0].id.assign("low-speed"));
            speedDefinition.limits.options[0].value = 500;
            ASSERT_TRUE(speedDefinition.limits.options[1].id.assign("high-speed"));
            speedDefinition.limits.options[1].value = 1500;

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
            EXPECT_STREQ(group->settings[1]->type, "int");
            const auto* speedValue = static_cast<const NumberSetting<int32_t>*>(group->settings[1]->value);
            EXPECT_EQ(speedValue->value, 1500);
            ASSERT_TRUE(speedValue->limits.minimum.has_value());
            EXPECT_EQ(*speedValue->limits.minimum, 0);
            ASSERT_TRUE(speedValue->limits.maximum.has_value());
            EXPECT_EQ(*speedValue->limits.maximum, 2000);
            ASSERT_TRUE(speedValue->limits.step.has_value());
            EXPECT_EQ(*speedValue->limits.step, 100);
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

    } // namespace
} // namespace IntegralMotions::Config
