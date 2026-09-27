// tests/read-device-message-write-tests.cpp
#include "Configuration.h"
#include "DefaultMessagePayloads.h"
#include "Messages.h"
#include "mpack/mpack-writer.h"
#include <cstdlib>
#include <gtest/gtest.h>

static mpack_error_t writeMessage(Message& msg) {
    mpack_writer_t writer;
    char* data = nullptr;
    size_t size = 0;

    mpack_writer_init_growable(&writer, &data, &size);

    msg.write(writer, 0);

    auto err = mpack_writer_destroy(&writer);

    delete data;

    return err;
}

static Message makeReadDeviceResponse() {
    auto* device = new Device();

    // deviceInfo
    auto* info = new DeviceInfo();
    info->model = "DummyDevice";
    info->firmwareVersion = "0.1.0";
    device->deviceInfo = info;

    // modules
    device->modules.size = 1;
    device->modules.p = new Module*[1];

    auto* module = new Module();
    module->id = "motor";
    device->modules[0] = module;

    // groups
    module->groups.size = 1;
    module->groups.p = new Group*[1];

    auto* group = new Group();
    group->id = "main";
    module->groups[0] = group;

    // settings
    group->settings.size = 2;
    group->settings.p = new Setting*[2];

    // setting 1: bool
    {
        auto* setting = new Setting();
        setting->type = "bool";

        auto* value = new BoolSetting();
        value->id = "enable";
        value->unit = "";
        value->value = true;
        value->readonly = false;

        setting->value = value;
        group->settings[0] = setting;
    }

    // setting 2: int
    {
        auto* setting = new Setting();
        setting->type = "int";

        auto* value = new NumberSetting<int>();
        value->id = "speed";
        value->unit = "rpm";
        value->value = 1000;
        value->limits.minimum = 0;
        value->limits.maximum = 2000;
        value->limits.isRange = false;

        value->limits.options.size = 2;
        value->limits.options.p = new MessageSettingOption<int>*[2];
        value->limits.options[0] = new MessageSettingOption<int>();
        value->limits.options[0]->value = 500;
        value->limits.options[0]->id = "low-speed";
        value->limits.options[1] = new MessageSettingOption<int>();
        value->limits.options[1]->value = 1500;
        value->limits.options[1]->id = "high-speed";

        setting->value = value;
        group->settings[1] = setting;
    }

    Message toSend;
    toSend.msgType = "response";
    toSend.opCode = DefaultReadKeys::readDevice;
    toSend.payload = device;
    return toSend;
}

TEST(ReadDeviceMessageWriteTests, WritesWithoutMPackErrors) {
    auto toSend = makeReadDeviceResponse();
    EXPECT_EQ(writeMessage(toSend), mpack_ok);
}
