// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Access to the nRF Core from QMK.
//
// `nrf_core` points at the API table the core passed to nrf_app_entry(). It is
// valid for the whole life of the app (set before any constructor or main()).
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "nrf_core_api.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const struct nrf_core_api *nrf_core;

/* ---- Connection / BLE control (for keymaps) --------------------------------
 * QMK's connection keycodes (BT_PRF1, BT_UNPR, OU_BT, ...) already call these
 * (nrf_keycodes.c); use them directly for anything else, e.g. in
 * process_record_user():
 *     case MY_CLR_ALL: if (record->event.pressed) nrf_core_ble_clear_all(); return false;
 */
static inline int32_t nrf_core_command(uint32_t cmd, uint32_t arg) {
    return nrf_core->command(cmd, arg);
}
static inline uint32_t nrf_core_conn_state(void) {
    return nrf_core->conn_state();
}
static inline bool nrf_core_ble_connected(void) {
    return (nrf_core->conn_state() & NRF_CORE_CONN_BLE_SECURED) != 0;
}
static inline int32_t nrf_core_ble_clear_current(void) {
    return nrf_core->command(NRF_CORE_CMD_BLE_CLEAR_CURRENT, 0);
}
static inline int32_t nrf_core_ble_clear_all(void) {
    return nrf_core->command(NRF_CORE_CMD_BLE_CLEAR_ALL, 0);
}
static inline int32_t nrf_core_ble_profile_select(uint8_t profile) {
    return nrf_core->command(NRF_CORE_CMD_BLE_PROFILE_SELECT, profile);
}
static inline int32_t nrf_core_ble_profile_next(void) {
    return nrf_core->command(NRF_CORE_CMD_BLE_PROFILE_NEXT, 0);
}
static inline int32_t nrf_core_ble_profile_prev(void) {
    return nrf_core->command(NRF_CORE_CMD_BLE_PROFILE_PREV, 0);
}
static inline uint8_t nrf_core_ble_profile(void) {
    return (uint8_t)((nrf_core->conn_state() & NRF_CORE_CONN_PROFILE_MASK) >> NRF_CORE_CONN_PROFILE_SHIFT);
}
/* Output selection: auto (USB when plugged in), force USB, force wireless
 * (BLE profile or dongle, whichever is selected), toggle */
static inline int32_t nrf_core_output_auto(void) {
    return nrf_core->command(NRF_CORE_CMD_OUTPUT_AUTO, 0);
}
static inline int32_t nrf_core_output_usb(void) {
    return nrf_core->command(NRF_CORE_CMD_OUTPUT_USB, 0);
}
static inline int32_t nrf_core_output_ble(void) {
    return nrf_core->command(NRF_CORE_CMD_OUTPUT_BLE, 0);
}
static inline int32_t nrf_core_output_toggle(void) {
    return nrf_core->command(NRF_CORE_CMD_OUTPUT_TOGGLE, 0);
}
static inline bool nrf_core_output_is_usb(void) {
    return (nrf_core->conn_state() & NRF_CORE_CONN_OUTPUT_USB) != 0;
}
/* 2.4 GHz USB dongle: type to it (pairs with a dongle if there is no dongle
 * bond; nrf_core_ble_clear_current() then forgets the dongle). A BLE profile
 * key goes back to the BLE hosts. */
static inline int32_t nrf_core_output_rf(void) {
    return nrf_core->command(NRF_CORE_CMD_OUTPUT_RF, 0);
}
/* The wireless link is the dongle, not a BLE profile (also while USB is used) */
static inline bool nrf_core_output_is_rf(void) {
    return (nrf_core->conn_state() & NRF_CORE_CONN_RF) != 0;
}
static inline uint8_t nrf_core_battery_level(void) {
    return (uint8_t)nrf_core->battery_level();
}
/* Charger (keyboards declaring NRF_CHARGER_*): NRF_CORE_CHARGE_UNKNOWN,
 * _DISCHARGING (on battery), _CHARGING, _FULL, _DISABLED. Changes are
 * signalled with NRF_CORE_EVT_BATTERY. */
static inline uint32_t nrf_core_charge_state(void) {
    return nrf_core->charge_state();
}
static inline bool nrf_core_charging(void) {
    return nrf_core->charge_state() == NRF_CORE_CHARGE_CHARGING;
}
/* Charging is on by default; switch it off / on again (e.g. from a keycode). */
static inline int32_t nrf_core_charge_enable(bool on) {
    return nrf_core->command(NRF_CORE_CMD_CHARGE_ENABLE, on ? 1 : 0);
}

/* ---- Power management hooks (nrf_power.c) ----------------------------------
 * The _kb / _user hooks and the matrix wake functions are weak and can be
 * overridden by a keyboard.
 */

/* Return false to keep scanning continuously (e.g. while an RGB animation runs). */
bool nrf_idle_allowed_kb(void);
bool nrf_idle_allowed_user(void);

/* Put the matrix in a state where any key press drives a wake pin, and arm the
 * wake pins with nrf_core->gpio_wake_config(). The default implementation
 * handles the standard COL2ROW / ROW2COL / DIRECT_PINS matrix and quadrature
 * encoders. Return false if the matrix cannot wake the MCU (custom matrix):
 * the keyboard then keeps scanning and never enters idle or System OFF. */
bool nrf_matrix_wake_prepare(void);
/* Undo nrf_matrix_wake_prepare() so matrix_scan() works again. */
void nrf_matrix_wake_finish(void);

/* Called from the main loop (protocol_post_task): paces scanning and handles
 * idle / System OFF. */
void nrf_power_task(void);

/* Called right before System OFF. */
void nrf_power_off_kb(void);
void nrf_power_off_user(void);

/* ---- Platform internals (not for keymaps) ---------------------------------- */

#if defined(AUDIO_ENABLE)
/* drivers/audio_pwm_hardware.c: advance the notes being played (main loop
 * and wait_ms()); true while a note or melody plays. */
void nrf_audio_task(void);
bool nrf_audio_playing(void);
#endif

#ifdef __cplusplus
}
#endif
