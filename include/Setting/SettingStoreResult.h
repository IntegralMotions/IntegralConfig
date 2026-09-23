#pragma once

#include <cstdint>

namespace IntegralMotions::Config {

    enum class SettingsStoreResult : uint8_t {
        Ok,
        NotFound,
        NoSpace,
        CorruptData,
        UnsupportedVersion,
        InvalidArgument,
        NotOpen,
        StorageError,
    };

}
