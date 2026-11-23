// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Power management for the QMK app on the nRF Core.
//
//   active  : USB -> scan at a fixed rate, every NRF_SCAN_INTERVAL_MS
//             dongle (2.4 GHz, 1 ms connection interval) -> fixed rate,
//                    every NRF_SCAN_INTERVAL_RF_MS
//             BLE -> scan once right before each connection event
//                    (NRF_CORE_EVT_BLE_SYNC), or at a fixed rate of
//                    NRF_SCAN_INTERVAL_BLE_MS if the core cannot sync
//             The fixed rate uses absolute deadlines, so the period does not
//             grow with the time spent scanning.
//   idle    : no key held and no matrix activity for NRF_IDLE_TIMEOUT_MS
//             (keys held: NRF_HELD_IDLE_TIMEOUT_MS)
//             -> matrix is put in "any key pulls a wake pin" mode and the
//                thread blocks in nrf_core->wait_event() (CPU sleeps)
//   off     : no input for NRF_POWER_OFF_TIMEOUT_MS on battery (never on USB
//             power) -> System OFF; a key press resets the chip (the core
//             and QMK boot again)

#include <stdint.h>
#include <stdbool.h>
#include "quantum.h"
#include "matrix.h"
#include "keyboard.h"
#include "nrf_core.h"

/* ---- Policy hooks ----------------------------------------------------------- */

__attribute__((weak)) bool nrf_idle_allowed_user(void) {
    return true;
}

/* Idle (no scanning, wait for a key) is not allowed while something needs
 * the main loop to keep running: lit LED animations, backlight breathing,
 * audio playing, a pointing device.
 * LEDs that are off or suspended (RGB_MATRIX_TIMEOUT, host asleep) do not
 * prevent it. Keyboards / keymaps can refine this with the _kb/_user hooks. */
__attribute__((weak)) bool nrf_idle_allowed_kb(void) {
#if defined(POINTING_DEVICE_ENABLE)
    return false;
#endif
#if defined(RGB_MATRIX_ENABLE)
    if (rgb_matrix_is_enabled() && !rgb_matrix_get_suspend_state()) {
        return false;
    }
#endif
#if defined(LED_MATRIX_ENABLE)
    if (led_matrix_is_enabled() && !led_matrix_get_suspend_state()) {
        return false;
    }
#endif
#if defined(RGBLIGHT_ENABLE)
    if (rgblight_is_enabled()) {
        return false;
    }
#endif
#if defined(AUDIO_ENABLE)
    if (nrf_audio_playing()) {
        return false;
    }
#endif
#if defined(BACKLIGHT_ENABLE) && defined(BACKLIGHT_BREATHING)
    /* A steady backlight is held by PWM hardware; breathing needs the loop */
    if (is_backlight_enabled() && is_breathing()) {
        return false;
    }
#endif
    return nrf_idle_allowed_user();
}

__attribute__((weak)) void nrf_power_off_user(void) {}

__attribute__((weak)) void nrf_power_off_kb(void) {
    nrf_power_off_user();
}

/* ---- Default matrix wake implementation -------------------------------------- */

/* Wake on any change of a sensed line: arm the level opposite to its current
 * one. Normally every line is high (pull-up) and wakes on a press; a held or
 * stuck key keeps its line low, which then wakes on release instead of
 * waking the chip again at once. */
#define WAKE_SETTLE_US 10

__attribute__((unused)) static void sense_line(pin_t pin) {
    if (pin != NO_PIN) {
        gpio_set_pin_input_high(pin);
    }
}

__attribute__((unused)) static void arm_change(pin_t pin) {
    if (pin != NO_PIN) {
        nrf_core->gpio_wake_config(pin, gpio_read_pin(pin) ? NRF_CORE_WAKE_LEVEL_LOW : NRF_CORE_WAKE_LEVEL_HIGH);
    }
}

__attribute__((unused)) static void disarm(pin_t pin) {
    if (pin != NO_PIN) {
        nrf_core->gpio_wake_config(pin, NRF_CORE_WAKE_DISABLE);
    }
}

__attribute__((unused)) static void unselect_line(pin_t pin) {
    if (pin == NO_PIN) {
        return;
    }
#ifdef MATRIX_UNSELECT_DRIVE_HIGH
    gpio_set_pin_output(pin);
    gpio_write_pin_high(pin);
#else
    gpio_set_pin_input_high(pin);
#endif
}

__attribute__((unused)) static void select_line(pin_t pin) {
    if (pin != NO_PIN) {
        gpio_set_pin_output(pin);
        gpio_write_pin_low(pin);
    }
}

