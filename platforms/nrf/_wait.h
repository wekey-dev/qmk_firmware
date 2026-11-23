// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>
#include "nrf_core.h"

/* wait_ms() sleeps (lets the core run BLE and saves power), wait_us() spins.
 * With audio, wait_ms() also advances the notes being played
 * (platforms/nrf/drivers/audio_pwm_hardware.c). */
#if defined(AUDIO_ENABLE)
void nrf_wait_ms(uint32_t ms);
#    define wait_ms(ms) nrf_wait_ms((uint32_t)(ms))
#else
#    define wait_ms(ms) nrf_core->sleep_ms((uint32_t)(ms))
#endif
#define wait_us(us) nrf_core->busy_wait_us((uint32_t)(us))

/* ~0.25 us at 64 MHz: lets an input settle after its row/column was selected. */
#ifndef GPIO_INPUT_PIN_DELAY
#    define GPIO_INPUT_PIN_DELAY 16
#endif
#define waitInputPinDelay()                                    \
    do {                                                       \
        for (int __i = 0; __i < GPIO_INPUT_PIN_DELAY; __i++) { \
            __asm__ volatile("nop");                           \
        }                                                      \
    } while (0)
