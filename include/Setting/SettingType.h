#pragma once

#include "SupportedSettingType.h"

#include <cstdint>
#include <type_traits>

namespace IntegralMotions::Config {

    enum class SettingType : uint8_t {
        Bool,
        I8,
        U8,
        I16,
        U16,
        I32,
        U32,
        I64,
        U64,
        F32,
        F64,
    };

    template <SupportedSettingType T>
    constexpr SettingType settingTypeOf() {
        using ValueType = std::remove_cvref_t<T>;

        if constexpr (std::same_as<ValueType, bool>) {
            return SettingType::Bool;
        } else if constexpr (std::same_as<ValueType, int8_t>) {
            return SettingType::I8;
        } else if constexpr (std::same_as<ValueType, uint8_t>) {
            return SettingType::U8;
        } else if constexpr (std::same_as<ValueType, int16_t>) {
            return SettingType::I16;
        } else if constexpr (std::same_as<ValueType, uint16_t>) {
            return SettingType::U16;
        } else if constexpr (std::same_as<ValueType, int32_t>) {
            return SettingType::I32;
        } else if constexpr (std::same_as<ValueType, uint32_t>) {
            return SettingType::U32;
        } else if constexpr (std::same_as<ValueType, int64_t>) {
            return SettingType::I64;
        } else if constexpr (std::same_as<ValueType, uint64_t>) {
            return SettingType::U64;
        } else if constexpr (std::same_as<ValueType, float>) {
            return SettingType::F32;
        } else {
            return SettingType::F64;
        }
    }

} // namespace IntegralMotions::Config