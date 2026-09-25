#include "Setting/StoredSettingCodec.h"

#include "Math/CRC.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <type_traits>
#include <variant>

namespace IntegralMotions::Config {
    namespace {
        inline constexpr uint8_t BitsInByte = 8U;

        template <typename T>
        void writeLittleEndian(std::span<std::byte> destination, size_t offset, T value) {
            static_assert(std::is_unsigned_v<T>);

            for (size_t i = 0; i < sizeof(T); ++i) {
                destination[offset + i] = static_cast<std::byte>(value >> (i * BitsInByte));
            }
        }

        template <typename T>
        T readLittleEndian(std::span<const std::byte> source, size_t offset) {
            static_assert(std::is_unsigned_v<T>);

            T value = 0;
            for (size_t i = 0; i < sizeof(T); ++i) {
                value |= static_cast<T>(std::to_integer<uint8_t>(source[offset + i])) << (i * BitsInByte);
            }
            return value;
        }

        template <typename T>
        void writePayload(std::span<std::byte> destination, size_t offset, const T& value) {
            if constexpr (std::is_same_v<T, bool>) {
                writeLittleEndian<uint8_t>(destination, offset, value ? 1U : 0U);
            } else if constexpr (std::is_floating_point_v<T>) {
                if constexpr (sizeof(T) == sizeof(float)) {
                    writeLittleEndian<uint32_t>(destination, offset, std::bit_cast<uint32_t>(value));
                } else {
                    writeLittleEndian<uint64_t>(destination, offset, std::bit_cast<uint64_t>(value));
                }
            } else {
                using Unsigned = std::make_unsigned_t<T>;
                writeLittleEndian<Unsigned>(destination, offset, std::bit_cast<Unsigned>(value));
            }
        }

        template <typename T>
        T readPayload(std::span<const std::byte> source, size_t offset) {
            if constexpr (std::is_floating_point_v<T>) {
                if constexpr (sizeof(T) == sizeof(float)) {
                    return std::bit_cast<float>(readLittleEndian<uint32_t>(source, offset));
                } else {
                    return std::bit_cast<double>(readLittleEndian<uint64_t>(source, offset));
                }
            } else {
                using Unsigned = std::make_unsigned_t<T>;
                return std::bit_cast<T>(readLittleEndian<Unsigned>(source, offset));
            }
        }

        bool typeOfValue(const SettingValue& value, SettingType& type) {
            return std::visit(
                [&type](const auto& storedValue) {
                    using ValueType = std::remove_cvref_t<decltype(storedValue)>;
                    if constexpr (std::is_same_v<ValueType, std::monostate>) {
                        return false;
                    } else {
                        type = settingTypeOf<ValueType>();
                        return true;
                    }
                },
                value);
        }
    } // namespace

    size_t StoredSettingCodec::payloadSize(SettingType type) {
        switch (type) {
        case SettingType::Bool:
        case SettingType::I8:
        case SettingType::U8:
            return sizeof(int8_t);
        case SettingType::I16:
        case SettingType::U16:
            return sizeof(int16_t);
        case SettingType::I32:
        case SettingType::U32:
        case SettingType::F32:
            return sizeof(int32_t);
        case SettingType::I64:
        case SettingType::U64:
        case SettingType::F64:
            return sizeof(int64_t);
        }

        return 0;
    }

    size_t StoredSettingCodec::encodedSize(SettingType type) {
        const size_t payload = payloadSize(type);
        return payload == 0 ? 0 : headerSize + payload + crcSize;
    }

    size_t StoredSettingCodec::storedSize(SettingType type, size_t writeAlignment) {
        const size_t size = encodedSize(type);
        if (size == 0 || writeAlignment == 0) {
            return 0;
        }

        const size_t remainder = size % writeAlignment;
        return remainder == 0 ? size : size + writeAlignment - remainder;
    }

    size_t StoredSettingCodec::encodedSize(const SettingValue& value) {
        return std::visit(
            [](const auto& storedValue) -> size_t {
                using ValueType = std::remove_cvref_t<decltype(storedValue)>;
                if constexpr (std::is_same_v<ValueType, std::monostate>) {
                    return 0;
                } else {
                    return encodedSize(settingTypeOf<ValueType>());
                }
            },
            value);
    }

    size_t StoredSettingCodec::storedSize(const SettingValue& value, size_t writeAlignment) {
        return std::visit(
            [writeAlignment](const auto& storedValue) -> size_t {
                using ValueType = std::remove_cvref_t<decltype(storedValue)>;
                if constexpr (std::is_same_v<ValueType, std::monostate>) {
                    return 0;
                } else {
                    return storedSize(settingTypeOf<ValueType>(), writeAlignment);
                }
            },
            value);
    }

    StoredSettingResult StoredSettingCodec::decodeHeader(std::span<const std::byte> source, SettingType& type) {
        if (source.size() < headerSize || std::to_integer<uint8_t>(source[0]) != recordMagic) {
            return StoredSettingResult::CorruptData;
        }

        const auto typeAndVersion = std::to_integer<uint8_t>(source[magicSize]);
        if ((typeAndVersion >> versionShift) != recordVersion) {
            return StoredSettingResult::UnsupportedVersion;
        }

        const auto typeValue = static_cast<uint8_t>(typeAndVersion & typeMask);
        if (typeValue > static_cast<uint8_t>(SettingType::F64)) {
            return StoredSettingResult::UnsupportedVersion;
        }

        type = static_cast<SettingType>(typeValue);
        return StoredSettingResult::Ok;
    }