__attribute__((unused)) static void encoders_arm(void) {
#if defined(ENCODER_ENABLE) && defined(ENCODER_A_PINS) && defined(ENCODER_B_PINS)
    static const pin_t a[] = ENCODER_A_PINS;
    static const pin_t b[] = ENCODER_B_PINS;
    for (unsigned i = 0; i < ARRAY_SIZE(a); i++) {
        arm_change(a[i]);
        arm_change(b[i]);
    }
#endif
}

__attribute__((unused)) static void encoders_disarm(void) {
#if defined(ENCODER_ENABLE) && defined(ENCODER_A_PINS) && defined(ENCODER_B_PINS)
    static const pin_t a[] = ENCODER_A_PINS;
    static const pin_t b[] = ENCODER_B_PINS;
    for (unsigned i = 0; i < ARRAY_SIZE(a); i++) {
        disarm(a[i]);
        disarm(b[i]);
    }
#endif
}

#if defined(DIRECT_PINS)

static const pin_t wake_direct[MATRIX_ROWS][MATRIX_COLS] = DIRECT_PINS;

__attribute__((weak)) bool nrf_matrix_wake_prepare(void) {
    for (unsigned r = 0; r < MATRIX_ROWS; r++) {
        for (unsigned c = 0; c < MATRIX_COLS; c++) {
            sense_line(wake_direct[r][c]);
        }
    }
    nrf_core->busy_wait_us(WAKE_SETTLE_US);
    for (unsigned r = 0; r < MATRIX_ROWS; r++) {
        for (unsigned c = 0; c < MATRIX_COLS; c++) {
            arm_change(wake_direct[r][c]);
        }
    }
    encoders_arm();
    return true;
}

__attribute__((weak)) void nrf_matrix_wake_finish(void) {
    for (unsigned r = 0; r < MATRIX_ROWS; r++) {
        for (unsigned c = 0; c < MATRIX_COLS; c++) {
            disarm(wake_direct[r][c]);
        }
    }
    encoders_disarm();
}

#elif defined(MATRIX_ROW_PINS) && defined(MATRIX_COL_PINS) && defined(DIODE_DIRECTION) && !defined(SPLIT_KEYBOARD)

static const pin_t wake_rows[] = MATRIX_ROW_PINS;
static const pin_t wake_cols[] = MATRIX_COL_PINS;

#    if DIODE_DIRECTION == COL2ROW
/* Rows are driven, columns are read */
#        define DRIVEN wake_rows
#        define SENSED wake_cols
#    else
/* Columns are driven, rows are read */
#        define DRIVEN wake_cols
#        define SENSED wake_rows
#    endif

__attribute__((weak)) bool nrf_matrix_wake_prepare(void) {
    for (unsigned i = 0; i < ARRAY_SIZE(DRIVEN); i++) {
        select_line(DRIVEN[i]);
    }
    for (unsigned i = 0; i < ARRAY_SIZE(SENSED); i++) {
        sense_line(SENSED[i]);
    }
    nrf_core->busy_wait_us(WAKE_SETTLE_US);
    for (unsigned i = 0; i < ARRAY_SIZE(SENSED); i++) {
        arm_change(SENSED[i]);
    }
    encoders_arm();
    return true;
}

__attribute__((weak)) void nrf_matrix_wake_finish(void) {
    for (unsigned i = 0; i < ARRAY_SIZE(SENSED); i++) {
        disarm(SENSED[i]);
    }
    for (unsigned i = 0; i < ARRAY_SIZE(DRIVEN); i++) {
        unselect_line(DRIVEN[i]);
    }
    encoders_disarm();
}

#else

/* Custom matrix: the keyboard must provide its own implementation to allow
 * idle and System OFF. */
__attribute__((weak)) bool nrf_matrix_wake_prepare(void) {
    return false;
}

__attribute__((weak)) void nrf_matrix_wake_finish(void) {}

#endif

/* ---- Main loop hook ---------------------------------------------------------------- */

static bool any_key_held(void) {
    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        if (matrix_get_row(r) != 0) {
            return true;
        }
    }
    return false;
}

static void ble_sync_set(bool on);

static void power_off(void) {
    static bool impossible = false;
    if (impossible) {
        return;
    }
    ble_sync_set(false);
    /* LEDs, displays, backlight off, as when the host sleeps */
    suspend_power_down_quantum();
    nrf_power_off_kb();
    clear_keyboard(); /* release everything on the host before going away */
    nrf_core->eeprom_flush();
    if (!nrf_matrix_wake_prepare()) {
        impossible = true; /* cannot wake up again: stay on */
        return;
    }
    nrf_core->power_off();
}

/* ---- Scan pacing ----------------------------------------------------------------- */

static uint32_t next_scan_ms;   /* deadline of the next fixed-rate scan */
static bool     pin_woke;       /* idle ended by a wake pin, see nrf_power_task */
static uint32_t pin_wake_ms;    /* when that happened */
static bool     sync_on;        /* core is posting NRF_CORE_EVT_BLE_SYNC */
static bool     sync_supported = NRF_BLE_SYNC_ENABLE;

