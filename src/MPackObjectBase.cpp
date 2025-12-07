#include "MPackObjectBase.h"
#include "MPackArray.h"
#include "mpack/mpack-common.h"
#include "mpack/mpack-expect.h"
#include "mpack/mpack-reader.h"
#include "mpack/mpack-writer.h"
#include <cstddef>
#include <cstdint>

void MPackObjectBase::read(mpack_reader_t &reader, int depth) {
    if (depth > MPACK_MAX_DEPTH) {
        mpack_reader_flag_error(&reader, mpack_error_too_big);
        return;
    }

    MPackHeader header{};
    if (!readHeader(reader, header) || header.type != mpack_type_map) {
        mpack_reader_flag_error(&reader, mpack_error_type);
        return;
    }

    for (size_t i = 0; i < header.countOrLength; i++) {
        MPackHeader keyHeader{};
        if (!readHeader(reader, keyHeader) || keyHeader.type != mpack_type_str) {
            mpack_reader_flag_error(&reader, mpack_error_type);
            return;
        }

        char *key = new char[keyHeader.countOrLength + 1];
        if (keyHeader.countOrLength != 0U) {
            mpack_read_bytes(&reader, key, keyHeader.countOrLength);
        }
        key[keyHeader.countOrLength] = '\0';
        mpack_done_str(&reader);

        readValue(reader, key, depth);

        delete[] key;
        if (!ok(reader)) {
            return;
        }
    }

    mpack_done_map(&reader);
}

void MPackObjectBase::write(mpack_writer_t &writer, int depth) const {
    if (depth > MPACK_MAX_DEPTH) {
        mpack_writer_flag_error(&writer, mpack_error_too_big);
        return;
    }

    const MPackObjectMember *members = this->members();
    const size_t memberCount = this->memberCount();
    mpack_start_map(&writer, memberCount);

    for (size_t i = 0; i < memberCount; i++) {
        writeMember(writer, members[i].name, members[i].type, getMemberAddress(members[i]));
    }

    mpack_finish_map(&writer);
}

bool MPackObjectBase::getMember(const char *name, MPackObjectMember &member) const {
    const MPackObjectMember *members = this->members();
    const size_t memberCount = this->memberCount();

    for (size_t i = 0; i < memberCount; i++) {
        if (strcmp(members[i].name, name) == 0) {
            member = members[i];
            return true;
        }
    }
    return false;
}

bool MPackObjectBase::nextIsNil(mpack_reader_t &reader) {
    mpack_tag_t tag = mpack_peek_tag(&reader);
    if (mpack_reader_error(&reader) != mpack_ok) {
        return false;
    }
    return tag.type == mpack_type_nil;
}

void *MPackObjectBase::createArray(const CppType &type, size_t length) {
    void *array;

    switch (type) {
    case CppType::I8:
        array = new int8_t[length];
        break;
    case CppType::U8:
        array = new uint8_t[length];
        break;
    case CppType::I16:
        array = new int16_t[length];
        break;
    case CppType::U16:
        array = new uint16_t[length];
        break;
    case CppType::I32:
        array = new int32_t[length];
        break;
    case CppType::U32:
        array = new uint32_t[length];
        break;
    case CppType::I64:
        array = new int64_t[length];
        break;
    case CppType::U64:
        array = new uint64_t[length];
        break;
    case CppType::F32:
        array = new float[length];
        break;
    case CppType::F64:
        array = new double[length];
        break;
    case CppType::Bool:
        array = new bool[length];
        break;
    case CppType::String:
        array = reinterpret_cast<void *>(new const char *[length]);
        break;
    case CppType::ObjectPtr:
    case CppType::Array:
        array = reinterpret_cast<void *>(new void *[length]);
        break;
    default:
        array = nullptr;
    }

    return array;
}

inline bool MPackObjectBase::ok(mpack_reader_t &reader) {
    return mpack_reader_error(&reader) == mpack_ok;
}

inline bool MPackObjectBase::ok(mpack_writer_t &writer) {
    return mpack_writer_error(&writer) == mpack_ok;
}

