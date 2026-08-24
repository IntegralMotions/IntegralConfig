#pragma once

struct DefaultReadKeys {
    static constexpr const char* readDevice = "read.device";
    static constexpr const char* writeDevice = "write.device";
};

bool registerDefaultMessagePayloads();
