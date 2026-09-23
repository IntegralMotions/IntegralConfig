#pragma once

#include <cstdint>

namespace IntegralMotions::Config {

    enum class StoredSettingResult : uint8_t {
        Ok,
        BufferTooSmall,
        InvalidValue,
        CorruptData,
        UnsupportedVersion,
    };

}