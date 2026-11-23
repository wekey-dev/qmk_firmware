// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* Ebyte E73 module (nRF52840).
 *
 * 32.768 kHz crystal: fitted on the PCB at the module's XL1/XL2 pins
 * (P0.00/P0.01), so this keyboard uses the core variant nrf/nrf52840/xtal.
 * P0.00 and P0.01 are therefore not available as GPIO. */
#define NRF_LFCLK_CRYSTAL

/* Power (schematic): USB 5 V or the LiPo -> power path (AO3407 + BAT60)
 * -> XC6220 3.3 V LDO -> the module's VDD *and* VDDH, i.e. normal mode, not
 * the high voltage mode. USB VBUS goes to the module's VBUS pin.
 *
 * DC/DC (saves current, mostly while the radio is active):
 *   REG0  never on this PCB: DCCH is not connected (and VDDH = VDD), LDO.
 *   REG1  DC/DC: the inductor on DCC/DEC4 is inside the E73 module (not
 *         exposed). Tested on we6k 2026-11-21: runs over USB and BLE on
 *         battery without resets. If a board with another module keeps
 *         resetting, its module has no inductor: double-tap reset to reach
 *         the bootloader and flash an image without this line. */
#define NRF_POWER_DCDC_REG1

/* Battery: LiPo through a 820k / 2M divider into P0.05 (AIN3), like nice!nano:
 *   VBAT -> 820k -> P0.05 -> 2M -> GND
 * (+ 10 nF from P0.05 to GND, C7). 4.2 V at the battery gives ~2.98 V at the
 * pin, below VDD = 3.3 V. The divider is always connected (no enable pin):
 * ~1.5 uA at 4.2 V. */
#define NRF_BATTERY_METHOD   NRF_BATTERY_DIVIDER
#define NRF_BATTERY_PIN      P0_05
#define NRF_BATTERY_R_TOP    820000
#define NRF_BATTERY_R_BOTTOM 2000000

/* Charger: MCP73831 (U4), 200 mA (PROG 5k, R7).
 *   CHARGE_CTRL P1.04 -> JP1 (closed) -> R7 -> PROG. Low = charge,
 *     floating = charge disabled (MCP73831 PROG high impedance). With JP1
 *     cut and JP2 closed the PCB charges on its own, without P1.04.
 *   CHARGE_STAT P1.06 <- Q3 (gate driven by STAT): pulled low only when STAT
 *     is high = charge complete; floating while charging (STAT low) and
 *     without USB (STAT high impedance, R12). No pull-up on the PCB.
 * The core keeps charging enabled and reports the state (needs USB VBUS,
 * which also feeds the charger). The bi-color LED2 on the PCB shows it too. */
#define NRF_CHARGER_EN_PIN              P1_04
#define NRF_CHARGER_EN_ACTIVE_LOW
#define NRF_CHARGER_STAT_PIN            P1_06
#define NRF_CHARGER_STAT_PULLUP
#define NRF_CHARGER_STAT_CHARGING_LEVEL 1

/* RGB Matrix: IS31FL3731 (U7) over I2C, 7 common-anode RGB LEDs on its
 * matrix A (CA2..CA7, charlieplexed). LED map in we6k.c.
 *   I2C  SCL P0.26, SDA P0.06 (5.1k pull-ups R30/R31 on the PCB)
 *   AD   to GND -> address 0x74;  SDB P0.04 (100k pull-down R27: off by default)
 *   INTB P0.08 (not used) */
#define I2C1_SCL_PIN P0_26
#define I2C1_SDA_PIN P0_06
#define I2C1_CLOCK_SPEED 400000
#define IS31FL3731_I2C_ADDRESS_1 IS31FL3731_I2C_ADDRESS_GND
#define IS31FL3731_SDB_PIN P0_04
#define IS31FL3731_DEGHOST
