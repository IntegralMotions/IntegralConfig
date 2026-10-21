#pragma once

#include "StoredSettingResult.h"
#include <cstddef>

namespace IntegralMotions::Config {

    struct StoredSettingEncodeResult {
        StoredSettingResult result = StoredSettingResult::Ok;
        size_t size = 0;
    };

} // namespace IntegralMotions::Config