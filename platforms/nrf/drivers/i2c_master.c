// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// QMK I2C master on the core: it drives TWIM0 (EasyDMA, interrupt,
// timeout). Pins as on ChibiOS: I2C1_SCL_PIN / I2C1_SDA_PIN in config.h,
// bus speed I2C1_CLOCK_SPEED (100000, 250000 or 400000 Hz).

#include <string.h>
#include "i2c_master.h"
#include "gpio.h"
#include "nrf_core.h"

#ifndef I2C1_SCL_PIN
#    error "I2C1_SCL_PIN must be defined (e.g. #define I2C1_SCL_PIN P0_26)"
#endif
#ifndef I2C1_SDA_PIN
#    error "I2C1_SDA_PIN must be defined (e.g. #define I2C1_SDA_PIN P0_06)"
#endif
#ifndef I2C1_CLOCK_SPEED
#    define I2C1_CLOCK_SPEED 400000
#endif
/* Register writes are sent in one transaction: register address + data */
#ifndef I2C_REGISTER_WRITE_MAX
#    define I2C_REGISTER_WRITE_MAX 256
#endif

static i2c_status_t to_status(int32_t err) {
    if (err == NRF_CORE_OK) {
        return I2C_STATUS_SUCCESS;
    }
    return err == NRF_CORE_ETIMEDOUT ? I2C_STATUS_TIMEOUT : I2C_STATUS_ERROR;
}

__attribute__((weak)) void i2c_init(void) {
    static bool done = false;
    if (!done) {
        done = true;
        nrf_core->i2c_init(I2C1_SCL_PIN, I2C1_SDA_PIN, I2C1_CLOCK_SPEED);
    }
}

i2c_status_t i2c_transmit(uint8_t address, const uint8_t *data, uint16_t length, uint16_t timeout) {
    /* QMK passes the 7-bit address shifted left (as ChibiOS: address >> 1).
     * i2c_transmit_P is a macro for i2c_transmit outside AVR; flash data is
     * copied to RAM by the core. */
    return to_status(nrf_core->i2c_xfer(address >> 1, data, length, NULL, 0, timeout));
}

i2c_status_t i2c_receive(uint8_t address, uint8_t *data, uint16_t length, uint16_t timeout) {
    return to_status(nrf_core->i2c_xfer(address >> 1, NULL, 0, data, length, timeout));
}

i2c_status_t i2c_transmit_and_receive(uint8_t address, const uint8_t *tx_data, uint16_t tx_length, uint8_t *rx_data, uint16_t rx_length, uint16_t timeout) {
    return to_status(nrf_core->i2c_xfer(address >> 1, tx_data, tx_length, rx_data, rx_length, timeout));
}

static i2c_status_t write_register(uint8_t devaddr, const uint8_t *reg, uint8_t reg_len, const uint8_t *data, uint16_t length, uint16_t timeout) {
    static uint8_t buf[2 + I2C_REGISTER_WRITE_MAX];
    if (length > I2C_REGISTER_WRITE_MAX) {
        return I2C_STATUS_ERROR;
    }
    memcpy(buf, reg, reg_len);
    memcpy(buf + reg_len, data, length);
    return i2c_transmit(devaddr, buf, reg_len + length, timeout);
}

i2c_status_t i2c_write_register(uint8_t devaddr, uint8_t regaddr, const uint8_t *data, uint16_t length, uint16_t timeout) {
    return write_register(devaddr, &regaddr, 1, data, length, timeout);
}

i2c_status_t i2c_write_register16(uint8_t devaddr, uint16_t regaddr, const uint8_t *data, uint16_t length, uint16_t timeout) {
    const uint8_t reg[2] = {regaddr >> 8, regaddr & 0xFF};
    return write_register(devaddr, reg, 2, data, length, timeout);
}

i2c_status_t i2c_read_register(uint8_t devaddr, uint8_t regaddr, uint8_t *data, uint16_t length, uint16_t timeout) {
    return i2c_transmit_and_receive(devaddr, &regaddr, 1, data, length, timeout);
}

i2c_status_t i2c_read_register16(uint8_t devaddr, uint16_t regaddr, uint8_t *data, uint16_t length, uint16_t timeout) {
    const uint8_t reg[2] = {regaddr >> 8, regaddr & 0xFF};
    return i2c_transmit_and_receive(devaddr, reg, 2, data, length, timeout);
}

i2c_status_t i2c_ping_address(uint8_t address, uint16_t timeout) {
    uint8_t dummy;
    return i2c_receive(address, &dummy, 1, timeout);
}
