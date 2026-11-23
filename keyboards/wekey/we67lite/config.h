// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* Minew MS88SF3 module (nRF52840), on the back of the PCB.
 *
 * 32.768 kHz: XL1/XL2 (P0.00/P0.01) are not connected on this PCB, so the
 * core variant with the RC oscillator (nrf/nrf52840) is used. If the module
 * has the crystal inside, NRF_LFCLK_CRYSTAL saves a little current: try it
 * on the board (BLE must still connect and stay connected).
 *
 * Power (schematic): USB 5 V or the LiPo (slide switch SW3) ->
 * power path (AO3407) -> XC6220 3.3 V LDO -> the module's VDD *and* VDDH,
 * i.e. normal mode. USB VBUS goes to the module's VBUS pin.
 *
 * DC/DC: REG0 never (DCCH not connected, VDDH = VDD), LDO. REG1 needs an
 * inductor inside the module (not on this PCB): untested, so LDO. To try it,
 * uncomment the line below; if the keyboard keeps resetting, double-tap
 * reset to reach the bootloader and flash an image without it. */
// #define NRF_POWER_DCDC_REG1

/* Battery: LiPo through a 820k / 2M divider into P0.04 (AIN2), as on we6k:
 *   VBAT -> 820k (R5) -> P0.04 -> 2M (R6) -> GND, 10 nF (C5). */
#define NRF_BATTERY_METHOD   NRF_BATTERY_DIVIDER
#define NRF_BATTERY_PIN      P0_04
#define NRF_BATTERY_R_TOP    820000
#define NRF_BATTERY_R_BOTTOM 2000000

/* Charger: MCP73831 (U4), PROG 5k (R7), as on we6k.
 *   As shipped JP3 (ON-CHARGE, closed) ties R7 to GND: the board always
 *   charges, CHARGE_CTRL does nothing. With JP3 cut and JP2 (CTRL-CHARGE)
 *   closed, CHARGE_CTRL P0.05 low = charge, floating = no charge.
 *   CHARGE_STAT P0.19 <- Q3 (gate = STAT): low only when the charge is
 *   complete; floating while charging and without USB. No pull-up on the PCB.
 * LED0 (under Esc) shows the charge state by itself. */
#define NRF_CHARGER_EN_PIN              P0_05
#define NRF_CHARGER_EN_ACTIVE_LOW
#define NRF_CHARGER_STAT_PIN            P0_19
#define NRF_CHARGER_STAT_PULLUP
#define NRF_CHARGER_STAT_CHARGING_LEVEL 1

/* Indicator LEDs: common anode on +3.3 V, the module sinks through 470R,
 * so low = on (we67lite.c). Caps Lock is LED4 blue, P0.26 (keyboard.json).
 *   LED1 (right column, row 2): green P0.02, blue P1.15
 *   LED2 (right column, row 3): green P1.13
 *   LED3 (right column, row 4): green P0.03
 * (In the PCB the nets led0 / led1 are swapped against the schematic; the
 * pins drive the same LED channels either way.) */
#define WE67LITE_LED_PROFILE_PINS { P0_02, P1_13, P0_03 }
#define WE67LITE_LED_USB_PIN      P1_15
/* How long the connection LEDs stay on after the output, the profile or the
 * connection changed (ms) */
#define WE67LITE_LED_SHOW_MS 3000

/* Underglow (not enabled yet: the number of LEDs is not known):
 *   WS2812 data P0.16 (470R, R29) on J2 / J9;
 *   their GND is switched by Q6, gate RGB_CTRL P0.14 (high = LEDs powered,
 *   100k pull-down R28: off by default). */
#define WE67LITE_RGB_POWER_PIN P0_14
