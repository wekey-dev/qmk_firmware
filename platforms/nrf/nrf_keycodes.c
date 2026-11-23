// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// QMK's standard connection keycodes, handled by the nrf platform for every
// keyboard (quantum.c calls process_record_nrf() right after
// process_record_kb(), so a keyboard or keymap can still override them):
//
//   OU_AUTO          USB while plugged in, wireless (BLE or dongle) otherwise
//   OU_USB           force USB; leaves the wireless link after 5 s (core
//                    CONFIG_NRF_CORE_USB_WIRELESS_OFF_MS)
//   OU_BT            force BLE, current profile (leaves the dongle)
//   OU_2P4G          type to the 2.4 GHz USB dongle, also with USB plugged in;
//                    without a dongle bond the keyboard waits for one to pair
//   OU_NEXT/OU_PREV  toggle USB <-> wireless
//   BT_PRF1..BT_PRF5 type to BLE host 1..5 (one profile each), also with USB
//                    plugged in; profiles beyond CONFIG_NRF_CORE_BLE_PROFILES
//                    (3 by default) do nothing
//   BT_NEXT/BT_PREV  next / previous BLE profile
//   BT_UNPR          forget the host of the current profile (or the dongle,
//                    after OU_2P4G), advertise again
//
// OU_NONE does nothing. QMK's own CONNECTION_ENABLE is not used: the core
// decides the output (back to USB when it is plugged in again).
#include "quantum.h"
#include "nrf_core.h"

bool process_record_nrf(uint16_t keycode, keyrecord_t *record) {
    if (!IS_CONNECTION_KEYCODE(keycode)) {
        return true;
    }
    if (!record->event.pressed) {
        return false;
    }
    switch (keycode) {
        case QK_OUTPUT_AUTO:
            nrf_core_output_auto();
            break;
        case QK_OUTPUT_USB:
            nrf_core_output_usb();
            break;
        case QK_OUTPUT_BLUETOOTH:
            if (nrf_core_output_is_rf()) {
                nrf_core_ble_profile_select(nrf_core_ble_profile()); // back to the BLE host
            } else {
                nrf_core_output_ble();
            }
            break;
        case QK_OUTPUT_2P4GHZ:
            nrf_core_output_rf();
            break;
        case QK_OUTPUT_NEXT:
        case QK_OUTPUT_PREV:
            nrf_core_output_toggle();
            break;
        case QK_BLUETOOTH_PROFILE1 ... QK_BLUETOOTH_PROFILE5:
            nrf_core_ble_profile_select(keycode - QK_BLUETOOTH_PROFILE1);
            break;
        case QK_BLUETOOTH_PROFILE_NEXT:
            nrf_core_ble_profile_next();
            break;
        case QK_BLUETOOTH_PROFILE_PREV:
            nrf_core_ble_profile_prev();
            break;
        case QK_BLUETOOTH_UNPAIR:
            nrf_core_ble_clear_current();
            break;
        default: // QK_OUTPUT_NONE: not available
            break;
    }
    return false;
}
