#include "mbi5153matrix_bridge.h"

#include "Matrix.h"
#include "app_constants.hpp"

namespace Mbi5153Bridge {

const uint8_t PINS[PIN_COUNT] = {
  ADDR_A_PIN, ADDR_B_PIN, ADDR_C_PIN, ADDR_D_PIN, ADDR_E_PIN,
  MBI_GCLK, MBI_LAT, MBI_DCLK, MBI_SRCLK,
  MBI_G1, MBI_B1, MBI_R1,
  MBI_G2, MBI_B2, MBI_R2,
  MBI_G3, MBI_B3, MBI_R3,
  MBI_G4, MBI_B4, MBI_R4,
  8, // SPI2 octal-bus MOSI validation pin (spi_dma_seg_tx_loop.c) - electrically unused but
     // claimed by spi_bus_initialize(), so it must be reserved too. MISO's validation pin is
     // GPIO 9, already covered above via MBI_SRCLK.
};

#ifndef MBI5153_DRIVE_CURRENT
// Hardware current-limit register (0-63, config reg1 bits[5:0], see BRIGHTNESS_* above) - a
// fixed mechanical/thermal ceiling, not a dimmer. WLED's ABL (see BusMbi5153Matrix in
// bus_manager.cpp) does the actual dynamic, content-aware current limiting against your real
// PSU's rating - see MBI5153_LED_MILLIAMPS_DEFAULT in platformio.ini, which must be derived
// against whatever level is configured here (they are coupled: this sets the true maximum the
// panel can ever draw, ABL only ever dims down from it). BRIGHTNESS_HIGH gives ABL real headroom
// (~5A rated) while staying inside the driver's own documented-safe range - only BRIGHTNESS_MAX
// is flagged "gets very hot, use with caution". Override via platformio.ini build_flags, e.g.
// -D MBI5153_DRIVE_CURRENT=BRIGHTNESS_MEDIUM, and update MBI5153_LED_MILLIAMPS_DEFAULT to match.
#define MBI5153_DRIVE_CURRENT BRIGHTNESS_HIGH
#endif

namespace {
  Matrix matrix;
  bool began = false;
}

bool begin() {
  if (began) return true;
  matrix.initMatrix();
  matrix.setBrightness(MBI5153_DRIVE_CURRENT);
  // WLED owns the authoritative frame state (it redraws every changed pixel each frame via its
  // own segment buffer) - keep the panel showing the last-pushed frame between updates rather
  // than having the driver clear it, matching how every other WLED bus type behaves.
  matrix.setImagePersistence(true);
  began = true;
  return began;
}

void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
  if (!began) return;
  matrix.drawPixel((uint8_t)x, (uint8_t)y, r, g, b);
}

void show() {
  if (!began) return;
  matrix.update();
}

uint8_t correctBrightness(uint8_t v) {
  return Matrix::correctBrightness(v); // pure lookup table, no hardware/began dependency
}

uint8_t inverseCorrectBrightness(uint8_t targetCieValue) {
  return Matrix::inverseCorrectBrightness(targetCieValue); // pure lookup table, no hardware/began dependency
}

} // namespace Mbi5153Bridge
