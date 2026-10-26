#pragma once

#include <cstdint>

namespace IntegralMotions::Config {

    enum class SettingsStoreResult : uint8_t {
        Ok,
        Blank,
        Recovered,
        NotFound,
        NoSpace,
        CorruptData,
        UnsupportedVersion,
        InvalidArgument,
        NotOpen,
        StorageError,
    };

}
