// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* Nordic DKs: 32.768 kHz crystal fitted, DC/DC inductor on REG1 */
#define NRF_LFCLK_CRYSTAL
#define NRF_POWER_DCDC_REG1

/* The DKs have no battery: measure VDD (about 3.0 V from USB / the on-board
 * regulator) to exercise the battery path. A real keyboard would use e.g.
 *   #define NRF_BATTERY_METHOD   NRF_BATTERY_DIVIDER
 *   #define NRF_BATTERY_PIN      P0_04
 *   #define NRF_BATTERY_R_TOP    1000000
 *   #define NRF_BATTERY_R_BOTTOM 1000000
 * See platforms/nrf/nrf_app_header.c for all options. */
#define NRF_BATTERY_METHOD NRF_BATTERY_VDD
#define NRF_BATTERY_EMPTY_MV 1800
#define NRF_BATTERY_FULL_MV 3000
