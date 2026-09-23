#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace IntegralMotions::Config {

    template <typename T>
    concept SupportedSettingType =
        std::same_as<std::remove_cvref_t<T>, bool> || std::same_as<std::remove_cvref_t<T>, int8_t> ||
        std::same_as<std::remove_cvref_t<T>, uint8_t> || std::same_as<std::remove_cvref_t<T>, int16_t> ||
        std::same_as<std::remove_cvref_t<T>, uint16_t> || std::same_as<std::remove_cvref_t<T>, int32_t> ||
        std::same_as<std::remove_cvref_t<T>, uint32_t> || std::same_as<std::remove_cvref_t<T>, int64_t> ||
        std::same_as<std::remove_cvref_t<T>, uint64_t> || std::same_as<std::remove_cvref_t<T>, float> ||
        std::same_as<std::remove_cvref_t<T>, double>;

}