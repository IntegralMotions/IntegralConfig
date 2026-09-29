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
        constexpr SettingKey FrequentKey{.id = 0, .scope = SettingScope::Motor, .instance = 0};
        constexpr SettingKey MemoryKey{.id = 0, .scope = SettingScope::Motor, .instance = 1};
        constexpr SettingKey RestartKey{.id = 0, .scope = SettingScope::Motor, .instance = 2};

        struct ApplyRecorder {
            void record(const int32_t& value) {
                latestValue = value;
                ++callCount;
            }

            int32_t latestValue = 0;
            size_t callCount = 0;
        };

        struct SnapshotRecorder {
            static bool record(void* context, const SettingSnapshot& setting) {
                auto& recorder = *static_cast<SnapshotRecorder*>(context);
                recorder.snapshot = setting;
                ++recorder.callCount;
                return true;
            }

            SettingSnapshot snapshot{};
            size_t callCount = 0;
        };

        template <typename Registry>
        concept CanAddMutableDefinition = requires(Registry& registry, SettingDefinition<int32_t>& definition,
                                                   int32_t& value) {
            registry.add(definition, value);
        };

        template <typename Registry>
        concept CanAddOwnedDefinition = requires(Registry& registry, int32_t& value) {
            registry.add(SettingDefinition<int32_t>{}, value);
        };

        static_assert(!CanAddMutableDefinition<SettingsRegistry<1>>);
        static_assert(CanAddOwnedDefinition<SettingsRegistry<1>>);

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

            const auto definition = [] {
                SettingDefinition<int32_t> value{
                    .key = MemoryKey,
                    .defaultValue = 10,
                    .limits = {.optionCount = 2},
                };
                value.limits.options[0].value = 10;
                value.limits.options[1].value = 20;
                EXPECT_TRUE(value.limits.options[0].id.assign("Low"));
                EXPECT_TRUE(value.limits.options[1].id.assign("High"));
                return value;
            }();
            ASSERT_EQ(registry.add(definition, value), SettingResult::Ok);
            EXPECT_EQ(registry.set(MemoryKey, uint16_t{10}), SettingResult::TypeMismatch);
            EXPECT_EQ(registry.set(MemoryKey, int32_t{15}), SettingResult::InvalidOption);
        }

        TEST(SettingsRegistry, RequiresIdsForDefinedOptions) {
            const auto definition = [] {
                SettingDefinition<int32_t> value{
                    .key = MemoryKey,
                    .defaultValue = 10,
                    .limits = {.optionCount = 1},
                };
                value.limits.options[0].value = 10;
                return value;
            }();

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

        TEST(SettingsRegistry, ReferencesLongLivedDefinitions) {
            static const SettingDefinition<int32_t> definition = [] {
                SettingDefinition<int32_t> value{
                    .key = MemoryKey,
                    .defaultValue = 10,
                    .limits = {.minimum = 0, .maximum = 20, .step = 5},
                };
                value.id.assign("Static");
                return value;
            }();
            SettingsRegistry<1> registry;
            int32_t value = 0;
            ASSERT_EQ(registry.add(definition, value), SettingResult::Ok);

            ASSERT_EQ(registry.set(MemoryKey, int32_t{15}), SettingResult::Ok);
            EXPECT_EQ(value, 15);

            SnapshotRecorder snapshotRecorder;
            ASSERT_EQ(registry.visit(&SnapshotRecorder::record, &snapshotRecorder), SettingResult::Ok);
            ASSERT_EQ(snapshotRecorder.callCount, 1);
            EXPECT_EQ(snapshotRecorder.snapshot.id, "Static");
            ASSERT_TRUE(snapshotRecorder.snapshot.maximum.has_value());
            EXPECT_EQ(std::get<int32_t>(*snapshotRecorder.snapshot.maximum), 20);
        }

        TEST(SettingsRegistry, OwnsMovedDefinitionsAndValues) {
            SettingsRegistry<2> registry;
            {
                SettingDefinition<int32_t> definition{
                    .key = MemoryKey,
                    .defaultValue = 10,
                    .limits = {.maximum = 20},
                };
                ASSERT_TRUE(definition.id.assign("Slave"));
                ASSERT_EQ(registry.add(std::move(definition), int32_t{15}), SettingResult::Ok);
            }

            const auto storedValue = registry.value(MemoryKey);
            ASSERT_TRUE(storedValue.has_value());
            EXPECT_EQ(std::get<int32_t>(*storedValue), 15);
            EXPECT_EQ(registry.set(MemoryKey, int32_t{25}), SettingResult::AboveMaximum);
            ASSERT_EQ(registry.set(MemoryKey, int32_t{20}), SettingResult::Ok);

            SnapshotRecorder snapshotRecorder;
            ASSERT_EQ(registry.visit(&SnapshotRecorder::record, &snapshotRecorder), SettingResult::Ok);
            EXPECT_EQ(snapshotRecorder.snapshot.id, "Slave");
        }

        TEST(SettingsRegistry, AllowsBoolOptionsButRejectsBoolBoundsAndRange) {
            const auto optionsDefinition = [] {
                SettingDefinition<bool> value{
                    .key = MemoryKey,
                    .defaultValue = false,
                    .limits = {.optionCount = 2},
                };
                value.limits.options[0].value = false;
                value.limits.options[1].value = true;
                EXPECT_TRUE(value.limits.options[0].id.assign("Off"));
                EXPECT_TRUE(value.limits.options[1].id.assign("On"));
                return value;
            }();

            bool value = false;
            SettingsRegistry<1> registry;
            ASSERT_EQ(registry.add(optionsDefinition, value), SettingResult::Ok);
            EXPECT_EQ(registry.set(MemoryKey, true), SettingResult::Ok);
            EXPECT_TRUE(value);

            const SettingDefinition<bool> minimumDefinition{
                .key = FrequentKey,
                .limits = {.minimum = false},
            };
            const SettingDefinition<bool> maximumDefinition{
                .key = FrequentKey,
                .limits = {.maximum = true},
            };
            const SettingDefinition<bool> stepDefinition{
                .key = FrequentKey,
                .limits = {.step = true},
            };
            const SettingDefinition<bool> rangeDefinition{
                .key = FrequentKey,
                .limits = {.isRange = true},
            };
            EXPECT_EQ(registry.add(minimumDefinition, value), SettingResult::InvalidDefinition);
            EXPECT_EQ(registry.add(maximumDefinition, value), SettingResult::InvalidDefinition);
            EXPECT_EQ(registry.add(stepDefinition, value), SettingResult::InvalidDefinition);
            EXPECT_EQ(registry.add(rangeDefinition, value), SettingResult::InvalidDefinition);
        }

        TEST(SettingsRegistry, SnapshotReportsIsRange) {
            const SettingDefinition<int32_t> definition{
                .key = MemoryKey,
                .defaultValue = 10,
                .limits = {.minimum = 0, .maximum = 20, .isRange = true},
            };
            int32_t value = 0;
            SettingsRegistry<1> registry;
            ASSERT_EQ(registry.add(definition, value), SettingResult::Ok);

            SnapshotRecorder snapshotRecorder;
            ASSERT_EQ(registry.visit(&SnapshotRecorder::record, &snapshotRecorder), SettingResult::Ok);
            ASSERT_EQ(snapshotRecorder.callCount, 1);
            EXPECT_TRUE(snapshotRecorder.snapshot.isRange);
        }

    } // namespace
} // namespace IntegralMotions::Config
