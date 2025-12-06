#pragma once

#include "MPackArray.h"
#include "MPackObject.hpp"
#include <cstddef>
#include <cstdint>

constexpr size_t SETTING_VALUE_MEMBERS = 6;

template <typename TDerived, typename TValue, size_t AddedMembers>
class SettingValue : public MPackObject<TDerived, SETTING_VALUE_MEMBERS + AddedMembers> {
  private:
    SettingValue() = default;
    friend TDerived;

  public:
    static void registerMembers() {
        using Obj = MPackObject<TDerived, SETTING_VALUE_MEMBERS + AddedMembers>;
        Obj::registerMember("address", CppType::U32, &TDerived::address);
        Obj::registerMember("id", CppType::String, &TDerived::id);
        Obj::registerMember("label", CppType::String, &TDerived::label);
        Obj::registerMember("unit", CppType::String, &TDerived::unit);
        Obj::registerMember("value", getType<TValue>(), &TDerived::value);
        Obj::registerMember("readonly", CppType::Bool, &TDerived::readonly);
    }

  protected:
    template <typename T> static CppType getType() {
        CppType valueType = CppType::None;
        if constexpr (std::is_same_v<T, int8_t>) {
            valueType = CppType::I8;
        } else if constexpr (std::is_same_v<T, uint8_t>) {
            valueType = CppType::U8;
        } else if constexpr (std::is_same_v<T, int16_t>) {
            valueType = CppType::I16;
        } else if constexpr (std::is_same_v<T, uint16_t>) {
            valueType = CppType::U16;
        } else if constexpr (std::is_same_v<T, int32_t>) {
            valueType = CppType::I32;
        } else if constexpr (std::is_same_v<T, uint32_t>) {
            valueType = CppType::U32;
        } else if constexpr (std::is_same_v<T, int64_t>) {
            valueType = CppType::I64;
        } else if constexpr (std::is_same_v<T, uint64_t>) {
            valueType = CppType::U64;
        } else if constexpr (std::is_same_v<T, float>) {
            valueType = CppType::F32;
        } else if constexpr (std::is_same_v<T, double>) {
            valueType = CppType::F64;
        } else if constexpr (std::is_same_v<T, bool>) {
            valueType = CppType::Bool;
        } else if constexpr (std::is_same_v<T, const char*>) {
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

class BoolSetting : public SettingValue<BoolSetting, bool, 0> {
  public:
    static void registerMembers() {
        using Base = SettingValue<BoolSetting, bool, 0>;
        Base::registerMembers();
    }
};

class StringSetting : public SettingValue<StringSetting, const char*, 1> {
  public:
    static void registerMembers() {
        using Base = SettingValue<StringSetting, const char*, 1>;
        Base::registerMembers();
        registerMember("options", {CppType::Array, CppType::String}, &StringSetting::options);
    }

  private:
    MPackArray<const char*> options;
};

template <typename TValue> class NumberSetting : public SettingValue<NumberSetting<TValue>, TValue, 4> {
  public:
    static void registerMembers() {
        using Base = SettingValue<NumberSetting<TValue>, TValue, 4>;
        Base::registerMembers();
        Base::registerMember("min", Base::template getType<TValue>(), &NumberSetting::min);
        Base::registerMember("max", Base::template getType<TValue>(), &NumberSetting::max);
        Base::registerMember("isRange", CppType::Bool, &NumberSetting::isRange);
        Base::registerMember("options", {CppType::Array, Base::template getType<TValue>()}, &NumberSetting::options);
    }

  private:
    MPackArray<TValue> options;
    TValue min;
    TValue max;
    TValue step;
    bool isRange = false;
};