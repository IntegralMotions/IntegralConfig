#include "Setting/SettingDefinition.h"
#include "Setting/SettingsRegistry.h"
#include "Storage/MemoryStorageDevice.h"

#include <cstdint>
#include <gtest/gtest.h>

namespace IntegralMotions::Config {
    namespace {

        using namespace IntegralMotions::Storage;

        constexpr StorageRegion BankA{.offset = 0, .size = 64};
        constexpr StorageRegion BankB{.offset = 64, .size = 64};
        constexpr SettingKey FrequentKey{.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 0};
        constexpr SettingKey MemoryKey{.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 1};
        constexpr SettingKey RestartKey{.id = SettingId::Unknown, .scope = SettingScope::Motor, .instance = 2};

        struct ApplyRecorder {
            void record(const int32_t& value) {
                latestValue = value;
                ++callCount;
            }

            int32_t latestValue = 0;
            size_t callCount = 0;
        };

        TEST(SettingsRegistry, PersistsAndRestoresFrequentValues) {
            MemoryStorageDevice<128> storage;
            SettingsStore<2> store{storage, BankA, BankB};
            ASSERT_EQ(store.open(), SettingsStoreResult::Ok);

            const SettingDefinition<int32_t> definition{
                .key = FrequentKey,
                .defaultValue = 100,
                .persistencePolicy = PersistencePolicy::Frequent,
            };

            int32_t value = 0;
            SettingsRegistry<2> registry;
            registry.configureFrequentStore(store);
            ASSERT_EQ(registry.add(definition, value), SettingResult::Ok);
            EXPECT_EQ(value, 100);
            ASSERT_EQ(registry.set(FrequentKey, int32_t{250}), SettingResult::Ok);

            int32_t restoredValue = 0;
            SettingsRegistry<2> restoredRegistry;
            restoredRegistry.configureFrequentStore(store);
            ASSERT_EQ(restoredRegistry.add(definition, restoredValue), SettingResult::Ok);
            EXPECT_EQ(restoredValue, 250);
        }

        TEST(SettingsRegistry, KeepsDefaultForInvalidPersistedValue) {
            MemoryStorageDevice<128> storage;
            SettingsStore<2> store{storage, BankA, BankB};
            ASSERT_EQ(store.open(), SettingsStoreResult::Ok);
            ASSERT_EQ(store.store(FrequentKey, SettingValue{int32_t{200}}), SettingsStoreResult::Ok);

            const SettingDefinition<int32_t> definition{
                .key = FrequentKey,
                .defaultValue = 50,
                .limits = {.maximum = 100},
                .persistencePolicy = PersistencePolicy::Frequent,
            };
            int32_t value = 0;
            SettingsRegistry<2> registry;
            registry.configureFrequentStore(store);
            ASSERT_EQ(registry.add(definition, value), SettingResult::Ok);
            EXPECT_EQ(value, 50);
        }

        TEST(SettingsRegistry, PersistsLongTermValuesToLongTermStore) {
            MemoryStorageDevice<128> storage;
            SettingsStore<2> store{storage, BankA, BankB};
            ASSERT_EQ(store.open(), SettingsStoreResult::Ok);

            const SettingDefinition<int32_t> definition{
                .key = FrequentKey,
                .defaultValue = 100,
                .persistencePolicy = PersistencePolicy::LongTerm,
            };
            int32_t value = 0;
            SettingsRegistry<2> registry;
            registry.configureLongTermStore(store);
            ASSERT_EQ(registry.add(definition, value), SettingResult::Ok);
            ASSERT_EQ(registry.set(FrequentKey, int32_t{300}), SettingResult::Ok);

            SettingValue persistedValue{};
            ASSERT_EQ(store.load(FrequentKey, persistedValue), SettingsStoreResult::Ok);
            EXPECT_EQ(std::get<int32_t>(persistedValue), 300);
        }

        TEST(SettingsRegistry, ValidatesAndRejectsReadOnlyValues) {
            const SettingDefinition<int32_t> limitedDefinition{
                .key = MemoryKey,
                .defaultValue = 10,
                .limits = {.minimum = 0, .maximum = 20, .step = 5},
            };
            int32_t value = 0;
            SettingsRegistry<2> registry;
            ASSERT_EQ(registry.add(limitedDefinition, value), SettingResult::Ok);
            EXPECT_EQ(registry.set(MemoryKey, int32_t{12}), SettingResult::InvalidStep);
            EXPECT_EQ(registry.set(MemoryKey, int32_t{25}), SettingResult::AboveMaximum);
            EXPECT_EQ(registry.set(MemoryKey, int32_t{15}), SettingResult::Ok);
            EXPECT_EQ(value, 15);

            const SettingDefinition<int32_t> readonlyDefinition{
                .key = FrequentKey,
                .defaultValue = 5,
                .readonly = true,
            };
            ASSERT_EQ(registry.add(readonlyDefinition, value), SettingResult::Ok);
            EXPECT_EQ(registry.set(FrequentKey, int32_t{10}), SettingResult::ReadOnly);
        }

        TEST(SettingsRegistry, RequiresConfiguredStoreForPersistentSetting) {
            const SettingDefinition<int32_t> definition{
                .key = FrequentKey,
                .defaultValue = 100,
                .persistencePolicy = PersistencePolicy::Frequent,
            };
            int32_t value = 0;
            SettingsRegistry<2> registry;
            EXPECT_EQ(registry.add(definition, value), SettingResult::StorageError);
        }

