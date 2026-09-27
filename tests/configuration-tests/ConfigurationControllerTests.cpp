#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <vector>

#include "Configuration.h"
#include "ConfigurationController.h"
#include "DefaultMessagePayloads.h"
#include "IntegralCommunication/CobsEncodedCommunication.h"
#include "IntegralCommunication/Communication.h"
#include "IntegralCommunication/SevenBitEncodedCommunication.h"
#include "MPackObject.hpp"
#include "Messages.h"
#include "SettingValues.h"
#include "mpack/mpack-writer.h"

struct ReceiveCtx {
    bool& called;
    MsgType& type;
};

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
    size_t writeImpl(const uint8_t* data, size_t size) override {
        outgoingBytes.insert(outgoingBytes.end(), data, data + size);
        return true;
    }

    size_t availableImpl() override {
        return incomingBytes.size() - readPosition;
    }

    size_t readImpl(uint8_t* data, size_t size) override {
        size_t availableBytes = availableImpl();
        size_t countToRead = std::min(availableBytes, size);
        if (countToRead == 0) {
            return 0;
        }

        std::copy(incomingBytes.begin() + readPosition, incomingBytes.begin() + readPosition + countToRead, data);

        readPosition += countToRead;
        return countToRead;
    }

  private:
    size_t readPosition = 0;
};

static TestCommunication globalCommunication;

using TestController = ConfigurationController<1024, 1024>;

class ConfigurationControllerTests : public ::testing::Test {
  protected:
    void SetUp() override {
        globalCommunication.reset();
        TestController::init(globalCommunication);

        Message::registerMembers();
        TestWriteObject::registerMembers();

        DeviceInfo::registerMembers();
        Device::registerMembers();
        Module::registerMembers();
        Group::registerMembers();
        Setting::registerMembers();
        BoolSetting::registerMembers();
        NumberSetting<int>::registerMembers();
        StringSetting::registerMembers();

        registerDefaultMessagePayloads();
    }

    static TestController& controller() {
        return TestController::get();
    }

    class TestWriteObject : public MPackObject<TestWriteObject, 1> {
      public:
        static void registerMembers() {
            registerMember("value", CppType::I32, &TestWriteObject::value);
        }

        int32_t value{};
    };
};

template <size_t TransmissionSize, size_t ReceptionSize>
void injectEncodedMessage(TestCommunication& communication, const uint8_t* messageData, size_t messageLength) {
    TestCommunication temporaryCommunication;
    CobsEncodedCommunication encoder(temporaryCommunication, TransmissionSize, ReceptionSize);

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
    MsgType receivedMessageType = MsgType::Unknown;

    ReceiveCtx ctx{callbackCalled, receivedMessageType};

    controller().setOnReceived(
        [](void* context, const Message& message) {
            auto* ctx = static_cast<ReceiveCtx*>(context);
            ctx->called = true;
            ctx->type = message.getMsgType();
        },
        &ctx);
    controller().loop();

    EXPECT_FALSE(callbackCalled);
}

TEST_F(ConfigurationControllerTests, LoopParsesMessageAndCallsCallback) {
    std::array<uint8_t, 256> buffer;
    mpack_writer_t writer;
    mpack_writer_init(&writer, reinterpret_cast<char*>(buffer.data()), buffer.size());

    mpack_build_map(&writer);

    mpack_write_cstr(&writer, "msgType");
    mpack_write_cstr(&writer, "request");

    mpack_write_cstr(&writer, "opCode");
    mpack_write_cstr(&writer, DefaultReadKeys::readDevice);

    mpack_write_cstr(&writer, "payload");
    mpack_write_nil(&writer);

    mpack_complete_map(&writer);

    mpack_error_t error = mpack_writer_destroy(&writer);
    ASSERT_EQ(error, mpack_ok);

    size_t usedBytes = mpack_writer_buffer_used(&writer);

    injectEncodedMessage<256, 256>(globalCommunication, buffer.data(), usedBytes);

    bool callbackCalled = false;
    MsgType receivedMessageType = MsgType::Unknown;

    ReceiveCtx ctx{callbackCalled, receivedMessageType};

    controller().setOnReceived(
        [](void* context, const Message& message) {
            auto* ctx = static_cast<ReceiveCtx*>(context);
            ctx->called = true;
            ctx->type = message.getMsgType();
        },
        &ctx);
    controller().loop();

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(receivedMessageType, MsgType::Request);
}

