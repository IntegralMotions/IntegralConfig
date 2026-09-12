#pragma once

#include "MPackArray.h"
#include "MPackObject.hpp"
#include <cstddef>
#include <cstdint>
#include <type_traits>

constexpr size_t SettingValueMembers = 6;

template <typename TDerived, typename TValue, size_t AddedMembers>
class SettingValue : public MPackObject<TDerived, SettingValueMembers + AddedMembers> {
  private:
    SettingValue() = default;
    friend TDerived;

  public:
    static void registerMembers() {
        using Obj = MPackObject<TDerived, SettingValueMembers + AddedMembers>;
        Obj::registerMember("address", CppType::U32, &TDerived::address);
        Obj::registerMember("id", CppType::String, &TDerived::id);
        Obj::registerMember("label", CppType::String, &TDerived::label);
        Obj::registerMember("unit", CppType::String, &TDerived::unit);
        Obj::registerMember("value", getType<TValue>(), &TDerived::value);
        Obj::registerMember("readonly", CppType::Bool, &TDerived::readonly);
    }

  protected:
    template <typename T>
    static CppType getType() {
        using Unqualified = std::remove_cv_t<T>;
        CppType valueType = CppType::None;
        if constexpr (std::is_same_v<Unqualified, bool>) {
            valueType = CppType::Bool;
        } else if constexpr (std::is_same_v<Unqualified, const char*>) {
            valueType = CppType::String;
        } else if constexpr (std::is_floating_point_v<Unqualified>) {
            if constexpr (sizeof(Unqualified) == sizeof(double)) {
                valueType = CppType::F64;
            } else {
                valueType = CppType::F32;
            }
        } else if constexpr (std::is_integral_v<Unqualified>) {
            static_assert(std::is_signed_v<Unqualified> || std::is_unsigned_v<Unqualified>, "unexpected integral type");
            if constexpr (std::is_signed_v<Unqualified>) {
                if constexpr (sizeof(Unqualified) == sizeof(int8_t)) {
                    valueType = CppType::I8;
                } else if constexpr (sizeof(Unqualified) == sizeof(int16_t)) {
                    valueType = CppType::I16;
                } else if constexpr (sizeof(Unqualified) == sizeof(int32_t)) {
                    valueType = CppType::I32;
                } else if constexpr (sizeof(Unqualified) == sizeof(int64_t)) {
                    valueType = CppType::I64;
                }
            } else {
                if constexpr (sizeof(Unqualified) == sizeof(uint8_t)) {
                    valueType = CppType::U8;
                } else if constexpr (sizeof(Unqualified) == sizeof(uint16_t)) {
                    valueType = CppType::U16;
                } else if constexpr (sizeof(Unqualified) == sizeof(uint32_t)) {
                    valueType = CppType::U32;
                } else if constexpr (sizeof(Unqualified) == sizeof(uint64_t)) {
                    valueType = CppType::U64;
                }
            }
        }

        return valueType;
    }

  public:
    uint32_t address = 0;
    const char* id = nullptr;
    const char* label = nullptr;
    const char* unit = nullptr;
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

    MPackArray<const char*> options;
};

template <typename TValue>
class NumberSetting : public SettingValue<NumberSetting<TValue>, TValue, 5> {
  public:
    static void registerMembers() {
        using Base = SettingValue<NumberSetting<TValue>, TValue, 5>;
        Base::registerMembers();
        Base::registerMember("min", Base::template getType<TValue>(), &NumberSetting::min);
        Base::registerMember("max", Base::template getType<TValue>(), &NumberSetting::max);
        Base::registerMember("step", Base::template getType<TValue>(), &NumberSetting::step);
        Base::registerMember("isRange", CppType::Bool, &NumberSetting::isRange);
        Base::registerMember("options", {CppType::Array, Base::template getType<TValue>()}, &NumberSetting::options);
    }

    MPackArray<TValue> options;
    TValue min;
    TValue max;
    TValue step;
    bool isRange = false;
};