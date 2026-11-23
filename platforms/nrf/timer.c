// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include "timer.h"
#include "nrf_core.h"

static uint32_t ms_offset = 0;
static uint32_t saved_ms  = 0;

void timer_init(void) {
    timer_clear();
}

void timer_clear(void) {
    ms_offset = nrf_core->uptime_ms();
}

__attribute__((weak)) void platform_timer_save_value(uint32_t value) {
    saved_ms = value;
}

__attribute__((weak)) uint32_t platform_timer_restore_value(void) {
    return saved_ms;
}

void timer_save(void) {
    platform_timer_save_value(timer_read32());
}

void timer_restore(void) {
    ms_offset = nrf_core->uptime_ms() - platform_timer_restore_value();
}

uint16_t timer_read(void) {
    return (uint16_t)timer_read32();
}

uint32_t timer_read32(void) {
    return nrf_core->uptime_ms() - ms_offset;
}
