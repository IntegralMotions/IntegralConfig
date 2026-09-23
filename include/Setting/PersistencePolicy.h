#pragma once

#include <cstdint>

namespace IntegralMotions::Config {

    enum class PersistencePolicy : uint8_t {
        Memory,
        Frequent,
        LongTerm,
    };

}