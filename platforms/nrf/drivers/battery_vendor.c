// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// QMK battery feature on the core (BATTERY_DRIVER = vendor, the nrf default):
// the core measures the battery as declared with NRF_BATTERY_* in config.h.

#include "battery_driver.h"
#include "nrf_core.h"

void battery_driver_init(void) {}

uint8_t battery_driver_sample_percent(void) {
    uint32_t level = nrf_core->battery_level();
    return level > 100 ? 0 : (uint8_t)level; /* 0xFF = unknown */
}
