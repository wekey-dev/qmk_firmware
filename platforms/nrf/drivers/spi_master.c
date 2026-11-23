// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// QMK SPI master on the core: it drives SPIM2 (EasyDMA, up to 8 MHz);
// chip select is a plain GPIO handled here. Pins as on ChibiOS: SPI_SCK_PIN,
// SPI_MOSI_PIN, SPI_MISO_PIN (NO_PIN if unused).
//
// divisor: SPI clock = 64 MHz / divisor (as a peripheral clock divider on
// ChibiOS), rounded down to 125 kHz .. 8 MHz by the core.

#include "spi_master.h"
#include "gpio.h"
#include "nrf_core.h"

#ifndef SPI_SCK_PIN
#    error "SPI_SCK_PIN must be defined"
#endif
#ifndef SPI_MOSI_PIN
#    define SPI_MOSI_PIN NO_PIN
#endif
#ifndef SPI_MISO_PIN
#    define SPI_MISO_PIN NO_PIN
#endif
#ifndef SPI_BASE_CLOCK_HZ
#    define SPI_BASE_CLOCK_HZ 64000000UL
#endif

static pin_t current_cs = NO_PIN;
static bool  cs_low     = true;

__attribute__((weak)) void spi_init(void) {
    static bool done = false;
    if (!done) {
        done = true;
        nrf_core->spi_init(SPI_SCK_PIN, SPI_MOSI_PIN, SPI_MISO_PIN);
    }
}

bool spi_start_extended(spi_start_config_t *config) {
    if (current_cs != NO_PIN || config->mode > 3) {
        return false;
    }
    uint32_t freq_hz = SPI_BASE_CLOCK_HZ / (config->divisor ? config->divisor : 1);
    uint32_t flags   = config->mode | (config->lsb_first ? NRF_CORE_SPI_LSB_FIRST : 0);
    /* Before CS goes active: a mode change moves SCK to its new idle level,
     * which the device would otherwise take for a clock edge. */
    if (nrf_core->spi_config(freq_hz, flags) != NRF_CORE_OK) {
        return false;
    }
    current_cs = config->slave_pin;
    cs_low     = config->cs_active_low;
    if (current_cs != NO_PIN) {
        gpio_set_pin_output(current_cs);
        gpio_write_pin(current_cs, cs_low ? 0 : 1);
    }
    return true;
}

bool spi_start(pin_t slavePin, bool lsbFirst, uint8_t mode, uint16_t divisor) {
    spi_start_config_t config = {
        .slave_pin     = slavePin,
        .lsb_first     = lsbFirst,
        .mode          = mode,
        .divisor       = divisor,
        .cs_active_low = true,
    };
    return spi_start_extended(&config);
}

static spi_status_t xfer(const void *tx, uint16_t tx_len, void *rx, uint16_t rx_len) {
    int32_t err = nrf_core->spi_xfer(tx, tx_len, rx, rx_len);
    if (err == NRF_CORE_OK) {
        return SPI_STATUS_SUCCESS;
    }
    return err == NRF_CORE_ETIMEDOUT ? SPI_STATUS_TIMEOUT : SPI_STATUS_ERROR;
}

spi_status_t spi_write(uint8_t data) {
    uint8_t rx;
    spi_status_t st = xfer(&data, 1, &rx, 1);
    return st == SPI_STATUS_SUCCESS ? rx : st;
}

spi_status_t spi_read(void) {
    uint8_t rx;
    spi_status_t st = xfer(NULL, 0, &rx, 1); /* sends 0xFF */
    return st == SPI_STATUS_SUCCESS ? rx : st;
}

spi_status_t spi_transmit(const uint8_t *data, uint16_t length) {
    return length ? xfer(data, length, NULL, 0) : SPI_STATUS_SUCCESS;
}

spi_status_t spi_receive(uint8_t *data, uint16_t length) {
    return length ? xfer(NULL, 0, data, length) : SPI_STATUS_SUCCESS;
}

void spi_stop(void) {
    if (current_cs != NO_PIN) {
        gpio_write_pin(current_cs, cs_low ? 1 : 0);
        current_cs = NO_PIN;
    }
}
