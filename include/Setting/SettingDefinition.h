#pragma once

#include "ApplyPolicy.h"
#include "Containers/FixedString.h"
#include "Functional/Delegate.h"
#include "PersistencePolicy.h"
#include "SettingKey.h"
#include "SettingLimits.h"
#include "SettingType.h"
#include "SettingValue.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace IntegralMotions::Config {

    inline constexpr uint8_t ModuleIdMaxLength = 64;
    inline constexpr uint8_t GroupIdMaxLength = 64;
    inline constexpr uint8_t SettingIdMaxLength = 64;
    inline constexpr uint8_t UnitMaxLength = 16;

    template <SupportedSettingType T>
    struct SettingDefinition {
        SettingKey key{};
        IntegralMotions::Containers::FixedString<ModuleIdMaxLength> moduleId;
        IntegralMotions::Containers::FixedString<GroupIdMaxLength> groupId;
        IntegralMotions::Containers::FixedString<SettingIdMaxLength> id;
        IntegralMotions::Containers::FixedString<UnitMaxLength> unit;
        T defaultValue{};
        SettingLimits<T> limits{};
        bool readonly = false;
        PersistencePolicy persistencePolicy = PersistencePolicy::Memory;
        ApplyPolicy applyPolicy = ApplyPolicy::Immediate;
        IntegralMotions::Functional::Delegate<void(const T&)> apply{};
    };

    struct SettingOptionSnapshot {
        SettingValue value;
        std::string_view id;
    };

    struct SettingSnapshot {
        SettingKey key{};
        SettingType type{};
        std::string_view moduleId;
        std::string_view groupId;
        std::string_view id;
        std::string_view unit;
        SettingValue value;
        std::optional<SettingValue> minimum;
        std::optional<SettingValue> maximum;
        std::optional<SettingValue> step;
        bool isRange = false;
        std::array<SettingOptionSnapshot, MaxOptions> options{};
        uint8_t optionCount = 0;
        bool readonly = false;
    };

} // namespace IntegralMotions::Config
