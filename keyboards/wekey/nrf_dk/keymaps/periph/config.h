// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* I2C (OLED) */
#define I2C1_SCL_PIN P0_26
#define I2C1_SDA_PIN P0_27

/* SPI (PMW3360, ST7789) */
#define SPI_SCK_PIN  P1_04
#define SPI_MOSI_PIN P1_05
#define SPI_MISO_PIN P1_06
#define POINTING_DEVICE_CS_PIN P1_07
#define LCD_CS_PIN  P1_02
#define LCD_DC_PIN  P1_03
#define LCD_RST_PIN P1_01

/* WS2812 */
#define WS2812_DI_PIN P1_08
#define RGBLIGHT_LED_COUNT 8

/* Analog test input (AIN1) */
#define TEST_ANALOG_PIN P0_03

/* Backlight: two PWM channels, breathing */
#define BACKLIGHT_PINS {P0_28, P0_29}
#define BACKLIGHT_LEVELS 5
#define BACKLIGHT_BREATHING

/* Audio: piezo between two pins, driven in opposite phase */
#define AUDIO_PIN P0_30
#define AUDIO_PIN_ALT P0_31
#define AUDIO_PIN_ALT_AS_NEGATIVE
