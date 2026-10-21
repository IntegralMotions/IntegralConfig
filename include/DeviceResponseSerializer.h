#pragma once

#include "DefaultMessagePayloads.h"
#include "Setting/SettingsRegistry.h"
#include "Setting/StoredSettingCodec.h"

#include <array>
#include <concepts>
#include <cstdint>
#include <mpack/mpack.h>
#include <string_view>
#include <type_traits>
#include <variant>

namespace IntegralMotions::Config {

    struct DeviceIdentity {
        const char* model = nullptr;
        const char* firmwareVersion = nullptr;
    };

    template <size_t Capacity>
    class DeviceResponseSerializer {
      public:
        DeviceResponseSerializer(const SettingsRegistry<Capacity>& registry, DeviceIdentity identity)
            : _registry(registry), _identity(identity) {}

        void write(mpack_writer_t& writer) const {
            mpack_start_map(&writer, 3);
            writeKey(writer, "msgType");
            mpack_write_cstr(&writer, "response");
            writeKey(writer, "opCode");
            mpack_write_cstr(&writer, DefaultReadKeys::readDevice);
            writeKey(writer, "payload");
            writeReadDevicePayload(writer);
            mpack_finish_map(&writer);
        }

      private:
        struct SeenNames {
            std::array<std::string_view, Capacity> names{};
            size_t size = 0;

            bool add(std::string_view name) {
                for (size_t i = 0; i < size; ++i) {
                    if (names[i] == name) {
                        return false;
                    }
                }
                names[size++] = name;
                return true;
            }
        };

        static void writeKey(mpack_writer_t& writer, const char* key) {
            mpack_write_cstr(&writer, key);
        }

        static void writeString(mpack_writer_t& writer, std::string_view value) {
            mpack_write_str(&writer, value.data(), value.size());
        }

        static const char* typeName(SettingType type) {
            switch (type) {
            case SettingType::Bool:
                return "bool";
            case SettingType::I8:
                return "i8";
            case SettingType::U8:
                return "u8";
            case SettingType::I16:
                return "i16";
            case SettingType::U16:
                return "u16";
            case SettingType::I32:
                return "i32";
            case SettingType::U32:
                return "u32";
            case SettingType::I64:
                return "i64";
            case SettingType::U64:
                return "u64";
            case SettingType::F32:
                return "f32";
            case SettingType::F64:
                return "f64";
            }
            return "unknown";
        }

        static void writeValue(mpack_writer_t& writer, const SettingValue& value) {
            std::visit(
                [&writer](const auto& item) {
                    using T = std::remove_cvref_t<decltype(item)>;
                    if constexpr (std::same_as<T, std::monostate>) {
                        mpack_write_nil(&writer);
                    } else if constexpr (std::same_as<T, bool>) {
                        mpack_write_bool(&writer, item);
                    } else if constexpr (std::same_as<T, int8_t>) {
                        mpack_write_i8(&writer, item);
                    } else if constexpr (std::same_as<T, uint8_t>) {
                        mpack_write_u8(&writer, item);
                    } else if constexpr (std::same_as<T, int16_t>) {
                        mpack_write_i16(&writer, item);
                    } else if constexpr (std::same_as<T, uint16_t>) {
                        mpack_write_u16(&writer, item);
                    } else if constexpr (std::same_as<T, int32_t>) {
                        mpack_write_i32(&writer, item);
                    } else if constexpr (std::same_as<T, uint32_t>) {
                        mpack_write_u32(&writer, item);
                    } else if constexpr (std::same_as<T, int64_t>) {
                        mpack_write_i64(&writer, item);
                    } else if constexpr (std::same_as<T, uint64_t>) {
                        mpack_write_u64(&writer, item);
                    } else if constexpr (std::same_as<T, float>) {
                        mpack_write_float(&writer, item);
                    } else {
                        mpack_write_double(&writer, item);
                    }
                },
                value);
        }

        static SettingAddress addressOf(const SettingKey& key) {
            std::array<std::byte, StoredSettingCodec::keySize> keyBytes{};
            if (!StoredSettingCodec::packKey(key, keyBytes)) {
                return 0;
            }
            return static_cast<SettingAddress>(std::to_integer<uint8_t>(keyBytes[0])) |
                   (static_cast<SettingAddress>(std::to_integer<uint8_t>(keyBytes[1])) << 8U) |
                   (static_cast<SettingAddress>(std::to_integer<uint8_t>(keyBytes[2])) << 16U);
        }

        size_t moduleCount() const {
            SeenNames seen;
            _registry.visit(
                [](void* context, const SettingSnapshot& setting) {
                    static_cast<SeenNames*>(context)->add(setting.moduleId);
                    return true;
                },
                &seen);
            return seen.size;
        }

        size_t groupCount(std::string_view moduleId) const {
            struct Context {
                std::string_view moduleId;
                SeenNames seen;
            } context{.moduleId = moduleId};
            _registry.visit(
                [](void* rawContext, const SettingSnapshot& setting) {
                    auto& context = *static_cast<Context*>(rawContext);
                    if (setting.moduleId == context.moduleId) {
                        context.seen.add(setting.groupId);
                    }
                    return true;
                },
                &context);
            return context.seen.size;
        }

        size_t settingCount(std::string_view moduleId, std::string_view groupId) const {
            struct Context {
                std::string_view moduleId;
                std::string_view groupId;
                size_t count = 0;
            } context{.moduleId = moduleId, .groupId = groupId};
            _registry.visit(
                [](void* rawContext, const SettingSnapshot& setting) {
                    auto& context = *static_cast<Context*>(rawContext);
                    context.count += setting.moduleId == context.moduleId && setting.groupId == context.groupId;
                    return true;
                },
                &context);
            return context.count;
        }

