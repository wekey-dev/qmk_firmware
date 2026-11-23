// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Audio on the core (AUDIO_DRIVER = pwm_hardware, the default outside
// ChibiOS): a square wave on AUDIO_PIN from the core's PWM generator 1
// (PWM2, channels 4-7), separate from the backlight's. The note frequency
// is the PWM frequency, the duty cycle follows the timbre (50 % by default).
//
//   AUDIO_PIN                        buzzer / piezo pin
//   AUDIO_PIN_ALT + AUDIO_PIN_ALT_AS_NEGATIVE
//                                    second pin driven in opposite phase
//                                    (piezo between the two pins: twice the
//                                    voltage swing, louder)
//
// QMK advances notes from a timer (ChibiOS: a virtual timer every 16 ms).
// Here it is done from the main loop (nrf_audio_task) and from wait_ms(),
// which sleeps in short steps while a note plays (see _wait.h), so melodies
// played around a wait (goodbye song before QK_BOOT) still progress. The
// keyboard does not idle while audio plays (nrf_idle_allowed_kb).

#include "audio.h"
#include "gpio.h" /* pin names */
#include "nrf_core.h"

#if !defined(AUDIO_PIN)
#    error "Audio feature enabled, but no pin selected: define AUDIO_PIN"
#endif
#if defined(AUDIO_PIN_ALT) && !defined(AUDIO_PIN_ALT_AS_NEGATIVE)
#    error "nrf: AUDIO_PIN_ALT is only supported with AUDIO_PIN_ALT_AS_NEGATIVE (one voice)"
#endif

/* Notes below this are played as silence (the PWM goes down to 4 Hz) */
#ifndef AUDIO_PWM_MIN_FREQUENCY
#    define AUDIO_PWM_MIN_FREQUENCY 20
#endif

#define AUDIO_CH(i) NRF_CORE_PWM_CHANNEL(NRF_CORE_PWM_GEN_AUDIO, i)

/* Note advance period, as the ChibiOS driver */
#define AUDIO_UPDATE_MS 16

extern bool    playing_note;
extern bool    playing_melody;
extern uint8_t note_timbre;

static float channel_1_frequency = 0.0f;

static void audio_duty(uint16_t duty) {
    nrf_core->pwm_duty(AUDIO_CH(0), duty);
#if defined(AUDIO_PIN_ALT)
    nrf_core->pwm_duty(AUDIO_CH(1), duty);
#endif
}

void channel_1_set_frequency(float freq) {
    channel_1_frequency = freq;

    if (freq < AUDIO_PWM_MIN_FREQUENCY) {
        audio_duty(0);
        return;
    }
    nrf_core->pwm_frequency(NRF_CORE_PWM_GEN_AUDIO, (uint32_t)(freq + 0.5f));
    /* note_timbre = percentage of the period spent low, as ChibiOS */
    uint8_t timbre = note_timbre > 100 ? 100 : note_timbre;
    audio_duty((uint16_t)((100U - timbre) * 0xFFFFU / 100U));
}

float channel_1_get_frequency(void) {
    return channel_1_frequency;
}

void channel_1_stop(void) {
    channel_1_frequency = 0.0f;
    audio_duty(0);
}

bool nrf_audio_playing(void) {
    return playing_note || playing_melody;
}

void nrf_audio_task(void) {
    if (!nrf_audio_playing()) {
        return;
    }
    if (audio_update_state()) {
        channel_1_set_frequency(audio_get_processed_frequency(0)); // TODO: freq_alt
    }
}

/* wait_ms() while audio is enabled: keep notes advancing during long waits */
void nrf_wait_ms(uint32_t ms) {
    while (ms > 0) {
        uint32_t step = (nrf_audio_playing() && ms > AUDIO_UPDATE_MS) ? AUDIO_UPDATE_MS : ms;

        nrf_core->sleep_ms(step);
        ms -= step;
        nrf_audio_task();
    }
}

void audio_driver_initialize_impl(void) {
    nrf_core->pwm_config(AUDIO_CH(0), AUDIO_PIN, 0);
#if defined(AUDIO_PIN_ALT)
    nrf_core->pwm_config(AUDIO_CH(1), AUDIO_PIN_ALT, NRF_CORE_PWM_ACTIVE_LOW);
#endif
}

void audio_driver_start_impl(void) {
    /* The first note was just pushed: play it now, the rest follows from
     * nrf_audio_task() */
    nrf_audio_task();
}

void audio_driver_stop_impl(void) {
    channel_1_stop();
}
