#pragma once

#include "MPackArray.h"
#include "MPackObject.hpp"
#include "Setting/SupportedSettingType.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

constexpr size_t SettingValueMembers = 4;

template <typename T>
CppType settingValueCppType() {
    using Unqualified = std::remove_cv_t<T>;
    if constexpr (std::is_same_v<Unqualified, bool>) {
        return CppType::Bool;
    } else if constexpr (std::is_same_v<Unqualified, const char*>) {
        return CppType::String;
    } else if constexpr (std::is_floating_point_v<Unqualified>) {
        if constexpr (sizeof(Unqualified) == sizeof(double)) {
            return CppType::F64;
        } else {
            return CppType::F32;
        }
    } else if constexpr (std::is_integral_v<Unqualified> && std::is_signed_v<Unqualified>) {
        if constexpr (sizeof(Unqualified) == sizeof(int8_t)) {
            return CppType::I8;
        } else if constexpr (sizeof(Unqualified) == sizeof(int16_t)) {
            return CppType::I16;
        } else if constexpr (sizeof(Unqualified) == sizeof(int32_t)) {
            return CppType::I32;
        } else {
            return CppType::I64;
        }
    } else if constexpr (std::is_integral_v<Unqualified> && std::is_unsigned_v<Unqualified>) {
        if constexpr (sizeof(Unqualified) == sizeof(uint8_t)) {
            return CppType::U8;
        } else if constexpr (sizeof(Unqualified) == sizeof(uint16_t)) {
            return CppType::U16;
        } else if constexpr (sizeof(Unqualified) == sizeof(uint32_t)) {
            return CppType::U32;
        } else {
            return CppType::U64;
        }
    } else {
        return CppType::None;
    }
}

template <typename TDerived, typename TValue, size_t AddedMembers>
class SettingValue : public MPackObject<TDerived, SettingValueMembers + AddedMembers> {
  private:
    SettingValue() = default;
    friend TDerived;

  public:
    static void registerMembers() {
        using Obj = MPackObject<TDerived, SettingValueMembers + AddedMembers>;
        Obj::registerMember("id", CppType::String, &TDerived::id);
        Obj::registerMember("unit", CppType::String, &TDerived::unit);
        Obj::registerMember("value", getType<TValue>(), &TDerived::value);
        Obj::registerMember("readonly", CppType::Bool, &TDerived::readonly);
    }

    template <typename T>
    static CppType getType() {
        return settingValueCppType<T>();
    }

    const char* id = nullptr;
    const char* unit = nullptr;
    TValue value;
    bool readonly = false;
};

template <typename TValue>
class MessageSettingOption : public MPackObject<MessageSettingOption<TValue>, 2> {
  public:
    static void registerMembers() {
        MPackObject<MessageSettingOption<TValue>, 2>::registerMember("value", settingValueCppType<TValue>(),
                                                                     &MessageSettingOption::value);
        MPackObject<MessageSettingOption<TValue>, 2>::registerMember("id", CppType::String,
                                                                      &MessageSettingOption::id);
    }

    TValue value{};
    const char* id{};
};

template <typename TValue>
class MessageSettingLimits : public MPackObject<MessageSettingLimits<TValue>, 5> {
  public:
    static void registerMembers() {
        using Obj = MPackObject<MessageSettingLimits<TValue>, 5>;
        Obj::registerOptionalMember("min", settingValueCppType<TValue>(), &MessageSettingLimits::minimum);
        Obj::registerOptionalMember("max", settingValueCppType<TValue>(), &MessageSettingLimits::maximum);
        Obj::registerOptionalMember("step", settingValueCppType<TValue>(), &MessageSettingLimits::step);
        Obj::registerMember("isRange", CppType::Bool, &MessageSettingLimits::isRange);
        Obj::registerMember("options", {CppType::Array, CppType::ObjectPtr}, &MessageSettingLimits::options);
    }

  protected:
    MPackObjectBase* createObject(const char* /*name*/) override {
        return new MessageSettingOption<TValue>();
    }

  public:
    std::optional<TValue> minimum{};
    std::optional<TValue> maximum{};
    std::optional<TValue> step{};
    bool isRange = false;
    MPackArray<MessageSettingOption<TValue>*> options;
};

class BoolSetting : public SettingValue<BoolSetting, bool, 1> {
  public:
    static void registerMembers() {
        using Base = SettingValue<BoolSetting, bool, 1>;
        Base::registerMembers();
        Base::registerMember("limits", CppType::Object, &BoolSetting::limits);
    }

    MessageSettingLimits<bool> limits{};
};

class StringSetting : public SettingValue<StringSetting, const char*, 1> {
  public:
    static void registerMembers() {
        using Base = SettingValue<StringSetting, const char*, 1>;
        Base::registerMembers();
        registerMember("options", {CppType::Array, CppType::ObjectPtr}, &StringSetting::options);
    }

  protected:
    MPackObjectBase* createObject(const char* /*name*/) override {
        return new MessageSettingOption<const char*>();
    }

  public:
    MPackArray<MessageSettingOption<const char*>*> options;
};

template <typename TValue>
class NumberSetting : public SettingValue<NumberSetting<TValue>, TValue, 1> {
  public:
    static void registerMembers() {
        using Base = SettingValue<NumberSetting<TValue>, TValue, 1>;
        Base::registerMembers();
        Base::registerMember("limits", CppType::Object, &NumberSetting::limits);
    }

  public:
    MessageSettingLimits<TValue> limits{};
};
