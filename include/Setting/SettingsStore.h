#pragma once

#include "Math/Crc16.h"
#include "SettingKey.h"
#include "SettingStoreResult.h"
#include "SettingValue.h"
#include "Storage/StorageDevice.h"
#include "Storage/StorageRegion.h"
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
        SettingsStore(IntegralMotions::Storage::StorageDevice& storage, IntegralMotions::Storage::StorageRegion bankA,
                      IntegralMotions::Storage::StorageRegion bankB);

        SettingsStoreResult open();
        SettingsStoreResult store(const SettingKey& key, const SettingValue& value);
        SettingsStoreResult load(const SettingKey& key, SettingValue& value) const;

      private:
        using StorageDevice = IntegralMotions::Storage::StorageDevice;
        using StorageRegion = IntegralMotions::Storage::StorageRegion;
        using StorageResult = IntegralMotions::Storage::StorageResult;

        static constexpr uint8_t bankMagic = 0xB6;
        static constexpr uint8_t bankVersion = 0;
        static constexpr size_t bankHeaderSizeUnaligned = 8;
        static constexpr size_t maximumIoSize = 256;
        static constexpr size_t secondByteOffset = 8;
        static constexpr size_t thirdByteOffset = 16;
        static constexpr size_t fourthByteOffset = 24;
        static constexpr size_t generationFirstByteIndex = 2;
        static constexpr size_t generationSecondByteIndex = 3;
        static constexpr size_t generationThirdByteIndex = 4;
        static constexpr size_t generationFourthByteIndex = 5;
        static constexpr size_t crcFirstByteIndex = 6;
        static constexpr size_t crcSecondByteIndex = 7;
        static constexpr size_t bitsInByte = 8;
        static constexpr size_t firstByteMask = 0xFFU;

        struct BankHeader {
            uint32_t generation = 0;
        };

        struct IndexEntry {
            bool used = false;
            SettingKey key{};
            size_t recordOffset = 0;
            size_t recordSize = 0;
        };

        struct BankState {
            BankHeader header{};
            std::array<IndexEntry, MaxSettings> indices{};
            size_t nextWriteOffset = 0;
            SettingsStoreResult result = SettingsStoreResult::CorruptData;
        };

        [[nodiscard]] size_t alignment() const;
        [[nodiscard]] size_t bankHeaderSize() const;
        [[nodiscard]] bool validRegions() const;
        [[nodiscard]] StorageRegion inactiveBank() const;

        SettingsStoreResult readBankHeader(StorageRegion bank, BankHeader& header) const;
        SettingsStoreResult inspectBank(StorageRegion bank, BankState& state) const;
        SettingsStoreResult writeBankHeader(StorageRegion bank, uint32_t generation);
        SettingsStoreResult clearBank(StorageRegion bank);
        SettingsStoreResult scanActiveBank();
        SettingsStoreResult scanBank(StorageRegion bank, std::array<IndexEntry, MaxSettings>& indices,
                                     size_t& nextWriteOffset) const;
        SettingsStoreResult compact();
        SettingsStoreResult append(const SettingKey& key, const SettingValue& value);
        SettingsStoreResult updateIndex(const SettingKey& key, size_t recordOffset, size_t recordSize);

        IndexEntry* findFreeIndex();
        IndexEntry* findIndex(const SettingKey& key);
        const IndexEntry* findIndex(const SettingKey& key) const;

        static SettingsStoreResult mapCodecResult(StoredSettingResult result);
        static bool generationIsNewer(uint32_t lhs, uint32_t rhs);
        static bool hasIndexedSettings(const std::array<IndexEntry, MaxSettings>& indices);

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
    SettingsStore<MaxSettings>::SettingsStore(IntegralMotions::Storage::StorageDevice& storage,
                                              IntegralMotions::Storage::StorageRegion bankA,
                                              IntegralMotions::Storage::StorageRegion bankB)
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
    IntegralMotions::Storage::StorageRegion SettingsStore<MaxSettings>::inactiveBank() const {
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
            const size_t chunkSize = maximumIoSize - (maximumIoSize % _storage.geometry().readAlignment);
            if (chunkSize == 0) {
                return SettingsStoreResult::InvalidArgument;
            }
            bool blank = true;
            size_t offset = bank.offset;
            size_t remaining = bank.size;
            while (remaining != 0) {
                const size_t size = std::min(chunkSize, remaining);
                auto chunk = std::span<std::byte>{bytes.data(), size};
                if (_storage.read(offset, chunk) != StorageResult::Ok) {
                    return SettingsStoreResult::StorageError;
                }
                if (std::any_of(chunk.begin(), chunk.end(),
                                [this](std::byte value) { return value != _storage.geometry().erasedValue; })) {
                    blank = false;
                    break;
                }
                offset += size;
                remaining -= size;
            }
            if (blank) {
                return SettingsStoreResult::Blank;
            }
            return SettingsStoreResult::CorruptData;
        }
        if (std::to_integer<uint8_t>(bytes[1]) != bankVersion) {
            return SettingsStoreResult::UnsupportedVersion;
        }

        header.generation =
            static_cast<uint32_t>(std::to_integer<uint8_t>(bytes[generationFirstByteIndex])) |
            (static_cast<uint32_t>(std::to_integer<uint8_t>(bytes[generationSecondByteIndex])) << secondByteOffset) |
            (static_cast<uint32_t>(std::to_integer<uint8_t>(bytes[generationThirdByteIndex])) << thirdByteOffset) |
            (static_cast<uint32_t>(std::to_integer<uint8_t>(bytes[generationFourthByteIndex])) << fourthByteOffset);
        const uint16_t storedCrc = static_cast<uint16_t>(std::to_integer<uint8_t>(bytes[6])) |
                                   (static_cast<uint16_t>(std::to_integer<uint8_t>(bytes[7])) << 8U);
        if (!IntegralMotions::Math::Crc16::validate(reinterpret_cast<const uint8_t*>(bytes.data()),
                                                    bankHeaderSizeUnaligned - 2, storedCrc)) {
            return SettingsStoreResult::CorruptData;
        }

        return SettingsStoreResult::Ok;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::inspectBank(StorageRegion bank, BankState& state) const {
        state = {};
        state.result = readBankHeader(bank, state.header);
        if (state.result == SettingsStoreResult::Ok) {
            state.result = scanBank(bank, state.indices, state.nextWriteOffset);
        }
        return state.result;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::writeBankHeader(StorageRegion bank, uint32_t generation) {
        const size_t storedHeaderSize = bankHeaderSize();
        std::array<std::byte, maximumIoSize> bytes{};
        std::fill_n(bytes.begin(), storedHeaderSize, _storage.geometry().erasedValue);
        bytes[0] = static_cast<std::byte>(bankMagic);
        bytes[1] = static_cast<std::byte>(bankVersion);
        for (size_t i = 0; i < sizeof(generation); ++i) {
            bytes[2 + i] = static_cast<std::byte>(generation >> (i * bitsInByte));
        }
        const uint16_t crc = IntegralMotions::Math::Crc16::calculate(reinterpret_cast<const uint8_t*>(bytes.data()), 6);
        bytes[crcFirstByteIndex] = static_cast<std::byte>(crc & firstByteMask);
        bytes[crcSecondByteIndex] = static_cast<std::byte>(crc >> secondByteOffset);

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
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    SettingsStoreResult SettingsStore<MaxSettings>::open() {
        _open = false;
        _indices = {};
        if (!validRegions()) {
            return SettingsStoreResult::InvalidArgument;
        }

        BankState stateA{};
        BankState stateB{};
        const auto resultA = inspectBank(_bankA, stateA);
        const auto resultB = inspectBank(_bankB, stateB);
        if (resultA == SettingsStoreResult::StorageError || resultB == SettingsStoreResult::StorageError) {
            return SettingsStoreResult::StorageError;
        }
        if (resultA != SettingsStoreResult::Ok && resultA != SettingsStoreResult::Blank &&
            resultA != SettingsStoreResult::CorruptData) {
            return resultA;
        }
        if (resultB != SettingsStoreResult::Ok && resultB != SettingsStoreResult::Blank &&
            resultB != SettingsStoreResult::CorruptData) {
            return resultB;
        }

        const bool validA = resultA == SettingsStoreResult::Ok;
        const bool validB = resultB == SettingsStoreResult::Ok;
        if (validA || validB) {
            const bool selectA =
                validA && (!validB || generationIsNewer(stateA.header.generation, stateB.header.generation));
            const auto& selected = selectA ? stateA : stateB;
            const StorageRegion damaged = selectA ? _bankB : _bankA;
            const auto damagedResult = selectA ? resultB : resultA;

            _activeBank = selectA ? _bankA : _bankB;
            _generation = selected.header.generation;
            _indices = selected.indices;
            _nextWriteOffset = selected.nextWriteOffset;

            if (damagedResult == SettingsStoreResult::CorruptData) {
                const auto clearResult = clearBank(damaged);
                if (clearResult != SettingsStoreResult::Ok) {
                    return clearResult;
                }
                _open = true;
                return SettingsStoreResult::Recovered;
            }
            _open = true;
            return SettingsStoreResult::Ok;
        }

        if (resultA == SettingsStoreResult::Blank && resultB == SettingsStoreResult::Blank) {
            const auto result = writeBankHeader(_bankA, 0);
            if (result != SettingsStoreResult::Ok) {
                return result;
            }
            _activeBank = _bankA;
            _generation = 0;
            _indices = {};
            _nextWriteOffset = _bankA.offset + bankHeaderSize();
            _open = true;
            return SettingsStoreResult::Blank;
        }

        // A damaged header can leave intact, CRC-validated records. Only rebuild from one
        // source bank; two damaged populated banks have no trustworthy ordering.
        BankState salvagedA{};
        BankState salvagedB{};
        const auto salvageResultA = resultA == SettingsStoreResult::CorruptData
                                        ? scanBank(_bankA, salvagedA.indices, salvagedA.nextWriteOffset)
                                        : SettingsStoreResult::CorruptData;
        const auto salvageResultB = resultB == SettingsStoreResult::CorruptData
                                        ? scanBank(_bankB, salvagedB.indices, salvagedB.nextWriteOffset)
                                        : SettingsStoreResult::CorruptData;
        if (salvageResultA == SettingsStoreResult::StorageError ||
            salvageResultB == SettingsStoreResult::StorageError) {
            return SettingsStoreResult::StorageError;
        }
        const bool canSalvageA = salvageResultA == SettingsStoreResult::Ok && hasIndexedSettings(salvagedA.indices);
        const bool canSalvageB = salvageResultB == SettingsStoreResult::Ok && hasIndexedSettings(salvagedB.indices);
        if (canSalvageA == canSalvageB) {
            return SettingsStoreResult::CorruptData;
        }

        const bool selectA = canSalvageA;
        const auto& salvaged = selectA ? salvagedA : salvagedB;
        _activeBank = selectA ? _bankA : _bankB;
        _generation = 0;
        _indices = salvaged.indices;
        _nextWriteOffset = salvaged.nextWriteOffset;
        const auto result = compact();
        if (result != SettingsStoreResult::Ok) {
            _open = false;
            return result;
        }
        _open = true;
        return SettingsStoreResult::Recovered;
    }

    template <size_t MaxSettings>
    SettingsStoreResult SettingsStore<MaxSettings>::scanActiveBank() {
        return scanBank(_activeBank, _indices, _nextWriteOffset);
    }

    template <size_t MaxSettings>
    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    SettingsStoreResult SettingsStore<MaxSettings>::scanBank(StorageRegion bank,
                                                             std::array<IndexEntry, MaxSettings>& indices,
                                                             size_t& nextWriteOffset) const {
        indices = {};
        nextWriteOffset = bank.offset + bankHeaderSize();
        const size_t bankEnd = bank.offset + bank.size;
        const size_t readAlignment = _storage.geometry().readAlignment;
        const size_t prefixRemainder = StoredSettingCodec::headerSize % readAlignment;
        const size_t prefixSize = prefixRemainder == 0
                                      ? StoredSettingCodec::headerSize
                                      : StoredSettingCodec::headerSize + readAlignment - prefixRemainder;
        if (prefixSize > maximumIoSize) {
            return SettingsStoreResult::InvalidArgument;
        }

        std::array<std::byte, maximumIoSize> bytes{};
        while (nextWriteOffset + prefixSize <= bankEnd) {
            auto prefix = std::span<std::byte>{bytes.data(), prefixSize};
            if (_storage.read(nextWriteOffset, prefix) != StorageResult::Ok) {
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
            if (recordSize == 0 || recordSize > maximumIoSize || recordSize > bankEnd - nextWriteOffset) {
                return SettingsStoreResult::CorruptData;
            }
            auto record = std::span<std::byte>{bytes.data(), recordSize};
            if (_storage.read(nextWriteOffset, record) != StorageResult::Ok) {
                return SettingsStoreResult::StorageError;
            }

            SettingKey key{};
            SettingValue value{};
            const auto decodeResult = StoredSettingCodec::decode(record, key, value);
            if (decodeResult == StoredSettingResult::Ok) {
                auto* index = std::find_if(indices.begin(), indices.end(),
                                           [&key](const IndexEntry& entry) { return entry.used && entry.key == key; });
                if (index == indices.end()) {
                    index = std::find_if(indices.begin(), indices.end(),
                                         [](const IndexEntry& entry) { return !entry.used; });
                    if (index == indices.end()) {
                        return SettingsStoreResult::NoSpace;
                    }
                }
                *index = {.used = true, .key = key, .recordOffset = nextWriteOffset, .recordSize = recordSize};
            } else if (decodeResult != StoredSettingResult::CorruptData) {
                return mapCodecResult(decodeResult);
            }
            nextWriteOffset += recordSize;
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
    SettingsStore<MaxSettings>::IndexEntry* SettingsStore<MaxSettings>::findFreeIndex() {
        for (auto& index : _indices) {
            if (!index.used) {
                return &index;
            }
        }
        return nullptr;
    }

    template <size_t MaxSettings>
    const SettingsStore<MaxSettings>::IndexEntry* SettingsStore<MaxSettings>::findIndex(const SettingKey& key) const {
        for (const auto& index : _indices) {
            if (index.used && index.key == key) {
                return &index;
            }
        }
        return nullptr;
    }

    template <size_t MaxSettings>
    SettingsStore<MaxSettings>::IndexEntry* SettingsStore<MaxSettings>::findIndex(const SettingKey& key) {
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

    template <size_t MaxSettings>
    bool SettingsStore<MaxSettings>::hasIndexedSettings(const std::array<IndexEntry, MaxSettings>& indices) {
        return std::any_of(indices.begin(), indices.end(), [](const IndexEntry& entry) { return entry.used; });
    }

} // namespace IntegralMotions::Config
