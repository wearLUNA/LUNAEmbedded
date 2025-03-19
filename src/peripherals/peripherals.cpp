#include "peripherals.h"

namespace Touch {

  const uint32_t threshold = 25000;

  bool is_touched() {
    uint32_t val = touchRead(TOUCH_PIN);

    return val > threshold;
  }
}

namespace Haptic {

  HapticMotor::HapticMotor(uint8_t motor_pin) {
    pin = motor_pin;
  }

  void HapticMotor::setup() {
    Serial.println(pin);
    pinMode(pin, OUTPUT);
    start_task();
  }

  void HapticMotor::vibe() {
    Serial.println(pin);
    Serial.println("vibed");
    digitalWrite(pin, HIGH);
  }

  void HapticMotor::pulse_start() {
    pulse = true;
  }

  void HapticMotor::pulse_stop() {
    pulse = false;
  }

  void HapticMotor::start_task() {
    if (is_task_running) {
      return;
    }
    is_task_running = true;
    xTaskCreate(task_wrapper, "Haptic Motor Task", 2048, this, 1, NULL);
  }

  void HapticMotor::task_wrapper(void *param) {
    HapticMotor *runner = static_cast<HapticMotor*>(param);
    runner->task();
  }

  void HapticMotor::task() {
    Serial.println(pin);
    while (is_task_running) {
      if (pulse) {
        digitalWrite(pin, HIGH);
        vTaskDelay(200);
        digitalWrite(pin, LOW);
        vTaskDelay(400);
      } else {
        digitalWrite(pin, LOW);
      }
      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

}