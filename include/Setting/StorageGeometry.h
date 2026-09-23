#pragma once

#include <cstddef>
namespace IntegralMotions::Config {

    inline constexpr std::byte ErasedValue = std::byte{0xFF};

    struct StorageGeometry {
        size_t capacity = 0;

        size_t readAlignment = 1;
        size_t writeAlignment = 1;
        size_t eraseBlockSize = 0; // Zero means erase is not required/supported.
        size_t atomicWriteSize = 1;

        std::byte erasedValue{ErasedValue};
    };

} // namespace IntegralMotions::Config