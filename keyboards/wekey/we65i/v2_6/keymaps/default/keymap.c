// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _FN,
};

// BT_PRF1..3, BT_UNPR, OU_USB, OU_2P4G: QMK connection keycodes (platforms/nrf/nrf_keycodes.c)

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───────┬───┐
     * │Esc│ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │ 0 │ - │ = │ Bkspc │Mut│ EC11 push
     * ├───┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─────┼───┤
     * │ Tab │ Q │ W │ E │ R │ T │ Y │ U │ I │ O │ P │ [ │ ] │  \  │Del│
     * ├─────┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴┬──┴─────┼───┤
     * │ Caps │ A │ S │ D │ F │ G │ H │ J │ K │ L │ ; │ ' │  Enter │PgU│
     * ├──────┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴────┬───┼───┤
     * │ Shift  │ Z │ X │ C │ V │ B │ N │ M │ , │ . │ / │ Shift│ ↑ │PgD│
     * ├────┬───┴┬──┴─┬─┴───┴───┴───┴───┴───┴──┬┴───┼───┴┬─┬───┼───┼───┤
     * │Ctrl│GUI │Alt │                        │Alt │ Fn │ │ ← │ ↓ │ → │
     * └────┴────┴────┴────────────────────────┴────┴────┘ └───┴───┴───┘
     * EC11: volume down / up
     */
    [_BASE] = LAYOUT(
        KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_MINS, KC_EQL,  KC_BSPC,          KC_MUTE,
        KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_LBRC, KC_RBRC, KC_BSLS,          KC_DEL,
        KC_CAPS, KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT, KC_ENT,                    KC_PGUP,
        KC_LSFT, KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,                   KC_UP,   KC_PGDN,
        KC_LCTL, KC_LGUI, KC_LALT,                   KC_SPC,                    KC_RALT, MO(_FN),                   KC_LEFT, KC_DOWN, KC_RGHT
    ),
    /*
     * Fn + Del / PgUp / PgDn: type to BLE host 1 / 2 / 3 (also with USB
     *   plugged in); the LED of the key (LED1..LED3) shows the profile
     * Fn + EC11 push: type over USB
     * Fn + Right Alt: type to the 2.4 GHz USB dongle (pairs one if none is);
     *   LED1..LED3 together show it
     * Fn + \: forget the host of the current BLE profile (or the dongle)
     * Fn + Enter: bootloader (UF2 drive WEKEY840)
     * Fn + EC11: screen brightness
     */
    [_FN] = LAYOUT(
        KC_GRV,  KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6,   KC_F7,   KC_F8,   KC_F9,   KC_F10,  KC_F11,  KC_F12,  KC_DEL,           OU_USB,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, BT_UNPR,          BT_PRF1,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, QK_BOOT,                   BT_PRF2,
        _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______, _______,                   KC_PGUP, BT_PRF3,
        _______, _______, _______,                   _______,                   OU_2P4G, _______,                   KC_HOME, KC_PGDN, KC_END
    ),
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [_FN]   = { ENCODER_CCW_CW(KC_BRID, KC_BRIU) },
};
#endif
// clang-format on
