#pragma once

#include <cstddef>

struct MPackArrayBase {
  size_t size = 0;
  void *p = nullptr;
};

template <typename T> struct MPackArray : MPackArrayBase {
  operator T *() { return static_cast<T *>(p); }
  operator const T *() const { return static_cast<const T *>(p); }

  T &operator[](size_t index) { return static_cast<T *>(p)[index]; }
  const T &operator[](size_t index) const { return static_cast<const T *>(p)[index]; }

  T *begin() { return static_cast<T *>(p); }
  T *end() { return static_cast<T *>(p) + size; }

  const T *begin() const { return static_cast<const T *>(p); }
  const T *end() const { return static_cast<const T *>(p) + size; }
};
