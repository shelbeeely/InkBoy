#!/usr/bin/env python3
"""Standing status screens go through the FreeInk SDK display, with the menu's panel released first."""
from host_build import run, DISPLAY_MODULES

CODE = r'''
#include <assert.h>
#include "screen.h"
#include "host_fakes.h"
#include "host_panel.h"

int main(void)
{
    host_fakes_reset(); host_panel_reset();
    /* A menu holds the panel; the status screen must release it before the SDK takes over. */
    ssd1681_handle_t menu;
    assert(!ssd1681_create(&menu) && !ssd1681_power_on(menu));
    assert(screen_take_display(menu, 0, false));
    assert(screen_message("OFF", "GAME PROGRESS SAVED", "PRESS BOOT TO TURN ON", ""));
    assert(host_panel.destroys == 1 && !host_panel.powered);
    assert(host_board_status_shows == 1 && host_board_status_frame == screen_panel());
    assert(host_panel.fulls == 0); /* the firmware's own driver path is not used */
    assert(!screen_failed());

    host_board_status_result = ESP_FAIL;
    assert(!screen_message("STARTUP ERROR", "RESET TO RETRY", "", ""));
    assert(host_board_status_shows == 2 && screen_failed());
    return 0;
}
'''

run(CODE, modules=DISPLAY_MODULES, fakes=['host_panel.c'])
print('PASS: status screens use the SDK display after releasing the menu panel; failures latch')
