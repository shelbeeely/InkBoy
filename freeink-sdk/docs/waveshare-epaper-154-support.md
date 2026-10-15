# Waveshare ESP32-S3-ePaper-1.54 V2

Device flag: `-DFREEINK_DEVICE_WS154`. Profile: `BoardConfig::WS_EPAPER_154`
(`Board::WsEpaper154`, name `ws154`). Driver: `Ssd1681Driver`
(`FREEINK_DRIVER_SSD1681`, `DisplayController::SSD1681`).

| Part | Wiring |
| --- | --- |
| MCU | ESP32-S3, 8 MB flash, 8 MB octal PSRAM |
| Panel | 1.54" 200x200 B/W, SSD1681: SCK 12, MOSI 13, CS 11, DC 10, RST 9, BUSY 8 |
| Panel supply | GPIO 6, **active-low** (AO3401 P-FET) |
| Keys | BOOT GPIO 0 (`input.confirm`, deep-sleep wake), PWR GPIO 18 (`input.power`), both active-low |
| Battery latch | GPIO 17 (`power.latch0`): drive high early and hold through deep sleep |
| microSD | 1-bit SDMMC: CLK 39, CMD 41, D0 40 |
| Audio | ES8311 at I2C 0x18 (SDA 47, SCL 48), active-low codec rail GPIO 42, amp enable GPIO 46, I2S MCLK 14, BCLK 15, WS 38, DOUT 45 |

## Panel driver

`Ssd1681Driver` is an external-bus driver, like `LgfxEpdDriver`: it hands the
panel to the [`ssd1681_fast`](https://github.com/shelbeeely/FreeInkBoy/tree/main/components/ssd1681_fast)
ESP-IDF component, which owns the SPI bus, the BUSY interrupt and the panel
supply, and exposes only a reviewed whitelist of SSD1681 commands (no OTP or
caller-supplied LUT path). The driver passes the profile's display pins to the
component before each session, so `EpdBus` never touches them.

- `FULL_REFRESH` / `HALF_REFRESH`: Waveshare's stock full waveform.
- `FAST_REFRESH`: after one full refresh in a session, mixed BLACK/WHITE
  one-frame scans; every changed pixel gets 8 pulses toward its new colour, then
  one all-HOLD scan ends the waveform.
- `displayBuffer(..., turnOffScreen = true)` powers the panel down and allocates
  nothing. A session kept powered reserves three 5 KB DMA buffers until it powers
  down.
- `displayCommitted()` is false after a failed refresh; the component has then cut
  panel power, and the next refresh starts a new session.
- Grayscale planes are not supported through the facade.

The device env must build the `ssd1681_fast` component alongside the SDK.
FreeInkBoy (https://github.com/shelbeeely/FreeInkBoy) is the reference
consumer and runs on this board.

## Status

Pin map, panel bring-up, keys, SD and audio are what FreeInkBoy runs on this board.
The `Ssd1681Driver` facade path is host-tested against FreeInkBoy's
SSD1681 state model; its fast-refresh path has not yet been checked on hardware.
