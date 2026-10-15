#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

/* The FreeInk SDK board profile (BoardConfig::ACTIVE) for the C firmware. Every board pin the
 * firmware uses comes from here; board.cpp is the only file that talks to the SDK. */

/* Hand the panel wiring to ssd1681_fast and the audio wiring to game_audio. Touches no GPIO. */
esp_err_t board_begin(void);
/* Configure the keys through the SDK input manager. Call after any RTC wake hold is released. */
void board_input_begin(void);

/* BOOT key: select / previous, and the deep-sleep wake source. Active-low. */
int board_select_pin(void);
/* PWR key: next / back. Active-low. */
int board_next_pin(void);
/* Battery latch: held high while running and through deep sleep. */
int board_battery_latch_pin(void);
/* Active-low panel supply switch, held off through deep sleep. */
int board_panel_power_pin(void);
/* SD card SDMMC wiring; width 1 or 4. */
void board_sdmmc(int *clk, int *cmd, int *d0, int *d1, int *d2, int *d3, unsigned *width);
/* Show a 200x200 1-bit frame (1 = white) with the SDK display's full refresh, then power the
 * panel down. The caller must not hold the panel. */
esp_err_t board_show_status(const uint8_t *frame);
/* Raw (undebounced) key levels from the SDK input manager. */
void board_buttons(bool *select, bool *next);