bool MPackObjectBase::readHeader(mpack_reader_t &reader, MPackHeader &header) {
    header.tag = mpack_read_tag(&reader);
    if (!ok(reader)) {
        return false;
    }

    header.type = mpack_tag_type(&header.tag);
    header.countOrLength = 0;
    header.extType = 0;

    switch (header.type) {
    case mpack_type_map:
        header.countOrLength = mpack_tag_map_count(&header.tag);
        break;
    case mpack_type_array:
        header.countOrLength = mpack_tag_array_count(&header.tag);
        break;
    case mpack_type_str:
        header.countOrLength = mpack_tag_str_length(&header.tag);
        break;
    case mpack_type_bin:
        header.countOrLength = mpack_tag_bin_length(&header.tag);
        break;
    default:
        break;
    }
    return true;
}

bool MPackObjectBase::readValue(mpack_reader_t &reader, const char *name, int depth) {
    MPackObjectMember member;
    if (!getMember(name, member)) {
        return false;
    }

    switch (member.type.type) {
    case CppType::I8:
        return readNumeric<int8_t>(reader, *static_cast<int8_t *>(this->getMemberAddress(member)));
    case CppType::U8:
        return readNumeric<uint8_t>(reader, *static_cast<uint8_t *>(this->getMemberAddress(member)));
    case CppType::I16:
        return readNumeric<int16_t>(reader, *static_cast<int16_t *>(this->getMemberAddress(member)));
    case CppType::U16:
        return readNumeric<uint16_t>(reader, *static_cast<uint16_t *>(this->getMemberAddress(member)));
    case CppType::I32:
        return readNumeric<int32_t>(reader, *static_cast<int32_t *>(this->getMemberAddress(member)));
    case CppType::U32:
        return readNumeric<uint32_t>(reader, *static_cast<uint32_t *>(this->getMemberAddress(member)));
    case CppType::I64:
        return readNumeric<int64_t>(reader, *static_cast<int64_t *>(this->getMemberAddress(member)));
    case CppType::U64:
        return readNumeric<uint64_t>(reader, *static_cast<uint64_t *>(this->getMemberAddress(member)));
    case CppType::F32:
        return readNumeric<float>(reader, *static_cast<float *>(this->getMemberAddress(member)));
    case CppType::F64:
        return readNumeric<double>(reader, *static_cast<double *>(this->getMemberAddress(member)));
    case CppType::Bool:
        return readBool(reader, *static_cast<bool *>(this->getMemberAddress(member)));
    case CppType::String:
        return readString(reader, *static_cast<char **>(this->getMemberAddress(member)));
    case CppType::Object: {
        if (nextIsNil(reader)) {
            mpack_expect_nil(&reader);
            return ok(reader);
        }

        auto *obj = static_cast<MPackObjectBase *>(this->getMemberAddress(member));
        if (obj == nullptr) {
            mpack_reader_flag_error(&reader, mpack_error_data);
            return false;
        }

        obj->read(reader, depth + 1);
        return ok(reader);
    }
    case CppType::ObjectPtr: {
        auto **obj = static_cast<MPackObjectBase **>(this->getMemberAddress(member));
        if (nextIsNil(reader)) {
            mpack_expect_nil(&reader);
            *obj = nullptr;
            return ok(reader);
        }

        if (*obj == nullptr) {
            *obj = createObject(name);
        }
        if (*obj == nullptr) {
            mpack_reader_flag_error(&reader, mpack_error_data);
            return false;
        }

        (*obj)->read(reader, depth + 1);
        return ok(reader);
    }
    case CppType::Array:
        return readArray(reader, member.name, member.type, getMemberAddress(member), depth + 1);
    default:
        mpack_reader_flag_error(&reader, mpack_error_type);
        return false;
    }
}

bool MPackObjectBase::readBool(mpack_reader_t &reader, bool &value) {
    value = false;
    MPackHeader header;
    readHeader(reader, header);
    if (!ok(reader)) {
        return false;
    }

    if (header.type != mpack_type_bool) {
        mpack_reader_flag_error(&reader, mpack_error_type);
        return false;
    }

    value = mpack_tag_bool_value(&header.tag);
    return true;
}

bool MPackObjectBase::readString(mpack_reader_t &reader, char *&value) {
    value = nullptr;
    MPackHeader header;
    readHeader(reader, header);
    if (!ok(reader)) {
        return false;
    }

    if (header.type != mpack_type_str) {
        mpack_reader_flag_error(&reader, mpack_error_type);
        return false;
    }

    if (header.countOrLength > (MPACK_MAX_STRING - 1)) {
        mpack_reader_flag_error(&reader, mpack_error_memory);
        return false;
    }

    value = new char[header.countOrLength + 1];
    if (header.countOrLength != 0U) {
        mpack_read_bytes(&reader, value, header.countOrLength);
    }
    value[header.countOrLength] = '\0';
    mpack_done_str(&reader);

    return true;
}

