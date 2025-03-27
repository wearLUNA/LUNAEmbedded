#include "audio.h"

namespace Audio {
  AudioIO::AudioIO()
    : infoOut(24000, 1, 16),
      infoIn(16000, 1, 16),
      eq(out),
      volumeOut(eq),
      bufferOut(OUT_BUFFER_SIZE),
      queueOut(bufferOut),
      copierOut(volumeOut, queueOut) {
    queueOut.begin();

    auto inConfig = in.defaultConfig(RX_MODE);
    inConfig.copyFrom(infoIn);
    inConfig.signal_type = PDM;
    inConfig.i2s_format = I2S_STD_FORMAT;
    inConfig.pin_bck = I2S_BCLK_IN;
    inConfig.pin_ws = I2S_WS_IN;
    inConfig.pin_data = I2S_DOUT_IN;
    inConfig.pin_data_rx = I2S_DOUT_IN;
    inConfig.port_no = I2S_NUM_0;
    in.begin(inConfig);

    auto outConfig = out.defaultConfig(TX_MODE);
    outConfig.copyFrom(infoOut);
    outConfig.pin_bck = I2S_BCLK_OUT;
    outConfig.pin_ws = I2S_WS_OUT;
    outConfig.pin_data = I2S_DOUT_OUT;
    outConfig.port_no = I2S_NUM_1;
    out.begin(outConfig);


    eqConfig = eq.defaultConfig();
    eqConfig.copyFrom(outConfig);
    eqConfig.gain_high = 0.5;
    eqConfig.gain_medium = 0.7;
    eqConfig.gain_low = 1;
    eq.begin(eqConfig);

    auto vcfg = volumeOut.defaultConfig();
    vcfg.copyFrom(outConfig);
    vcfg.allow_boost = true;
    volumeOut.begin(vcfg);
    volumeOut.setVolume(1);
  }

  size_t AudioIO::availableToWrite() {
    return this->queueOut.availableForWrite();
  }

  size_t AudioIO::read() {
    return this->in.readBytes(this->bufferIn, IN_BUFFER_SIZE);
  }

  bool AudioIO::setOutVolume(float vol) {
    return volumeOut.setVolume(vol);
  }

  void AudioIO::loop() {
    copierOut.copy();
  }
}