        TEST(SettingsRegistry, ReportsValuesAndRejectsDuplicateKeys) {
            const SettingDefinition<int32_t> definition{
                .key = MemoryKey,
                .defaultValue = 10,
            };
            int32_t value = 0;
            SettingsRegistry<1> registry;
            ASSERT_EQ(registry.add(definition, value), SettingResult::Ok);
            ASSERT_EQ(registry.set(MemoryKey, int32_t{20}), SettingResult::Ok);

            const auto storedValue = registry.value(MemoryKey);
            ASSERT_TRUE(storedValue.has_value());
            EXPECT_EQ(std::get<int32_t>(*storedValue), 20);
            EXPECT_EQ(registry.value(FrequentKey), std::nullopt);
            EXPECT_EQ(registry.add(definition, value), SettingResult::DuplicateKey);
        }

        TEST(SettingsRegistry, RejectsInvalidDefinitionsAndTypeMismatches) {
            const SettingDefinition<int32_t> invalidDefinition{
                .key = MemoryKey,
                .defaultValue = 10,
                .limits = {.minimum = 20, .maximum = 10},
            };
            int32_t value = 0;
            SettingsRegistry<2> registry;
            EXPECT_EQ(registry.add(invalidDefinition, value), SettingResult::InvalidDefinition);

            SettingDefinition<int32_t> definition{
                .key = MemoryKey,
                .defaultValue = 10,
                .limits = {.optionCount = 2},
            };
            definition.limits.options[0].value = 10;
            definition.limits.options[1].value = 20;
            ASSERT_TRUE(definition.limits.options[0].id.assign("Low"));
            ASSERT_TRUE(definition.limits.options[1].id.assign("High"));
            ASSERT_EQ(registry.add(definition, value), SettingResult::Ok);
            EXPECT_EQ(registry.set(MemoryKey, uint16_t{10}), SettingResult::TypeMismatch);
            EXPECT_EQ(registry.set(MemoryKey, int32_t{15}), SettingResult::InvalidOption);
        }

        TEST(SettingsRegistry, RequiresLabelsForDefinedOptions) {
            SettingDefinition<int32_t> definition{
                .key = MemoryKey,
                .defaultValue = 10,
                .limits = {.optionCount = 1},
            };
            definition.limits.options[0].value = 10;

            int32_t value = 0;
            SettingsRegistry<1> registry;
            EXPECT_EQ(registry.add(definition, value), SettingResult::InvalidDefinition);
        }

        TEST(SettingsRegistry, AppliesImmediatelyAndAtStartupAccordingToPolicy) {
            ApplyRecorder immediateRecorder;
            ApplyRecorder disabledRecorder;
            ApplyRecorder restartRecorder;
            const SettingDefinition<int32_t> immediateDefinition{
                .key = FrequentKey,
                .defaultValue = 10,
                .apply = IntegralMotions::Functional::Delegate<void(const int32_t&)>::bind<&ApplyRecorder::record>(
                    immediateRecorder),
            };
            const SettingDefinition<int32_t> disabledDefinition{
                .key = MemoryKey,
                .defaultValue = 20,
                .applyPolicy = ApplyPolicy::WhenDisabled,
                .apply = IntegralMotions::Functional::Delegate<void(const int32_t&)>::bind<&ApplyRecorder::record>(
                    disabledRecorder),
            };
            const SettingDefinition<int32_t> restartDefinition{
                .key = RestartKey,
                .defaultValue = 30,
                .applyPolicy = ApplyPolicy::OnRestart,
                .apply = IntegralMotions::Functional::Delegate<void(const int32_t&)>::bind<&ApplyRecorder::record>(
                    restartRecorder),
            };
            int32_t immediateValue = 0;
            int32_t disabledValue = 0;
            int32_t restartValue = 0;
            SettingsRegistry<3> registry;

            ASSERT_EQ(registry.add(immediateDefinition, immediateValue), SettingResult::Ok);
            ASSERT_EQ(registry.add(disabledDefinition, disabledValue), SettingResult::Ok);
            ASSERT_EQ(registry.add(restartDefinition, restartValue), SettingResult::Ok);
            ASSERT_EQ(registry.set(FrequentKey, int32_t{11}), SettingResult::Ok);
            ASSERT_EQ(registry.set(MemoryKey, int32_t{21}), SettingResult::Ok);
            ASSERT_EQ(registry.set(RestartKey, int32_t{31}), SettingResult::Ok);

            EXPECT_EQ(immediateRecorder.latestValue, 11);
            EXPECT_EQ(immediateRecorder.callCount, 1);
            EXPECT_EQ(disabledRecorder.latestValue, 21);
            EXPECT_EQ(disabledRecorder.callCount, 1);
            EXPECT_EQ(restartRecorder.callCount, 0);

            registry.applyAtStartup();

            EXPECT_EQ(immediateRecorder.latestValue, 11);
            EXPECT_EQ(immediateRecorder.callCount, 2);
            EXPECT_EQ(disabledRecorder.latestValue, 21);
            EXPECT_EQ(disabledRecorder.callCount, 2);
            EXPECT_EQ(restartRecorder.latestValue, 31);
            EXPECT_EQ(restartRecorder.callCount, 1);
        }

    } // namespace
} // namespace IntegralMotions::Config