bool MPackObjectBase::readArray(mpack_reader_t &reader, const char *name, const MPackObjectType &type, void *address,
                                int depth) {
    if (nextIsNil(reader)) {
        mpack_expect_nil(&reader);
        return ok(reader);
    }

    MPackHeader header{};
    if (!readHeader(reader, header) || header.type != mpack_type_array) {
        mpack_reader_flag_error(&reader, mpack_error_type);
        return false;
    }

    MPackObjectType *innerType = type.innerType.get();
    if (innerType == nullptr) {
        mpack_reader_flag_error(&reader, mpack_error_type);
        return false;
    }

    size_t count = header.countOrLength;

    switch (innerType->type) {
    case CppType::I8: {
        auto *arr = reinterpret_cast<MPackArray<int8_t> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new int8_t[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<int8_t>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::U8: {
        auto *arr = reinterpret_cast<MPackArray<uint8_t> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new uint8_t[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<uint8_t>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::I16: {
        auto *arr = reinterpret_cast<MPackArray<int16_t> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new int16_t[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<int16_t>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::U16: {
        auto *arr = reinterpret_cast<MPackArray<uint16_t> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new uint16_t[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<uint16_t>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::I32: {
        auto *arr = reinterpret_cast<MPackArray<int32_t> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new int32_t[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<int32_t>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::U32: {
        auto *arr = reinterpret_cast<MPackArray<uint32_t> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new uint32_t[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<uint32_t>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::I64: {
        auto *arr = reinterpret_cast<MPackArray<int64_t> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new int64_t[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<int64_t>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::U64: {
        auto *arr = reinterpret_cast<MPackArray<uint64_t> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new uint64_t[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<uint64_t>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::F32: {
        auto *arr = reinterpret_cast<MPackArray<float> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new float[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<float>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::F64: {
        auto *arr = reinterpret_cast<MPackArray<double> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new double[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readNumeric<double>(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::Bool: {
        auto *arr = reinterpret_cast<MPackArray<bool> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? new bool[count] : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readBool(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::String: {
        auto *arr = reinterpret_cast<MPackArray<char *> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? reinterpret_cast<void *>(new char *[count]) : nullptr;
        for (size_t i = 0; i < count; ++i) {
            if (!readString(reader, (*arr)[i])) {
                return false;
            }
        }
    } break;

    case CppType::ObjectPtr: {
        auto *arr = reinterpret_cast<MPackArray<MPackObjectBase *> *>(address);
        arr->size = count;
        arr->p = (count != 0U) ? reinterpret_cast<void *>(new MPackObjectBase *[count]) : nullptr;
        for (size_t i = 0; i < count; ++i) {
            MPackObjectBase *obj = createObject(name);
            obj->read(reader, depth + 1);
            if (!ok(reader)) {
                return false;
            }
            (*arr)[i] = obj;
        }
    } break;

    case CppType::Array: {
        if (!innerType->innerType) {
            mpack_reader_flag_error(&reader, mpack_error_type);
            return false;
        }

        auto *outer = reinterpret_cast<MPackArray<MPackArrayBase> *>(address);
        outer->size = count;
        outer->p = (count != 0U) ? new MPackArrayBase[count] : nullptr;

        for (size_t i = 0; i < count; ++i) {
            if (!readArray(reader, name, *innerType, &(*outer)[i], depth + 1)) {
                return false;
            }
        }
    } break;

    default:
        mpack_reader_flag_error(&reader, mpack_error_invalid);
        return false;
    }

    mpack_done_array(&reader);
    return ok(reader);
}

bool MPackObjectBase::writeMember(mpack_writer_t &writer, const char *name, const MPackObjectType &type, void *address,
                                  int depth) const {
    mpack_write_cstr(&writer, name);

    switch (type.type) {
    case CppType::I8:
        mpack_write_i8(&writer, *static_cast<int8_t *>(address));
        break;
    case CppType::U8:
        mpack_write_u8(&writer, *static_cast<uint8_t *>(address));
        break;
    case CppType::I16:
        mpack_write_i16(&writer, *static_cast<int16_t *>(address));
        break;
    case CppType::U16:
        mpack_write_u16(&writer, *static_cast<uint16_t *>(address));
        break;
    case CppType::I32:
        mpack_write_i32(&writer, *static_cast<int32_t *>(address));
        break;
    case CppType::U32:
        mpack_write_u32(&writer, *static_cast<uint32_t *>(address));
        break;
    case CppType::I64:
        mpack_write_i64(&writer, *static_cast<int64_t *>(address));
        break;
    case CppType::U64:
        mpack_write_u64(&writer, *static_cast<uint64_t *>(address));
        break;
    case CppType::F32:
        mpack_write_float(&writer, *static_cast<float *>(address));
        break;
    case CppType::F64:
        mpack_write_double(&writer, *static_cast<double *>(address));
        break;
    case CppType::Bool:
        mpack_write_bool(&writer, *static_cast<bool *>(address));
        break;
    case CppType::String: {
        const char *str = *static_cast<const char *const *>(address);
        if (str == nullptr) {
            mpack_write_nil(&writer);
        } else {
            mpack_write_cstr(&writer, str);
        }
    } break;
    case CppType::Object: {
        auto *obj = static_cast<MPackObjectBase *>(address);
        obj->write(writer, depth + 1);
        return ok(writer);
    } break;
    case CppType::ObjectPtr: {
        auto **obj = static_cast<MPackObjectBase **>(address);
        if (*obj == nullptr) {
            mpack_write_nil(&writer);
        } else {
            (*obj)->write(writer, depth + 1);
        }
        return ok(writer);
    } break;
    case CppType::Array:
        return writeArray(writer, name, type, address, depth + 1);
    default:
        mpack_writer_flag_error(&writer, mpack_error_invalid);
        return false;
    }

    return mpack_writer_error(&writer) == mpack_ok;
}

bool MPackObjectBase::writeArray(mpack_writer_t &writer, const char *name, const MPackObjectType &type, void *address,
                                 int depth) const {
    MPackObjectType *innerType = type.innerType.get();
    if (innerType == nullptr) {
        mpack_writer_flag_error(&writer, mpack_error_type);
        return false;
    }

    switch (innerType->type) {
    case CppType::I8: {
        auto *arr = static_cast<MPackArray<int8_t> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_i8(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::U8: {
        auto *arr = static_cast<MPackArray<uint8_t> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_u8(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::I16: {
        auto *arr = static_cast<MPackArray<int16_t> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_i16(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::U16: {
        auto *arr = static_cast<MPackArray<uint16_t> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_u16(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::I32: {
        auto *arr = static_cast<MPackArray<int32_t> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_i32(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::U32: {
        auto *arr = static_cast<MPackArray<uint32_t> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_u32(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::I64: {
        auto *arr = static_cast<MPackArray<int64_t> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_i64(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::U64: {
        auto *arr = static_cast<MPackArray<uint64_t> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_u64(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::F32: {
        auto *arr = static_cast<MPackArray<float> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_float(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::F64: {
        auto *arr = static_cast<MPackArray<double> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_double(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::Bool: {
        auto *arr = static_cast<MPackArray<bool> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            mpack_write_bool(&writer, (*arr)[i]);
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::String: {
        auto *arr = static_cast<MPackArray<const char *> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            if ((*arr)[i] == nullptr) {
                mpack_write_nil(&writer);
            } else {
                mpack_write_cstr(&writer, (*arr)[i]);
            }
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::ObjectPtr: {
        auto *arr = static_cast<MPackArray<MPackObjectBase *> *>(address);
        if (arr->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }
        mpack_start_array(&writer, arr->size);
        for (size_t i = 0; i < arr->size; i++) {
            if ((*arr)[i] == nullptr) {
                mpack_write_nil(&writer);
            } else {
                (*arr)[i]->write(writer, depth + 1);
                if (!ok(writer)) {
                    return false;
                }
            }
        }
        mpack_finish_array(&writer);
    } break;

    case CppType::Array: {
        auto *outer = static_cast<MPackArray<MPackArrayBase> *>(address);
        if (outer->p == nullptr) {
            mpack_write_nil(&writer);
            return ok(writer);
        }

        mpack_start_array(&writer, outer->size);
        for (size_t i = 0; i < outer->size; i++) {
            if (!writeArray(writer, name, *innerType, &(*outer)[i], depth + 1)) {
                return false;
            }
        }
        mpack_finish_array(&writer);
    } break;

    default:
        mpack_writer_flag_error(&writer, mpack_error_invalid);
        return false;
    }

    return ok(writer);
}

MPackObjectBase *MPackObjectBase::createObject(const char *name) {
    return nullptr;
}