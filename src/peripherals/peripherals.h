#pragma once
#include <Arduino.h>

// TODO: this entire part needs work
// The haptic code doesn't work.
namespace Touch {

  #define TOUCH_PIN D0

  extern const uint32_t threshold;
  
  bool is_touched();

}

namespace Haptic {

  #define MOTOR_PIN D7

  class HapticMotor {
    private:
      uint8_t pin;
      bool pulse;
      bool is_task_running;

      void start_task();
      static void task_wrapper(void *param);
      void task();

    public:
      HapticMotor(uint8_t motor_pin = MOTOR_PIN);

      void setup();

      void vibe();

      void pulse_start();

      void pulse_stop();
  };

}