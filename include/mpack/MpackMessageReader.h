#pragma once

#include <mpack/mpack.h>
#include <mpack/mpack-reader.h>
#include "im/configuration/configuration.h"

class MpackMessageReader {
public:
  bool HasError() { return mpack_reader_error(&m_reader) != mpack_ok; }
  void Reset() {
    mpack_reader_destroy(&m_reader);
    mpack_reader_init(&m_reader, g_buf, sizeof(g_buf), 0);
    mpack_reader_set_fill(&m_reader, serial_fill);
  }

private:
  size_t serial_fill(mpack_reader_t *reader, char *buffer, size_t count) {
    size_t got = Serial.readBytes(buffer, count);
    return got;
  }

private:
  mpack_reader_t m_reader;
  static char g_buf[512];
};
