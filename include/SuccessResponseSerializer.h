#pragma once

#include "DefaultMessagePayloads.h"
#include "Setting/SettingResult.h"

#include <mpack/mpack.h>

namespace IntegralMotions::Config {

    class SuccessResponseSerializer {
      public:
        SuccessResponseSerializer(const char* operation, SettingResult result) : _operation(operation), _result(result) {}

        void write(mpack_writer_t& writer) const {
            mpack_start_map(&writer, 3);
            mpack_write_cstr(&writer, "msgType");
            mpack_write_cstr(&writer, "response");
            mpack_write_cstr(&writer, "opCode");
            mpack_write_cstr(&writer, _operation);
            mpack_write_cstr(&writer, "payload");
            mpack_start_map(&writer, _result == SettingResult::Ok ? 1 : 2);
            mpack_write_cstr(&writer, "success");
            mpack_write_bool(&writer, _result == SettingResult::Ok);
            if (_result != SettingResult::Ok) {
                mpack_write_cstr(&writer, "errorMessage");
                mpack_write_cstr(&writer, errorId(_result));
            }
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

        const char* _operation;
        SettingResult _result;
    };

} // namespace IntegralMotions::Config
