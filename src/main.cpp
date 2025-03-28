#include "./network/accesspoint.h"
#include "Adafruit_DRV2605.h"
#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>

#include "api/api.h"
#include "audio/audio.h"
#include "camera/camera.h"
#include "peripherals/peripherals.h"

// ====== TEMP Wifi Details ======
const char *ssid = "Rogers 2608";
const char *password = "9955FFC361A9";
// ===============================

// """
// idle
// connection has been initiated but not complete
// connection is live
// """

// const uint32_t TOUCH_THRESHOLD = 60000;

int i = 0;

// const int *D0 = d0;
// D0 touch 1
// D1 touch 2

bool touched = false;
bool touchedMotorFlag = false;

//

Haptic::HapticMotor haptic;
Audio::AudioIO audio;
API::LiveClient liveClient(audio, String(ESP.getEfuseMac()));
API::APIClient apiClient(audio, String(ESP.getEfuseMac()));
Adafruit_DRV2605 drv;

// On first initialization, to avoid error spamming, we use this variable in the
// loop() function
bool useAccessPoint = false;

void MotorTask(void *);
void LEDTask(void *);

void sleepIRS() {

}


void setup() {

  // turn on DAC. Set to LOW to turn off
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // AccessPoint::clearWifiCreds();

  // delay(2000);
  // while(true) {
  //   Serial.println(touchRead(D0));
  //   delay(1);
  // }
  if (!AccessPoint::hasWifiCreds()) {
    Serial.println("No Wifi credentials found. Starting server.");
    AccessPoint::setupAccessPoint();
    useAccessPoint = true;
  } else {
    bool connected = AccessPoint::connectToWifi();
    if (!connected) {
      Serial.println("Starting Server");
      AccessPoint::setupAccessPoint();
      useAccessPoint = true;
    }
  }

  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin(D9, D8);
  drv.begin();
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  drv.setWaveform(0, 82);
  Serial.println("wakeup sound");
  drv.setWaveform(1, 0);
  drv.go();

  xTaskCreate(LEDTask,    // name of function
              "Led Task", // name for my own sake
              2048,       // stack size
              NULL, // if multiple tasks of the same function are started, we
                    // can keep track by passing the reference to a variable
                    // that uniquely keeps track of the task
              2,   // priority of the task
              NULL // task handler, no need here
  );
  xTaskCreate(MotorTask,    // name of function
              "Motor Task", // name for my own sake
              2048,         // stack size
              NULL, // if multiple tasks of the same function are started, we
                    // can keep track by passing the reference to a variable
                    // that uniquely keeps track of the task
              2,   // priority of the task
              NULL // task handler, no need here
  );

  camera_config_t config;
  Camera::setup(&config);
  esp_err_t err = Camera::initalize_camera(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  haptic.setup();

  touchAttachInterrupt(D0, sleepIRS, 80000);
  esp_sleep_enable_touchpad_wakeup();

  // ====== TEMP WIFI Code =======
  // Serial.print("Connecting to ");
  // Serial.println(ssid);
  // Serial.print("With password ");
  // Serial.println(password);

  // WiFi.begin(ssid, password);
  // WiFi.setSleep(false);
  // unsigned long long startTime = millis();
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  //   if(millis() - startTime > 5000){
  //     ESP.restart();
  //   }
  // }
  // Serial.println("");
  // Serial.println("WiFi connected");
  // ==============================
}

bool started = false;
bool stopped = true;
ulong startTime = millis();
bool startedDeepSleep = false;
bool playTurnOff = false;
ulong somethingTime = millis();
ulong offTime;


void loop() {

  if (useAccessPoint) {
    AccessPoint::connectClients();
    //theres a bunch of these that should be blocking, go through them and make sure they are.
  }

  if (touchRead(D0) > 65000 && !started && millis() - startTime >= 2000 && !startedDeepSleep) {
    touchedMotorFlag = true;
    if(liveClient.beginLive()){
      started = true;
      startTime = millis();
    }
  } else if (touchRead(D0) > 65000 && started && millis() - startTime >= 2000 &&!startedDeepSleep && started) {
    started = false;
    touchedMotorFlag = true;
    Serial.println("Its off");
    liveClient.endLive();
    startTime = millis();
  }
  if (touchRead(D0) > 65000) {
    if (!startedDeepSleep)
      somethingTime = millis();
    startedDeepSleep = true;
  } else {
    startedDeepSleep = false;
  }

  if (startedDeepSleep && millis() - somethingTime >= 3000) {
    startedDeepSleep = false;
    playTurnOff  = true;
    liveClient.endLive();

    delay(2000);
    Serial.println("Sleeping");
  }

  audio.loop();
  apiClient.loop();

  // if (touchAttachInterrupt  )
  // Serial.println(touchRead(D0));

  // if ( ) {
  //   // apiClient.startRequest();
  //   liveClient.beginLive();
  //   if (!started) {
  //       Serial.println("touched!");
  //       started = true;

  //       // liveClient.beginLive();
  //   }
  // }
  // i++;

  // Start http request using apiClient.startRequest()
  // Start live websocket using liveClient.beginLive()
  // if (Touch::is_touched() && !started) {
  //   Serial.println("touched!");
  //   started = true;

  //   liveClient.beginLive();
  // }
}

void LEDTask(void *) {
  while (true) {
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(500);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(500);
  }
  vTaskDelete(NULL);
}

void MotorTask(void *) {
  unsigned long long lastTime = millis();
  const unsigned long long connectedPeriod = 1000; // 300 ms
  bool connectedLiveClient = false;
  while (true) {

    if (playTurnOff) {
      drv.setWaveform(0, 70);
      drv.setWaveform(1, 0);
      drv.go();
      delay(2000);
      esp_deep_sleep_start();
    }

    if (touchedMotorFlag) {
      drv.setWaveform(0, 1);
      drv.setWaveform(1, 0);
      drv.go();
      touchedMotorFlag = false;
    }

    if (started) {
      if (millis() - lastTime > connectedPeriod) {
        drv.setWaveform(0, 17);
        drv.setWaveform(1, 0);
        drv.go();
        lastTime = millis();
        if (!connectedLiveClient && liveClient.isConnected()) {
          connectedLiveClient = true;
          drv.setWaveform(0, 17);
          drv.setWaveform(1, 0);
          drv.go();
          vTaskDelay(500);

          drv.setWaveform(0, 17);
          drv.setWaveform(1, 0);
          drv.go();
          vTaskDelay(50);

          drv.setWaveform(0, 17);
          drv.setWaveform(1, 0);
          drv.go();
        }
      }
    } else {
      if (connectedLiveClient && !liveClient.isConnected()){
          connectedLiveClient = false;
          drv.setWaveform(0, 17);
          drv.setWaveform(1, 0);
          drv.go();
          vTaskDelay(500);

          drv.setWaveform(0, 17);
          drv.setWaveform(1, 0);
          drv.go();
          vTaskDelay(50);

          drv.setWaveform(0, 17);
          drv.setWaveform(1, 0);
          drv.go();
      }
      drv.setWaveform(0,0);
    }
    vTaskDelay(5);
  }
  vTaskDelete(NULL);
}