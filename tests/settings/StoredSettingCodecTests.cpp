#include "Setting/StoredSettingCodec.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>

namespace IntegralMotions::Config {
    namespace {

        constexpr SettingKey TestKey{
            .id = SettingId::Unknown,
            .scope = SettingScope::Motor,
            .instance = 63,
        };

        template <typename T>
        void roundTrip(const T& input) {
            std::array<std::byte, 32> buffer{};
            const auto encoded = StoredSettingCodec::encode(TestKey, SettingValue{input}, 4, std::byte{0xFF}, buffer);
            ASSERT_EQ(encoded.result, StoredSettingResult::Ok);

            SettingKey decodedKey{};
            SettingValue decodedValue{};
            ASSERT_EQ(StoredSettingCodec::decode(std::span<const std::byte>{buffer.data(), encoded.size}, decodedKey,
                                                 decodedValue),
                      StoredSettingResult::Ok);

            EXPECT_EQ(decodedKey, TestKey);
            EXPECT_EQ(std::get<T>(decodedValue), input);
        }

        TEST(StoredSettingCodec, RoundTripsAllSupportedValueSizes) {
            roundTrip(bool{true});
            roundTrip(int8_t{-12});
            roundTrip(uint16_t{50000});
            roundTrip(int32_t{-123456});
            roundTrip(uint64_t{9000000000000000000ULL});
            roundTrip(-3.5F);
            roundTrip(1.25e10);
        }

        TEST(StoredSettingCodec, AlignsRecordsToStorageWriteSize) {
            EXPECT_EQ(StoredSettingCodec::storedSize(SettingType::Bool, 4), 8U);
            EXPECT_EQ(StoredSettingCodec::storedSize(SettingType::U16, 4), 12U);
            EXPECT_EQ(StoredSettingCodec::storedSize(SettingType::I32, 4), 12U);
            EXPECT_EQ(StoredSettingCodec::storedSize(SettingType::F64, 4), 16U);
        }

        TEST(StoredSettingCodec, RejectsPayloadCorruption) {
            std::array<std::byte, 32> buffer{};
            const auto encoded =
                StoredSettingCodec::encode(TestKey, SettingValue{int32_t{42}}, 4, std::byte{0xFF}, buffer);
            ASSERT_EQ(encoded.result, StoredSettingResult::Ok);

            buffer[StoredSettingCodec::headerSize] ^= std::byte{0x01};

            SettingKey decodedKey{};
            SettingValue decodedValue{};
            EXPECT_EQ(StoredSettingCodec::decode(std::span<const std::byte>{buffer.data(), encoded.size}, decodedKey,
                                                 decodedValue),
                      StoredSettingResult::CorruptData);
        }

    } // namespace
} // namespace IntegralMotions::Config
