# we67lite

65 % WeKey keyboard: 67 keys and an EC11 encoder (top right), USB-C and BLE,
Minew MS88SF3 module (nRF52840) on the back of the PCB, LiPo with charger.

* Keyboard Maintainer: WeKey
* Hardware: PCB WE67LITE rev 3.0 (KiCad 5)

Matrix: 9 rows x 8 columns, COL2ROW (diode anode on the switch, cathode on
the row). Each physical row uses two matrix rows, the keys alternating
between them (Esc = 0,0, 1 = 1,0, 2 = 0,1 ...); the EC11 push button is
matrix 0,7 (the key position top right).

| Module pin | GPIO | Net | Use in firmware |
|---|---|---|---|
| 10, 11, 39, 40, 41, 42, 43, 44, 5 | P0.29, P0.28, P0.13, P0.15, P0.17, P0.21, P0.23, P0.20, P1.14 | row0..row8 | matrix rows |
| 12, 13, 14, 2, 48, 47, 46, 45 | P0.30, P0.31, P0.27, P1.11, P0.25, P1.00, P0.24, P0.22 | col0..col7 | matrix columns |
| 4, 3 | P1.10, P1.12 | rA, rB | EC11 A / B (100k pull-ups, 470R + 10 nF) |
| 17 | P0.26 | LED_BLUE | Caps Lock: LED4 blue, under Caps Lock (low = on) |
| 9, 8 | P0.02, P1.15 | led0, led1 (PCB) | LED1 green / blue (right column, row 2) |
| 7 | P1.13 | led2 | LED2 green (right column, row 3) |
| 6 | P0.03 | led3 | LED3 green (right column, row 4) |
| 16 | P0.04 (AIN2) | VDIV | battery, 820k / 2M divider |
| 15 | P0.05 | CHARGE_CTRL | MCP73831 PROG via JP2 (open as shipped; JP3 closed = always charging) |
| 38 | P0.19 | CHARGE_STAT | charge complete (low), via Q3 |
| 29 | P0.16 | RGB | WS2812 data on J2 / J9 (470R) |
| 30 | P0.14 | RGB_CTRL | underglow power (Q6, high = on) |
| 28 | P0.18 | RST | reset button SW1 (double tap: bootloader) |
| 22, 25, 26, 27 | P0.11, P0.08, P1.09, P0.12 | J5 | expansion header (+ GND, 3.3 V) |
| 21, 20, 19 | P1.08, P0.06, P0.07 | J8 | expansion header (+ GND, 3.3 V) |
| 23, 24 | P0.00, P0.01 | n.c. | no 32.768 kHz crystal on the PCB: RC core |
| 31, 32, 33 | VDD, VDDH, DCCH | +3.3V, +3.3V, n.c. | XC6220 LDO, normal mode |
| 35, 36, 37 | VBUS, D-, D+ | USB | USB HID |

LED0 under Esc is the charge LED (MCP73831 STAT, no firmware).

```
Esc 1 2 3 4 5 6 7 8 9 0 - = Bkspc   [EC11: volume, push: mute]
Tab Q W E R T Y U I O P [ ] \       Del
Caps A S D F G H J K L ; ' Enter    PgUp
Shift Z X C V B N M , . / Shift  Up PgDn
Ctrl GUI Alt  Space   Alt Fn  Left Down Right

Fn: F1..F12 on the number row, Fn + ` (Esc) = `
    Fn + Q / W / E   BLE profile 1 / 2 / 3     Fn + R   forget current host
    Fn + U           USB / BLE                 Fn + \   bootloader
    Fn + Right Alt   2.4 GHz USB dongle (WeKey dongle)
    Fn + EC11        screen brightness
```

Connection LEDs (right column), for 3 s after the output, the profile or the
connection changes: LED1 blue = USB; green LED1 / LED2 / LED3 = BLE profile
1 / 2 / 3; all three green = the dongle.

Build:

    qmk compile -kb wekey/we67lite -km default

Flash: double-tap reset (or Fn + `\`), then copy onto the `WEKEY840` drive:
on a new board or after a platform update first the core `nrf52840.uf2` (RC
oscillator; distributed with the core, not part of this repository), then
after another double-tap `wekey_we67lite_default.uf2`; for keymap changes
only `wekey_we67lite_default.uf2`. The core's own build also makes
`full/wekey_we67lite_default.uf2` (core + QMK in one file). A blank
board first needs the WeKey UF2 bootloader, flashed once with a debugger on
J4 (SWD).

Not tested on the board yet. Open: underglow (number of LEDs), whether the
MS88SF3 has a 32.768 kHz crystal and the DC/DC inductor inside (both would
save current), the expansion headers J5 / J8.
