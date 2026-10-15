# FreeInk SDK in FreeInkBoy

This folder is a vendored subset of the [FreeInk SDK](https://github.com/Free-Ink/freeink-sdk)
(MIT, see `LICENSE` and `NOTICE`), plus the Waveshare ESP32-S3-ePaper-1.54 V2
device that FreeInkBoy runs on.

Upstream commit: `a3a920d27874695c6c4f85d48543cefe98623b0d` (2026-10-15)

## What is vendored

Only the libraries FreeInkBoy uses, without their host tests:

| Library | Path | Used for |
| --- | --- | --- |
| BoardConfig | `libs/hardware/BoardConfig` | The board profile: every pin the firmware uses |
| InputManager | `libs/hardware/InputManager` | BOOT and PWR keys |
| FreeInkDisplay (`EInkDisplay`) | `libs/display/FreeInkDisplay` | Standing screens (startup, power off, errors) |

`components/freeink_sdk` builds them as one ESP-IDF component and selects the
device with `FREEINK_DEVICE_WS154`. Gameplay and menus keep FreeInkBoy's own
fast display path (`main/fast_display.c` over `components/ssd1681_fast`), which
the SDK facade does not model.

## FreeInkBoy's additions

The device was added the way the SDK's
[Adding a new device](https://github.com/Free-Ink/freeink-sdk#adding-a-new-device)
section describes:

1. `BoardConfig.h`: `FREEINK_DEVICE_WS154` (ESP32-S3 family), `Board::WsEpaper154`,
   `DisplayController::SSD1681`, the `WS_EPAPER_154` profile, its framebuffer-size
   term, `DEFAULT_DEVICE` and `selectDevice()` entries, `isWsEpaper154()`, and the
   1-bit SDMMC default.
2. A new controller, so a new `PanelDriver`: `Ssd1681Driver` in
   `libs/display/FreeInkDisplay/src/driver/`, behind `FREEINK_DRIVER_SSD1681`, selected
   in `FreeInkDisplay::selectDriver()`. It is an external-bus driver over the
   `ssd1681_fast` component (the "devices backed by external libraries" pattern).
3. `docs/waveshare-epaper-154-support.md`: wiring and driver behaviour.

These changes are also kept as one patch against the upstream commit, ready to
send upstream: `patches/0001-waveshare-epaper-1.54.patch`. Regenerate it after
changing anything here with `tools/freeink_sdk_patch.sh`.

## Updating the SDK

1. Copy the three library folders from the new upstream commit over these,
   leaving out each library's `test/` folder.
2. Apply `patches/0001-waveshare-epaper-1.54.patch` (`git apply` from this folder);
   resolve any conflicts in `BoardConfig.h`.
3. Update the upstream commit above, run `tools/freeink_sdk_patch.sh`, build with
   `pio run` and run `tools/run_tests.sh`.
