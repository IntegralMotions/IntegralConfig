#pragma once

#include <cstdint>

namespace IntegralMotions::Config {

    enum class StorageResult : uint8_t {
        Ok,
        OutOfRange,
        Unaligned,
        InvalidArgument,
        IoError,
        Busy,
        Unsupported,
    };

}