// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// QMK protocol layer for the nRF Core: converts QMK reports into the ABI
// formats of nrf_core_api.h and hands them to the core, which queues them and
// delivers them over USB or BLE (the core's HID routing).

#include <string.h>
#include "keyboard.h"
#include "host.h"
#include "host_driver.h"
#include "report.h"
#include "sendchar.h"
#include "suspend.h"
#include "nrf_core.h"

#ifdef RAW_ENABLE
#    include "raw_hid.h"
#endif

_Static_assert(NKRO_REPORT_BITS == NRF_CORE_NKRO_BYTES, "QMK NKRO report size differs from nrf_core ABI");

/* ---- host driver -------------------------------------------------------- */

static uint8_t nrf_keyboard_leds(void) {
    return (uint8_t)nrf_core->hid_leds();
}

static void nrf_send_keyboard(report_keyboard_t *report) {
    /* ABI keyboard report: mods, reserved, keys[6] (no report id) */
    nrf_core->hid_send(NRF_CORE_HID_KEYBOARD, &report->mods, 8);
}

static void nrf_send_nkro(report_nkro_t *report) {
    nrf_core->hid_send(NRF_CORE_HID_NKRO, &report->mods, 1 + NRF_CORE_NKRO_BYTES);
}

static void nrf_send_mouse(report_mouse_t *report) {
    struct nrf_core_mouse_report m = {
        .buttons = report->buttons,
        .x       = report->x,
        .y       = report->y,
        .v       = (int8_t)report->v,
        .h       = (int8_t)report->h,
    };
    nrf_core->hid_send(NRF_CORE_HID_MOUSE, &m, sizeof(m));
}

static void nrf_send_extra(report_extra_t *report) {
    uint32_t type;

    switch (report->report_id) {
        case REPORT_ID_SYSTEM:
            type = NRF_CORE_HID_SYSTEM;
            break;
        case REPORT_ID_CONSUMER:
            type = NRF_CORE_HID_CONSUMER;
            break;
        default:
            return;
    }
    uint8_t usage[2] = {report->usage & 0xFF, report->usage >> 8};
    nrf_core->hid_send(type, usage, sizeof(usage));
}

#ifdef RAW_ENABLE
static void nrf_send_raw_hid(uint8_t *data, uint8_t length) {
    uint8_t pkt[NRF_CORE_RAW_HID_SIZE] = {0};
    memcpy(pkt, data, length < sizeof(pkt) ? length : sizeof(pkt));
    nrf_core->hid_send(NRF_CORE_HID_RAW, pkt, sizeof(pkt));
}
#endif

static host_driver_t nrf_driver = {
    .keyboard_leds = nrf_keyboard_leds,
    .send_keyboard = nrf_send_keyboard,
    .send_nkro     = nrf_send_nkro,
    .send_mouse    = nrf_send_mouse,
    .send_extra    = nrf_send_extra,
#ifdef RAW_ENABLE
    .send_raw_hid = nrf_send_raw_hid,
#endif
};

/* ---- QMK main loop hooks (quantum/main.c) -------------------------------- */

void protocol_setup(void) {}

void protocol_pre_init(void) {}

void protocol_post_init(void) {
    host_set_driver(&nrf_driver);
}

/* The host suspended the USB bus (computer asleep) while reports go to USB. */
static bool usb_host_asleep(void) {
    uint32_t s = nrf_core->conn_state();
    return (s & NRF_CORE_CONN_OUTPUT_USB) && (s & NRF_CORE_CONN_USB_SUSPENDED);
}

/* As the ChibiOS protocol: while the host sleeps, suspend_power_down()
 * (RGB, LED matrix, OLED, backlight off: RGB_MATRIX_SLEEP...) until it
 * resumes, the output moves to BLE (USB unplugged), or a key press wakes it:
 * a report sent while suspended makes the core signal USB remote wakeup. */
void protocol_pre_task(void) {
    if (!usb_host_asleep()) {
        return;
    }
    while (usb_host_asleep()) {
        suspend_power_down(); /* includes a 17 ms sleep */
        if (suspend_wakeup_condition()) {
            static const uint8_t no_keys[8] = {0};
            nrf_core->hid_send(NRF_CORE_HID_KEYBOARD, no_keys, sizeof(no_keys));
            nrf_core->sleep_ms(100);
        }
    }
    suspend_wakeup_init();
}

void protocol_post_task(void) {
#ifdef AUDIO_ENABLE
    nrf_audio_task();
#endif
    nrf_power_task();
}

#ifdef RAW_ENABLE
void raw_hid_task(void) {
    uint8_t buffer[NRF_CORE_RAW_HID_SIZE];
    while (nrf_core->hid_raw_recv(buffer, sizeof(buffer)) > 0) {
        raw_hid_receive(buffer, sizeof(buffer));
    }
}
#endif

#ifdef CONSOLE_ENABLE
/* Console output goes to the core's log (UART / RTT). */
static char    console_buf[64];
static uint8_t console_len;

static void console_flush(void) {
    if (console_len > 0) {
        nrf_core->log_write(console_buf, console_len);
        console_len = 0;
    }
}

int8_t sendchar(uint8_t c) {
    console_buf[console_len++] = (char)c;
    if (c == '\n' || console_len == sizeof(console_buf)) {
        console_flush();
    }
    return 0;
}

void console_task(void) {
    console_flush();
}
#endif
