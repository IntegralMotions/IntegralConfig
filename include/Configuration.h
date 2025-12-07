#pragma once

#include "MPackArray.h"
#include "MPackObject.hpp"
#include "SettingValues.h"
#include <cstring>

class Setting : public MPackObject<Setting, 2> { // NOLINT(readability-magic-numbers)
  public:
    static void registerMembers() {
        registerMember("type", CppType::String, &Setting::type);
        registerMember("value", CppType::ObjectPtr, &Setting::value);
    }

  protected:
    MPackObjectBase *createObject(const char * /*name*/) override {
        if (std::strcmp(type, "bool") == 0) {
            return new BoolSetting();
        }
        if (std::strcmp(type, "int") == 0) {
            return new NumberSetting<int>();
        }
        if (std::strcmp(type, "float") == 0) {
            return new NumberSetting<float>();
        }
        if (std::strcmp(type, "double") == 0) {
            return new NumberSetting<double>();
        }
        if (std::strcmp(type, "string") == 0) {
            return new StringSetting();
        }
        return nullptr;
    }

  private:
    const char *type{};
    MPackObjectBase *value{};
};

class Group : public MPackObject<Group, 3> { // NOLINT(readability-magic-numbers)
  public:
    static void registerMembers() {
        registerMember("id", CppType::String, &Group::id);
        registerMember("label", CppType::String, &Group::label);
        registerMember("settings", {CppType::Array, CppType::ObjectPtr}, &Group::settings);
    }

  protected:
    MPackObjectBase *createObject(const char *name) override {
        if (std::strcmp(name, "settings") == 0) {
            return new Setting();
        }
        return nullptr;
    }

  private:
    const char *id{};
    const char *label{};
    MPackArray<Setting *> settings{};
};

class Module : public MPackObject<Module, 3> { // NOLINT(readability-magic-numbers)
  public:
    static void registerMembers() {
        registerMember("id", CppType::String, &Module::id);
        registerMember("label", CppType::String, &Module::label);
        registerMember("groups", {CppType::Array, CppType::ObjectPtr}, &Module::groups);
    }

  protected:
    MPackObjectBase *createObject(const char * /*name*/) override {
        return new Group();
    }

  private:
    const char *id{};
    const char *label{};
    MPackArray<Group *> groups;
};

class DeviceInfo : public MPackObject<DeviceInfo, 2> { // NOLINT(readability-magic-numbers)
  public:
    static void registerMembers() {
        registerMember("model", CppType::String, &DeviceInfo::model);
        registerMember("firmwareVersion", {CppType::String, CppType::None}, &DeviceInfo::firmwareVersion);
    }

  private:
    const char *model{};
    const char *firmwareVersion{};
};

class Device : public MPackObject<Device, 2> { // NOLINT(readability-magic-numbers)
  public:
    static void registerMembers() {
        registerMember("deviceInfo", CppType::ObjectPtr, &Device::deviceInfo);
        registerMember("modules", {CppType::Array, CppType::ObjectPtr}, &Device::modules);
    }

  protected:
    MPackObjectBase *createObject(const char *name) override {
        if (std::strcmp(name, "deviceInfo") == 0) {
            return new DeviceInfo();
        }
        if (std::strcmp(name, "modules") == 0) {
            return new Module();
        }
        return nullptr;
    }

  private:
    DeviceInfo *deviceInfo{};
    MPackArray<Module *> modules;
};
