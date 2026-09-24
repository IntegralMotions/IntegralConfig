#pragma once

#include "../FixedString.h"
#include "SupportedSettingType.h"

#include <cstdint>

namespace IntegralMotions::Config {

    inline constexpr uint8_t LabelMaxLength = 64;

    template <SupportedSettingType T>
    struct SettingOption {
        T value{};
        FixedString<LabelMaxLength> label{};
    };

} // namespace IntegralMotions::Config
