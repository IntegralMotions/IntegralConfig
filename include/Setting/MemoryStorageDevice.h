#pragma once

#include "StorageDevice.h"
#include "StorageGeometry.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <span>

namespace IntegralMotions::Config {

    template <size_t Capacity>
    class MemoryStorageDevice : public StorageDevice {
      public:
        explicit MemoryStorageDevice(StorageGeometry geometry = {}) : StorageDevice(withCapacity(geometry)) {
            _bytes.fill(this->geometry().erasedValue);
        }

        StorageResult read(size_t offset, std::span<std::byte> destination) override;
        StorageResult write(size_t offset, std::span<const std::byte> source) override;
        StorageResult erase(size_t offset, size_t length) override;
        StorageResult flush() override;

      private:
        static StorageGeometry withCapacity(StorageGeometry geometry) {
            geometry.capacity = Capacity;
            return geometry;
        }

        [[nodiscard]] bool contains(size_t offset, size_t length) const {
            return offset <= Capacity && length <= Capacity - offset;
        }

        std::array<std::byte, Capacity> _bytes{};
    };

    template <size_t Capacity>
    StorageResult MemoryStorageDevice<Capacity>::read(size_t offset, std::span<std::byte> destination) {
        if (!contains(offset, destination.size())) {
            return StorageResult::OutOfRange;
        }

        const auto alignment = geometry().readAlignment;
        if (alignment == 0 || offset % alignment != 0 || destination.size() % alignment != 0) {
            return StorageResult::Unaligned;
        }

        std::memcpy(destination.data(), _bytes.data() + offset, destination.size());

        return StorageResult::Ok;
    }

    template <size_t Capacity>
    StorageResult MemoryStorageDevice<Capacity>::write(size_t offset, std::span<const std::byte> source) {
        if (!contains(offset, source.size())) {
            return StorageResult::OutOfRange;
        }

        const auto alignment = geometry().writeAlignment;
        if (alignment == 0 || offset % alignment != 0 || source.size() % alignment != 0) {
            return StorageResult::Unaligned;
        }

        std::memcpy(_bytes.data() + offset, source.data(), source.size());

        return StorageResult::Ok;
    }

    template <size_t Capacity>
    StorageResult MemoryStorageDevice<Capacity>::erase(size_t offset, size_t length) {
        const auto blockSize = geometry().eraseBlockSize;
        if (blockSize == 0) {
            return StorageResult::Unsupported;
        }

        if (length == 0) {
            return StorageResult::InvalidArgument;
        }

        if (!contains(offset, length)) {
            return StorageResult::OutOfRange;
        }

        if (offset % blockSize != 0 || length % blockSize != 0) {
            return StorageResult::Unaligned;
        }

        std::fill_n(_bytes.data() + offset, length, geometry().erasedValue);

        return StorageResult::Ok;
    }

    template <size_t Capacity>
    StorageResult MemoryStorageDevice<Capacity>::flush() {
        return StorageResult::Ok;
    }

} // namespace IntegralMotions::Config
