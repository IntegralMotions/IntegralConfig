#pragma once

#include "StorageGeometry.h"
#include "StorageResult.h"
#include <cstddef>
#include <span>

namespace IntegralMotions::Config {

    class StorageDevice {
      public:
        StorageDevice(StorageGeometry geometry) : _geometry(geometry) {};
        virtual ~StorageDevice() = default;

        [[nodiscard]] const StorageGeometry& geometry() const {
            return _geometry;
        }

        virtual StorageResult read(size_t offset, std::span<std::byte> destination) = 0;
        virtual StorageResult write(size_t offset, std::span<const std::byte> source) = 0;
        virtual StorageResult erase(size_t offset, size_t length) = 0;
        virtual StorageResult flush() = 0;

      private:
        StorageGeometry _geometry;
    };

} // namespace IntegralMotions::Config
