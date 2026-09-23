#pragma once

#include <array>
#include <cstddef>
#include <string_view>

template <size_t Capacity>
class FixedString {
  public:
    bool assign(std::string_view text);
    void clear();

    [[nodiscard]] const char* cStr() const;
    [[nodiscard]] std::string_view view() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] static constexpr size_t capacity();
    [[nodiscard]] bool empty() const;

    friend bool operator==(const FixedString&, const FixedString&) = default;

  private:
    std::array<char, Capacity + 1> _storage{};
    size_t _size = 0;
};

template <size_t Capacity>
bool FixedString<Capacity>::assign(std::string_view text) {
    if (text.size() > Capacity) {
        return false;
    }

    for (size_t i = 0; i < text.size(); ++i) {
        _storage[i] = text[i];
    }

    _size = text.size();
    _storage[_size] = '\0';
    return true;
}

template <size_t Capacity>
void FixedString<Capacity>::clear() {
    _size = 0;
    _storage[0] = '\0';
}

template <size_t Capacity>
const char* FixedString<Capacity>::cStr() const {
    return _storage.data();
}

template <size_t Capacity>
std::string_view FixedString<Capacity>::view() const {
    return {_storage.data(), _size};
}

template <size_t Capacity>
size_t FixedString<Capacity>::size() const {
    return _size;
}

template <size_t Capacity>
constexpr size_t FixedString<Capacity>::capacity() {
    return Capacity;
}

template <size_t Capacity>
bool FixedString<Capacity>::empty() const {
    return _size == 0;
}