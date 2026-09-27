#pragma once

#include "SettingDefinition.h"
#include "SettingResult.h"
#include "SettingType.h"
#include "SettingValue.h"
#include "SettingsStore.h"
#include "SupportedSettingType.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <type_traits>

namespace IntegralMotions::Config {

    template <size_t Capacity>
    class SettingsRegistry {
      public:
        using VisitCallback = bool (*)(void* context, const SettingSnapshot& setting);

        template <SupportedSettingType T>
        SettingResult add(const SettingDefinition<T>& definition, T& value);

        template <SupportedSettingType T>
        SettingResult set(const SettingKey& key, const T& value);

        [[nodiscard]] std::optional<SettingValue> value(const SettingKey& key) const;

        [[nodiscard]] size_t size() const;
        SettingResult visit(VisitCallback callback, void* context) const;

        void configureLongTermStore(SettingsStore<Capacity>& store);
        void configureFrequentStore(SettingsStore<Capacity>& store);
        void applyAtStartup();

      private:
        struct Entry {
            bool registered = false;

            SettingKey key{};
            SettingType type{};
            PersistencePolicy persistencePolicy = PersistencePolicy::Memory;
            ApplyPolicy applyPolicy = ApplyPolicy::Immediate;
            bool readonly = false;

            const void* definition = nullptr;
            void* context = nullptr;

            SettingResult (*validate)(const void* definition, const SettingValue& candidate) = nullptr;
            SettingResult (*read)(void* context, SettingValue& output) = nullptr;
            SettingResult (*write)(void* context, const SettingValue& candidate) = nullptr;
            void (*apply)(const void* definition, const SettingValue& value) = nullptr;
            void (*snapshot)(const void* definition, const SettingValue& value, SettingSnapshot& output) = nullptr;
        };

        template <SupportedSettingType T>
        static SettingResult validateLocal(const void* definition, const SettingValue& candidate);

        template <SupportedSettingType T>
        static SettingResult validateDefinition(const SettingDefinition<T>& definition);

        template <SupportedSettingType T>
        static SettingResult readLocal(void* context, SettingValue& output);

        template <SupportedSettingType T>
        static SettingResult writeLocal(void* context, const SettingValue& input);

        template <SupportedSettingType T>
        static void applyDefinition(const void* definition, const SettingValue& value);

        template <SupportedSettingType T>
        static void snapshotDefinition(const void* definition, const SettingValue& value, SettingSnapshot& output);

        Entry* find(const SettingKey& key);
        const Entry* find(const SettingKey& key) const;

        SettingsStore<Capacity>* storeFor(PersistencePolicy policy);
        const SettingsStore<Capacity>* storeFor(PersistencePolicy policy) const;

        std::array<Entry, Capacity> _entries{};
        size_t _size = 0;

        SettingsStore<Capacity>* _longTermStore = nullptr;
        SettingsStore<Capacity>* _frequentStore = nullptr;
    };

    template <size_t Capacity>
    void SettingsRegistry<Capacity>::configureLongTermStore(SettingsStore<Capacity>& store) {
        _longTermStore = &store;
    }

    template <size_t Capacity>
    void SettingsRegistry<Capacity>::configureFrequentStore(SettingsStore<Capacity>& store) {
        _frequentStore = &store;
    }

    template <size_t Capacity>
    SettingsStore<Capacity>* SettingsRegistry<Capacity>::storeFor(PersistencePolicy policy) {
        switch (policy) {
        case PersistencePolicy::Memory:
            return nullptr;
        case PersistencePolicy::Frequent:
            return _frequentStore;
        case PersistencePolicy::LongTerm:
            return _longTermStore;
        }
        return nullptr;
    }

    template <size_t Capacity>
    const SettingsStore<Capacity>* SettingsRegistry<Capacity>::storeFor(PersistencePolicy policy) const {
        return const_cast<SettingsRegistry<Capacity>*>(this)->storeFor(policy);
    }

    template <size_t Capacity>
    template <SupportedSettingType T>
    SettingResult SettingsRegistry<Capacity>::validateLocal(const void* definition, const SettingValue& candidate) {
        const auto* settingDefinition = static_cast<const SettingDefinition<T>*>(definition);
        const auto* value = std::get_if<T>(&candidate);
        if (value == nullptr) {
            return SettingResult::TypeMismatch;
        }

        if constexpr (std::floating_point<T>) {
            if (!std::isfinite(*value)) {
                return SettingResult::ValidationFailed;
            }
        }
        const auto& limits = settingDefinition->limits;
        if (limits.minimum.has_value() && *value < *limits.minimum) {
            return SettingResult::BelowMinimum;
        }
        if (limits.maximum.has_value() && *value > *limits.maximum) {
            return SettingResult::AboveMaximum;
        }
        if (limits.optionCount != 0) {
            bool found = false;
            for (uint8_t i = 0; i < limits.optionCount; ++i) {
                if (*value == limits.options[i].value) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return SettingResult::InvalidOption;
            }
        }
        if (limits.step.has_value()) {
            if constexpr (std::same_as<T, bool>) {
                return SettingResult::InvalidStep;
            } else {
                const T origin = limits.minimum.value_or(T{});
                if constexpr (std::floating_point<T>) {
                    const T remainder = std::fmod(*value - origin, *limits.step);
                    const T tolerance = std::numeric_limits<T>::epsilon() *
                                        std::max({T{1}, std::abs(*value), std::abs(origin), std::abs(*limits.step)});
                    if (std::abs(remainder) > tolerance && std::abs(remainder - *limits.step) > tolerance) {
                        return SettingResult::InvalidStep;
                    }
                } else {
                    using Unsigned = std::make_unsigned_t<T>;
                    const auto difference = static_cast<Unsigned>(*value) - static_cast<Unsigned>(origin);
                    if (difference % static_cast<Unsigned>(*limits.step) != 0) {
                        return SettingResult::InvalidStep;
                    }
                }
            }
        }
        return SettingResult::Ok;
    }

