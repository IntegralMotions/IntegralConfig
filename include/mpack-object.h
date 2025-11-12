#pragma once

#include "mpack-header.h"
#include "mpack/mpack-common.h"

enum class CppType : uint8_t {
  I8,
  U8,
  I16,
  U16,
  I32,
  U32,
  I64,
  U64,
  F32,
  F64,
  Bool,
  Object,
  ObjectPtr,
  NestedArray
};

class MPackObject {
public:
  void read(mpack_reader_t &reader, int depth = 0);
  virtual void write(mpack_writer_t &writer, int depth = 0) = 0;

private:
  static inline bool ok(mpack_reader_t &reader);
  static bool readHeader(mpack_reader_t &reader, MPackHeader &header);
  bool readArray(mpack_reader_t &reader, int depth, const char *name,
                 const CppType &type, void *array, const size_t &size);

private:
  virtual bool isObject(const char *name);
  virtual bool isArray(const char *name);

  virtual MPackObject *getObject(const char *name);
  virtual void *getArray(const char *name, size_t count, CppType &outType);

  virtual bool setMPackValue(const char *name, void *value,
                             const mpack_type_t &type) = 0;
};
