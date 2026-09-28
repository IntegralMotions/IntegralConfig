#pragma once

#include "MPackHeader.h"
#include "MPackObjectMember.h"
#include "MPackObjectType.h"
#include "mpack/mpack-common.h"
#include "mpack/mpack-expect.h"
#include "mpack/mpack-reader.h"
#include "mpack/mpack-writer.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>

#ifndef MPACK_MAX_STRING
#define MPACK_MAX_STRING 1024
#endif

#ifndef MPACK_MAX_DEPTH
#define MPACK_MAX_DEPTH 32
#endif

class MPackObjectBase {
  public:
    virtual ~MPackObjectBase() = default;
    void read(mpack_reader_t& reader, int depth = 0);
    void write(mpack_writer_t& writer, int depth = 0) const;

  protected:
    [[nodiscard]] virtual const MPackObjectMember* getMembers() const = 0;
    [[nodiscard]] virtual size_t memberCount() const = 0;
    [[nodiscard]] virtual void* getMemberAddress(const MPackObjectMember& member) const = 0;
    void clearDecodedMembers();

  private:
    bool getMember(const char* name, MPackObjectMember& member, size_t& index) const;
    static bool nextIsNil(mpack_reader_t& reader);

    static bool ok(mpack_reader_t& reader);
    static bool ok(mpack_writer_t& writer);
    static bool readHeader(mpack_reader_t& reader, MPackHeader& header);
    bool readValue(mpack_reader_t& reader, const char* name, int depth = 0);

    template <typename T>
    static bool readNumeric(mpack_reader_t& reader, T& value);
    template <typename T>
    static bool readOptionalNumeric(mpack_reader_t& reader, std::optional<T>& value);
    static bool readBool(mpack_reader_t& reader, bool& value);
    static bool readOptionalBool(mpack_reader_t& reader, std::optional<bool>& value);
    static bool readString(mpack_reader_t& reader, char*& value);
    bool readArray(mpack_reader_t& reader, const char* name, const MPackObjectType& type, void* address, int depth = 0);

    [[nodiscard]] static bool shouldWrite(mpack_writer_t& writer, const MPackObjectMember& member,
                                          const void* address);
    static void resetReadState(const MPackObjectMember& member, void* address);
    void releaseDecodedMember(size_t index);
    static void releaseArray(const MPackObjectType& type, void* address);
    bool writeMember(mpack_writer_t& writer, const MPackObjectMember& member, void* address, int depth = 0) const;
    bool writeArray(mpack_writer_t& writer, const char* name, const MPackObjectType& type, void* address,
                    int depth = 0) const;

    virtual MPackObjectBase* createObject(const char* name);

    uint64_t _decodedMembers = 0;
};

template <typename T>
bool MPackObjectBase::readNumeric(mpack_reader_t& reader, T& value) {
    MPackHeader header{};
    if (!readHeader(reader, header)) {
        return false;
    }

    if (header.type != mpack_type_int && header.type != mpack_type_uint && header.type != mpack_type_float &&
        header.type != mpack_type_double) {
        mpack_reader_flag_error(&reader, mpack_error_type);
        return false;
    }

    T parsed{};
    bool valid = false;

    if constexpr (std::is_integral_v<T>) {
        switch (header.type) {
        case mpack_type_int: {
            const int64_t source = mpack_tag_int_value(&header.tag);
            if constexpr (std::is_signed_v<T>) {
                valid = source >= std::numeric_limits<T>::min() && source <= std::numeric_limits<T>::max();
            } else {
                valid = source >= 0 && static_cast<uint64_t>(source) <= std::numeric_limits<T>::max();
            }
            if (valid) {
                parsed = static_cast<T>(source);
            }
            break;
        }
        case mpack_type_uint: {
            const uint64_t source = mpack_tag_uint_value(&header.tag);
            valid = source <= static_cast<uint64_t>(std::numeric_limits<T>::max());
            if (valid) {
                parsed = static_cast<T>(source);
            }
            break;
        }
        case mpack_type_float:
        case mpack_type_double: {
            const long double source = header.type == mpack_type_float
                                           ? static_cast<long double>(mpack_tag_float_value(&header.tag))
                                           : static_cast<long double>(mpack_tag_double_value(&header.tag));
            const long double upper = std::ldexp(1.0L, std::numeric_limits<T>::digits);
            const long double lower = std::is_signed_v<T> ? -upper : 0.0L;
            valid = std::isfinite(source) && std::trunc(source) == source && source >= lower && source < upper;
            if (valid) {
                parsed = static_cast<T>(source);
            }
            break;
        }
        default: break;
        }
    } else {
        long double source{};
        switch (header.type) {
        case mpack_type_int: source = static_cast<long double>(mpack_tag_int_value(&header.tag)); break;
        case mpack_type_uint: source = static_cast<long double>(mpack_tag_uint_value(&header.tag)); break;
        case mpack_type_float: source = static_cast<long double>(mpack_tag_float_value(&header.tag)); break;
        case mpack_type_double: source = static_cast<long double>(mpack_tag_double_value(&header.tag)); break;
        default: break;
        }
        valid = std::isfinite(source) && source >= -static_cast<long double>(std::numeric_limits<T>::max()) &&
                source <= static_cast<long double>(std::numeric_limits<T>::max());
        if (valid) {
            parsed = static_cast<T>(source);
            valid = std::isfinite(parsed) && (source == 0.0L || parsed != static_cast<T>(0));
        }
    }

    if (!valid) {
        mpack_reader_flag_error(&reader, mpack_error_data);
        return false;
    }
    value = parsed;
    return true;
}

template <typename T>
bool MPackObjectBase::readOptionalNumeric(mpack_reader_t& reader, std::optional<T>& value) {
    if (nextIsNil(reader)) {
        mpack_expect_nil(&reader);
        value.reset();
        return ok(reader);
    }

    T parsed{};
    if (!readNumeric(reader, parsed)) {
        return false;
    }
    value = parsed;
    return true;
}
