// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include "bootloader.h"
#include "nrf_core.h"

__attribute__((weak)) void bootloader_jump(void) {
    nrf_core->reboot(NRF_CORE_REBOOT_BOOTLOADER);
}

__attribute__((weak)) void mcu_reset(void) {
    nrf_core->reboot(NRF_CORE_REBOOT_NORMAL);
}

__attribute__((weak)) void enter_bootloader_mode_if_requested(void) {}
