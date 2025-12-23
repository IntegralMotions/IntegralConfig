#pragma once

struct DefaultReadKeys {
    static constexpr const char *ReadDevice = "read.device";
    static constexpr const char *WriteDevice = "write.device";
};

bool registerDefaultMessagePayloads();
