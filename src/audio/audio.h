#pragma once
#include <Audio.h>

namespace Speaker {

  #define I2S_BCLK D2
  #define I2S_LRCLK D1
  #define I2S_DOUT D3

  void setup(Audio &audio, uint8_t vol = 12, uint16_t timeout_ms = 10000, uint16_t timeout_ssl_ms = 10000);
}