#pragma once

#include "mpack-object-base.h"
#include "mpack-object-type.h"
#include "mpack-object.hpp"
#include "setting-values.h"
#include <cstring>

class Setting : public MPackObject<Setting> {
  public:
    void registerMembers() {
        this->registerMember("type", MPackObjectType{CppType::String, CppType::None}, &type);
        this->registerMember("value", MPackObjectType{CppType::ObjectPtr, CppType::None}, &value);
    }

  protected:
    MPackObjectBase* createObject(const char* name) override {
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
    const char* type;
    MPackObjectBase* value;
};

class Group : public MPackObject<Group> {
  public:
    void registerMembers() {
        this->registerMember("id", CppType::String, &id);
        this->registerMember("label", CppType::String, &label);
        this->registerMember("settings", {CppType::Array, CppType::ObjectPtr}, &settings);
    }

  protected:
    MPackObjectBase* createObject(const char* name) override {
        if (std::strcmp(name, "settings") == 0) {
            return new Setting();
        }
        return nullptr;
    }

  private:
    const char* id;
    const char* label;
    Setting** settings;
};

class Module : public MPackObject<Module> {
  public:
    void registerMembers() {
        this->registerMember("id", CppType::String, &id);
        this->registerMember("label", CppType::String, &label);
        this->registerMember("groups", {CppType::Array, CppType::ObjectPtr}, &groups);
    }

  protected:
    MPackObjectBase* createObject(const char* name) override {
        return new Group();
    }

  private:
    const char* id;
    const char* label;
    Group** groups;
};

class DeviceInfo : public MPackObject<DeviceInfo> {
  public:
    void registerMembers() {
        this->registerMember("model", CppType::String, &model);
        this->registerMember("firwareVersion", CppType::String, &firwareVersion);
    }

  private:
    const char* model;
    const char* firwareVersion;
};

class Device : public MPackObject<Device> {
  public:
    void registerMembers() {
        this->registerMember("deviceInfo", CppType::ObjectPtr, &deviceInfo);
        this->registerMember("modules", {CppType::Array, CppType::ObjectPtr}, &modules);
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

  private:
    DeviceInfo* deviceInfo;
    Module** modules;
};
