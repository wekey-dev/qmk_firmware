// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Single-color backlight on the core (BACKLIGHT_DRIVER = pwm, the default):
// the core drives BACKLIGHT_PIN, or up to 4 BACKLIGHT_PINS, with PWM
// generator 0 (PWM1, channels 0-3; the buzzer has its own generator). The
// level is held by hardware, so the keyboard can idle with the backlight
// on; only breathing needs the main loop (it then keeps the keyboard out of
// idle, see nrf_idle_allowed_kb()).
//
//   BACKLIGHT_ON_STATE       0 = pin low while lit (default 1)
//   BACKLIGHT_PWM_FREQUENCY  Hz, default 1000 (16000 steps of resolution)
//   BACKLIGHT_LIMIT_VAL      maximum brightness, 0..255

#include "backlight.h"
#include "gpio.h" /* pin names */
#include "timer.h"
#include "wait.h"
#include "nrf_core.h"

#ifndef BACKLIGHT_LIMIT_VAL
#    define BACKLIGHT_LIMIT_VAL 255
#endif

#ifndef BACKLIGHT_PWM_FREQUENCY
#    define BACKLIGHT_PWM_FREQUENCY 1000
#endif

#if defined(BACKLIGHT_PINS)
static const pin_t backlight_pins[] = BACKLIGHT_PINS;
#    define BACKLIGHT_PIN_COUNT (sizeof(backlight_pins) / sizeof(backlight_pins[0]))
_Static_assert(BACKLIGHT_PIN_COUNT <= NRF_CORE_PWM_CHANNELS_PER_GEN, "nrf: at most 4 BACKLIGHT_PINS (PWM channels)");
#elif defined(BACKLIGHT_PIN)
static const pin_t backlight_pins[] = {BACKLIGHT_PIN};
#    define BACKLIGHT_PIN_COUNT 1
#else
#    error "BACKLIGHT_PIN or BACKLIGHT_PINS must be defined"
#endif

// See http://jared.geek.nz/2013/feb/linear-led-pwm
static uint16_t cie_lightness(uint16_t v) {
    if (v <= 5243) {
        return v / 9;
    }
    uint32_t y = (((uint32_t)v + 10486) << 8) / (10486 + 0xFFFFUL);
    y          = y * y * y >> 8;
    return y > 0xFFFFUL ? 0xFFFFU : (uint16_t)y;
}

static uint32_t rescale_limit_val(uint32_t val) {
    return (val * (BACKLIGHT_LIMIT_VAL + 1)) / 256;
}

#define BL_CHANNEL(i) NRF_CORE_PWM_CHANNEL(NRF_CORE_PWM_GEN_BACKLIGHT, i)

/* duty 0..0xFFFF on every backlight channel */
static void backlight_duty(uint16_t duty) {
    for (uint32_t i = 0; i < BACKLIGHT_PIN_COUNT; i++) {
        nrf_core->pwm_duty(BL_CHANNEL(i), duty);
    }
}

void backlight_init_ports(void) {
    nrf_core->pwm_frequency(NRF_CORE_PWM_GEN_BACKLIGHT, BACKLIGHT_PWM_FREQUENCY);
    for (uint32_t i = 0; i < BACKLIGHT_PIN_COUNT; i++) {
        nrf_core->pwm_config(BL_CHANNEL(i), backlight_pins[i], BACKLIGHT_ON_STATE == 0 ? NRF_CORE_PWM_ACTIVE_LOW : 0);
    }

    backlight_set(get_backlight_level());

#ifdef BACKLIGHT_BREATHING
    if (is_backlight_breathing()) {
        breathing_enable();
    }
#endif
}

void backlight_set(uint8_t level) {
    if (level > BACKLIGHT_LEVELS) {
        level = BACKLIGHT_LEVELS;
    }
    backlight_duty(level == 0 ? 0 : cie_lightness(rescale_limit_val(0xFFFF * (uint32_t)level / BACKLIGHT_LEVELS)));
}

#ifdef BACKLIGHT_BREATHING

#    define BREATHING_STEPS 128

/* from math import sin, pi; [int(sin(x/128.0*pi)**4*255) for x in range(128)] */
static const uint8_t breathing_table[BREATHING_STEPS] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 17, 20, 24, 28, 32, 36, 41, 46, 51, 57, 63, 70, 76, 83, 91, 98, 106, 113, 121, 129, 138, 146, 154, 162, 170, 178, 185, 193, 200, 207, 213, 220, 225, 231, 235, 240, 244, 247, 250, 252, 253, 254, 255, 254, 253, 252, 250, 247, 244, 240, 235, 231, 225, 220, 213, 207, 200, 193, 185, 178, 170, 162, 154, 146, 138, 129, 121, 113, 106, 98, 91, 83, 76, 70, 63, 57, 51, 46, 41, 36, 32, 28, 24, 20, 17, 15, 12, 10, 8, 6, 5, 4, 3, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static bool     breathing;
static uint32_t breathing_start;
static uint8_t  breathing_index = 0xFF;

bool is_breathing(void) {
    return breathing;
}

void breathing_enable(void) {
    breathing       = true;
    breathing_start = timer_read32();
    breathing_index = 0xFF;
}

void breathing_disable(void) {
    breathing = false;
    backlight_set(get_backlight_level());
}

void breathing_pulse(void) {
    backlight_set(is_backlight_enabled() ? 0 : BACKLIGHT_LEVELS);
    wait_ms(10);
    backlight_set(is_backlight_enabled() ? get_backlight_level() : 0);
}

void backlight_task(void) {
    if (!breathing) {
        return;
    }
    /* One cycle lasts breathing_period seconds, 128 steps */
    uint32_t period = get_breathing_period();
    if (period == 0) {
        period = 1;
    }
    uint32_t elapsed = timer_elapsed32(breathing_start) % (period * 1000);
    uint8_t  index   = (uint8_t)(elapsed * BREATHING_STEPS / (period * 1000));
    if (index == breathing_index) {
        return;
    }
    breathing_index = index;

    uint16_t v = (uint16_t)(breathing_table[index] * 256) / BACKLIGHT_LEVELS * get_backlight_level();
    backlight_duty(cie_lightness(rescale_limit_val(v)));
}

#else

void backlight_task(void) {}

#endif
