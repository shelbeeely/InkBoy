/* Host stand-in for main/board.cpp: the Waveshare 1.54 V2 profile, keys read from host GPIO. */
#include "board.h"
#include "driver/gpio.h"
#include "host_fakes.h"

esp_err_t board_begin(void) { return ESP_OK; }
void board_input_begin(void) {}
int board_select_pin(void) { return 0; }
int board_next_pin(void) { return 18; }
int board_battery_latch_pin(void) { return 17; }
int board_panel_power_pin(void) { return 6; }
void board_sdmmc(int *clk, int *cmd, int *d0, int *d1, int *d2, int *d3, unsigned *width)
{
    *clk = 39; *cmd = 41; *d0 = 40; *d1 = *d2 = *d3 = -1; *width = 1;
}
void board_buttons(bool *select, bool *next)
{
    *select = gpio_get_level(0) == 0;
    *next = gpio_get_level(18) == 0;
}

/* The SDK display path is covered on hardware; host tests observe the call and its frame. */
unsigned host_board_status_shows;
esp_err_t host_board_status_result;
const uint8_t *host_board_status_frame;
esp_err_t board_show_status(const uint8_t *frame)
{
    ++host_board_status_shows;
    host_board_status_frame = frame;
    return host_board_status_result;
}
