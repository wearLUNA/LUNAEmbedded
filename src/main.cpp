#include "./network/accesspoint.h"
#include "Adafruit_DRV2605.h"
#include "api/api.h"
#include "audio/audio.h"
#include "camera/camera.h"
#include "esp_camera.h"
#include "peripherals/peripherals.h"
#include <Arduino.h>
#include <Preferences.h> // GET RID OF THIS NOW
#include <WiFi.h>

// ====== TEMP Wifi Details ======
const char *ssid = "Rogers 2608";
const char *password = "9955FFC361A9";
// ===============================

// """
// idle
// connection has been initiated but not complete
// connection is live
// """

const uint32_t TOUCH_THRESHOLD_DELTA = 2000;

int i = 0;

// const int *D0 = d0;
// D0 touch 1
// D1 touch 2

volatile bool touched = false;
volatile bool touchedMotorFlag = false;


// Haptic::HapticMotor haptic;
Audio::AudioIO audio;
API::LiveClient liveClient(audio, String(ESP.getEfuseMac()));
API::APIClient apiClient(audio, String(ESP.getEfuseMac()));
Adafruit_DRV2605 drv;

// On first initialization, to avoid error spamming, we use this variable in the
// loop() function
bool useAccessPoint = false;

void MotorTask(void *);
void LEDTask(void *);

uint32_t lastISRTouch = 0;
void touchISR() {

  touched = true;
  touchedMotorFlag = true;
  
}

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


void print_wakeup_touchpad(){
  touch_pad_t touchPin = esp_sleep_get_touchpad_wakeup_status();

  switch(touchPin)
  {
    case 0  : Serial.println("Touch detected on GPIO 4"); break;
    case 1  : Serial.println("Touch detected on GPIO 0"); break;
    case 2  : Serial.println("Touch detected on GPIO 2"); break;
    case 3  : Serial.println("Touch detected on GPIO 15"); break;
    case 4  : Serial.println("Touch detected on GPIO 13"); break;
    case 5  : Serial.println("Touch detected on GPIO 12"); break;
    case 6  : Serial.println("Touch detected on GPIO 14"); break;
    case 7  : Serial.println("Touch detected on GPIO 27"); break;
    case 8  : Serial.println("Touch detected on GPIO 33"); break;
    case 9  : Serial.println("Touch detected on GPIO 32"); break;
    default : Serial.println("Wakeup not by touchpad"); break;
  }
}

void setup() {

  // turn on DAC. Set to LOW to turn off
  pinMode(D4, OUTPUT);
  digitalWrite(D4, HIGH);
  pinMode(LED_BUILTIN, OUTPUT);
  touchAttachInterrupt(D0, touchISR, TOUCH_THRESHOLD_DELTA);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  Wire.begin(D9, D8);
  drv.begin();
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);

  drv.setWaveform(0, 82);
  drv.setWaveform(1, 0);
  drv.go();
  Serial.println("wakeup sound");
  print_wakeup_reason();

  // delay(2000);
  Serial.println("skibidi");
  // while(true) {
  //   Serial.println(touchRead(D0));
  //   delay(1);
  // }

  if (!AccessPoint::hasWifiCreds()) {
    Serial.println("No Wifi credentials found. Starting server.");
    AccessPoint::setupAccessPoint();
    useAccessPoint = true;
  } else {
    unsigned long long startTime = millis();
    while(!AccessPoint::connectToWifi()){
      // if(millis() - startTime > 5000) ESP.restart();

      // if (!connected) {
      //   Serial.println("Starting Server");
      //   AccessPoint::setupAccessPoint();
      //   useAccessPoint = true;
      // }
      drv.setWaveform(0, 49); //pulsing sharp 60%
      drv.setWaveform(1, 0);
      drv.go();

      // delay(500);
      Serial.println("Couldn't connect to wifi, trying again");

    }
  }
  drv.setWaveform(0, 47); //pulsing sharp 100%
  drv.setWaveform(1, 0);
  drv.go();
  delay(200);

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


    // haptic.setup();
    // delay(10);
    // touch_pad_sleep_set_threshold(D0, benchmark * threshold);
    // esp_sleep_enable_touchpad_wakeup();
    touchSleepWakeUpEnable(D0, TOUCH_THRESHOLD_DELTA);

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

