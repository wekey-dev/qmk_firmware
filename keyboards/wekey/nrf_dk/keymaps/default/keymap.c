// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "nrf_core.h"

enum custom_keycodes {
    BT_CLR_ALL = SAFE_RANGE, // forget the hosts of all BLE profiles, advertise again
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_A,    KC_CAPS, KC_VOLU, BT_CLR_ALL
    )
};
// clang-format on

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case BT_CLR_ALL:
            if (record->event.pressed) {
                nrf_core_ble_clear_all();
            }
            return false;
    }
    return true;
}

// Show Caps Lock state from the host in the core log (UART/RTT console).
bool led_update_user(led_t led_state) {
    static const char on[]  = "caps lock on\n";
    static const char off[] = "caps lock off\n";
    if (led_state.caps_lock) {
        nrf_core->log_write(on, sizeof(on) - 1);
    } else {
        nrf_core->log_write(off, sizeof(off) - 1);
    }
    return true;
}
