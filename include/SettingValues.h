#pragma once

#include "MPackObject.hpp"
#include <cstdint>
template <typename TDerived, typename TValue> class SettingValue : public MPackObject<TDerived> {
  private:
    SettingValue() = default;
    friend TDerived;

  public:
    virtual void registerMembers() {
        this->registerMember("address", CppType::U32, &address);
        this->registerMember("id", CppType::String, &id);
        this->registerMember("label", CppType::String, &label);
        this->registerMember("unit", CppType::String, &unit);
        this->registerMember("value", getType<TValue>(), &value);
        this->registerMember("readonly", CppType::Bool, &readonly);
    }

  protected:
    template <typename T> CppType getType() {
        CppType valueType = CppType::None;
        if constexpr (std::is_same<T, int8_t>::value) {
            valueType = CppType::I8;
        } else if constexpr (std::is_same<T, uint8_t>::value) {
            valueType = CppType::U8;
        } else if constexpr (std::is_same<T, int16_t>::value) {
            valueType = CppType::I16;
        } else if constexpr (std::is_same<T, uint16_t>::value) {
            valueType = CppType::U16;
        } else if constexpr (std::is_same<T, int32_t>::value) {
            valueType = CppType::I32;
        } else if constexpr (std::is_same<T, uint32_t>::value) {
            valueType = CppType::U32;
        } else if constexpr (std::is_same<T, int64_t>::value) {
            valueType = CppType::I64;
        } else if constexpr (std::is_same<T, uint64_t>::value) {
            valueType = CppType::U64;
        } else if constexpr (std::is_same<T, float>::value) {
            valueType = CppType::F32;
        } else if constexpr (std::is_same<T, double>::value) {
            valueType = CppType::F64;
        } else if constexpr (std::is_same<T, bool>::value) {
            valueType = CppType::Bool;
        } else if constexpr (std::is_same<T, const char*>::value) {
            valueType = CppType::String;
        }

        return valueType;
    }

  private:
    uint32_t address;
    const char* id;
    const char* label;
    const char* unit;
    TValue value;
    bool readonly = false;
};

class BoolSetting : public SettingValue<BoolSetting, bool> {};
class StringSetting : public SettingValue<StringSetting, const char*> {
  public:
    void registerMembers() override {
        SettingValue<StringSetting, const char*>::registerMembers();
        this->registerMember("options", {CppType::Array, CppType::String}, &options);
    }

  private:
    const char** options;
};

template <typename TValue> class NumberSetting : public SettingValue<NumberSetting<TValue>, TValue> {
  public:
    void registerMembers() override {
        SettingValue<NumberSetting<TValue>, TValue>::registerMembers();
        this->registerMember("min", this->template getType<TValue>(), &min);
        this->registerMember("max", this->template getType<TValue>(), &max);
        this->registerMember("isRange", CppType::Bool, &isRange);
        this->registerMember("options", {CppType::Array, this->template getType<TValue>()}, &options);
    }

  private:
    TValue* options;
    TValue min;
    TValue max;
    TValue step;
    bool isRange = false;
};