    template <size_t Capacity>
    template <SupportedSettingType T>
    SettingResult SettingsRegistry<Capacity>::validateDefinition(const SettingDefinition<T>& definition) {
        const auto& limits = definition.limits;
        if (limits.optionCount > MaxOptions ||
            (limits.minimum.has_value() && limits.maximum.has_value() && *limits.minimum > *limits.maximum)) {
            return SettingResult::InvalidDefinition;
        }
        if constexpr (std::floating_point<T>) {
            if ((limits.minimum.has_value() && !std::isfinite(*limits.minimum)) ||
                (limits.maximum.has_value() && !std::isfinite(*limits.maximum)) ||
                (limits.step.has_value() && !std::isfinite(*limits.step))) {
                return SettingResult::InvalidDefinition;
            }
        }
        if (limits.step.has_value()) {
            if constexpr (std::same_as<T, bool>) {
                return SettingResult::InvalidDefinition;
            } else if (*limits.step <= T{}) {
                return SettingResult::InvalidDefinition;
            }
        }

        if (validateLocal<T>(&definition, SettingValue{definition.defaultValue}) != SettingResult::Ok) {
            return SettingResult::InvalidDefinition;
        }
        for (uint8_t i = 0; i < limits.optionCount; ++i) {
            if (limits.options[i].id.empty() ||
                validateLocal<T>(&definition, SettingValue{limits.options[i].value}) != SettingResult::Ok) {
                return SettingResult::InvalidDefinition;
            }
            for (uint8_t j = 0; j < i; ++j) {
                if (limits.options[i].value == limits.options[j].value) {
                    return SettingResult::InvalidDefinition;
                }
            }
        }
        return SettingResult::Ok;
    }

    template <size_t Capacity>
    template <SupportedSettingType T>
    SettingResult SettingsRegistry<Capacity>::readLocal(void* context, SettingValue& output) {
        output = *static_cast<T*>(context);
        return SettingResult::Ok;
    }

    template <size_t Capacity>
    template <SupportedSettingType T>
    SettingResult SettingsRegistry<Capacity>::writeLocal(void* context, const SettingValue& input) {
        const auto* value = std::get_if<T>(&input);
        if (value == nullptr) {
            return SettingResult::TypeMismatch;
        }

        *static_cast<T*>(context) = *value;
        return SettingResult::Ok;
    }

    template <size_t Capacity>
    template <SupportedSettingType T>
    void SettingsRegistry<Capacity>::applyDefinition(const void* definition, const SettingValue& value) {
        const auto* settingDefinition = static_cast<const SettingDefinition<T>*>(definition);
        const auto* typedValue = std::get_if<T>(&value);
        if (settingDefinition->apply && typedValue != nullptr) {
            settingDefinition->apply(*typedValue);
        }
    }

    template <size_t Capacity>
    template <SupportedSettingType T>
    void SettingsRegistry<Capacity>::snapshotDefinition(const void* definition, const SettingValue& value,
                                                        SettingSnapshot& output) {
        const auto* settingDefinition = static_cast<const SettingDefinition<T>*>(definition);
        const auto& limits = settingDefinition->limits;
        output.key = settingDefinition->key;
        output.type = settingTypeOf<T>();
        output.moduleId = settingDefinition->moduleId.view();
        output.groupId = settingDefinition->groupId.view();
        output.id = settingDefinition->id.view();
        output.unit = settingDefinition->unit.view();
        output.value = value;
        if (limits.minimum.has_value()) {
            output.minimum = SettingValue{*limits.minimum};
        }
        if (limits.maximum.has_value()) {
            output.maximum = SettingValue{*limits.maximum};
        }
        if (limits.step.has_value()) {
            output.step = SettingValue{*limits.step};
        }
        output.optionCount = limits.optionCount;
        output.readonly = settingDefinition->readonly;
        for (uint8_t i = 0; i < limits.optionCount; ++i) {
            output.options[i] = {.value = SettingValue{limits.options[i].value}, .id = limits.options[i].id.view()};
        }
    }

