// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

enum layers {
    _BASE,
    _BT,
    _SYS,
    _MOUSE,
};

// BT_PRF1..3, BT_UNPR, OU_NEXT, OU_2P4G: QMK connection keycodes (platforms/nrf/nrf_keycodes.c)

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌─────┬─────┬─────┐
     * │  1  │  A  │  X  ├─────┐
     * ├─────┼─────┼─────┤MOUSE│   * (or the EC11 push): hold for _MOUSE
     * │ BT  │ SYS │ OUT ├─────┘   EC11: volume down / up
     * └─────┴─────┴─────┘
     * BT / SYS: hold for _BT / _SYS; OUT: toggle USB / BLE
     */
    [_BASE] = LAYOUT(
        KC_1,    KC_A,    KC_X,    MO(_MOUSE),
        MO(_BT), MO(_SYS), OU_NEXT
    ),
    /*
     * BT_PRF1..3: type to BLE host 1 / 2 / 3 (also with USB plugged in)
     * OU_2P4G (*): type to the 2.4 GHz USB dongle (pairs one if none is)
     */
    [_BT] = LAYOUT(
        BT_PRF1, BT_PRF2, BT_PRF3, OU_2P4G,
        _______, _______, _______
    ),
    /*
     * BT_UNPR: forget the host of the current BLE profile (or the dongle); BOOT: bootloader;
     * CAPS: Caps Lock (LED on P0.09)
     */
    [_SYS] = LAYOUT(
        BT_UNPR, OU_NEXT, QK_BOOT, _______,
        _______, _______, KC_CAPS
    ),
    /*
     * RGB on/off, mouse cursor, next RGB effect
     */
    [_MOUSE] = LAYOUT(
        RM_TOGG, MS_UP,   RM_NEXT, _______,
        MS_LEFT, MS_DOWN, MS_RGHT
    )
};

#if defined(ENCODER_MAP_ENABLE)
// EC11 (optional, shares the matrix position of the * key: its push button)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE]  = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU) },
    [_BT]    = { ENCODER_CCW_CW(_______, _______) },
    [_SYS]   = { ENCODER_CCW_CW(_______, _______) },
    [_MOUSE] = { ENCODER_CCW_CW(_______, _______) },
};
#endif
// clang-format on
