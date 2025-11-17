#pragma once

#include <cstdint>

class Communication {
public:
  Communication();
  virtual ~Communication();

  virtual void write(uint8_t *data, const uint8_t data_size) = 0;
  virtual bool tryRead(uint8_t *data, const uint8_t data_size) = 0;
};
