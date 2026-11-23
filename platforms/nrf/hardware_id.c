// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include "hardware_id.h"
#include "nrf_core.h"

__attribute__((weak)) hardware_id_t get_hardware_id(void) {
    hardware_id_t id = {0};
    nrf_core->device_id(id.data);
    return id;
}
