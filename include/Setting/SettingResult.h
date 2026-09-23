#pragma once

#include <cstdint>

namespace IntegralMotions::Config {

    enum class SettingResult : uint8_t {
        Ok = 0,

        NotFound = 1,
        DuplicateKey = 2,
        CapacityExceeded = 3,

        ReadOnly = 4,
        InvalidAccess = 5,
        TypeMismatch = 6,

        InvalidDefinition = 7,
        MetadataTooLong = 8,

        ValidationFailed = 9,
        BelowMinimum = 10,
        AboveMaximum = 11,
        InvalidStep = 12,
        InvalidOption = 13,

        ReentrantOperation = 14,
    };

} // namespace IntegralMotions::Config