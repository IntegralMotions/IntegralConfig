#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <vector>

#include "ConfigurationController.h"
#include "IntegralCommunication/Communication.h"
#include "IntegralCommunication/SevenBitEncodedCommunication.h"
#include "MPackObject.hpp"
#include "Messages.h"
#include "mpack/mpack-writer.h"

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
        readPosition = 0;
    }

  protected:
    void writeImpl(const uint8_t* data, size_t size) override {
        outgoingBytes.insert(outgoingBytes.end(), data, data + size);
    }

    size_t availableImpl() override {
        return incomingBytes.size() - readPosition;
    }

    size_t readImpl(uint8_t* data, size_t size) override {
        size_t availableBytes = availableImpl();
        size_t countToRead = std::min(availableBytes, size);
        if (countToRead == 0)
            return 0;

        std::copy(incomingBytes.begin() + readPosition, incomingBytes.begin() + readPosition + countToRead, data);

        readPosition += countToRead;
        return countToRead;
    }

  private:
    size_t readPosition = 0;
};

static TestCommunication globalCommunication;

using TestController = ConfigurationController<256, 256>;

class ConfigurationControllerTests : public ::testing::Test {
  protected:
    void SetUp() override {
        globalCommunication.reset();
        TestController::init(globalCommunication);
        Message::registerMembers();
        TestWriteObject::registerMembers();
    }

    TestController& controller() {
        return TestController::get();
    }

    // Simple object that uses the MPackObject mechanism and registered members
    class TestWriteObject : public MPackObject<TestWriteObject, 1> {
      public:
        static void registerMembers() {
            registerMember("value", CppType::I32, &TestWriteObject::value);
        }

        int32_t value{};
    };
};

// helper to encode a buffer using the real seven bit encoder
template <size_t TransmissionSize, size_t ReceptionSize>
void injectEncodedMessage(TestCommunication& communication, const uint8_t* messageData, size_t messageLength) {
    TestCommunication temporaryCommunication;
    SevenBitEncodedCommunication<TransmissionSize, ReceptionSize> encoder(temporaryCommunication);

    bool success = encoder.writeMessage(messageData, messageLength);
    ASSERT_TRUE(success);

    communication.injectIncomingBytes(temporaryCommunication.outgoingBytes.data(),
                                      temporaryCommunication.outgoingBytes.size());
}

TEST_F(ConfigurationControllerTests, WriteSendsBytesThroughCommunication) {
    ConfigurationControllerTests::TestWriteObject object;
    object.value = 42;

    bool success = controller().write(object);

    EXPECT_TRUE(success);
    EXPECT_FALSE(globalCommunication.outgoingBytes.empty());
}

TEST_F(ConfigurationControllerTests, LoopDoesNothingWhenNoMessageAvailable) {
    bool callbackCalled = false;

    controller().setOnReceived([&](const Message&) { callbackCalled = true; });

    controller().loop();

    EXPECT_FALSE(callbackCalled);
}

TEST_F(ConfigurationControllerTests, LoopParsesMessageAndCallsCallback) {
    std::array<uint8_t, 256> buffer;
    mpack_writer_t writer;
    mpack_writer_init(&writer, reinterpret_cast<char*>(buffer.data()), buffer.size());

    // { "msgType": "request", "opCode": "read.device", "payload": nil }
    mpack_build_map(&writer);

    mpack_write_cstr(&writer, "msgType");
    mpack_write_cstr(&writer, "request");

    mpack_write_cstr(&writer, "opCode");
    mpack_write_cstr(&writer, ReadKeys::ReadDevice);

    mpack_write_cstr(&writer, "payload");
    mpack_write_nil(&writer);

    mpack_complete_map(&writer);

    mpack_error_t error = mpack_writer_destroy(&writer);
    ASSERT_EQ(error, mpack_ok);

    size_t usedBytes = mpack_writer_buffer_used(&writer);

    injectEncodedMessage<256, 256>(globalCommunication, buffer.data(), usedBytes);

    bool callbackCalled = false;
    MsgType receivedMessageType = MsgType::Unknown;

    controller().setOnReceived([&](const Message& message) {
        callbackCalled = true;
        receivedMessageType = message.getMsgType();
    });

    controller().loop();

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(receivedMessageType, MsgType::Request);
}
