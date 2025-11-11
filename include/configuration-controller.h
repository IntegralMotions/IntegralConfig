#pragma once

#include <mpack/mpack.h>
#include "communication.h"

class ConfigurationController {
public:
	static void init(Communication* comm);
	static ConfigurationController& get();

	void loop();

    size_t readBuffer(char* buffer, size_t count);

private:
	ConfigurationController(Communication* comm);
    ~ConfigurationController() = default;
    ConfigurationController(const ConfigurationController&) = delete;
    ConfigurationController& operator=(const ConfigurationController&) = delete;

    void readMpack();

private:
    Communication* _communication = nullptr;
    mpack_reader_t _reader{};
    uint8_t _readBuffer[255];

    static ConfigurationController* _instance;
};
