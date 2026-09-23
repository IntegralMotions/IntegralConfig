#pragma once

#include "SettingId.h"
#include <cstdint>

namespace IntegralMotions::Config {

    enum class SettingScope : uint8_t {
        Board,
        Motor,
    };

    struct SettingKey {
        SettingId id = SettingId::Unknown;
        SettingScope scope = SettingScope::Board;
        uint8_t instance = 0; // Max 64 instances

        friend bool operator==(const SettingKey&, const SettingKey&) = default;
    };

} // namespace IntegralMotions::Config