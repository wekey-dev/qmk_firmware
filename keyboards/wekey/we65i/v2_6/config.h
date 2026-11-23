// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* WE65i PCB rev 2.6, Ebyte E73 module (nRF52840) on the back of the PCB.
 *
 * 32.768 kHz crystal Y1 on XL1/XL2 (P0.00/P0.01): core variant
 * nrf/nrf52840/xtal. */
#define NRF_LFCLK_CRYSTAL

/* Power (schematic): USB 5 V or the LiPo (slide switch SW3) ->
 * power path (AO3407 + BAT60) -> XC6220 3.3 V LDO -> VDD *and* VDDH, normal
 * mode. USB VBUS goes to the module's VBUS pin.
 *   REG0  LDO: DCCH is not connected (and VDDH = VDD).
 *   REG1  DC/DC: the E73 has the inductor inside (as on we6k). If a board
 *         keeps resetting, double-tap reset and flash an image without it. */
#define NRF_POWER_DCDC_REG1

/* Battery: LiPo through a 820k / 2M divider into P0.05 (AIN3), as on we6k:
 *   VBAT -> 820k (R5) -> P0.05 -> 2M (R6) -> GND, 10 nF (C5). */
#define NRF_BATTERY_METHOD   NRF_BATTERY_DIVIDER
#define NRF_BATTERY_PIN      P0_05
#define NRF_BATTERY_R_TOP    820000
#define NRF_BATTERY_R_BOTTOM 2000000

/* Charger: MCP73831 (U4), PROG 5k (R8; JP1 closed adds R7 in parallel:
 * 2.5k, about twice the current).
 *   As shipped JP3 (ON-CHARGE, closed on the PCB) ties R8 to GND: the board
 *   always charges, CHARGE_CTRL does nothing. With JP3 cut and JP2
 *   (CTRL-CHARGE) closed, CHARGE_CTRL P0.24 low = charge, floating = none.
 *   CHARGE_STAT P0.13 <- Q3 (gate = STAT): low only when the charge is
 *   complete; floating while charging and without USB. No pull-up on the PCB.
 * LED0 (under Esc) shows the charge state by itself. */
#define NRF_CHARGER_EN_PIN              P0_24
#define NRF_CHARGER_EN_ACTIVE_LOW
#define NRF_CHARGER_STAT_PIN            P0_13
#define NRF_CHARGER_STAT_PULLUP
#define NRF_CHARGER_STAT_CHARGING_LEVEL 1

/* Indicator LEDs (v2_6.c). LED1..LED3 (right column, rows 2..4): common
 * anode on +3.3 V, the module sinks through 470R: low = on.
 *   LED1  blue P0.08, green P0.04 (via JP6, closed)
 *   LED2  blue P0.06
 *   LED3  blue P0.10 (NFC2 used as GPIO; via JP7, closed)
 * Caps Lock: LED4 blue under Caps Lock, its anode on P0.09 (NFC1), cathode
 * to GND through 470R: high = on (keyboard.json). */
#define WE65I_LED_PROFILE_PINS { P0_08, P0_06, P0_10 }
#define WE65I_LED_USB_PIN      P0_04
#define WE65I_LED_CAPS_PIN     P0_09
/* How long the connection LEDs stay on after the output, the profile or the
 * connection changed (ms) */
#define WE65I_LED_SHOW_MS 3000

/* Underglow (not used: no LED strip fitted):
 *   WS2812 data P1.09 (470R, R29) on J9 / J10;
 *   their GND is switched by Q6, gate RGB_CTRL P0.26 (high = LEDs powered,
 *   100k pull-down R28: off by default). */
#define WE65I_RGB_POWER_PIN P0_26
