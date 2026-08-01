#pragma once

#include "MPackObjectBase.h"
#include "MPackObjectMember.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

template <typename Derived, size_t MaxMembers>
class MPackObject : public MPackObjectBase {
  public:
    MPackObject();

  protected:
    template <typename Class, typename Member>
    static void registerMember(const char* name, const MPackObjectType& type, Member Class::* memberPtr);

    [[nodiscard]] const MPackObjectMember* getMembers() const override;
    [[nodiscard]] size_t memberCount() const override;
    [[nodiscard]] void* getMemberAddress(const MPackObjectMember& member) const override;

  private:
    static inline std::array<MPackObjectMember, MaxMembers> members{};
    static inline size_t memberIndex = 0;
};

template <typename Derived, size_t MaxMembers>
MPackObject<Derived, MaxMembers>::MPackObject() {
    Derived::registerMembers();
}

template <typename Derived, size_t MaxMembers>
template <typename Class, typename Member>
void MPackObject<Derived, MaxMembers>::registerMember(const char* name, const MPackObjectType& type,
                                                      Member Class::* memberPtr) {
    static_assert(std::is_base_of_v<Class, Derived>, "Class must be a base of Derived");

    if (memberIndex >= MaxMembers) {
        return;
    }

    const auto Offset = reinterpret_cast<std::size_t>(&(static_cast<const Class*>(nullptr)->*memberPtr));

    members[memberIndex++] = {name, type, Offset};
}
template <typename Derived, size_t MaxMembers>
const MPackObjectMember* MPackObject<Derived, MaxMembers>::getMembers() const {
    return members.data();
}

template <typename Derived, size_t MaxMembers>
size_t MPackObject<Derived, MaxMembers>::memberCount() const {
    return memberIndex;
}

template <typename Derived, size_t MaxMembers>
void* MPackObject<Derived, MaxMembers>::getMemberAddress(const MPackObjectMember& member) const {
    auto* self = static_cast<const Derived*>(this);
    const auto* base = reinterpret_cast<const uint8_t*>(self);
    return const_cast<uint8_t*>(base + member.offset);
}