    bool StoredSettingCodec::packKey(const SettingKey& key, std::span<std::byte, keySize> destination) {
        constexpr uint8_t lower8BitsMask = 0xFFU;
        constexpr uint8_t upper8BitsMask = 8U;
        constexpr uint8_t instanceMask = 0x3F;
        constexpr uint8_t scopeMask = 0x03;
        constexpr uint8_t scopeShift = 6;

        const auto identifier = static_cast<uint16_t>(key.id);
        const auto scope = static_cast<uint8_t>(key.scope);
        if (key.instance > instanceMask || scope > scopeMask) {
            return false;
        }

        destination[0] = static_cast<std::byte>(identifier & lower8BitsMask);
        destination[1] = static_cast<std::byte>(identifier >> upper8BitsMask);
        destination[2] = static_cast<std::byte>(key.instance | (scope << scopeShift));
        return true;
    }

    bool StoredSettingCodec::unpackKey(std::span<const std::byte, keySize> source, SettingKey& key) {
        constexpr uint8_t instanceMask = 0x3F;
        constexpr uint8_t highByteMask = 8U;
        constexpr uint8_t scopeShift = 6;

        const auto lowByte = std::to_integer<uint8_t>(source[0]);
        const auto highByte = std::to_integer<uint8_t>(source[1]);
        const auto packedScopeInstance = std::to_integer<uint8_t>(source[2]);

        key.id =
            static_cast<SettingId>(static_cast<uint16_t>(lowByte) | (static_cast<uint16_t>(highByte) << highByteMask));
        key.scope = static_cast<SettingScope>(packedScopeInstance >> scopeShift);
        key.instance = packedScopeInstance & instanceMask;
        return true;
    }

    StoredSettingEncodeResult StoredSettingCodec::encode(const SettingKey& key, const SettingValue& value,
                                                         size_t writeAlignment, std::byte erasedValue,
                                                         std::span<std::byte> destination) {
        SettingType type{};
        if (!typeOfValue(value, type) || writeAlignment == 0) {
            return {.result = StoredSettingResult::InvalidValue};
        }

        const size_t encodedRecordSize = encodedSize(type);
        const size_t alignedRecordSize = storedSize(type, writeAlignment);
        if (destination.size() < alignedRecordSize) {
            return {.result = StoredSettingResult::BufferTooSmall};
        }

        auto record = destination.first(alignedRecordSize);
        std::ranges::fill(record, erasedValue);
        record[0] = static_cast<std::byte>(recordMagic);

        record[magicSize] = static_cast<std::byte>(static_cast<uint8_t>(type) | (recordVersion << versionShift));

        auto keyBytes = std::span<std::byte, keySize>{record.data() + magicSize + typeSize, keySize};
        if (!packKey(key, keyBytes)) {
            return {.result = StoredSettingResult::InvalidValue};
        }

        std::visit(
            [&record](const auto& storedValue) {
                using ValueType = std::remove_cvref_t<decltype(storedValue)>;
                if constexpr (!std::is_same_v<ValueType, std::monostate>) {
                    writePayload(record, headerSize, storedValue);
                }
            },
            value);

        const size_t crcOffset = encodedRecordSize - crcSize;
        const auto crc =
            IntegralMotions::Math::CRC::calculate(reinterpret_cast<const uint8_t*>(record.data()), crcOffset);
        writeLittleEndian<uint16_t>(record, crcOffset, crc);

        return {.result = StoredSettingResult::Ok, .size = alignedRecordSize};
    }

    StoredSettingResult StoredSettingCodec::decode(std::span<const std::byte> source, SettingKey& key,
                                                   SettingValue& value) {
        SettingType type{};
        const auto headerResult = decodeHeader(source, type);
        if (headerResult != StoredSettingResult::Ok) {
            return headerResult;
        }

        const size_t encodedRecordSize = encodedSize(type);
        if (source.size() < encodedRecordSize) {
            return StoredSettingResult::CorruptData;
        }

        const size_t crcOffset = encodedRecordSize - crcSize;
        const auto crc = readLittleEndian<uint16_t>(source, crcOffset);

        if (!IntegralMotions::Math::CRC::validate(reinterpret_cast<const uint8_t*>(source.data()), crcOffset, crc)) {
            return StoredSettingResult::CorruptData;
        }

        const auto keyBytes = std::span<const std::byte, keySize>{source.data() + magicSize + typeSize, keySize};
        if (!unpackKey(keyBytes, key)) {
            return StoredSettingResult::CorruptData;
        }

        if (static_cast<uint8_t>(key.scope) > static_cast<uint8_t>(SettingScope::Motor)) {
            return StoredSettingResult::InvalidValue;
        }

        switch (type) {
        case SettingType::Bool: {
            const auto rawValue = readLittleEndian<uint8_t>(source, headerSize);
            if (rawValue > 1) {
                return StoredSettingResult::InvalidValue;
            }
            value = rawValue != 0;
        } break;
        case SettingType::I8:
            value = readPayload<int8_t>(source, headerSize);
            break;
        case SettingType::U8:
            value = readPayload<uint8_t>(source, headerSize);
            break;
        case SettingType::I16:
            value = readPayload<int16_t>(source, headerSize);
            break;
        case SettingType::U16:
            value = readPayload<uint16_t>(source, headerSize);
            break;
        case SettingType::I32:
            value = readPayload<int32_t>(source, headerSize);
            break;
        case SettingType::U32:
            value = readPayload<uint32_t>(source, headerSize);
            break;
        case SettingType::I64:
            value = readPayload<int64_t>(source, headerSize);
            break;
        case SettingType::U64:
            value = readPayload<uint64_t>(source, headerSize);
            break;
        case SettingType::F32:
            value = readPayload<float>(source, headerSize);
            break;
        case SettingType::F64:
            value = readPayload<double>(source, headerSize);
            break;
        }

        
        return StoredSettingResult::Ok;
    }

} // namespace IntegralMotions::Config