    template <size_t Capacity>
    const typename SettingsRegistry<Capacity>::Entry* SettingsRegistry<Capacity>::find(const SettingKey& key) const {
        for (size_t i = 0; i < _size; ++i) {
            if (_entries[i].key == key) {
                return &_entries[i];
            }
        }

        return nullptr;
    }

    template <size_t Capacity>
    typename SettingsRegistry<Capacity>::Entry* SettingsRegistry<Capacity>::find(const SettingKey& key) {
        const auto* entry = static_cast<const SettingsRegistry<Capacity>&>(*this).find(key);
        return const_cast<Entry*>(entry);
    }

    template <size_t Capacity>
    template <SupportedSettingType T>
    SettingResult SettingsRegistry<Capacity>::add(const SettingDefinition<T>& definition, T& value) {
        const auto definitionResult = validateDefinition(definition);
        if (definitionResult != SettingResult::Ok) {
            return definitionResult;
        }
        if (find(definition.key) != nullptr) {
            return SettingResult::DuplicateKey;
        }

        if (_size == Capacity) {
            return SettingResult::CapacityExceeded;
        }

        Entry entry{};
        entry.registered = true;
        entry.key = definition.key;
        entry.type = settingTypeOf<T>();
        entry.persistencePolicy = definition.persistencePolicy;
        entry.applyPolicy = definition.applyPolicy;
        entry.readonly = definition.readonly;
        entry.definition = &definition;
        entry.context = &value;
        entry.validate = &validateLocal<T>;
        entry.read = &readLocal<T>;
        entry.write = &writeLocal<T>;
        entry.apply = &applyDefinition<T>;
        entry.snapshot = &snapshotDefinition<T>;

        value = definition.defaultValue;
        if (entry.persistencePolicy != PersistencePolicy::Memory) {
            auto* store = storeFor(entry.persistencePolicy);
            if (store == nullptr) {
                return SettingResult::StorageError;
            }
            SettingValue persistedValue{};
            const auto result = store->load(entry.key, persistedValue);
            if (result == SettingsStoreResult::StorageError) {
                return SettingResult::StorageError;
            }
            if (result == SettingsStoreResult::Ok &&
                entry.validate(entry.definition, persistedValue) == SettingResult::Ok) {
                const auto writeResult = entry.write(entry.context, persistedValue);
                if (writeResult != SettingResult::Ok) {
                    return writeResult;
                }
            }
        }

        _entries[_size++] = entry;

        return SettingResult::Ok;
    }

    template <size_t Capacity>
    template <SupportedSettingType T>
    SettingResult SettingsRegistry<Capacity>::set(const SettingKey& key, const T& value) {
        auto* entry = find(key);
        if (entry == nullptr) {
            return SettingResult::NotFound;
        }
        if (entry->readonly) {
            return SettingResult::ReadOnly;
        }

        const SettingValue candidate{value};
        const auto validationResult = entry->validate(entry->definition, candidate);
        if (validationResult != SettingResult::Ok) {
            return validationResult;
        }
        if (entry->persistencePolicy != PersistencePolicy::Memory) {
            auto* store = storeFor(entry->persistencePolicy);
            if (store == nullptr || store->store(key, candidate) != SettingsStoreResult::Ok) {
                return SettingResult::StorageError;
            }
        }

        const auto writeResult = entry->write(entry->context, candidate);
        if (writeResult == SettingResult::Ok) {
            if (entry->applyPolicy != ApplyPolicy::OnRestart) {
                entry->apply(entry->definition, candidate);
            }
        }
        return writeResult;
    }

    template <size_t Capacity>
    void SettingsRegistry<Capacity>::applyAtStartup() {
        for (size_t i = 0; i < _size; ++i) {
            auto& entry = _entries[i];
            SettingValue value{};
            if (entry.read(entry.context, value) == SettingResult::Ok) {
                entry.apply(entry.definition, value);
            }
        }
    }

    template <size_t Capacity>
    std::optional<SettingValue> SettingsRegistry<Capacity>::value(const SettingKey& key) const {
        const auto* entry = find(key);
        if (entry == nullptr) {
            return std::nullopt;
        }

        SettingValue output;
        if (entry->read(entry->context, output) != SettingResult::Ok) {
            return std::nullopt;
        }

        return output;
    }

    template <size_t Capacity>
    size_t SettingsRegistry<Capacity>::size() const {
        return _size;
    }

    template <size_t Capacity>
    SettingResult SettingsRegistry<Capacity>::visit(VisitCallback callback, void* context) const {
        if (callback == nullptr) {
            return SettingResult::InvalidDefinition;
        }

        for (size_t i = 0; i < _size; ++i) {
            const auto& entry = _entries[i];
            SettingValue value{};
            const auto readResult = entry.read(entry.context, value);
            if (readResult != SettingResult::Ok) {
                return readResult;
            }

            SettingSnapshot snapshot{};
            entry.snapshot(entry.definition, value, snapshot);
            if (!callback(context, snapshot)) {
                break;
            }
        }
        return SettingResult::Ok;
    }
} // namespace IntegralMotions::Config
