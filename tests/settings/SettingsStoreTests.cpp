#include "Setting/MemoryStorageDevice.h"
#include "Setting/SettingsStore.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>

namespace IntegralMotions::Config {
    namespace {

        constexpr StorageRegion BankA{.offset = 0, .size = 64};
        constexpr StorageRegion BankB{.offset = 64, .size = 64};
        constexpr SettingKey SpeedKey{.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 0};
        constexpr SettingKey CurrentKey{.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 1};

        TEST(SettingsStore, StoresLoadsAndRebuildsIndexAtBoot) {
            MemoryStorageDevice<128> storage;
            SettingsStore<4> store{storage, BankA, BankB};
            ASSERT_EQ(store.open(), SettingsStoreResult::Ok);

            EXPECT_EQ(store.store(SpeedKey, SettingValue{int32_t{1000}}), SettingsStoreResult::Ok);
            EXPECT_EQ(store.store(CurrentKey, SettingValue{uint16_t{25}}), SettingsStoreResult::Ok);
            EXPECT_EQ(store.store(SpeedKey, SettingValue{int32_t{1200}}), SettingsStoreResult::Ok);

            SettingValue value{};
            ASSERT_EQ(store.load(SpeedKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(value), 1200);

            SettingsStore<4> reopened{storage, BankA, BankB};
            ASSERT_EQ(reopened.open(), SettingsStoreResult::Ok);
            ASSERT_EQ(reopened.load(SpeedKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(value), 1200);
            ASSERT_EQ(reopened.load(CurrentKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<uint16_t>(value), 25);
        }

        TEST(SettingsStore, CompactsLatestValuesIntoInactiveBank) {
            MemoryStorageDevice<128> storage;
            SettingsStore<4> store{storage, BankA, BankB};
            ASSERT_EQ(store.open(), SettingsStoreResult::Ok);
            ASSERT_EQ(store.store(SpeedKey, SettingValue{int32_t{1000}}), SettingsStoreResult::Ok);
            ASSERT_EQ(store.store(CurrentKey, SettingValue{int32_t{20}}), SettingsStoreResult::Ok);

            for (int32_t speed = 1100; speed <= 1500; speed += 100) {
                ASSERT_EQ(store.store(SpeedKey, SettingValue{speed}), SettingsStoreResult::Ok);
            }

            SettingsStore<4> reopened{storage, BankA, BankB};
            ASSERT_EQ(reopened.open(), SettingsStoreResult::Ok);
            SettingValue value{};
            ASSERT_EQ(reopened.load(SpeedKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(value), 1500);
            ASSERT_EQ(reopened.load(CurrentKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(value), 20);
        }

        TEST(SettingsStore, SkipsInterruptedRecordUsingStoredType) {
            MemoryStorageDevice<128> storage;
            SettingsStore<4> store{storage, BankA, BankB};
            ASSERT_EQ(store.open(), SettingsStoreResult::Ok);
            ASSERT_EQ(store.store(SpeedKey, SettingValue{int32_t{1000}}), SettingsStoreResult::Ok);

            // Bank header is 8 bytes and an I32 record is 11 bytes at byte alignment.
            constexpr size_t InterruptedOffset = 19;
            const std::array typeOnly{
                static_cast<std::byte>(static_cast<uint8_t>(SettingType::I32)),
            };
            ASSERT_EQ(storage.write(InterruptedOffset + StoredSettingCodec::magicSize, typeOnly), StorageResult::Ok);

            SettingsStore<4> reopened{storage, BankA, BankB};
            ASSERT_EQ(reopened.open(), SettingsStoreResult::Ok);

            SettingValue value{};
            ASSERT_EQ(reopened.load(SpeedKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(value), 1000);
            EXPECT_EQ(reopened.load(CurrentKey, value), SettingsStoreResult::NotFound);

            ASSERT_EQ(reopened.store(CurrentKey, SettingValue{int32_t{30}}), SettingsStoreResult::Ok);
            ASSERT_EQ(reopened.load(CurrentKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(value), 30);
        }

        TEST(SettingsStore, CompactsEraseBlockBackedStorage) {
            StorageGeometry geometry{
                .capacity = 128,
                .readAlignment = 1,
                .writeAlignment = 1,
                .eraseBlockSize = 64,
                .atomicWriteSize = 1,
                .erasedValue = std::byte{0xFF},
            };
            MemoryStorageDevice<128> storage{geometry};
            SettingsStore<2> store{storage, BankA, BankB};
            ASSERT_EQ(store.open(), SettingsStoreResult::Ok);

            for (int32_t speed = 1000; speed <= 1600; speed += 100) {
                ASSERT_EQ(store.store(SpeedKey, SettingValue{speed}), SettingsStoreResult::Ok);
            }

            SettingsStore<2> reopened{storage, BankA, BankB};
            ASSERT_EQ(reopened.open(), SettingsStoreResult::Ok);
            SettingValue value{};
            ASSERT_EQ(reopened.load(SpeedKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(value), 1600);
        }

    } // namespace
} // namespace IntegralMotions::Config
