#pragma once
#include "esp_camera.h"
#include "camera_pins.h"

namespace Camera {
  
  /**
   * @brief Setups the ESP Camera Config
   * 
   * The function assigns the pinout of the camera module according to the definitions in camera_pins.h
   * It also sets the frame size, pixel format, grab mode, frame buffer, image quality related configs.
   * 
   * Relevant Settings:
   *  - frame_size: FRAMESIZE_SVGA
   *  - pixel_format: PIXFORMAT_JPEG
   *  - grab_mode: grab_mode
   *  - fb_location: CAMERA_FB_IN_PSRAM
   *  - jpeg_quality: 12
   *  - fb_count: 2
   * 
   * @param config pointer to the camera config struct
   */
   void setup(camera_config_t* config);

  /**
   * @brief Initializes the camera
   * 
   * This function initializes the camera according to the config struct provided.
   * It also makes some additional configurations to the sensor settings.
   * 
   * Relevant Settings:
   *  - raw_gma: 0
   *  - lenc: 0
   * 
   * @param config Pointer to the camera config struct
   * @return int ESP if initialization is successful
   */
  esp_err_t initalize_camera(camera_config_t *config);

  camera_fb_t *take_photo();

  void return_fb(camera_fb_t *fb);
}