#include "setup.h"

namespace Camera {

  camera_config_t* setup() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_SVGA; // #TODO: Test if we can increase
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 12;
    config.fb_count = 2;

    return &config;
  }

  esp_err_t initalize_camera(camera_config_t *config) {
    esp_err_t err = esp_camera_init(config);
    if (err != ESP_OK) {
      return err;
    }

    sensor_t *s = esp_camera_sensor_get();
    // Image looks better to humans without gamma adjustment for OV5640
    // TODO: Check whether gamma on or off is better for AI
    s->set_raw_gma(s, 0); 

    // For some reason enabling lens correction introduces purple tint to left side of image
    // TODO: Check if there's some configuration that needs to be changed to use lens correction on OV5640
    s->set_lenc(s, 0);

    return ESP_OK;
  }
}