static void ble_sync_set(bool on) {
    if (on == sync_on) {
        return;
    }
    int32_t err = nrf_core->command(NRF_CORE_CMD_BLE_SYNC, on ? 1 : 0);
    if (on && err != NRF_CORE_OK) {
        sync_supported = false; /* controller cannot sync: timed scanning */
        return;
    }
    sync_on = on;
}

/* Fixed rate: wake on absolute deadlines spaced `interval` apart. */
static void pace_fixed(uint32_t interval) {
    uint32_t now = nrf_core->uptime_ms();

    next_scan_ms += interval;
    int32_t ahead = (int32_t)(next_scan_ms - now);

    if (ahead > (int32_t)interval || ahead <= -(int32_t)interval) {
        /* first scan after idle / sync / a long stall: restart the grid */
        next_scan_ms = now + interval;
    } else if (ahead <= 0) {
        return; /* slightly late: scan again right away */
    }
    nrf_core->sleep_until_ms(next_scan_ms);
}

/* Wait for the next scan slot while keys are active. */
static void pace_scan(void) {
    uint32_t state = nrf_core->conn_state();
    bool     ble   = !(state & NRF_CORE_CONN_OUTPUT_USB);

    if (ble && (state & NRF_CORE_CONN_RF)) {
        /* The dongle polls every 1 ms: no sync needed (nor useful) */
        ble_sync_set(false);
        pace_fixed(NRF_SCAN_INTERVAL_RF_MS);
        return;
    }
    if (ble && sync_supported && (state & NRF_CORE_CONN_BLE_SECURED)) {
        ble_sync_set(true);
        if (sync_on) {
            /* Returns at the connection event notice; other events (LEDs,
             * connection changes) just cause an extra scan. */
            nrf_core->wait_event(NRF_BLE_SYNC_TIMEOUT_MS);
            next_scan_ms = nrf_core->uptime_ms();
            return;
        }
    }

    ble_sync_set(false);
    pace_fixed(ble ? NRF_SCAN_INTERVAL_BLE_MS : NRF_SCAN_INTERVAL_MS);
}

void nrf_power_task(void) {
#if NRF_POWER_OFF_TIMEOUT_MS > 0
    /* Before the idle checks: lit LEDs or a pointing device keep the
     * keyboard from idling, but must not keep it on until the battery is
     * flat (power_off() switches the LEDs off first). Not on USB power:
     * System OFF would drop the keyboard from the host (and plugged-in VBUS
     * cannot wake the chip, only a VBUS change can). */
    if (last_input_activity_elapsed() >= NRF_POWER_OFF_TIMEOUT_MS && !(nrf_core->conn_state() & NRF_CORE_CONN_USB_POWERED)) {
        power_off();
    }
#endif

    if (!nrf_idle_allowed_kb()) {
        pace_scan();
        return;
    }
    /* A held key keeps scanning (tap-hold, combos... need it), but not
     * forever: after NRF_HELD_IDLE_TIMEOUT_MS without any matrix change it
     * is treated as held or stuck and the keyboard may idle; its line then
     * wakes on release (arm_change). */
    if (any_key_held()) {
#if NRF_HELD_IDLE_TIMEOUT_MS > 0
        if (last_matrix_activity_elapsed() < NRF_HELD_IDLE_TIMEOUT_MS) {
            pace_scan();
            return;
        }
#else
        pace_scan();
        return;
#endif
    }

#if NRF_IDLE_TIMEOUT_MS > 0
    /* Woken by a key (or encoder) line: the change is still in debounce, so
     * the matrix has not changed for QMK yet (last_matrix_activity counts
     * debounced changes only). Idling again right away would stop scanning
     * mid-debounce and turn one press into several, or lose it: keep
     * scanning for a full idle timeout first. */
    if (pin_woke) {
        if (nrf_core->uptime_ms() - pin_wake_ms < NRF_IDLE_TIMEOUT_MS) {
            pace_scan();
            return;
        }
        pin_woke = false;
    }

    if (last_matrix_activity_elapsed() >= NRF_IDLE_TIMEOUT_MS) {
        /* No connection event wake-ups while idle, and drop events left over
         * from the active phase (the scans since handled them), so that
         * wait_event() does not return immediately. */
        ble_sync_set(false);
        nrf_core->wait_event(0);
        if (nrf_matrix_wake_prepare()) {
            uint32_t ev = nrf_core->wait_event(NRF_IDLE_WAKE_INTERVAL_MS);
            nrf_matrix_wake_finish();
            if (ev & NRF_CORE_EVT_GPIO_WAKE) {
                pin_woke    = true;
                pin_wake_ms = nrf_core->uptime_ms();
            }
            return;
        }
    }
#endif

    pace_scan();
}
