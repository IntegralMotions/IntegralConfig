#pragma once

#include "SettingDefinition.h"
#include "SettingResult.h"
#include "SettingType.h"
#include "SettingValue.h"
#include "StorageDevice.h"
#include "SupportedSettingType.h"
#include <cstddef>

namespace IntegralMotions::Config {

    template <size_t Capacity>
    class SettingsRegistry {
      public:
        template <SupportedSettingType T>
        SettingResult add(const SettingDefinition<T>& definition, T& value);

        template <SupportedSettingType T>
        SettingResult set(const SettingKey& key, const T& value);

        [[nodiscard]] std::optional<SettingValue> value(const SettingKey& key) const;

        [[nodiscard]] size_t size() const;

        void configureLongTermStorage(StorageDevice& storageDevice);
        void configureFrequentStorage(StorageDevice& storageDevice);

      private:
        struct Entry {
            bool registered = false;

            SettingKey key{};
            SettingType type{};

            const void* definition = nullptr;
            void* context = nullptr;

            SettingResult (*validate)(const void* definition, const SettingValue& candidate) = nullptr;
            SettingResult (*read)(void* context, SettingValue& output) = nullptr;
            SettingResult (*write)(void* context, const SettingValue& candidate) = nullptr;

            bool dirty = true;
        };

        template <SupportedSettingType T>
        static SettingResult readLocal(void* context, SettingValue& output);

        template <SupportedSettingType T>
        static SettingResult writeLocal(void* context, const SettingValue& input);

        Entry* find(const SettingKey& key);
        const Entry* find(const SettingKey& key) const;

        std::array<Entry, Capacity> _entries{};
        size_t _size = 0;

        StorageDevice* _longTermStorage = nullptr;
        StorageDevice* _frequentStorage = nullptr;
    };

    template <size_t Capacity>
    void SettingsRegistry<Capacity>::configureLongTermStorage(StorageDevice& storageDevice) {
        _longTermStorage = &storageDevice;
    }

    template <size_t Capacity>
    void SettingsRegistry<Capacity>::configureFrequentStorage(StorageDevice& storageDevice) {
        _frequentStorage = &storageDevice;
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
        if (find(definition.key) != nullptr) {
            return SettingResult::DuplicateKey;
        }

        if (_size == Capacity) {
            return SettingResult::CapacityExceeded;
        }

        value = definition.defaultValue;

        auto& entry = _entries[_size++];
        entry.registered = true;
        entry.key = definition.key;
        entry.type = settingTypeOf<T>();
        entry.definition = &definition;
        entry.context = &value;
        entry.read = &readLocal<T>;
        entry.write = &writeLocal<T>;

        return SettingResult::Ok;
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
} // namespace IntegralMotions::Config
