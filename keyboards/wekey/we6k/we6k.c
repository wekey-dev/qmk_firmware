// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include "quantum.h"
#include "nrf_core.h"

#ifdef RGB_MATRIX_ENABLE
/*
 * IS31FL3731 matrix A, charlieplexed: register Cx_y = cathode on CAx, anode
 * on the y-th of CA1..CA9 without CAx. The LEDs are common anode (pin "+"),
 * nets from the schematic:
 *
 *   LED   key  anode  R    G    B
 *   RGB1  7    CA7    CA4  CA6  CA5
 *   RGB2  8    CA6    CA5  CA7  CA4
 *   RGB3  9    CA5    CA4  CA6  CA7
 *   RGB7  *    CA4    CA7  CA5  CA6   footprint RGB_3528_small-rev
 *   RGB4  4    CA4    CA5  CA3  CA2
 *   RGB5  5    CA3    CA2  CA4  CA5
 *   RGB6  6    CA2    CA5  CA3  CA4
 *
 * In the schematic RGB4 red and RGB7 green are the same CA4 -> CA5 pair
 * (C5_4), so they cannot be driven separately. RGB7 uses a reversed
 * footprint: check on the PCB which of its pads really go where.
 */
const is31fl3731_led_t PROGMEM g_is31fl3731_leds[IS31FL3731_LED_COUNT] = {
/* Driver
 *   |  R     G     B */
    {0, C4_6, C6_6, C5_6}, // RGB1, key 7
    {0, C5_5, C7_6, C4_5}, // RGB2, key 8
    {0, C4_4, C6_5, C7_5}, // RGB3, key 9
    {0, C7_4, C5_4, C6_4}, // RGB7, key * (G shares C5_4 with RGB4 R)
    {0, C5_4, C3_3, C2_3}, // RGB4, key 4
    {0, C2_2, C4_3, C5_3}, // RGB5, key 5
    {0, C5_2, C3_2, C4_2}, // RGB6, key 6
};

/* Before System OFF (the LEDs are already off: suspend_power_down_quantum):
 * also shut the LED driver down (SDB low, ~1 uA). Idle while the LEDs are
 * off is handled by the platform (nrf_idle_allowed_kb). */
void nrf_power_off_kb(void) {
    gpio_write_pin_low(IS31FL3731_SDB_PIN);
    nrf_power_off_user();
}
#endif
