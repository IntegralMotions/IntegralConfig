#pragma once

#include "Containers/FixedString.h"
#include "Functional/Delegate.h"
#include "ApplyPolicy.h"
#include "PersistencePolicy.h"
#include "SettingKey.h"
#include "SettingLimits.h"
#include <cstdint>

namespace IntegralMotions::Config {

    inline constexpr uint8_t ModuleNameMaxLength = 64;
    inline constexpr uint8_t GroupNameMaxLength = 64;
    inline constexpr uint8_t UnitNameMaxLength = 16;

    template <SupportedSettingType T>
    struct SettingDefinition {
        SettingKey key{};
        IntegralMotions::Containers::FixedString<ModuleNameMaxLength> module;
        IntegralMotions::Containers::FixedString<GroupNameMaxLength> group;
        IntegralMotions::Containers::FixedString<UnitNameMaxLength> unit;
        IntegralMotions::Containers::FixedString<LabelMaxLength> label;
        T defaultValue{};
        SettingLimits<T> limits{};
        bool readonly = false;
        PersistencePolicy persistencePolicy = PersistencePolicy::Memory;
        ApplyPolicy applyPolicy = ApplyPolicy::Immediate;
        IntegralMotions::Functional::Delegate<void(const T&)> apply{};
    };

} // namespace IntegralMotions::Config
