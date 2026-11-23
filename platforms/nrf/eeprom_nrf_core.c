// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// EEPROM backed by the core (RAM mirror in the core, persisted lazily
// to flash through Zephyr settings). Selected automatically for PLATFORM=NRF.

#include <stdint.h>
#include <string.h>
#include "eeprom_driver.h"
#include "nrf_core.h"

void eeprom_driver_init(void) {
    if (nrf_core->eeprom_size < EEPROM_SIZE) {
        static const char msg[] = "qmk: EEPROM_SIZE larger than the core's emulated EEPROM\n";
        nrf_core->log_write(msg, sizeof(msg) - 1);
    }
}

void eeprom_driver_format(bool erase) {
    if (erase) {
        eeprom_driver_erase();
    }
}

void eeprom_driver_erase(void) {
    static const uint8_t zeros[64] = {0};
    for (uint32_t addr = 0; addr < EEPROM_SIZE; addr += sizeof(zeros)) {
        uint32_t len = EEPROM_SIZE - addr < sizeof(zeros) ? EEPROM_SIZE - addr : sizeof(zeros);
        nrf_core->eeprom_write(addr, zeros, len);
    }
    nrf_core->eeprom_flush();
}

void eeprom_read_block(void *buf, const void *addr, size_t len) {
    if (nrf_core->eeprom_read((uint32_t)(uintptr_t)addr, buf, (uint32_t)len) < 0) {
        memset(buf, 0, len);
    }
}

void eeprom_write_block(const void *buf, void *addr, size_t len) {
    nrf_core->eeprom_write((uint32_t)(uintptr_t)addr, buf, (uint32_t)len);
}
