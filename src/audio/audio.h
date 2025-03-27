#pragma once
#include "AudioTools.h"

namespace Audio {


  #define I2S_BCLK_OUT D2
  #define I2S_WS_OUT D1
  #define I2S_DOUT_OUT D3

  #define I2S_BCLK_IN 14
  #define I2S_WS_IN 42
  #define I2S_DOUT_IN 41

  #define OUT_BUFFER_SIZE 1024 * 10       // Size of speaker buffer
  #define IN_BUFFER_SIZE 1024             // Size of mic buffer

  class AudioIO {
    private:
      AudioInfo infoOut;                  // Audio configuration for speaker
      AudioInfo infoIn;                   // Audio configuration for mic
      ConfigEqualizer3Bands eqConfig;     // Configuration for speaker equalizer

      I2SStream out;                      // Speaker I2S stream
      I2SStream in;                       // Mic I2S stream

      Equalizer3Bands eq;                 // Equalizer for speaker (cuts high frequecy - speaker is not good so it squeals)
      VolumeStream volumeOut;             // Volume control for speaker
      
      BufferRTOS<uint8_t> bufferOut;      // Buffer for speaker stream
      QueueStream<uint8_t> queueOut;      // Stream wrapper for the buffer

      StreamCopy copierOut;               // Copy from QueueStream to VolumeStream

    public:
      uint8_t bufferIn[IN_BUFFER_SIZE];   // Buffer used for reading from microphone

      /**
       * @brief Construct a new AudioIO object
       * 
       * Speaker: 16bit, single channel, 24kHz
       * Mic: 16bit, single channel, 16kHz
       * Volume: 1 by default (boost enabled - can be set to >1)
       * Equalizer: gain_high - 0.5, gain_med - 0.7, gain_low - 1
       * 
       */
      AudioIO();

      /**
       * @brief Returns the space available to write on the speaker stream
       * 
       * @return size_t availble buffer space for writing to speaker stream in bytes
       */
      size_t availableToWrite();

      /**
       * @brief Writes to the speaker stream
       * 
       * Check how much is available to write by calling `availableToWrite` before writing
       * 
       * @param data pointer to the data buffer containing the data to write
       * @param len  length of the data buffer
       * @return size_t bytes of data written
       */
      size_t write(uint8_t *data, size_t len) {
        return this->queueOut.write(data, len);
      }

      /**
       * @brief Read the mic stream into buffer
       * 
       * The method reads any available data in the mic stream to `bufferIn`
       * The buffer is guaranteed to be full after this operation.
       * Data can be accessed through the public `bufferIn`
       * 
       * @return size_t bytes of data read
       */
      size_t read();

      /**
       * @brief Set speaker volume
       * 
       * Volume is a float between 0 and 1.
       * Value greater than 1 will boost the audio level in the original signal
       * 
       * @param vol volume value
       * @return true if successful
       * @return false if unsuccessful
       */
      bool setOutVolume(float vol);

      /**
       * @brief Loop function to be called in the main loop
       * 
       */
      void loop();
  };
}
