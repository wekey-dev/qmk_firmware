# we65i v2.6

65 % WeKey keyboard: 67 keys and an EC11 encoder (top right), USB-C and BLE,
Ebyte E73 module (nRF52840) on the back of the PCB, 32.768 kHz crystal, LiPo
with charger. Same key layout and matrix as `wekey/we67lite`.

* Keyboard Maintainer: WeKey
* Hardware: PCB WE65i rev 2.6 (KiCad 5)

Matrix: 9 rows x 8 columns, COL2ROW (diode anode on the switch, cathode on
the row). Each physical row uses two matrix rows, the keys alternating
between them (Esc = 0,0, 1 = 1,0, 2 = 0,1 ...); the EC11 push button is
matrix 0,7 (the key position top right).

| E73 pin | GPIO | Net | Use in firmware |
|---|---|---|---|
| 28, 30, 32, 34, 36, 38, 40, 42, 7 | P0.15, P0.17, P0.20, P0.22, P1.00, P1.02, P1.04, P1.06, P0.02 | row0..row8 | matrix rows |
| 8, 9, 10, 1, 6, 4, 3, 2 | P0.29, P0.31, P0.30, P1.11, P1.13, P0.28, P0.03, P1.10 | col0..col7 | matrix columns (col3 via JP5, closed; JP4 would put it on P0.10) |
| 20, 22 | P0.12, P0.07 | rA, rB | EC11 A / B (100k pull-ups, 470R + 10 nF) |
| 41 | P0.09 (NFC1) | LED_BLUE | Caps Lock: LED4 under Caps Lock, anode on the pin (high = on) |
| 16, 18 | P0.08, P0.04 | led1, led0 | LED1 blue / green (right column, row 2; green via JP6) |
| 14 | P0.06 | led2 | LED2 blue (right column, row 3) |
| 43 | P0.10 (NFC2) | led3 | LED3 blue (right column, row 4; via JP7) |
| 15 | P0.05 (AIN3) | VDIV | battery, 820k / 2M divider |
| 35 | P0.24 | CHARGE_CTRL | MCP73831 PROG via JP2 (open as shipped; JP3 closed = always charging) |
| 33 | P0.13 | CHARGE_STAT | charge complete (low), via Q3 |
| 17 | P1.09 | RGB | WS2812 data on J9 / J10 (470R), not used |
| 12 | P0.26 | RGB_CTRL | underglow power (Q6, high = on), kept off |
| 26 | P0.18 | RST | reset button SW1 (double tap: bootloader) |
| 11, 13 | P0.00, P0.01 | XL1, XL2 | 32.768 kHz crystal Y1 |
| 19, 23, 25 | VDD, VDDH, DCCH | +3.3V, +3.3V, n.c. | XC6220 LDO, normal mode; REG1 DC/DC (inductor inside the E73) |
| 27, 29, 31 | VBUS, D-, D+ | USB | USB HID |

J8 is a connector for an external encoder (A, B, GND) whose push button
would sit between col7 and row1, matrix 1,7 (no key there on the PCB).
LED0 under Esc is the charge LED (MCP73831 STAT, no firmware).

```
Esc 1 2 3 4 5 6 7 8 9 0 - = Bkspc   [EC11: volume, push: mute]
Tab Q W E R T Y U I O P [ ] \       Del
Caps A S D F G H J K L ; ' Enter    PgUp
Shift Z X C V B N M , . / Shift  Up PgDn
Ctrl GUI Alt  Space   Alt Fn  Left Down Right

Fn: F1..F12 on the number row, Fn + Esc = `, Fn + Bkspc = Del
    Fn + Del / PgUp / PgDn   BLE profile 1 / 2 / 3   Fn + EC11 push   USB
    Fn + \                   forget current host     Fn + Enter       bootloader
    Fn + Right Alt           2.4 GHz USB dongle (WeKey dongle)
    Fn + Up = PgUp, Fn + Left / Down / Right = Home / PgDn / End
    Fn + EC11                screen brightness
```

Connection LEDs (right column, under Del / PgUp / PgDn: the keys that select
the profiles with Fn), for 3 s after the output, the profile or the connection
changes: LED1 green = USB; blue LED1 / LED2 / LED3 = BLE profile 1 / 2 / 3;
all three blue = the dongle.

Build:

    qmk compile -kb wekey/we65i/v2_6 -km default

Flash: double-tap reset (or Fn + Enter), then copy onto the `WEKEY840` drive:
on a new board or after a platform update first the core `nrf52840_xtal.uf2`
(distributed with the core, not part of this repository), then after another
double-tap `wekey_we65i_v2_6_default.uf2`; for keymap changes only
`wekey_we65i_v2_6_default.uf2`. The core's own build also makes
`full/wekey_we65i_v2_6_default.uf2` (core + QMK in one file). A blank
board first needs the WeKey UF2 bootloader, flashed once with a debugger on
J4 (SWD). The debug build also enters the bootloader with `nrf dfu` on its
serial console.

Dongle: `rules.mk` sets `NRF_DONGLE = yes`, so the core's build also writes
`dongle/wekey_we65i_v2_6_default.uf2` (a QMK build with `NRF_CORE_SDK=<complete
core SDK>`: `wekey_we65i_v2_6_default_dongle.uf2`): the WeKey 2.4 GHz dongle (nRF52840
Dongle / E104-BT5040U) that shows up as this keyboard (`5559:5249` "we65i")
from the first plug-in. The generic `wekey_dongle.uf2` (distributed with the
core) works too: it becomes this keyboard once paired. Copy it onto the dongle's `WEKEYDONGLE` drive, then
pair with Fn + Right Alt with the keyboard close to the dongle. Uncomment
`NRF_DONGLE_LOCK = yes` in `rules.mk` and the dongle pairs no other keyboard
(`platforms/nrf/README.md`, "Dongle image").
