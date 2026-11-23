// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// WS2812 / SK6812 on the core (WS2812_DRIVER = vendor, the nrf default): the
// core generates the 800 kHz signal on WS2812_DI_PIN with PWM0 + EasyDMA, so
// timing does not depend on interrupts or the radio.

#include "ws2812.h"
#include "gpio.h" /* pin names */
#include "nrf_core.h"

#ifndef WS2812_DI_PIN
#    error "WS2812_DI_PIN must be defined"
#endif

static ws2812_led_t ws2812_leds[WS2812_LED_COUNT];

void ws2812_init(void) {}

void ws2812_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    if (index < 0 || index >= WS2812_LED_COUNT) {
        return;
    }
    ws2812_leds[index].r = red;
    ws2812_leds[index].g = green;
    ws2812_leds[index].b = blue;
#if defined(WS2812_RGBW)
    ws2812_rgb_to_rgbw(&ws2812_leds[index]);
#endif
}

void ws2812_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
    for (int i = 0; i < WS2812_LED_COUNT; i++) {
        ws2812_set_color(i, red, green, blue);
    }
}

void ws2812_flush(void) {
    /* ws2812_led_t is packed in the LED's byte order: send it as is */
    nrf_core->ws2812_write(WS2812_DI_PIN, ws2812_leds, sizeof(ws2812_leds));
}
