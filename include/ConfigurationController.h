#pragma once

#include "IntegralCommunication/Communication.h"
#include "IntegralCommunication/SevenBitEncodedCommunication.h"
#include "MPackObjectBase.h"
#include "Messages.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mpack/mpack.h>

template <size_t TxSize, size_t RxSize> class ConfigurationController {
  public:
    using ReceiveCallback = std::function<void(const Message &)>;

    ConfigurationController(const ConfigurationController &) = delete;
    ConfigurationController &operator=(const ConfigurationController &) = delete;

    static void init(Communication &communication);
    static ConfigurationController &get();

    void setOnReceived(ReceiveCallback callback);

    bool write(const MPackObjectBase &object);
    void loop();

  private:
    ConfigurationController(Communication &comm);
    ~ConfigurationController() = default;

    SevenBitEncodedCommunication<TxSize, RxSize> _communication;
    mpack_reader_t _reader{};
    ReceiveCallback _onReceived;

    static ConfigurationController *_instance;
};

// static member definition (important in a header for templates)
template <size_t TxSize, size_t RxSize>
ConfigurationController<TxSize, RxSize> *ConfigurationController<TxSize, RxSize>::_instance = nullptr;

template <size_t TxSize, size_t RxSize> void ConfigurationController<TxSize, RxSize>::init(Communication &comm) {
    if (_instance == nullptr) {
        _instance = new ConfigurationController(comm);
    }
}

template <size_t TxSize, size_t RxSize>
ConfigurationController<TxSize, RxSize> &ConfigurationController<TxSize, RxSize>::get() {
    return *_instance;
}

template <size_t TxSize, size_t RxSize>
ConfigurationController<TxSize, RxSize>::ConfigurationController(Communication &communication)
    : _communication(communication) {}

template <size_t TxSize, size_t RxSize>
void ConfigurationController<TxSize, RxSize>::setOnReceived(ReceiveCallback callback) {
    _onReceived = std::move(callback);
}

template <size_t TxSize, size_t RxSize>
bool ConfigurationController<TxSize, RxSize>::write(const MPackObjectBase &object) {
    mpack_writer_t writer;
    std::array<uint8_t, TxSize> buffer;

    mpack_writer_init(&writer, reinterpret_cast<char *>(buffer.data()), buffer.size());

    object.write(writer);
    size_t len = mpack_writer_buffer_used(&writer);
    mpack_error_t err = mpack_writer_destroy(&writer);
    if (err != mpack_ok) {
        return false;
    }

    return _communication.writeMessage(buffer.data(), len);
}

template <size_t TxSize, size_t RxSize> void ConfigurationController<TxSize, RxSize>::loop() {
    std::array<uint8_t, RxSize> messageBytes;
    size_t messageLength;

    if (!_communication.readMessage(messageBytes.data(), messageBytes.size(), messageLength)) {
        return;
    }

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, reinterpret_cast<const char *>(messageBytes.data()), messageLength);

    Message message;
    message.read(reader);

    mpack_error_t err = mpack_reader_destroy(&reader);
    if (err != mpack_ok) {
        return;
    }

    if (_onReceived) {
        _onReceived(message);
    }
}
