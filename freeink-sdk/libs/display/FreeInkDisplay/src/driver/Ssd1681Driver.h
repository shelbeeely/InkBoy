#pragma once

// SSD1681 driver for the Waveshare ESP32-S3-ePaper-1.54 V2 (200x200).
//
// An external-library driver: it does not use EpdBus. The panel is handed to the
// `ssd1681_fast` component, which owns the SPI bus, the BUSY interrupt and the
// active-low panel supply, and which exposes only a reviewed, whitelisted set of
// SSD1681 commands (no OTP or raw-LUT path). Pins still come from the active
// BoardProfile and are passed to the component before each session.
//
// Refresh modes:
//   Full / Half  the stock Waveshare full waveform (flashing, clears ghosting).
//   Fast         after one Full in the session, the component's mixed BLACK/WHITE
//                one-frame scans: every changed pixel gets FAST_PULSES pulses toward
//                its new colour, then one all-HOLD scan ends the waveform cleanly.
// A session is recycled (power down, new Full) before the component's per-session
// update cap is reached. A refresh with turnOff (standing screens) allocates nothing;
// a session kept powered reserves three 5 KB DMA buffers until it powers down.
//
// Selection: linked only when FREEINK_DRIVER_SSD1681 (derived from
// FREEINK_DEVICE_WS154). The device env must provide the ssd1681_fast component.

#include "PanelDriver.h"

namespace freeink {

class Ssd1681Driver : public PanelDriver {
 public:
  static constexpr unsigned FAST_PULSES = 8;

  uint32_t spiHz() const override;
  BusyPolarity busyPolarity() const override { return BusyPolarity::ActiveHigh; }
  bool usesExternalBus() const override { return true; }
  PanelGeometry geometry() const override;

  void begin(EpdBus& bus) override;
  void deepSleep(EpdBus& bus) override;
  void display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) override;
  // False when the last display() failed; the component then cut panel power.
  bool displayCommitted() const override { return _committed; }

 private:
  bool openSession();
  void closeSession();
  bool full(const uint8_t* fb);
  bool fast(const uint8_t* fb);
  bool fail(const char* step, int error);
  bool reserveFastBuffers();
  void releaseFastBuffers();

  void* _handle = nullptr;  // ssd1681_handle_t, kept opaque so the header stays component-free
  uint8_t* _previous = nullptr;
  uint8_t* _black = nullptr;
  uint8_t* _white = nullptr;
  unsigned _updates = 0;
  bool _fullDone = false;
  bool _mixed = false;
  bool _committed = true;
};

PanelDriver& ssd1681Driver();

}  // namespace freeink
