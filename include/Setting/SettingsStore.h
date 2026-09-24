#pragma once

#include "Math/CRC.h"
#include "SettingKey.h"
#include "SettingStoreResult.h"
#include "SettingValue.h"
#include "StorageDevice.h"
#include "StorageRegion.h"
#include "StoredSettingCodec.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>

namespace IntegralMotions::Config {

    template <size_t MaxSettings>
    class SettingsStore {
      public:
        SettingsStore(StorageDevice& storage, StorageRegion bankA, StorageRegion bankB);

        SettingsStoreResult open();
        SettingsStoreResult store(const SettingKey& key, const SettingValue& value);
        SettingsStoreResult load(const SettingKey& key, SettingValue& value) const;

      private:
        static constexpr uint8_t bankMagic = 0xB6;
        static constexpr uint8_t bankVersion = 0;
        static constexpr size_t bankHeaderSizeUnaligned = 8;
        static constexpr size_t maximumIoSize = 256;

        struct BankHeader {
            uint32_t generation = 0;
        };

        struct IndexEntry {
            bool used = false;
            SettingKey key{};
            size_t recordOffset = 0;
            size_t recordSize = 0;
        };

        [[nodiscard]] size_t alignment() const;
        [[nodiscard]] size_t bankHeaderSize() const;
        [[nodiscard]] bool validRegions() const;
        [[nodiscard]] StorageRegion inactiveBank() const;

        SettingsStoreResult readBankHeader(StorageRegion bank, BankHeader& header) const;
        SettingsStoreResult writeBankHeader(StorageRegion bank, uint32_t generation);
        SettingsStoreResult clearBank(StorageRegion bank);
        SettingsStoreResult scanActiveBank();
        SettingsStoreResult compact();
        SettingsStoreResult append(const SettingKey& key, const SettingValue& value);
        SettingsStoreResult updateIndex(const SettingKey& key, size_t recordOffset, size_t recordSize);

        IndexEntry* findFreeIndex();
        IndexEntry* findIndex(const SettingKey& key);
        const IndexEntry* findIndex(const SettingKey& key) const;

        static SettingsStoreResult mapCodecResult(StoredSettingResult result);
        static bool generationIsNewer(uint32_t lhs, uint32_t rhs);

        StorageDevice& _storage;
        StorageRegion _bankA;
        StorageRegion _bankB;
        StorageRegion _activeBank{};
        std::array<IndexEntry, MaxSettings> _indices{};
        size_t _nextWriteOffset = 0;
        uint32_t _generation = 0;
        bool _open = false;
    };

    template <size_t MaxSettings>
    SettingsStore<MaxSettings>::SettingsStore(StorageDevice& storage, StorageRegion bankA, StorageRegion bankB)
        : _storage(storage), _bankA(bankA), _bankB(bankB) {}

    template <size_t MaxSettings>
    size_t SettingsStore<MaxSettings>::alignment() const {
        const auto& geometry = _storage.geometry();
        if (geometry.readAlignment == 0 || geometry.writeAlignment == 0) {
            return 0;
        }
        return std::lcm(geometry.readAlignment, geometry.writeAlignment);
    }

    template <size_t MaxSettings>
    size_t SettingsStore<MaxSettings>::bankHeaderSize() const {
        const size_t requiredAlignment = alignment();
        if (requiredAlignment == 0) {
            return 0;
        }
        const size_t remainder = bankHeaderSizeUnaligned % requiredAlignment;
        return remainder == 0 ? bankHeaderSizeUnaligned : bankHeaderSizeUnaligned + requiredAlignment - remainder;
    }

    template <size_t MaxSettings>
    bool SettingsStore<MaxSettings>::validRegions() const {
        const auto& geometry = _storage.geometry();
        const size_t requiredAlignment = alignment();
        const size_t storedHeaderSize = bankHeaderSize();
        if (requiredAlignment == 0 || requiredAlignment > maximumIoSize || storedHeaderSize > maximumIoSize) {
            return false;
        }

        const auto validBank = [&](StorageRegion bank) {
            return bank.offset <= geometry.capacity && bank.size <= geometry.capacity - bank.offset &&
                   bank.offset % requiredAlignment == 0 && bank.size % requiredAlignment == 0 &&
                   bank.size >= storedHeaderSize + requiredAlignment;
        };
        if (!validBank(_bankA) || !validBank(_bankB)) {
            return false;
        }

        const size_t endA = _bankA.offset + _bankA.size;
        const size_t endB = _bankB.offset + _bankB.size;
        return endA <= _bankB.offset || endB <= _bankA.offset;
    }

