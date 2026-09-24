#include "Setting/MemoryStorageDevice.h"
#include "Setting/SettingsStore.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <limits>

namespace IntegralMotions::Config {
    namespace {

        constexpr StorageRegion BankA{.offset = 0, .size = 64};
        constexpr StorageRegion BankB{.offset = 64, .size = 64};
        constexpr SettingKey SpeedKey{.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 0};
        constexpr SettingKey CurrentKey{.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 1};

        template <size_t Capacity>
        class FailingStorageDevice final : public StorageDevice {
          public:
            explicit FailingStorageDevice(MemoryStorageDevice<Capacity>& storage)
                : StorageDevice(storage.geometry()), _storage(storage) {}

            void failAfterMutation(size_t successfulMutations) {
                _mutationsUntilFailure = successfulMutations;
            }

            void clearFailure() {
                _mutationsUntilFailure = std::numeric_limits<size_t>::max();
            }

            StorageResult read(size_t offset, std::span<std::byte> destination) override {
                return _storage.read(offset, destination);
            }

            StorageResult write(size_t offset, std::span<const std::byte> source) override {
                return shouldFail() ? StorageResult::IoError : _storage.write(offset, source);
            }

            StorageResult erase(size_t offset, size_t length) override {
                return shouldFail() ? StorageResult::IoError : _storage.erase(offset, length);
            }

            StorageResult flush() override {
                return shouldFail() ? StorageResult::IoError : _storage.flush();
            }

          private:
            bool shouldFail() {
                if (_mutationsUntilFailure == std::numeric_limits<size_t>::max()) {
                    return false;
                }
                if (_mutationsUntilFailure == 0) {
                    return true;
                }
                --_mutationsUntilFailure;
                return false;
            }

            MemoryStorageDevice<Capacity>& _storage;
            size_t _mutationsUntilFailure = std::numeric_limits<size_t>::max();
        };

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

        TEST(SettingsStore, ReturnsNotOpenBeforeOpening) {
            MemoryStorageDevice<128> storage;
            SettingsStore<2> store{storage, BankA, BankB};
            SettingValue value{};

            EXPECT_EQ(store.store(SpeedKey, SettingValue{int32_t{1000}}), SettingsStoreResult::NotOpen);
            EXPECT_EQ(store.load(SpeedKey, value), SettingsStoreResult::NotOpen);
        }

        TEST(SettingsStore, RecoversFromCorruptedNewerBankHeader) {
            MemoryStorageDevice<128> storage;
            SettingsStore<1> store{storage, BankA, BankB};
            ASSERT_EQ(store.open(), SettingsStoreResult::Ok);
            for (int32_t speed = 1000; speed <= 1500; speed += 100) {
                ASSERT_EQ(store.store(SpeedKey, SettingValue{speed}), SettingsStoreResult::Ok);
            }

            const std::array corruptedMagic{std::byte{0}};
            ASSERT_EQ(storage.write(BankB.offset, corruptedMagic), StorageResult::Ok);

            SettingsStore<1> reopened{storage, BankA, BankB};
            ASSERT_EQ(reopened.open(), SettingsStoreResult::Ok);
            SettingValue value{};
            ASSERT_EQ(reopened.load(SpeedKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(value), 1400);
        }

        TEST(SettingsStore, SupportsAlignedStorageGeometry) {
            StorageGeometry geometry{
                .capacity = 128,
                .readAlignment = 4,
                .writeAlignment = 4,
                .eraseBlockSize = 0,
                .atomicWriteSize = 4,
                .erasedValue = std::byte{0xFF},
            };
            MemoryStorageDevice<128> storage{geometry};
            SettingsStore<2> store{storage, BankA, BankB};
            ASSERT_EQ(store.open(), SettingsStoreResult::Ok);
            ASSERT_EQ(store.store(SpeedKey, SettingValue{int32_t{1200}}), SettingsStoreResult::Ok);

            SettingValue value{};
            ASSERT_EQ(store.load(SpeedKey, value), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(value), 1200);
        }

        TEST(SettingsStore, RejectsOverlappingBankRegions) {
            MemoryStorageDevice<128> storage;
            SettingsStore<1> store{storage, {.offset = 0, .size = 64}, {.offset = 32, .size = 64}};
            EXPECT_EQ(store.open(), SettingsStoreResult::InvalidArgument);
        }

        TEST(SettingsStore, KeepsActiveBankAfterCompactionWriteFailures) {
            for (size_t failurePoint = 0; failurePoint < 6; ++failurePoint) {
                MemoryStorageDevice<128> backingStorage;
                FailingStorageDevice<128> storage{backingStorage};
                SettingsStore<1> store{storage, BankA, BankB};
                ASSERT_EQ(store.open(), SettingsStoreResult::Ok);
                for (int32_t speed = 1000; speed <= 1400; speed += 100) {
                    ASSERT_EQ(store.store(SpeedKey, SettingValue{speed}), SettingsStoreResult::Ok);
                }

                storage.failAfterMutation(failurePoint);
                EXPECT_EQ(store.store(SpeedKey, SettingValue{int32_t{1500}}), SettingsStoreResult::StorageError);
                storage.clearFailure();

                SettingsStore<1> reopened{storage, BankA, BankB};
                ASSERT_EQ(reopened.open(), SettingsStoreResult::Ok);
                SettingValue value{};
                ASSERT_EQ(reopened.load(SpeedKey, value), SettingsStoreResult::Ok);
                EXPECT_EQ(std::get<int32_t>(value), 1400) << "failure point " << failurePoint;
            }
        }

    } // namespace
} // namespace IntegralMotions::Config
