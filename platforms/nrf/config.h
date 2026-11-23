// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* EEPROM emulated by the core (CONFIG_NRF_CORE_EEPROM_SIZE, 4 KB by
 * default). Must not exceed what the core provides; checked at start-up. */
#ifndef EEPROM_SIZE
#    define EEPROM_SIZE 4096
#endif

/* Busy-wait after each matrix line is unselected, for the other lines to
 * rise back (QMK's default is 30 us). The nRF52 pull-ups (~13 kOhm) charge
 * a matrix line in a few us; at 30 us the wait was most of every scan
 * (we6k: 60 us of 72). 5 us tested on we6k without ghost keys; a matrix
 * with long wires or extra capacitance can set more in its config.h. */
#ifndef MATRIX_IO_DELAY
#    define MATRIX_IO_DELAY 5
#endif

#ifndef SERIAL_NUMBER_USE_HARDWARE_ID
#    define SERIAL_NUMBER_USE_HARDWARE_ID TRUE
#endif

/* ---- Power management (nrf_power.c) ----------------------------------- */

/* Matrix scan period while keys are active and reports go to USB (ms). */
#ifndef NRF_SCAN_INTERVAL_MS
#    define NRF_SCAN_INTERVAL_MS 1
#endif

/* Matrix scan period while keys are active and reports go to BLE (ms). The BLE
 * connection interval is 7.5-15 ms, so scanning faster than this only costs
 * battery (each scan also busy-waits MATRIX_IO_DELAY per row/column). Adds at
 * most this much latency on top of debouncing. */
#ifndef NRF_SCAN_INTERVAL_BLE_MS
#    define NRF_SCAN_INTERVAL_BLE_MS 3
#endif

/* Matrix scan period while keys are active and reports go to the 2.4 GHz
 * dongle (ms). Its connection interval is 1 ms (LLPM), so a report leaves at
 * most 1 ms after it is queued: scan as fast as over USB. */
#ifndef NRF_SCAN_INTERVAL_RF_MS
#    define NRF_SCAN_INTERVAL_RF_MS 1
#endif

/* Over BLE, scan once right before each connection event instead of every
 * NRF_SCAN_INTERVAL_BLE_MS (falls back to timed scanning when the core
 * cannot sync): lowest and steadiest latency, fewest wake-ups. */
#ifndef NRF_BLE_SYNC_ENABLE
#    define NRF_BLE_SYNC_ENABLE 1
#endif

/* While synced, scan anyway if no connection event notice arrived for this
 * long (ms), e.g. while the connection parameters change. */
#ifndef NRF_BLE_SYNC_TIMEOUT_MS
#    define NRF_BLE_SYNC_TIMEOUT_MS 16
#endif

/* After this long without matrix activity (and no key held), stop scanning and
 * wait for a key press interrupt instead (ms). 0 disables. */
#ifndef NRF_IDLE_TIMEOUT_MS
#    define NRF_IDLE_TIMEOUT_MS 500
#endif

/* With keys held (or a stuck key) and no matrix change for this long, idle
 * anyway (ms): the held lines then wake on release. Long enough for
 * tap-hold / combos to have decided. 0 = keep scanning while any key is held. */
#ifndef NRF_HELD_IDLE_TIMEOUT_MS
#    define NRF_HELD_IDLE_TIMEOUT_MS 10000
#endif

/* While idle, wake up at least this often to run QMK housekeeping (ms). */
#ifndef NRF_IDLE_WAKE_INTERVAL_MS
#    define NRF_IDLE_WAKE_INTERVAL_MS 1000
#endif

/* After this long without any input, on battery (not while USB powers the
 * keyboard), enter System OFF; a key press reboots the keyboard (ms).
 * 0 disables. Default: 30 minutes. */
#ifndef NRF_POWER_OFF_TIMEOUT_MS
#    define NRF_POWER_OFF_TIMEOUT_MS (30UL * 60UL * 1000UL)
#endif
