#include "audio.h"

namespace Speaker {

  void setup(Audio &audio, uint8_t vol, uint16_t timeout_ms, uint16_t timeout_ssl_ms) {
    audio.setPinout(I2S_BCLK, I2S_LRCLK, I2S_DOUT);
    audio.setVolume(vol);
    audio.setConnectionTimeout(timeout_ms, timeout_ssl_ms);
    audio.setFileLoop(false);
  }

}