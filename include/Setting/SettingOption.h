#pragma once

#include "Containers/FixedString.h"
#include "SupportedSettingType.h"

#include <cstdint>

namespace IntegralMotions::Config {

    inline constexpr uint8_t IdMaxLength = 64;

    template <SupportedSettingType T>
    struct SettingOption {
        T value{};
        IntegralMotions::Containers::FixedString<IdMaxLength> id;
    };

} // namespace IntegralMotions::Config
