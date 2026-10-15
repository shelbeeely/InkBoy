#include <BoardConfig.h>
#include <EInkDisplay.h>
#include <InputManager.h>

#include <cstring>

extern "C" {
#include "board.h"
#include "game_audio.h"
#include "ssd1681_fast.h"
}

/* FreeInkBoy does not call initArduino(): it erases NVS on a version mismatch (user settings
 * must survive) and releases Bluetooth memory unless btInUse() is overridden. BoardConfig and
 * the GPIO input path only need the Arduino GPIO HAL, which works without it.
 *
 * The battery latch is declared in the profile (power.latch0) but asserted by power.c rather
 * than BoardConfig::holdPowerRails(): power.c preloads the level and enables the pull-up before
 * releasing the sleep hold, so the latch never glitches low on battery. */

static InputManager input;
/* Pins come from the active profile at begin(); the constructor arguments are unused. */
static EInkDisplay display(-1, -1, -1, -1, -1, -1);
static bool display_started;

extern "C" esp_err_t board_begin(void)
{
    const BoardConfig::BoardProfile &board = BoardConfig::ACTIVE;
    const auto &d = board.display;
    const ssd1681_pins_t panel = {d.sclk, d.mosi, d.cs, d.dc, d.rst, d.busy, d.powerEnable};
    esp_err_t error = ssd1681_configure_pins(&panel);
    if (error) return error;

    const auto &a = board.audio;
    const game_audio_pins_t audio = {a.mclk, a.bclk, a.lrclk, a.dout, a.codecSda, a.codecScl, a.enable, a.ampEnable};
    return game_audio_configure_pins(&audio);
}

extern "C" void board_input_begin(void) { input.begin(); }

extern "C" esp_err_t board_show_status(const uint8_t *frame)
{
    if (!frame) return ESP_ERR_INVALID_ARG;
    if (!display_started) { display.begin(); display_started = true; }
    if (!display.getFrameBuffer() || display.getBufferSize() != SSD1681_FRAME_BYTES) return ESP_ERR_INVALID_SIZE;
    memcpy(display.getFrameBuffer(), frame, SSD1681_FRAME_BYTES);
    display.displayBuffer(EInkDisplay::FULL_REFRESH, true);
    return display.displayCommitted() ? ESP_OK : ESP_FAIL;
}

extern "C" int board_select_pin(void) { return BoardConfig::ACTIVE.input.confirm; }
extern "C" int board_next_pin(void) { return BoardConfig::ACTIVE.input.power; }
extern "C" int board_battery_latch_pin(void) { return BoardConfig::ACTIVE.power.latch0; }
extern "C" int board_panel_power_pin(void) { return BoardConfig::ACTIVE.display.powerEnable; }

extern "C" void board_sdmmc(int *clk, int *cmd, int *d0, int *d1, int *d2, int *d3, unsigned *width)
{
    const auto &s = BoardConfig::ACTIVE.sdmmc;
    *clk = s.clk; *cmd = s.cmd; *d0 = s.d0; *d1 = s.d1; *d2 = s.d2; *d3 = s.d3;
    *width = s.busWidth;
}

extern "C" void board_buttons(bool *select, bool *next)
{
    const uint8_t state = input.getState();
    *select = state & (1u << InputManager::BTN_CONFIRM);
    *next = state & (1u << InputManager::BTN_POWER);
}
