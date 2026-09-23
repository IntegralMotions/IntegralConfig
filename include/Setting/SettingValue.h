#pragma once

#include <cstdint>
#include <variant>

namespace IntegralMotions::Config {

    using SettingValue = std::variant<std::monostate, bool, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                                      int64_t, uint64_t, float, double>;

} // namespace IntegralMotions::Config