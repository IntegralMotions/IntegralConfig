// tests/read-device-message-write-tests.cpp
#include "Configuration.h"
#include "DefaultMessagePayloads.h"
#include "Messages.h"
#include "mpack/mpack-writer.h"
#include <cstdlib>
#include <gtest/gtest.h>

static mpack_error_t writeMessage(Message &msg) {
    mpack_writer_t writer;
    char *data = nullptr;
    size_t size = 0;

    mpack_writer_init_growable(&writer, &data, &size);

    msg.write(writer, 0);

    auto err = mpack_writer_destroy(&writer);

    delete data;

    return err;
}

static Message makeReadDeviceResponse() {
    auto *device = new Device();

    // deviceInfo
    auto *info = new DeviceInfo();
    info->model = "DummyDevice";
    info->firmwareVersion = "0.1.0";
    device->deviceInfo = info;

    // modules
    device->modules.size = 1;
    device->modules.p = new Module *[1];

    auto *module = new Module();
    module->id = "motor";
    module->label = "Motor Module";
    device->modules[0] = module;

    // groups
    module->groups.size = 1;
    module->groups.p = new Group *[1];

    auto *group = new Group();
    group->id = "main";
    group->label = "Main Group";
    module->groups[0] = group;

    // settings
    group->settings.size = 2;
    group->settings.p = new Setting *[2];

    // setting 1: bool
    {
        auto *setting = new Setting();
        setting->type = "bool";

        auto *value = new BoolSetting();
        value->address = 1;
        value->id = "enable";
        value->label = "Enable";
        value->unit = "";
        value->value = true;
        value->readonly = false;

        setting->value = value;
        group->settings[0] = setting;
    }

    // setting 2: int
    {
        auto *setting = new Setting();
        setting->type = "int";

        auto *value = new NumberSetting<int>();
        value->address = 2;
        value->id = "speed";
        value->label = "Speed";
        value->unit = "rpm";
        value->value = 1000;
        value->min = 0;
        value->max = 2000;
        value->isRange = false;

        value->options.size = 2;
        value->options.p = new int[2]{500, 1500};

        setting->value = value;
        group->settings[1] = setting;
    }

    Message toSend;
    toSend.msgType = "response";
    toSend.opCode = DefaultReadKeys::ReadDevice;
    toSend.payload = device;
    return toSend;
}

TEST(ReadDeviceMessageWriteTests, WritesWithoutMPackErrors) {
    auto toSend = makeReadDeviceResponse();
    EXPECT_EQ(writeMessage(toSend), mpack_ok);
}