unsigned long long lastRisingTime = millis();
void loop() {

  if (useAccessPoint) {
    AccessPoint::connectClients();
    // theres a bunch of these that should be blocking, go through them and make
    // sure they are.
  }

  if (touched) {
    uint32_t newRead = touchRead(D0);
    if (newRead > lastISRTouch) {
      if (!liveClient.isConnected() && !liveClient.processing) {
        Serial.println("starting connection GIGGITY");
        liveClient.beginLive();
      } else if (liveClient.processing) {
        Serial.println("ending connection GIGGITY");
        liveClient.endLive();
      } else {
        Serial.println("Starting but not connected so ignoring press. GIGGITY");
        // liveClient.endLive();
      }

      lastRisingTime = millis();

      // Serial.print("Rising edge with a value of: ");
      // Serial.println(newRead);
    } else {
      if(millis() - lastRisingTime > 6000){
        Serial.println("GOING TO DEEP SLEEP NOW");
        playTurnOff = true;
      }
      // Serial.println("Falling edge with a value of: ");
      // Serial.println(newRead);
    }
    // Serial.println(touchRead(D0));

    lastISRTouch = newRead;
    touched = false;
  }

  // deep sleep only when is not connected and
  //

  // Serial.println(touchRead(D0));

  // if (touchRead(D0) > 65000 && !started && millis() - startTime >= 2000 &&
  // !startedDeepSleep) {
  //   touchedMotorFlag = true;
  //   if(liveClient.beginLive()){
  //     started = true;
  //     startTime = millis();
  //   }
  // } else if (touchRead(D0) > 65000 && started && millis() - startTime >= 2000
  // &&!startedDeepSleep && started) {
  //   started = false;
  //   touchedMotorFlag = true;
  //   Serial.println("Its off");
  //   liveClient.endLive();
  //   startTime = millis();
  // }
  // if (touchRead(D0) > 65000) {
  //   if (!startedDeepSleep)
  //     somethingTime = millis();
  //   startedDeepSleep = true;
  // } else {
  //   startedDeepSleep = false;
  // }

  // if (startedDeepSleep && millis() - somethingTime >= 3000) {
  //   startedDeepSleep = false;
  //   playTurnOff  = true;
  //   liveClient.endLive();

  //   delay(2000);
  //   Serial.println("Sleeping");
  // }

  audio.loop();
  // apiClient.loop();
  // liveClient.loop();
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
  unsigned long long tryingToConnectButNotConnectedYetLastTime = millis();
  const unsigned long long tryingToConnectButNotConnectedYetPeriod = 200; // 200 ms
  unsigned long long connectedLastTime = millis();
  const unsigned long long connectedPeriod = 1000; // 1000 ms
  bool connectedLiveClient = false;
  while (true) {

    if (playTurnOff) {
      drv.setWaveform(0, 70);
      drv.setWaveform(1, 0);
      drv.go();
      delay(5000);
      esp_deep_sleep_start();
    }

    if (touchedMotorFlag) {
      drv.setWaveform(0, 1);
      drv.setWaveform(1, 0);
      drv.go();
      touchedMotorFlag = false;
    }


    if(liveClient.isConnected()){
      //actually connected animation
      if(millis() - connectedLastTime > connectedPeriod){
        drv.setWaveform(0, 17);
        drv.setWaveform(1, 0);
        drv.go();


        connectedLastTime = millis();
      }
    }
    else if(liveClient.processing){
      //trying to connect now
      if(millis() - tryingToConnectButNotConnectedYetLastTime > tryingToConnectButNotConnectedYetPeriod){
        drv.setWaveform(0, 17);
        drv.setWaveform(1, 0);
        drv.go();


        tryingToConnectButNotConnectedYetLastTime = millis();
      }
    }
    else{
      //normie stuff
    }
    
    vTaskDelay(1);
  }
  vTaskDelete(NULL);
}