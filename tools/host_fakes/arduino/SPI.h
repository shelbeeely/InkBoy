#pragma once
#include <stdint.h>
#define MSBFIRST 1
#define SPI_MODE0 0
struct SPISettings {
  SPISettings() = default;
  SPISettings(uint32_t, uint8_t, uint8_t) {}
};
