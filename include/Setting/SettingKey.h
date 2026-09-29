#pragma once

#include <cstdint>

namespace IntegralMotions::Config {

    enum class SettingScope : uint8_t {
        Board,
        Motor,
    };

    using SettingAddress = uint32_t;
    using SettingId = uint16_t;

    struct SettingKey {
        SettingId id = 0;
        SettingScope scope = SettingScope::Board;
        uint8_t instance = 0; // Max 64 instances

        friend bool operator==(const SettingKey&, const SettingKey&) = default;
    };

} // namespace IntegralMotions::Config