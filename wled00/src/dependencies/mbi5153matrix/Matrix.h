#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>
#include <stddef.h>

#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_task_wdt.h>

#include "Arduino.h"
#include "app_constants.hpp"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd_dma_parallel16.hpp"
#include "sdkconfig.h"
#include "spi_dma_seg_tx_loop.h"

#include <array>
#include <GFX_Lite.h>

class Matrix : public GFX {
 public:
  Matrix();
  ~Matrix();

  void initMatrix();
  void refreshMatrixConfig();
  void update();
  void updateRegisters();

  // Sets the MBI5153 LED drive current (config register 1, bits [5:0]).
  // Accepts a raw 0-63 value, or one of the named BRIGHTNESS_* constants in app_constants.hpp.
  void setBrightness(uint8_t newBrightness);
  uint8_t getBrightness() const;

  // When enabled, update() keeps previous pixel data instead of clearing after each frame.
  void setImagePersistence(bool enabled);
  bool getImagePersistence() const;

  // Explicitly clear the internal frame buffer.
  void clearFrameBuffer();

  uint8_t getXResolution();
  uint8_t getYResolution();

  // Always implement basic AdaFruit_GFX virtual functions.
  void drawPixel(int16_t x, int16_t y, CRGB color);
  void drawPixel(uint8_t x, uint8_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data);
  void drawPixel(int16_t x, int16_t y, uint16_t color) override;
  void writePixel(uint8_t x, uint8_t y, uint8_t r_data, uint8_t g_data, uint8_t b_data);
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);

  // Pack 8-bit RGB into the 16-bit 565 format used by the uint16_t drawPixel()/fill*() overloads.
  static uint16_t color(uint8_t r, uint8_t g, uint8_t b);

  // Exposes the same CIE1931 perceptual-brightness lookup table mbi_set_pixel() applies
  // internally before writing to hardware. WLED's ABL current estimate (see
  // Mbi5153Bridge::correctBrightness()/BusMbi5153Matrix::estimateCurrent() in bus_manager.cpp)
  // needs this too: it assumes brightness scales linearly with actual current draw, which is
  // only true against the CIE-corrected value, not the raw 8-bit one - the curve compresses low
  // values well below linear, so estimating from raw values under-predicts how much ABL needs to
  // scale down at low brightness.
  static uint8_t correctBrightness(uint8_t v);

  // Inverse of correctBrightness(): the largest v (0-255) such that correctBrightness(v) <=
  // targetCieValue (CIE[] is monotonic non-decreasing, so this is well-defined; rounds toward a
  // dimmer result rather than an exact match when there's no exact hit, to stay at/under a
  // current budget rather than risk exceeding it). Needed by BusMbi5153Matrix::applyBriLimit():
  // ABL computes its brightness scale as a *linear* fraction of a current budget, but this
  // panel's hardware applies the CIE curve on top of whatever 8-bit value it's given - a linear
  // scale factor doesn't produce a linear fraction of rated current, so the target has to be
  // pushed back through the inverse curve to find the value that actually achieves it.
  static uint8_t inverseCorrectBrightness(uint8_t targetCieValue);

 protected:
  bool initialized = false;
  uint8_t currentLevel = BRIGHTNESS_MIN;  // LED drive current, see setBrightness()
  bool imagePersistenceEnabled = false;

  // D<A Data to send.
  Bus_Parallel16 dma_bus;

  ESP32_GREY_DMA_STORAGE_TYPE *dma_grey_gpio_data = nullptr;

  // Length in bits of the buffer -> sequence of 13 x 16 bits (2 bytes) sent in parallel = length value of 13.
  size_t dma_grey_buffer_parallel_bit_length = 0;
  // Length of buffer in memory used -> sequence of 13 x 16 bits (2 bytes) sent in parallel = value of 26 bytes.
  size_t dma_grey_buffer_size = 0;

  void mbi_update_frame(bool configure_latches);
  void mbi_set_pixel(uint8_t x, uint8_t y, uint8_t _r_data, uint8_t _g_data, uint8_t _b_data);

  void mbi_pre_active_dma();
  void mbi_v_sync_dma();
  void mbi_soft_reset_dma();
  void mbi_set_config_dma(ESP32_GREY_DMA_STORAGE_TYPE *out_buf,
                          unsigned int &dma_output_pos,
                          uint16_t config_reg_r,
                          uint16_t config_reg_gb,
                          bool latch,
                          bool reg2);
  void mbi_send_config_reg1_dma();
  void mbi_send_config_reg2_dma();
};

#endif  // MATRIX_H
