#pragma once

struct DefaultReadKeys {
    static constexpr const char* readDevice = "read.device";
    static constexpr const char* writeDevice = "write.device";
};

struct DefaultWriteKeys {
    static constexpr const char* writeSettings = "write.settings";
};

bool registerDefaultMessagePayloads();
