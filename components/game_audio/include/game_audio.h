#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ES8311 + speaker amplifier wiring. codec_rail is the active-low audio supply switch. */
typedef struct { int mclk, bclk, lrclk, dout, codec_sda, codec_scl, codec_rail, amp_enable; } game_audio_pins_t;
#define GAME_AUDIO_PINS_WAVESHARE_154 {.mclk = 14, .bclk = 15, .lrclk = 38, .dout = 45, \
    .codec_sda = 47, .codec_scl = 48, .codec_rail = 42, .amp_enable = 46}
/* Select the wiring before game_audio_init(). */
esp_err_t game_audio_configure_pins(const game_audio_pins_t *pins);

/* Initializes ES8311 output at sample_rate. Audio input is signed 16-bit mono. */
esp_err_t game_audio_init(unsigned sample_rate);

/* Disable output or pause playback. Both operations flush queued stale audio. */
void game_audio_set_enabled(bool enabled);
void game_audio_set_playing(bool playing);

/* Nonblocking bounded submission of at most 536 samples. Larger blocks are dropped;
 * queued blocks may also be dropped when the bounded queue is full. */
void game_audio_submit(const int16_t *mono, size_t sample_count);

/* Stops the worker and leaves the codec/amplifier powered down. */
void game_audio_shutdown(void);

#ifdef __cplusplus
}
#endif
