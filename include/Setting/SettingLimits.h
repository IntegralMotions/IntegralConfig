#pragma once

#include "SupportedSettingType.h"

#include <array>
#include <cstdint>
#include <optional>

namespace IntegralMotions::Config {

    inline constexpr uint8_t MaxOptions = 16;

    template <SupportedSettingType T>
    struct SettingLimits {
        std::optional<T> minimum{};
        std::optional<T> maximum{};
        std::optional<T> step{};
        std::array<T, MaxOptions> options{};
        uint8_t optionCount = 0;
    };

} // namespace IntegralMotions::Config