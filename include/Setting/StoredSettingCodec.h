#pragma once

#include "SettingKey.h"
#include "SettingType.h"
#include "SettingValue.h"
#include "StoredSettingEncodeResult.h"
#include "StoredSettingResult.h"
#include <cstddef>
#include <span>
namespace IntegralMotions::Config {

    class StoredSettingCodec {
      public:
        static constexpr size_t magicSize = 1;
        static constexpr size_t keySize = 3;
        static constexpr size_t typeSize = 1;
        static constexpr size_t headerSize = magicSize + keySize + typeSize;
        static constexpr size_t crcSize = 2;
        static constexpr uint8_t recordMagic = 0xA5;
        static constexpr uint8_t recordVersion = 0;
        static constexpr uint8_t typeMask = 0x0F;
        static constexpr uint8_t versionShift = 4;

        static size_t payloadSize(SettingType type);
        static size_t encodedSize(SettingType type);
        static size_t storedSize(SettingType type, size_t writeAlignment);

        static size_t encodedSize(const SettingValue& value);
        static size_t storedSize(const SettingValue& value, size_t writeAlignment);

        static StoredSettingResult decodeHeader(std::span<const std::byte> source, SettingType& type);

        static bool packKey(const SettingKey& key, std::span<std::byte, keySize> destination);
        static bool unpackKey(std::span<const std::byte, keySize> source, SettingKey& key);

        static StoredSettingEncodeResult encode(const SettingKey& key, const SettingValue& value, size_t writeAlignment,
                                                std::byte erasedValue, std::span<std::byte> destination);

        static StoredSettingResult decode(std::span<const std::byte> source, SettingKey& key, SettingValue& value);
    };

} // namespace IntegralMotions::Config
