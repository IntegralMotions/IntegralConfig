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

    MPackObjectType(CppType type) : type(type) {}

    MPackObjectType(CppType type, CppType inner) : type(type), innerType(std::make_unique<MPackObjectType>(inner)) {}

    MPackObjectType(CppType type, const MPackObjectType &inner)
        : type(type), innerType(std::make_unique<MPackObjectType>(inner)) {}

    MPackObjectType(const MPackObjectType &objectType) : type(objectType.type) {
        if (objectType.innerType) {
            innerType = std::make_unique<MPackObjectType>(*objectType.innerType);
        }
    }

    MPackObjectType &operator=(const MPackObjectType &objectType) {
        if (this == &objectType) {
            return *this;
        }
        type = objectType.type;
        innerType.reset(objectType.innerType ? new MPackObjectType(*objectType.innerType) : nullptr);
        return *this;
    }
};
