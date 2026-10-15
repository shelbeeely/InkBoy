# Development and releases

FreeInkBoy builds with PlatformIO (`platformio.ini`): the pioarduino platform, ESP-IDF 5.5 with Arduino-ESP32 3.3 as a component. `main/` and `components/` are ordinary ESP-IDF components; `components/freeink_sdk` builds the vendored [FreeInk SDK](../freeink-sdk/FREEINKBOY.md).

```sh
pio run                 # build the ws154 environment
pio run -t upload       # build and flash
pio device monitor      # serial log
```

Host tests compile the real firmware sources against fakes and need no hardware. They use cJSON from `managed_components/`, so run `pio run` once before the first run:

```sh
tools/run_tests.sh
python3 tools/check_safety.py
```

`check_safety.py` reads `sdkconfig.ws154`, which `pio run` writes. It fails if the display driver's command set changes, or if the SDK's `Ssd1681Driver` reaches the panel other than through `ssd1681_fast`. See [display command checks](how-it-works.md#what-the-safety-check-guards).

## Release images

Use a clean tree with the submodule at its committed revision. The release script always builds the production `ws154` environment, which never includes diagnostics.

```sh
tools/make_release.sh <version>
```

The script builds, runs host tests and the safety check, and writes `dist/freeinkboy-<version>.bin` (merged image at 0x0), `dist/freeinkboy-<version>-app.bin` (app only at 0x10000) and `dist/SHA256SUMS`. It stops on failure. See [flashing](flashing.md) for which image to use.

## Dependencies

The emulator core is the `components/esp32-gameboy` git submodule. Clone with `--recursive`. The FreeInk SDK libraries are vendored in `freeink-sdk/`; see [its notes](../freeink-sdk/FREEINKBOY.md) for what was added and how to update it. On the first build the component manager downloads `espressif/cjson` (and `espressif/esp-dsp` for Arduino-ESP32); versions are pinned in `dependencies.lock`. The ESP-IDF 5.5 USB host library is built in.

`platformio.ini` removes the managed components Arduino-ESP32 would otherwise pull in (RainMaker, Insights, Zigbee and others). FreeInkBoy does not call `initArduino()`: it would erase NVS after a format change and release Bluetooth memory. BoardConfig and the GPIO-based InputManager do not need it.

For repeated flashing and serial logging, select Bluetooth so USB stays available across resets. See [flashing and recovery](flashing.md).

## Diagnostic build

`pio run -e ws154-diagnostics -t upload` enables test-only USB input
commands (`ib boot 100`, `ib pwr 850`, `ib controller 00`, `ib usbstate`).
Use this only for development tests. The production `ws154` environment excludes
the diagnostic console and simulated input; flash it again when returning to production.

## Menu display handover

Either board button pauses on its debounced press during gameplay. The emulator
and audio stop, the live panel context passes to the menu, and the existing
battery-save/checkpoint path runs before Home appears. Home rendering can use
fast updates rather than a game-exit clean followed by a menu-entry clean.
The menu refresh counter starts anew after continuous gameplay and retains its
bounded lifetime policy.

## Verification

Host checks cover GPIO debounce, tap/hold/release, held wake, chords, menu event
routing, safe confirmations, board-only decrease editing, framebuffer layout,
partial mapping, failed mapping commits, Settings persistence/rollback, game
handover followed by Home rendering, and existing save/checkpoint behavior.
The SSD1681 command/voltage safety check passes.

### Hardware verification

V2-board checks covered settings navigation, mapping without full cleans, button pause/save, checkpoint reload and gameplay in both display modes. Observed command-to-Home times were 0.38 s grayscale and 0.34 s dithered. These timings are measurements from that build, not guarantees for every configuration.

The manual USB toggle was confirmed to show USB: PC and restore the Mac serial port. Opening the serial capture then reset the board and restored the saved USB-controller mode. Controller reconnect after the manual toggle still needs hardware verification.
