#pragma once

// Thin, CRGB-free bridge between the vendored MBI5153 panel driver (Matrix.h, which pulls in
// GFX_Lite and defines its own global `struct CRGB`) and WLED's bus_manager.cpp (which, when
// WLED_ENABLE_HUB75MATRIX is also active, pulls in src/dependencies/fastled_slim's own
// `struct CRGB`). Keeping GFX_Lite/Matrix.h out of bus_manager.cpp's translation unit avoids a
// duplicate-definition clash between those two unrelated CRGB types.
//
// Only plain integer types cross this boundary.

#include <stdint.h>

namespace Mbi5153Bridge {

  // Physical panel resolution (DMG1083, 78x78 after ghost-pixel compensation).
  // Mirrors PANEL_PHY_RES_X / PANEL_PHY_RES_Y in app_constants.hpp.
  constexpr uint16_t WIDTH  = 78;
  constexpr uint16_t HEIGHT = 78;

  // All GPIOs the underlying driver touches, including the two nominal (electrically inert)
  // SPI2 octal-bus validation pins that aren't named constants in app_constants.hpp. Used to
  // register ownership with WLED's PinManager so the UI can't hand these out to another
  // peripheral (IR, relay, button, etc.).
  constexpr uint8_t PIN_COUNT = 22;
  extern const uint8_t PINS[PIN_COUNT];

  // Brings up the panel: allocates DMA buffers, configures the MBI5153 registers, and starts
  // the background SPI-DMA GCLK/address loop. Safe to call once; subsequent calls are no-ops.
  bool begin();

  void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);

  // Pushes the pending frame out over DMA. The MBI5153 chips PWM-refresh autonomously afterward
  // via the background SPI-DMA loop started in begin() - this only needs to be called when pixel
  // data actually changed.
  void show();

  // Applies the same CIE1931 perceptual-brightness curve mbi_set_pixel() applies before writing
  // to hardware. WLED's ABL current estimate (BusMbi5153Matrix::estimateCurrent() in
  // bus_manager.cpp) needs this to correct a value BEFORE accumulating it, since actual current
  // draw is proportional to the CIE-corrected value, not the raw 8-bit one - see
  // Matrix::correctBrightness()'s comment for why.
  uint8_t correctBrightness(uint8_t v);

  // Inverse of correctBrightness() - see Matrix::inverseCorrectBrightness()'s comment. Needed by
  // BusMbi5153Matrix::applyBriLimit() to turn ABL's linear current-budget scale factor into the
  // actual brightness value that achieves it once the hardware's own CIE curve is applied.
  uint8_t inverseCorrectBrightness(uint8_t targetCieValue);

} // namespace Mbi5153Bridge