        void writeReadDevicePayload(mpack_writer_t& writer) const {
            mpack_start_map(&writer, 2);
            writeKey(writer, "deviceInfo");
            mpack_start_map(&writer, 2);
            writeKey(writer, "model");
            _identity.model == nullptr ? mpack_write_nil(&writer) : mpack_write_cstr(&writer, _identity.model);
            writeKey(writer, "firmwareVersion");
            _identity.firmwareVersion == nullptr ? mpack_write_nil(&writer)
                                                 : mpack_write_cstr(&writer, _identity.firmwareVersion);
            mpack_finish_map(&writer);
            writeKey(writer, "modules");
            mpack_start_array(&writer, moduleCount());
            struct Context {
                const DeviceResponseSerializer* serializer;
                mpack_writer_t* writer;
                SeenNames seen;
            } context{.serializer = this, .writer = &writer};
            _registry.visit(
                [](void* rawContext, const SettingSnapshot& setting) {
                    auto& context = *static_cast<Context*>(rawContext);
                    if (context.seen.add(setting.moduleId)) {
                        context.serializer->writeModule(*context.writer, setting);
                    }
                    return true;
                },
                &context);
            mpack_finish_array(&writer);
            mpack_finish_map(&writer);
        }

        void writeModule(mpack_writer_t& writer, const SettingSnapshot& module) const {
            mpack_start_map(&writer, 2);
            writeKey(writer, "id");
            writeString(writer, module.moduleId);
            writeKey(writer, "groups");
            mpack_start_array(&writer, groupCount(module.moduleId));
            struct Context {
                const DeviceResponseSerializer* serializer;
                mpack_writer_t* writer;
                std::string_view moduleId;
                SeenNames seen;
            } context{.serializer = this, .writer = &writer, .moduleId = module.moduleId};
            _registry.visit(
                [](void* rawContext, const SettingSnapshot& setting) {
                    auto& context = *static_cast<Context*>(rawContext);
                    if (setting.moduleId == context.moduleId && context.seen.add(setting.groupId)) {
                        context.serializer->writeGroup(*context.writer, setting);
                    }
                    return true;
                },
                &context);
            mpack_finish_array(&writer);
            mpack_finish_map(&writer);
        }

        void writeGroup(mpack_writer_t& writer, const SettingSnapshot& group) const {
            mpack_start_map(&writer, 2);
            writeKey(writer, "id");
            writeString(writer, group.groupId);
            writeKey(writer, "settings");
            mpack_start_array(&writer, settingCount(group.moduleId, group.groupId));
            struct Context {
                const DeviceResponseSerializer* serializer;
                mpack_writer_t* writer{};
                std::string_view moduleId;
                std::string_view groupId;
            } context{.serializer = this, .writer = &writer, .moduleId = group.moduleId, .groupId = group.groupId};
            _registry.visit(
                [](void* rawContext, const SettingSnapshot& setting) {
                    const auto& context = *static_cast<Context*>(rawContext);
                    if (setting.moduleId == context.moduleId && setting.groupId == context.groupId) {
                        context.serializer->writeSetting(*context.writer, setting);
                    }
                    return true;
                },
                &context);
            mpack_finish_array(&writer);
            mpack_finish_map(&writer);
        }

        void writeSetting(mpack_writer_t& writer, const SettingSnapshot& setting) const {
            mpack_start_map(&writer, 2);
            writeKey(writer, "type");
            writeKey(writer, typeName(setting.type));
            writeKey(writer, "value");
            const bool numeric = setting.type != SettingType::Bool;
            const size_t settingMemberCount = 4 + !setting.unit.empty() + setting.readonly;
            mpack_start_map(&writer, settingMemberCount);
            writeKey(writer, "address");
            mpack_write_u32(&writer, addressOf(setting.key));
            writeKey(writer, "id");
            writeString(writer, setting.id);
            if (!setting.unit.empty()) {
                writeKey(writer, "unit");
                writeString(writer, setting.unit);
            }
            writeKey(writer, "value");
            writeValue(writer, setting.value);
            if (setting.readonly) {
                writeKey(writer, "readonly");
                mpack_write_bool(&writer, true);
            }
            writeKey(writer, "limits");
            const size_t limitMemberCount = 2 + (numeric && setting.minimum.has_value()) +
                                            (numeric && setting.maximum.has_value()) +
                                            (numeric && setting.step.has_value());
            mpack_start_map(&writer, limitMemberCount);
            if (numeric) {
                if (setting.minimum.has_value()) {
                    writeKey(writer, "min");
                    writeValue(writer, *setting.minimum);
                }
                if (setting.maximum.has_value()) {
                    writeKey(writer, "max");
                    writeValue(writer, *setting.maximum);
                }
                if (setting.step.has_value()) {
                    writeKey(writer, "step");
                    writeValue(writer, *setting.step);
                }
            }
            writeKey(writer, "isRange");
            mpack_write_bool(&writer, setting.isRange);
            writeKey(writer, "options");
            mpack_start_array(&writer, setting.optionCount);
            for (uint8_t i = 0; i < setting.optionCount; ++i) {
                mpack_start_map(&writer, 2);
                writeKey(writer, "value");
                writeValue(writer, setting.options[i].value);
                writeKey(writer, "id");
                writeString(writer, setting.options[i].id);
                mpack_finish_map(&writer);
            }
            mpack_finish_array(&writer);
            mpack_finish_map(&writer);
            mpack_finish_map(&writer);
            mpack_finish_map(&writer);
        }

        const SettingsRegistry<Capacity>& _registry;
        DeviceIdentity _identity;
    };

} // namespace IntegralMotions::Config
