#pragma once

#include "IntegralCommunication/Communication.h"
#include "IntegralCommunication/SevenBitEncodedCommunication.h"
#include "mpack-object-base.h"
#include "mpack/mpack-writer.h"
#include <array>
#include <cstdint>
#include <mpack/mpack.h>

template <size_t TxSize, size_t RxSize> class ConfigurationController {
  public:
    ConfigurationController(const ConfigurationController&) = delete;
    ConfigurationController& operator=(const ConfigurationController&) = delete;

    static void init(Communication& comm);
    static ConfigurationController& get();

    bool write(const MPackObjectBase& object);
    void loop();

  private:
    ConfigurationController(Communication& comm);
    ~ConfigurationController() = default;

    SevenBitEncodedCommunication<TxSize, RxSize> _communication;
    mpack_reader_t _reader{};

    static ConfigurationController* _instance = nullptr;
};

template <size_t TxSize, size_t RxSize> void ConfigurationController<TxSize, RxSize>::init(Communication& comm) {
    if (_instance == nullptr) {
        _instance = new ConfigurationController(comm);
    }
}

template <size_t TxSize, size_t RxSize>
ConfigurationController<TxSize, RxSize>& ConfigurationController<TxSize, RxSize>::get() {
    return *_instance;
}

template <size_t TxSize, size_t RxSize>
ConfigurationController<TxSize, RxSize>::ConfigurationController(Communication& comm) : _communication(comm) {
    mpack_reader_init(&_reader, nullptr, 0);
}

template <size_t TxSize, size_t RxSize>
bool ConfigurationController<TxSize, RxSize>::write(const MPackObjectBase& object) {
    mpack_writer_t writer;
    std::array<uint8_t, TxSize> buffer;

    mpack_writer_init_buffer(&writer, buffer.data(), buffer.size());

    object.write(writer);
    mpack_error_t err = mpack_writer_destroy(&writer);
    if (err != mpack_ok) {
        return false;
    }

    size_t len = mpack_writer_buffer_used(&writer);
    return _communication.writeMessage(buffer.data(), len);
}
template <size_t TxSize, size_t RxSize> void ConfigurationController<TxSize, RxSize>::loop() {}
