#include "Ssd1681Driver.h"

#include <BoardConfig.h>

#include <cstring>

#if FREEINK_DRIVER_SSD1681
#include <esp_heap_caps.h>
#include <esp_log.h>
extern "C" {
#include <ssd1681_fast.h>  // provided by the device env (FreeInkBoy's components/ssd1681_fast)
}
#endif

namespace freeink {

#if FREEINK_DRIVER_SSD1681
namespace {
const char* TAG = "ssd1681_driver";
// Leave room for one Full, FAST_PULSES scans and the HOLD ending before the cap.
constexpr unsigned SESSION_RESERVE = Ssd1681Driver::FAST_PULSES + 4;

ssd1681_handle_t handleOf(void* handle) { return static_cast<ssd1681_handle_t>(handle); }

uint32_t frameBytes() {
  return static_cast<uint32_t>(BoardConfig::ACTIVE.displayWidth / 8) * BoardConfig::ACTIVE.displayHeight;
}
}  // namespace

uint32_t Ssd1681Driver::spiHz() const {
  return BoardConfig::ACTIVE.displaySpiHz ? BoardConfig::ACTIVE.displaySpiHz : SSD1681_SPI_HZ;
}

PanelGeometry Ssd1681Driver::geometry() const {
  const uint16_t w = BoardConfig::ACTIVE.displayWidth;
  const uint16_t h = BoardConfig::ACTIVE.displayHeight;
  return {w, h, static_cast<uint16_t>(w / 8), frameBytes()};
}

void Ssd1681Driver::begin(EpdBus& bus) {
  (void)bus;
  // ssd1681_fast's fixed packages are written for its 200x200 frame.
  if (frameBytes() != SSD1681_FRAME_BYTES) {
    ESP_LOGE(TAG, "profile geometry %ux%u does not match SSD1681 %ux%u", BoardConfig::ACTIVE.displayWidth,
             BoardConfig::ACTIVE.displayHeight, SSD1681_WIDTH, SSD1681_HEIGHT);
  }
}

bool Ssd1681Driver::reserveFastBuffers() {
  // 15 KB of internal DMA RAM, held only while a session stays powered for fast
  // updates; a Full that turns the panel off never needs them.
  const uint32_t caps = MALLOC_CAP_DMA | MALLOC_CAP_8BIT;
  if (!_previous) _previous = static_cast<uint8_t*>(heap_caps_malloc(SSD1681_FRAME_BYTES, caps));
  if (!_black) _black = static_cast<uint8_t*>(heap_caps_malloc(SSD1681_FRAME_BYTES, caps));
  if (!_white) _white = static_cast<uint8_t*>(heap_caps_malloc(SSD1681_FRAME_BYTES, caps));
  if (_previous && _black && _white) return true;
  ESP_LOGW(TAG, "no DMA memory for fast updates; using full refreshes");
  releaseFastBuffers();
  return false;
}

void Ssd1681Driver::releaseFastBuffers() {
  heap_caps_free(_previous);
  heap_caps_free(_black);
  heap_caps_free(_white);
  _previous = _black = _white = nullptr;
}

void Ssd1681Driver::deepSleep(EpdBus& bus) {
  (void)bus;
  closeSession();
}

void Ssd1681Driver::display(EpdBus& bus, const uint8_t* fb, const uint8_t* prev, RefreshMode mode, bool turnOff) {
  (void)bus;
  (void)prev;  // single-buffer facades pass nullptr; the driver keeps its own copy
  _committed = false;
  if (!fb || frameBytes() != SSD1681_FRAME_BYTES) return;
  if (_handle && _updates + SESSION_RESERVE >= SSD1681_MAX_UPDATES) closeSession();
  // Fast needs the previous frame of this session; without it, refresh fully.
  const bool canFast = mode == RefreshMode::Fast && _fullDone && _previous;
  if (!turnOff && !canFast) reserveFastBuffers();
  const bool ok = canFast ? fast(fb) : full(fb);
  if (ok && turnOff) closeSession();
  _committed = ok;
}

bool Ssd1681Driver::openSession() {
  if (_handle) return true;
  const auto& d = BoardConfig::ACTIVE.display;
  const ssd1681_pins_t pins = {d.sclk, d.mosi, d.cs, d.dc, d.rst, d.busy, d.powerEnable};
  esp_err_t error = ssd1681_configure_pins(&pins);
  if (error) return fail("pins", error);
  ssd1681_handle_t handle = nullptr;
  error = ssd1681_create(&handle);
  if (error) return fail("create", error);
  _handle = handle;
  error = ssd1681_power_on(handle);
  if (error) return fail("power on", error);
  _updates = 0;
  return true;
}

void Ssd1681Driver::closeSession() {
  if (_handle) {
    const esp_err_t error = ssd1681_destroy(handleOf(_handle));
    if (error) ESP_LOGW(TAG, "power off: %s", esp_err_to_name(error));
  }
  _handle = nullptr;
  _fullDone = _mixed = false;
  _updates = 0;
  releaseFastBuffers();
}

bool Ssd1681Driver::fail(const char* step, int error) {
  ESP_LOGE(TAG, "%s: %s", step, esp_err_to_name(error));
  // The component latches a fault and cuts panel power; start clean next time.
  closeSession();
  return false;
}

bool Ssd1681Driver::full(const uint8_t* fb) {
  // A Full also resets the component's mixed-mode setup and clean counter.
  if (!openSession()) return false;
  ssd1681_timing_t timing;
  const esp_err_t error = ssd1681_vendor_full(handleOf(_handle), fb, SSD1681_FRAME_BYTES, &timing);
  if (error) return fail("full refresh", error);
  ++_updates;
  _fullDone = true;
  _mixed = false;
  if (_previous) memcpy(_previous, fb, SSD1681_FRAME_BYTES);
  return true;
}

bool Ssd1681Driver::fast(const uint8_t* fb) {
  // Framebuffer bit 1 = white. BLACK drives pixels that turn black, WHITE those
  // that turn white; the two masks are disjoint by construction.
  bool changed = false;
  for (uint32_t i = 0; i < SSD1681_FRAME_BYTES; ++i) {
    _black[i] = static_cast<uint8_t>(_previous[i] & ~fb[i]);
    _white[i] = static_cast<uint8_t>(~_previous[i] & fb[i]);
    changed |= (_black[i] | _white[i]) != 0;
  }
  if (!changed) return true;
  ssd1681_handle_t handle = handleOf(_handle);
  ssd1681_timing_t timing;
  esp_err_t error = ESP_OK;
  if (!_mixed) {
    error = ssd1681_vendor_enable_mixed(handle);
    if (!error) error = ssd1681_vendor_set_mixed_frames(handle, 1);
    if (!error) error = ssd1681_vendor_set_fast_transport(handle, true);
    if (error) return fail("fast setup", error);
    _mixed = true;
  } else {
    error = ssd1681_vendor_mixed_resume(handle, &timing);
    if (error) return fail("fast resume", error);
  }
  for (unsigned pulse = 0; pulse < FAST_PULSES; ++pulse) {
    error = ssd1681_vendor_mixed_scan(handle, _black, _white, SSD1681_FRAME_BYTES, &timing);
    if (error) return fail("fast scan", error);
    ++_updates;
  }
  // All-HOLD ending lets the waveform finish instead of stopping mid-drive.
  memset(_black, 0, SSD1681_FRAME_BYTES);
  memset(_white, 0, SSD1681_FRAME_BYTES);
  error = ssd1681_vendor_mixed_scan(handle, _black, _white, SSD1681_FRAME_BYTES, &timing);
  if (error) return fail("hold scan", error);
  ++_updates;
  error = ssd1681_analog_power(handle, false, &timing);
  if (error) return fail("analog off", error);
  memcpy(_previous, fb, SSD1681_FRAME_BYTES);
  return true;
}

PanelDriver& ssd1681Driver() {
  static Ssd1681Driver instance;
  return instance;
}
#endif

}  // namespace freeink
