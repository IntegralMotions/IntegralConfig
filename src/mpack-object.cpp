#include "mpack-object.h"

inline bool MPackObject::ok(mpack_reader_t &reader) {
  return mpack_reader_error(&reader) == mpack_ok;
}

bool MPackObject::readHeader(mpack_reader_t &reader, MPackHeader &header) {
  mpack_tag_t t = mpack_read_tag(&reader);
  if (!ok(reader)) return false;

  header.type = mpack_tag_type(&t);
  header.countOrLength = 0;
  header.extType = 0;

  switch (header.type) {
    case mpack_type_map: header.countOrLength = mpack_tag_map_count(&t); break;
    case mpack_type_array: header.countOrLength = mpack_tag_array_count(&t); break;
    case mpack_type_str: header.countOrLength = mpack_tag_str_length(&t); break;
    case mpack_type_bin: header.countOrLength = mpack_tag_bin_length(&t); break;
    default: break;
  }
  return true;
}

bool MPackObject::readArray(mpack_reader_t &reader, int depth, const char *name,
                            const CppType &type, void *array, const size_t &size) {
  for (size_t i = 0; i < size; i++) {
    if (!ok(reader)) return false;

    switch (type) {
      case CppType::I8:   { auto *arr = (int8_t*)array;   arr[i] = mpack_expect_i8(&reader);   } break;
      case CppType::U8:   { auto *arr = (uint8_t*)array;  arr[i] = mpack_expect_u8(&reader);   } break;
      case CppType::I16:  { auto *arr = (int16_t*)array;  arr[i] = mpack_expect_i16(&reader);  } break;
      case CppType::U16:  { auto *arr = (uint16_t*)array; arr[i] = mpack_expect_u16(&reader);  } break;
      case CppType::I32:  { auto *arr = (int32_t*)array;  arr[i] = mpack_expect_i32(&reader);  } break;
      case CppType::U32:  { auto *arr = (uint32_t*)array; arr[i] = mpack_expect_u32(&reader);  } break;
      case CppType::I64:  { auto *arr = (int64_t*)array;  arr[i] = mpack_expect_i64(&reader);  } break;
      case CppType::U64:  { auto *arr = (uint64_t*)array; arr[i] = mpack_expect_u64(&reader);  } break;
      case CppType::F32:  { auto *arr = (float*)array;    arr[i] = mpack_expect_float(&reader);} break;
      case CppType::F64:  { auto *arr = (double*)array;   arr[i] = mpack_expect_double(&reader);} break;
      case CppType::Bool: { auto *arr = (bool*)array;     arr[i] = mpack_expect_bool(&reader); } break;
      case CppType::Object: {
        auto **arr = (MPackObject**)array;
        MPackObject *obj = getObject(name);
        obj->read(reader, depth + 1);
        arr[i] = obj;
      } break;
      case CppType::NestedArray:
        return false;
      default:
        mpack_reader_flag_error(&reader, mpack_error_type);
        return false;
    }
  }
  return mpack_reader_error(&reader) == mpack_ok;
}

void MPackObject::read(mpack_reader_t &reader, int depth) {
  if (depth > 32) {
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
    if (keyHeader.countOrLength)
      mpack_read_bytes(&reader, key, keyHeader.countOrLength);
    key[keyHeader.countOrLength] = '\0';
    mpack_done_str(&reader);

    if (isObject(key)) {
      MPackObject *object = getObject(key);
      delete[] key;
      if (object != nullptr) {
        object->read(reader, depth + 1);
        continue;
      } else {
        mpack_reader_flag_error(&reader, mpack_error_invalid);
        return;
      }
    }

    if (isArray(key)) {
      MPackHeader arrayHeader{};
      if (!readHeader(reader, arrayHeader) || arrayHeader.type != mpack_type_array) {
        mpack_reader_flag_error(&reader, mpack_error_type);
        delete[] key;
        return;
      }

      CppType arrayType;
      void *arr = getArray(key, arrayHeader.countOrLength, arrayType);

      bool success = readArray(reader, depth + 1, key, arrayType, arr,
                               arrayHeader.countOrLength);

      delete[] key;
      if (!success) return;
      continue;
    }

    delete[] key;
  }
}
