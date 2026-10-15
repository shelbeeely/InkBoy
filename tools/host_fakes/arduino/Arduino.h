#pragma once
/* Minimal Arduino surface for compiling FreeInk SDK sources in C++ host tests. */
#include <stdint.h>
#include "driver/gpio.h"
#define HIGH 1
#define LOW 0
#define INPUT 1
#define OUTPUT 2
#define INPUT_PULLUP 5
#define INPUT_PULLDOWN 9
#define ARDUINO_USB_CDC_ON_BOOT 0
inline void pinMode(int pin, int mode) { (void)pin; (void)mode; }
inline void digitalWrite(int pin, int level) { gpio_set_level(pin, (uint32_t)level); }
inline int digitalRead(int pin) { return gpio_get_level(pin); }
inline void delay(unsigned long ms) { (void)ms; }
inline void delayMicroseconds(unsigned us) { (void)us; }
inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }
struct HostSerial {};
inline HostSerial Serial;
