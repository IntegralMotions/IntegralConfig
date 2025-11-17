#pragma once

#include <cstddef>

struct MPackArrayBase {
  size_t size = 0;
};

template <typename T> struct MPackArray : MPackArrayBase {
  T *p = nullptr;

  operator T *() { return p; }
  operator const T *() const { return p; }

  T &operator[](size_t i) { return p[i]; }
  const T &operator[](size_t i) const { return p[i]; }

  T *begin() { return p; }
  T *end() { return p + size; }
  const T *begin() const { return p; }
  const T *end() const { return p + size; }
};