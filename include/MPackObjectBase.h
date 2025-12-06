#pragma once

#include "MPackHeader.h"
#include "MPackObjectMember.h"
#include "MPackObjectType.h"
#include "mpack/mpack-common.h"
#include "mpack/mpack-reader.h"
#include "mpack/mpack-writer.h"
#include <cstddef>

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
    [[nodiscard]] virtual const MPackObjectMember* members() const = 0;
    [[nodiscard]] virtual size_t memberCount() const = 0;
    [[nodiscard]] virtual void* getMemberAddress(const MPackObjectMember& member) const = 0;

  private:
    bool getMember(const char* name, MPackObjectMember& member) const;
    static bool nextIsNil(mpack_reader_t& reader);
    static void* createArray(const CppType& type, size_t length);

    static inline bool ok(mpack_reader_t& reader);
    static inline bool ok(mpack_writer_t& writer);
    static bool readHeader(mpack_reader_t& reader, MPackHeader& header);
    bool readValue(mpack_reader_t& reader, const char* name, int depth = 0);

    template <typename T> static bool readNumeric(mpack_reader_t& reader, T& value);
    static bool readBool(mpack_reader_t& reader, bool& value);
    static bool readString(mpack_reader_t& reader, char*& value);
    bool readArray(mpack_reader_t& reader, const char* name, const MPackObjectType& type, void* address, int depth = 0);

    bool writeMember(mpack_writer_t& writer, const char* name, const MPackObjectType& type, void* address,
                     int depth = 0) const;
    bool writeArray(mpack_writer_t& writer, const char* name, const MPackObjectType& type, void* address,
                    int depth = 0) const;

    virtual MPackObjectBase* createObject(const char* name);
};

template <typename T> bool MPackObjectBase::readNumeric(mpack_reader_t& reader, T& value) {
    value = static_cast<T>(0);
    MPackHeader header;
    readHeader(reader, header);
    if (!ok(reader)) {
        return false;
    }

    if (header.type != mpack_type_int && header.type != mpack_type_uint && header.type != mpack_type_float &&
        header.type != mpack_type_double) {
        mpack_reader_flag_error(&reader, mpack_error_type);
        return false;
    }

    switch (header.type) {
    case mpack_type_int:
        value = static_cast<T>(mpack_tag_int_value(&header.tag));
        break;
    case mpack_type_uint:
        value = static_cast<T>(mpack_tag_uint_value(&header.tag));
        break;
    case mpack_type_float:
        value = static_cast<T>(mpack_tag_float_value(&header.tag));
        break;
    case mpack_type_double:
        value = static_cast<T>(mpack_tag_double_value(&header.tag));
        break;
    default:
        mpack_reader_flag_error(&reader, mpack_error_type);
        return false;
    }

    return true;
}