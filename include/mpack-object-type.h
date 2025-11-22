#pragma once

#include <cstdint>
#include <memory>

enum class CppType : uint8_t {
  None,
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
  String,
  Object,
  ObjectPtr,
  Array,
};

struct MPackObjectType {
  CppType type = CppType::None;
  std::unique_ptr<MPackObjectType> innerType;

  MPackObjectType() = default;

  MPackObjectType(CppType t) : type(t) {}

  MPackObjectType(CppType t, CppType inner)
      : type(t), innerType(std::make_unique<MPackObjectType>(inner)) {}

  MPackObjectType(CppType t, MPackObjectType inner)
      : type(t),
        innerType(std::make_unique<MPackObjectType>(std::move(inner))) {}

  MPackObjectType(const MPackObjectType &o) : type(o.type) {
    if (o.innerType)
      innerType = std::make_unique<MPackObjectType>(*o.innerType);
  }

  MPackObjectType &operator=(const MPackObjectType &o) {
    if (this == &o)
      return *this;
    type = o.type;
    innerType.reset(o.innerType ? new MPackObjectType(*o.innerType) : nullptr);
    return *this;
  }
};
