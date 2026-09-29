#pragma once

#include "DefaultMessagePayloads.h"
#include "Setting/SettingResult.h"

#include <cstddef>
#include <cstdint>
#include <mpack/mpack.h>
#include <span>

namespace IntegralMotions::Config {

    struct WriteSettingsResultEntry {
        uint32_t address = 0;
        SettingResult result = SettingResult::Ok;
    };

    class WriteSettingsResponseSerializer {
      public:
        explicit WriteSettingsResponseSerializer(std::span<const WriteSettingsResultEntry> values) : _values(values) {}

        void write(mpack_writer_t& writer) const {
            mpack_start_map(&writer, 3);
            mpack_write_cstr(&writer, "msgType");
            mpack_write_cstr(&writer, "response");
            mpack_write_cstr(&writer, "opCode");
            mpack_write_cstr(&writer, DefaultWriteKeys::writeSettings);
            mpack_write_cstr(&writer, "payload");
            mpack_start_map(&writer, 1);
            mpack_write_cstr(&writer, "values");
            mpack_start_array(&writer, _values.size());
            for (const auto& value : _values) {
                const bool success = value.result == SettingResult::Ok;
                mpack_start_map(&writer, success ? 2 : 3);
                mpack_write_cstr(&writer, "address");
                mpack_write_u32(&writer, value.address);
                mpack_write_cstr(&writer, "success");
                mpack_write_bool(&writer, success);
                if (!success) {
                    mpack_write_cstr(&writer, "errorMessage");
                    mpack_write_cstr(&writer, errorId(value.result));
                }
                mpack_finish_map(&writer);
            }
            mpack_finish_array(&writer);
            mpack_finish_map(&writer);
            mpack_finish_map(&writer);
        }

      private:
        static const char* errorId(SettingResult result) {
            switch (result) {
            case SettingResult::NotFound: return "not-found";
            case SettingResult::ReadOnly: return "readonly";
            case SettingResult::TypeMismatch: return "type-mismatch";
            case SettingResult::InvalidOption: return "invalid-option";
            case SettingResult::StorageError: return "storage-error";
            default: return "validation-failed";
            }
        }

        std::span<const WriteSettingsResultEntry> _values;
    };
} // namespace IntegralMotions::Config
