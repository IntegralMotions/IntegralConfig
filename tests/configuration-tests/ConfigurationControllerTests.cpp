#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <gtest/gtest.h>

#include "Configuration.h"
#include "ConfigurationController.h"
#include "DefaultMessagePayloads.h"
#include "IntegralCommunication/CobsEncodedCommunication.h"
#include "IntegralCommunication/Communication.h"
#include "Messages.h"

namespace {
    class TestCommunication : public Communication {
      public:
        std::vector<uint8_t> outgoingBytes;
        std::vector<uint8_t> incomingBytes;

        void injectIncomingBytes(const uint8_t* data, size_t size) {
            incomingBytes.insert(incomingBytes.end(), data, data + size);
        }

        void reset() {
            outgoingBytes.clear();
            incomingBytes.clear();
            _readPosition = 0;
        }

      protected:
        size_t writeImpl(const uint8_t* data, size_t size) override {
            outgoingBytes.insert(outgoingBytes.end(), data, data + size);
            return true;
        }

        size_t availableImpl() override {
            return incomingBytes.size() - _readPosition;
        }

        size_t readImpl(uint8_t* data, size_t size) override {
            const size_t count = std::min(availableImpl(), size);
            std::copy(incomingBytes.begin() + _readPosition, incomingBytes.begin() + _readPosition + count, data);
            _readPosition += count;
            return count;
        }

      private:
        size_t _readPosition = 0;
    };

    TestCommunication communication;
    using TestController = ConfigurationController<256, 256>;

    void injectEncodedMessage(const uint8_t* messageData, size_t messageLength) {
        TestCommunication encoderCommunication;
        CobsEncodedCommunication encoder(encoderCommunication, 256, 256);
        ASSERT_TRUE(encoder.writeMessage(messageData, messageLength));
        communication.injectIncomingBytes(encoderCommunication.outgoingBytes.data(), encoderCommunication.outgoingBytes.size());
    }
} // namespace

TEST(ConfigurationController, LoopParsesCompactWriteSettings) {
    communication.reset();
    TestController::init(communication);
    ASSERT_TRUE(registerDefaultMessagePayloads());

    std::array<uint8_t, 128> bytes{};
    mpack_writer_t writer;
    mpack_writer_init(&writer, reinterpret_cast<char*>(bytes.data()), bytes.size());
    mpack_start_map(&writer, 3);
    mpack_write_cstr(&writer, "msgType");
    mpack_write_cstr(&writer, "request");
    mpack_write_cstr(&writer, "opCode");
    mpack_write_cstr(&writer, DefaultWriteKeys::writeSettings);
    mpack_write_cstr(&writer, "payload");
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "values");
    mpack_start_array(&writer, 1);
    mpack_start_map(&writer, 3);
    mpack_write_cstr(&writer, "address");
    mpack_write_u32(&writer, 0x410000U);
    mpack_write_cstr(&writer, "type");
    mpack_write_cstr(&writer, "i32");
    mpack_write_cstr(&writer, "value");
    mpack_start_map(&writer, 1);
    mpack_write_cstr(&writer, "value");
    mpack_write_i32(&writer, 1200);
    mpack_finish_map(&writer);
    mpack_finish_map(&writer);
    mpack_finish_array(&writer);
    mpack_finish_map(&writer);
    mpack_finish_map(&writer);
    const size_t size = mpack_writer_buffer_used(&writer);
    ASSERT_EQ(mpack_writer_destroy(&writer), mpack_ok);

    injectEncodedMessage(bytes.data(), size);

    bool called = false;
    TestController::get().setOnReceived(
        [](void* context, const Message& message) {
            auto& called = *static_cast<bool*>(context);
            const auto* writes = static_cast<const WriteSettings*>(message.payload);
            ASSERT_NE(writes, nullptr);
            ASSERT_EQ(writes->values.size, 1);
            EXPECT_EQ(writes->values[0]->address, 0x410000U);
            EXPECT_EQ(static_cast<const NumberSetting<int32_t>*>(writes->values[0]->value)->value, 1200);
            called = true;
        },
        &called);
    TestController::get().loop();

    EXPECT_TRUE(called);
}
