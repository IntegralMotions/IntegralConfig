#pragma once

#include "../FixedString.h"
#include "PersistencePolicy.h"
#include "SettingKey.h"
#include "SettingLimits.h"
#include <cstdint>

namespace IntegralMotions::Config {

    inline constexpr uint8_t ModuleNameMaxLength = 64;
    inline constexpr uint8_t GroupNameMaxLength = 64;
    inline constexpr uint8_t UnitNameMaxLength = 16;
    inline constexpr uint8_t LabelMaxLength = 64;

    template <SupportedSettingType T>
    struct SettingDefinition {
        SettingKey key{};
        FixedString<ModuleNameMaxLength> module;
        FixedString<GroupNameMaxLength> group;
        FixedString<UnitNameMaxLength> unit;
        FixedString<LabelMaxLength> label;
        T defaultValue{};
        SettingLimits<T> limits{};
        bool readonly = false;
        PersistencePolicy persistencePolicy = PersistencePolicy::Memory;
    };

} // namespace IntegralMotions::Config