TEST_F(ConfigurationControllerTests, LoopParsesWriteDeviceWithFullDeviceStructure) {
    std::array<uint8_t, 512> buffer{};
    mpack_writer_t writer;
    mpack_writer_init(&writer, reinterpret_cast<char*>(buffer.data()), buffer.size());

    ASSERT_EQ(mpack_writer_error(&writer), mpack_ok);

    // Root message: 3 entries: msgType, opCode, payload
    mpack_start_map(&writer, 3);

    mpack_write_cstr(&writer, "msgType");
    mpack_write_cstr(&writer, "event");

    mpack_write_cstr(&writer, "opCode");
    mpack_write_cstr(&writer, DefaultReadKeys::writeDevice);

    mpack_write_cstr(&writer, "payload");

    ASSERT_EQ(mpack_writer_error(&writer), mpack_ok);

    // Device: 2 entries: deviceInfo, modules
    mpack_start_map(&writer, 2);

    // deviceInfo: 2 entries: model, firmwareVersion
    mpack_write_cstr(&writer, "deviceInfo");
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "model");
    mpack_write_cstr(&writer, "TestModel");
    mpack_write_cstr(&writer, "firmwareVersion");
    mpack_write_cstr(&writer, "1.0.0");
    mpack_finish_map(&writer); // deviceInfo

    ASSERT_EQ(mpack_writer_error(&writer), mpack_ok);

    // modules: array of 1 module
    mpack_write_cstr(&writer, "modules");
    mpack_start_array(&writer, 1);

    // module: 2 entries: id, groups
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "id");
    mpack_write_cstr(&writer, "motor");

    ASSERT_EQ(mpack_writer_error(&writer), mpack_ok);

    // groups: array of 1 group
    mpack_write_cstr(&writer, "groups");
    mpack_start_array(&writer, 1);

    // group: 2 entries: id, settings
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "id");
    mpack_write_cstr(&writer, "group1");

    ASSERT_EQ(mpack_writer_error(&writer), mpack_ok);

    // settings: array of 2 settings
    mpack_write_cstr(&writer, "settings");
    mpack_start_array(&writer, 2);

    // setting 1: bool
    // Setting map has 2 entries: type, value
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "type");
    mpack_write_cstr(&writer, "bool");
    mpack_write_cstr(&writer, "value");

    // BoolSetting: id, value, limits
    mpack_start_map(&writer, 3);
    mpack_write_cstr(&writer, "id");
    mpack_write_cstr(&writer, "enable");
    mpack_write_cstr(&writer, "value");
    mpack_write_bool(&writer, true);
    mpack_write_cstr(&writer, "limits");
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "isRange");
    mpack_write_bool(&writer, false);
    mpack_write_cstr(&writer, "options");
    mpack_start_array(&writer, 0);
    mpack_finish_array(&writer);
    mpack_finish_map(&writer);
    mpack_finish_map(&writer); // BoolSetting
    mpack_finish_map(&writer); // Setting 1

    ASSERT_EQ(mpack_writer_error(&writer), mpack_ok);

    // setting 2: int NumberSetting
    // Setting map: 2 entries: type, value
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "type");
    mpack_write_cstr(&writer, "int");
    mpack_write_cstr(&writer, "value");

    // NumberSetting<int>: id, unit, value, limits
    mpack_start_map(&writer, 4);
    mpack_write_cstr(&writer, "id");
    mpack_write_cstr(&writer, "speed");
    mpack_write_cstr(&writer, "unit");
    mpack_write_cstr(&writer, "rpm");
    mpack_write_cstr(&writer, "value");
    mpack_write_i32(&writer, 1000);
    mpack_write_cstr(&writer, "limits");
    mpack_start_map(&writer, 5);
    mpack_write_cstr(&writer, "min");
    mpack_write_i32(&writer, 0);
    mpack_write_cstr(&writer, "max");
    mpack_write_i32(&writer, 2000);
    mpack_write_cstr(&writer, "step");
    mpack_write_i32(&writer, 100);
    mpack_write_cstr(&writer, "isRange");
    mpack_write_bool(&writer, false);
    mpack_write_cstr(&writer, "options");
    mpack_start_array(&writer, 2);
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "value");
    mpack_write_i32(&writer, 500);
    mpack_write_cstr(&writer, "id");
    mpack_write_cstr(&writer, "low-speed");
    mpack_finish_map(&writer);
    mpack_start_map(&writer, 2);
    mpack_write_cstr(&writer, "value");
    mpack_write_i32(&writer, 1500);
    mpack_write_cstr(&writer, "id");
    mpack_write_cstr(&writer, "high-speed");
    mpack_finish_map(&writer);
    mpack_finish_array(&writer); // options
    mpack_finish_map(&writer);   // limits
    mpack_finish_map(&writer);   // NumberSetting<int>
    mpack_finish_map(&writer);   // Setting 2

    ASSERT_EQ(mpack_writer_error(&writer), mpack_ok);

    mpack_finish_array(&writer); // settings
    mpack_finish_map(&writer);   // group
    mpack_finish_array(&writer); // groups
    mpack_finish_map(&writer);   // module
    mpack_finish_array(&writer); // modules
    mpack_finish_map(&writer);   // device
    mpack_finish_map(&writer);   // root message

    ASSERT_EQ(mpack_writer_error(&writer), mpack_ok);

    size_t usedBytes = mpack_writer_buffer_used(&writer);
    mpack_error_t errorCode = mpack_writer_destroy(&writer);
    ASSERT_EQ(errorCode, mpack_ok);

    injectEncodedMessage<1024, 1024>(globalCommunication, buffer.data(), usedBytes);

    struct OptionReceiveCtx {
        bool& called;
        MsgType& type;
        size_t& optionCount;
        int& optionValue;
        const char*& optionId;
    };

    bool callbackCalled = false;
    MsgType receivedMessageType = MsgType::Unknown;
    size_t optionCount = 0;
    int optionValue = 0;
    const char* optionId = nullptr;

    OptionReceiveCtx ctx{callbackCalled, receivedMessageType, optionCount, optionValue, optionId};

    controller().setOnReceived(
        [](void* context, const Message& message) {
            auto* ctx = static_cast<OptionReceiveCtx*>(context);
            ctx->called = true;
            ctx->type = message.getMsgType();

            const auto* device = static_cast<const Device*>(message.payload);
            const auto* numberSetting =
                static_cast<const NumberSetting<int>*>(device->modules[0]->groups[0]->settings[1]->value);
            ctx->optionCount = numberSetting->limits.options.size;
            ctx->optionValue = numberSetting->limits.options[1]->value;
            ctx->optionId = numberSetting->limits.options[1]->id;
        },
        &ctx);

    controller().loop();

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(receivedMessageType, MsgType::Event);
    EXPECT_EQ(optionCount, 2);
    EXPECT_EQ(optionValue, 1500);
    EXPECT_STREQ(optionId, "high-speed");
}
