#pragma once

#include "MPackArray.h"
#include "MPackObject.hpp"
#include "SettingValues.h"
#include <cstring>

inline MPackObjectBase* createSettingValue(const char* type) {
    if (type == nullptr) {
        return nullptr;
    }
    if (std::strcmp(type, "bool") == 0) {
        return new BoolSetting();
    }
    if (std::strcmp(type, "i8") == 0) {
        return new NumberSetting<int8_t>();
    }
    if (std::strcmp(type, "u8") == 0) {
        return new NumberSetting<uint8_t>();
    }
    if (std::strcmp(type, "i16") == 0) {
        return new NumberSetting<int16_t>();
    }
    if (std::strcmp(type, "u16") == 0) {
        return new NumberSetting<uint16_t>();
    }
    if (std::strcmp(type, "i32") == 0) {
        return new NumberSetting<int32_t>();
    }
    if (std::strcmp(type, "u32") == 0) {
        return new NumberSetting<uint32_t>();
    }
    if (std::strcmp(type, "i64") == 0) {
        return new NumberSetting<int64_t>();
    }
    if (std::strcmp(type, "u64") == 0) {
        return new NumberSetting<uint64_t>();
    }
    if (std::strcmp(type, "f32") == 0) {
        return new NumberSetting<float>();
    }
    if (std::strcmp(type, "f64") == 0) {
        return new NumberSetting<double>();
    }
    return nullptr;
}

class Setting : public MPackObject<Setting, 2> { // NOLINT(readability-magic-numbers)
  public:
    ~Setting() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("type", CppType::String, &Setting::type);
        registerMember("value", CppType::ObjectPtr, &Setting::value);
    }

    protected:
    MPackObjectBase* createObject(const char* /*name*/) override {
        return createSettingValue(type);
    }

  public:
    const char* type{};
    MPackObjectBase* value{};
};

class WriteSetting : public MPackObject<WriteSetting, 3> { // NOLINT(readability-magic-numbers)
  public:
    ~WriteSetting() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("address", CppType::U32, &WriteSetting::address);
        registerMember("type", CppType::String, &WriteSetting::type);
        registerMember("value", CppType::ObjectPtr, &WriteSetting::value);
    }

  protected:
    MPackObjectBase* createObject(const char* /*name*/) override {
        return createSettingValue(type);
    }

  public:
    uint32_t address = 0;
    const char* type{};
    MPackObjectBase* value{};
};

class WriteSettings : public MPackObject<WriteSettings, 1> {
  public:
    ~WriteSettings() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("values", {CppType::Array, CppType::ObjectPtr}, &WriteSettings::values);
    }

  protected:
    MPackObjectBase* createObject(const char* /*name*/) override {
        return new WriteSetting();
    }

  public:
    MPackArray<WriteSetting*> values{};
};

class WriteSettingsResultValue : public MPackObject<WriteSettingsResultValue, 3> {
  public:
    ~WriteSettingsResultValue() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("address", CppType::U32, &WriteSettingsResultValue::address);
        registerMember("success", CppType::Bool, &WriteSettingsResultValue::success);
        registerOmitNullCString("errorMessage", &WriteSettingsResultValue::errorMessage);
    }

    uint32_t address = 0;
    bool success = false;
    const char* errorMessage = nullptr;
};

class WriteSettingsResult : public MPackObject<WriteSettingsResult, 1> {
  public:
    ~WriteSettingsResult() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("values", {CppType::Array, CppType::ObjectPtr}, &WriteSettingsResult::values);
    }

  protected:
    MPackObjectBase* createObject(const char* /*name*/) override {
        return new WriteSettingsResultValue();
    }

  public:
    MPackArray<WriteSettingsResultValue*> values{};
};

class Group : public MPackObject<Group, 2> { // NOLINT(readability-magic-numbers)
  public:
    ~Group() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("id", CppType::String, &Group::id);
        registerMember("settings", {CppType::Array, CppType::ObjectPtr}, &Group::settings);
    }

  protected:
    MPackObjectBase* createObject(const char* name) override {
        if (std::strcmp(name, "settings") == 0) {
            return new Setting();
        }
        return nullptr;
    }

  public:
    const char* id{};
    MPackArray<Setting*> settings{};
};

class Module : public MPackObject<Module, 2> { // NOLINT(readability-magic-numbers)
  public:
    ~Module() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("id", CppType::String, &Module::id);
        registerMember("groups", {CppType::Array, CppType::ObjectPtr}, &Module::groups);
    }

  protected:
    MPackObjectBase* createObject(const char* /*name*/) override {
        return new Group();
    }

  public:
    const char* id{};
    MPackArray<Group*> groups;
};

class DeviceInfo : public MPackObject<DeviceInfo, 2> { // NOLINT(readability-magic-numbers)
  public:
    ~DeviceInfo() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("model", CppType::String, &DeviceInfo::model);
        registerMember("firmwareVersion", {CppType::String, CppType::None}, &DeviceInfo::firmwareVersion);
    }

    const char* model{};
    const char* firmwareVersion{};
};

class Device : public MPackObject<Device, 2> { // NOLINT(readability-magic-numbers)
  public:
    ~Device() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("deviceInfo", CppType::ObjectPtr, &Device::deviceInfo);
        registerMember("modules", {CppType::Array, CppType::ObjectPtr}, &Device::modules);
    }

  protected:
    MPackObjectBase* createObject(const char* name) override {
        if (std::strcmp(name, "deviceInfo") == 0) {
            return new DeviceInfo();
        }
        if (std::strcmp(name, "modules") == 0) {
            return new Module();
        }
        return nullptr;
    }

  public:
    DeviceInfo* deviceInfo{};
    MPackArray<Module*> modules;
};

class SuccessResult : public MPackObject<SuccessResult, 2> {
  public:
    ~SuccessResult() override {
        clearDecodedMembers();
    }

    static void registerMembers() {
        registerMember("success", CppType::Bool, &SuccessResult::success);
        registerOmitNullCString("errorMessage", &SuccessResult::errorMessage);
    }

    bool success = false;
    const char* errorMessage = nullptr;
};