    template <size_t MaxSettings>
    StorageRegion SettingsStore<MaxSettings>::inactiveBank() const {
        return _activeBank.offset == _bankA.offset ? _bankB : _bankA;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::readBankHeader(StorageRegion bank, BankHeader& header) const {
        const size_t storedHeaderSize = bankHeaderSize();
        std::array<std::byte, maximumIoSize> bytes{};
        if (_storage.read(bank.offset, std::span<std::byte>{bytes.data(), storedHeaderSize}) != StorageResult::Ok) {
            return SettingsStoreResult::StorageError;
        }

        if (std::to_integer<uint8_t>(bytes[0]) != bankMagic) {
            return SettingsStoreResult::CorruptData;
        }
        if (std::to_integer<uint8_t>(bytes[1]) != bankVersion) {
            return SettingsStoreResult::UnsupportedVersion;
        }

        header.generation = static_cast<uint32_t>(std::to_integer<uint8_t>(bytes[2])) |
                            (static_cast<uint32_t>(std::to_integer<uint8_t>(bytes[3])) << 8U) |
                            (static_cast<uint32_t>(std::to_integer<uint8_t>(bytes[4])) << 16U) |
                            (static_cast<uint32_t>(std::to_integer<uint8_t>(bytes[5])) << 24U);
        const uint16_t storedCrc = static_cast<uint16_t>(std::to_integer<uint8_t>(bytes[6])) |
                                   (static_cast<uint16_t>(std::to_integer<uint8_t>(bytes[7])) << 8U);
        if (!IntegralMotions::Math::CRC::validate(reinterpret_cast<const uint8_t*>(bytes.data()),
                                                  bankHeaderSizeUnaligned - 2, storedCrc)) {
            return SettingsStoreResult::CorruptData;
        }

        return SettingsStoreResult::Ok;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::writeBankHeader(StorageRegion bank, uint32_t generation) {
        const size_t storedHeaderSize = bankHeaderSize();
        std::array<std::byte, maximumIoSize> bytes{};
        std::fill_n(bytes.begin(), storedHeaderSize, _storage.geometry().erasedValue);
        bytes[0] = static_cast<std::byte>(bankMagic);
        bytes[1] = static_cast<std::byte>(bankVersion);
        for (size_t i = 0; i < sizeof(generation); ++i) {
            bytes[2 + i] = static_cast<std::byte>(generation >> (i * 8U));
        }
        const uint16_t crc = IntegralMotions::Math::CRC::calculate(reinterpret_cast<const uint8_t*>(bytes.data()), 6);
        bytes[6] = static_cast<std::byte>(crc & 0xFFU);
        bytes[7] = static_cast<std::byte>(crc >> 8U);

        if (_storage.write(bank.offset, std::span<const std::byte>{bytes.data(), storedHeaderSize}) !=
                StorageResult::Ok ||
            _storage.flush() != StorageResult::Ok) {
            return SettingsStoreResult::StorageError;
        }
        return SettingsStoreResult::Ok;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::clearBank(StorageRegion bank) {
        const auto& geometry = _storage.geometry();
        if (geometry.eraseBlockSize != 0) {
            if (bank.offset % geometry.eraseBlockSize != 0 || bank.size % geometry.eraseBlockSize != 0) {
                return SettingsStoreResult::InvalidArgument;
            }
            if (_storage.erase(bank.offset, bank.size) != StorageResult::Ok || _storage.flush() != StorageResult::Ok) {
                return SettingsStoreResult::StorageError;
            }
            return SettingsStoreResult::Ok;
        }

        const size_t chunkSize = maximumIoSize - (maximumIoSize % geometry.writeAlignment);
        if (chunkSize == 0) {
            return SettingsStoreResult::InvalidArgument;
        }
        std::array<std::byte, maximumIoSize> erased{};
        erased.fill(geometry.erasedValue);
        size_t offset = bank.offset;
        size_t remaining = bank.size;
        while (remaining != 0) {
            const size_t size = std::min(chunkSize, remaining);
            if (_storage.write(offset, std::span<const std::byte>{erased.data(), size}) != StorageResult::Ok) {
                return SettingsStoreResult::StorageError;
            }
            offset += size;
            remaining -= size;
        }
        return _storage.flush() == StorageResult::Ok ? SettingsStoreResult::Ok : SettingsStoreResult::StorageError;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::open() {
        _open = false;
        _indices = {};
        if (!validRegions()) {
            return SettingsStoreResult::InvalidArgument;
        }

        BankHeader headerA{};
        BankHeader headerB{};
        auto resultA = readBankHeader(_bankA, headerA);
        if (resultA != SettingsStoreResult::Ok && resultA != SettingsStoreResult::CorruptData) {
            return resultA;
        }
        const auto resultB = readBankHeader(_bankB, headerB);
        if (resultB != SettingsStoreResult::Ok && resultB != SettingsStoreResult::CorruptData) {
            return resultB;
        }
        const bool validA = resultA == SettingsStoreResult::Ok;
        const bool validB = resultB == SettingsStoreResult::Ok;

        if (!validA && !validB) {
            auto result = clearBank(_bankA);
            if (result != SettingsStoreResult::Ok) {
                return result;
            }
            result = writeBankHeader(_bankA, 0);
            if (result != SettingsStoreResult::Ok) {
                return result;
            }
            _activeBank = _bankA;
            _generation = 0;
        } else if (validA && (!validB || generationIsNewer(headerA.generation, headerB.generation))) {
            _activeBank = _bankA;
            _generation = headerA.generation;
        } else {
            _activeBank = _bankB;
            _generation = headerB.generation;
        }

        const auto result = scanActiveBank();
        if (result == SettingsStoreResult::Ok) {
            _open = true;
        }
        return result;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::scanActiveBank() {
        _indices = {};
        _nextWriteOffset = _activeBank.offset + bankHeaderSize();
        const size_t bankEnd = _activeBank.offset + _activeBank.size;
        const size_t readAlignment = _storage.geometry().readAlignment;
        const size_t prefixRemainder = StoredSettingCodec::headerSize % readAlignment;
        const size_t prefixSize = prefixRemainder == 0
                                      ? StoredSettingCodec::headerSize
                                      : StoredSettingCodec::headerSize + readAlignment - prefixRemainder;
        if (prefixSize > maximumIoSize) {
            return SettingsStoreResult::InvalidArgument;
        }

        std::array<std::byte, maximumIoSize> bytes{};
        while (_nextWriteOffset + prefixSize <= bankEnd) {
            auto prefix = std::span<std::byte>{bytes.data(), prefixSize};
            if (_storage.read(_nextWriteOffset, prefix) != StorageResult::Ok) {
                return SettingsStoreResult::StorageError;
            }

            const auto erasedValue = _storage.geometry().erasedValue;
            const bool erasedMagic = bytes[0] == erasedValue;
            const bool erasedType = bytes[StoredSettingCodec::magicSize] == erasedValue;
            if (erasedMagic && erasedType) {
                return SettingsStoreResult::Ok;
            }
            if (erasedMagic) {
                bytes[0] = static_cast<std::byte>(StoredSettingCodec::recordMagic);
            } else if (std::to_integer<uint8_t>(bytes[0]) != StoredSettingCodec::recordMagic) {
                return SettingsStoreResult::CorruptData;
            }

            SettingType type{};
            const auto headerResult = StoredSettingCodec::decodeHeader(prefix, type);
            if (headerResult != StoredSettingResult::Ok) {
                return mapCodecResult(headerResult);
            }

            const size_t recordSize = StoredSettingCodec::storedSize(type, alignment());
            if (recordSize == 0 || recordSize > maximumIoSize || recordSize > bankEnd - _nextWriteOffset) {
                return SettingsStoreResult::CorruptData;
            }
            auto record = std::span<std::byte>{bytes.data(), recordSize};
            if (_storage.read(_nextWriteOffset, record) != StorageResult::Ok) {
                return SettingsStoreResult::StorageError;
            }

            SettingKey key{};
            SettingValue value{};
            const auto decodeResult = StoredSettingCodec::decode(record, key, value);
            if (decodeResult == StoredSettingResult::Ok) {
                const auto indexResult = updateIndex(key, _nextWriteOffset, recordSize);
                if (indexResult != SettingsStoreResult::Ok) {
                    return indexResult;
                }
            } else if (decodeResult != StoredSettingResult::CorruptData) {
                return mapCodecResult(decodeResult);
            }
            _nextWriteOffset += recordSize;
        }
        return SettingsStoreResult::Ok;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::append(const SettingKey& key, const SettingValue& value) {
        const size_t recordSize = StoredSettingCodec::storedSize(value, alignment());
        const size_t bankEnd = _activeBank.offset + _activeBank.size;
        if (recordSize == 0 || recordSize > maximumIoSize) {
            return SettingsStoreResult::InvalidArgument;
        }
        if (_nextWriteOffset > bankEnd || recordSize > bankEnd - _nextWriteOffset) {
            return SettingsStoreResult::NoSpace;
        }
        if (findIndex(key) == nullptr && findFreeIndex() == nullptr) {
            return SettingsStoreResult::NoSpace;
        }

        std::array<std::byte, maximumIoSize> bytes{};
        const auto encoded = StoredSettingCodec::encode(key, value, alignment(), _storage.geometry().erasedValue,
                                                        std::span<std::byte>{bytes.data(), recordSize});
        if (encoded.result != StoredSettingResult::Ok) {
            return mapCodecResult(encoded.result);
        }

        const size_t writeAlignment = _storage.geometry().writeAlignment;
        const size_t prefixSize = std::max<size_t>(2, writeAlignment);
        const size_t prefixRemainder = prefixSize % writeAlignment;
        const size_t alignedPrefixSize =
            prefixRemainder == 0 ? prefixSize : prefixSize + writeAlignment - prefixRemainder;
        std::array<std::byte, maximumIoSize> prefix{};
        prefix.fill(_storage.geometry().erasedValue);
        prefix[StoredSettingCodec::magicSize] = bytes[StoredSettingCodec::magicSize];
        if (_storage.write(_nextWriteOffset, std::span<const std::byte>{prefix.data(), alignedPrefixSize}) !=
                StorageResult::Ok ||
            _storage.write(_nextWriteOffset, std::span<const std::byte>{bytes.data(), encoded.size}) !=
                StorageResult::Ok ||
            _storage.flush() != StorageResult::Ok) {
            return SettingsStoreResult::StorageError;
        }

        const auto result = updateIndex(key, _nextWriteOffset, encoded.size);
        if (result == SettingsStoreResult::Ok) {
            _nextWriteOffset += encoded.size;
        }
        return result;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::store(const SettingKey& key, const SettingValue& value) {
        if (!_open) {
            return SettingsStoreResult::NotOpen;
        }
        auto result = append(key, value);
        if (result != SettingsStoreResult::NoSpace) {
            return result;
        }
        result = compact();
        return result == SettingsStoreResult::Ok ? append(key, value) : result;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::load(const SettingKey& key, SettingValue& value) const {
        if (!_open) {
            return SettingsStoreResult::NotOpen;
        }
        const auto* index = findIndex(key);
        if (index == nullptr) {
            return SettingsStoreResult::NotFound;
        }

        std::array<std::byte, maximumIoSize> bytes{};
        auto record = std::span<std::byte>{bytes.data(), index->recordSize};
        if (_storage.read(index->recordOffset, record) != StorageResult::Ok) {
            return SettingsStoreResult::StorageError;
        }
        SettingKey decodedKey{};
        const auto decodeResult = StoredSettingCodec::decode(record, decodedKey, value);
        if (decodeResult != StoredSettingResult::Ok) {
            return mapCodecResult(decodeResult);
        }
        return decodedKey == key ? SettingsStoreResult::Ok : SettingsStoreResult::CorruptData;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::compact() {
        const StorageRegion destination = inactiveBank();
        auto result = clearBank(destination);
        if (result != SettingsStoreResult::Ok) {
            return result;
        }

        size_t destinationOffset = destination.offset + bankHeaderSize();
        const size_t destinationEnd = destination.offset + destination.size;
        std::array<std::byte, maximumIoSize> bytes{};
        for (const auto& index : _indices) {
            if (!index.used) {
                continue;
            }
            if (destinationOffset > destinationEnd || index.recordSize > destinationEnd - destinationOffset) {
                return SettingsStoreResult::NoSpace;
            }
            auto record = std::span<std::byte>{bytes.data(), index.recordSize};
            if (_storage.read(index.recordOffset, record) != StorageResult::Ok ||
                _storage.write(destinationOffset, std::span<const std::byte>{record.data(), record.size()}) !=
                    StorageResult::Ok ||
                _storage.flush() != StorageResult::Ok) {
                return SettingsStoreResult::StorageError;
            }

            std::array<std::byte, maximumIoSize> verificationBytes{};
            auto verification = std::span<std::byte>{verificationBytes.data(), index.recordSize};
            if (_storage.read(destinationOffset, verification) != StorageResult::Ok) {
                return SettingsStoreResult::StorageError;
            }
            SettingKey verificationKey{};
            SettingValue verificationValue{};
            if (StoredSettingCodec::decode(verification, verificationKey, verificationValue) !=
                    StoredSettingResult::Ok ||
                verificationKey != index.key) {
                return SettingsStoreResult::CorruptData;
            }
            destinationOffset += index.recordSize;
        }
        const uint32_t nextGeneration = _generation + 1U;
        result = writeBankHeader(destination, nextGeneration);
        if (result != SettingsStoreResult::Ok) {
            return result;
        }
        _activeBank = destination;
        _generation = nextGeneration;
        return scanActiveBank();
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::updateIndex(const SettingKey& key, size_t recordOffset,
                                                                size_t recordSize) {
        auto* index = findIndex(key);
        if (index == nullptr) {
            index = findFreeIndex();
        }
        if (index == nullptr) {
            return SettingsStoreResult::NoSpace;
        }
        *index = {.used = true, .key = key, .recordOffset = recordOffset, .recordSize = recordSize};
        return SettingsStoreResult::Ok;
    }

    template <size_t MaxSettings>
    typename SettingsStore<MaxSettings>::IndexEntry* SettingsStore<MaxSettings>::findFreeIndex() {
        for (auto& index : _indices) {
            if (!index.used) {
                return &index;
            }
        }
        return nullptr;
    }

    template <size_t MaxSettings>
    const typename SettingsStore<MaxSettings>::IndexEntry*
    SettingsStore<MaxSettings>::findIndex(const SettingKey& key) const {
        for (const auto& index : _indices) {
            if (index.used && index.key == key) {
                return &index;
            }
        }
        return nullptr;
    }

    template <size_t MaxSettings>
    typename SettingsStore<MaxSettings>::IndexEntry* SettingsStore<MaxSettings>::findIndex(const SettingKey& key) {
        const auto* index = static_cast<const SettingsStore<MaxSettings>&>(*this).findIndex(key);
        return const_cast<IndexEntry*>(index);
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::mapCodecResult(StoredSettingResult result) {
        switch (result) {
        case StoredSettingResult::Ok:
            return SettingsStoreResult::Ok;
        case StoredSettingResult::UnsupportedVersion:
            return SettingsStoreResult::UnsupportedVersion;
        case StoredSettingResult::CorruptData:
            return SettingsStoreResult::CorruptData;
        case StoredSettingResult::BufferTooSmall:
            return SettingsStoreResult::NoSpace;
        case StoredSettingResult::InvalidValue:
            return SettingsStoreResult::InvalidArgument;
        }
        return SettingsStoreResult::CorruptData;
    }

    template <size_t MaxSettings>
    bool SettingsStore<MaxSettings>::generationIsNewer(uint32_t lhs, uint32_t rhs) {
        return static_cast<int32_t>(lhs - rhs) > 0;
    }

} // namespace IntegralMotions::Config
