// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "nrf_core.h"

/*
 * Connection LEDs in the right column (config.h): after the output, the BLE
 * profile or the connection changed (and at start-up), for
 * WE67LITE_LED_SHOW_MS:
 *   USB output   LED1 blue
 *   BLE output   green LED of the active profile (LED1 / LED2 / LED3)
 *   dongle       LED1..LED3 green together (2.4 GHz USB dongle)
 * then off again, to save the battery. While the keyboard idles QMK runs
 * about once a second, so they may stay on up to a second longer.
 */
static const pin_t profile_leds[] = WE67LITE_LED_PROFILE_PINS;

#define LED_USB_BIT (1u << ARRAY_SIZE(profile_leds))

/* Common anode: low = on */
static void leds_write(uint32_t on) {
    for (uint8_t i = 0; i < ARRAY_SIZE(profile_leds); i++) {
        gpio_write_pin(profile_leds[i], !(on & (1u << i)));
    }
    gpio_write_pin(WE67LITE_LED_USB_PIN, !(on & LED_USB_BIT));
}

void keyboard_pre_init_kb(void) {
    /* Off before they become outputs: no flash at start-up */
    for (uint8_t i = 0; i < ARRAY_SIZE(profile_leds); i++) {
        gpio_write_pin_high(profile_leds[i]);
        gpio_set_pin_output(profile_leds[i]);
    }
    gpio_write_pin_high(WE67LITE_LED_USB_PIN);
    gpio_set_pin_output(WE67LITE_LED_USB_PIN);

    /* Underglow not enabled yet: keep its power switched off */
    gpio_write_pin_low(WE67LITE_RGB_POWER_PIN);
    gpio_set_pin_output(WE67LITE_RGB_POWER_PIN);

    keyboard_pre_init_user();
}

void housekeeping_task_kb(void) {
    static uint32_t last_state = UINT32_MAX;
    static uint32_t shown_at;
    static uint32_t leds = UINT32_MAX;

    uint32_t state = nrf_core_conn_state() & (NRF_CORE_CONN_PROFILE_MASK | NRF_CORE_CONN_OUTPUT_USB | NRF_CORE_CONN_RF | NRF_CORE_CONN_BLE_SECURED);

    if (state != last_state) {
        last_state = state;
        shown_at   = timer_read32();
    }

    uint32_t on = 0;

    if (timer_elapsed32(shown_at) < WE67LITE_LED_SHOW_MS) {
        if (state & NRF_CORE_CONN_OUTPUT_USB) {
            on = LED_USB_BIT;
        } else if (state & NRF_CORE_CONN_RF) {
            on = LED_USB_BIT - 1; /* every profile LED */
        } else {
            uint8_t profile = (state & NRF_CORE_CONN_PROFILE_MASK) >> NRF_CORE_CONN_PROFILE_SHIFT;

            if (profile < ARRAY_SIZE(profile_leds)) {
                on = 1u << profile;
            }
        }
    }
    if (on != leds) {
        leds = on;
        leds_write(on);
    }

    housekeeping_task_user();
}

/* System OFF keeps output levels: switch every LED (and the underglow power)
 * off first. Caps Lock (P0.26) is switched off by QMK's suspend handling. */
void nrf_power_off_kb(void) {
    leds_write(0);
    gpio_write_pin_high(P0_26);
    gpio_write_pin_low(WE67LITE_RGB_POWER_PIN);
    nrf_power_off_user();
}
