#include "configuration/configuration-controller.h"

ConfigurationController::ConfigurationController(Communication* comm) {
	_communication = comm;
	mpack_reader_init(&_reader, (char*)_readBuffer, sizeof(_readBuffer), 0);
	mpack_reader_set_context(&_reader, this);
	mpack_reader_set_fill(
	    &_reader,
	    +[](mpack_reader_t* r, char* buffer, size_t count) -> size_t {
	        auto* self = static_cast<ConfigurationController*>(mpack_reader_context(r));
	        size_t n = self->readBuffer(buffer, count);   // instance method
	        if (n == 0) mpack_reader_flag_error(r, mpack_error_io);
	        return n;
	    }
	);
}

void ConfigurationController::init(Communication* comm) {
    if (!_instance)
    {
        _instance = new ConfigurationController(comm);
    }
}

ConfigurationController& ConfigurationController::get() {
    return *_instance;
}

void ConfigurationController::loop() {
    readMpack();
}

void ConfigurationController::readMpack() {

}

size_t ConfigurationController::readBuffer(char* buffer, size_t count) {
	return 0;
}
