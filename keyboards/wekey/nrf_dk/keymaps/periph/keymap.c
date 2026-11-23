// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Build test of the nrf peripheral drivers: OLED (I2C), ST7789 (SPI,
// Quantum Painter), PMW3360 (SPI), WS2812, backlight and audio (PWM),
// analog input, battery.

#include QMK_KEYBOARD_H
#include "analog.h"
#include "battery.h"
#include "qp.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(BL_STEP, UG_NEXT, CK_TOGG, BL_BRTG)
};
// clang-format on

static painter_device_t lcd;

void keyboard_post_init_user(void) {
    lcd = qp_st7789_make_spi_device(240, 240, LCD_CS_PIN, LCD_DC_PIN, LCD_RST_PIN, 4, 0);
    qp_init(lcd, QP_ROTATION_0);
    qp_rect(lcd, 0, 0, 239, 239, 0, 0, 0, true);
}

bool oled_task_user(void) {
    oled_write_P(PSTR("bat "), false);
    oled_write(get_u8_str(battery_get_percent(), ' '), false);
    oled_write_P(PSTR("% ain "), false);
    oled_write(get_u16_str(analogReadPin(TEST_ANALOG_PIN), ' '), false);
    return false;
}
