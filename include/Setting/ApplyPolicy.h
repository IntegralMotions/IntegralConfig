#pragma once

#include <cstdint>

namespace IntegralMotions::Config {

    enum class ApplyPolicy : uint8_t {
        Immediate,
        WhenDisabled,
        OnRestart,
    };

}