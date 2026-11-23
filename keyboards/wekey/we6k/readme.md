# we6k

Seven-key WeKey test PCB: a 3 x 2 block plus a `*` key (or an EC11 encoder),
nRF52840 (Ebyte E73), USB and BLE.

* Keyboard Maintainer: WeKey
* Hardware: nRF52840, COL2ROW matrix (cols P0.31, P0.29, P0.02, P0.15;
  rows P0.30, P1.13), LiPo measured through 820k / 2M into P0.05,
  reset button

| E73 pin | GPIO | Net | Use in firmware |
|---|---|---|---|
| 9, 8, 7, 28 | P0.31, P0.29, P0.02, P0.15 | col0..col3 | matrix columns (COL2ROW) |
| 10, 6 | P0.30, P1.13 | row0, row1 | matrix rows |
| 22, 20 | P0.07, P0.12 | rA, rB | EC11 encoder (optional, push = matrix 0,3) |
| 41 | P0.09 (NFC1) | led0 | Caps Lock LED (active high) |
| 15 | P0.05 (AIN3) | VDIV | battery, 820k / 2M divider |
| 11, 13 | P0.00, P0.01 | XL1, XL2 | 32.768 kHz crystal |
| 26 | P0.18 | RST | reset button (double tap: bootloader) |
| 19, 23, 25 | VDD, VDDH, DCCH | +3.3V, +3.3V, n.c. | XC6220 LDO, normal mode; REG1 DC/DC (inductor inside the E73), REG0 LDO |
| 27, 29, 31 | VBUS, D-, D+ | USB | USB HID |
| 42, 40 | P1.06, P1.04 | CHARGE_STAT, CHARGE_CTRL | MCP73831 charger: status, enable (low = charge) |
| 12, 14, 18 | P0.26, P0.06, P0.04 | SCL, SDA, SDB | IS31FL3731 RGB Matrix (I2C 400 kHz, address 0x74; LED map in we6k.c) |
| 16 | P0.08 | INTB | IS31FL3731 interrupt (not used) |
| 36, 34, 32, 35, 30, 38, 33 | P1.00, P0.22, P0.20, P0.24, P0.17, P1.02, P0.13 | SCK, MISO, MOSI, H_SS, H_INT, H_RST, H_EN | MAX3421E USB host (needs SPI: not supported yet) |
| 43 | P0.10 (NFC2) | BOOT_CTRL | 5 V boost for the host port (not used yet) |
| 3, 4 | P0.03, P0.28 | A1, A4 | J3 expansion header |

```
┌─────┬─────┬─────┐
│  1  │  A  │  X  ├─────┐
├─────┼─────┼─────┤  *  │   hold *:   RGB on/off, mouse up, next RGB effect / mouse left, down, right
│ BT  │ SYS │ OUT ├─────┘   hold BT:  1 A X = BLE profile 1 2 3, * = 2.4 GHz dongle
└─────┴─────┴─────┘         hold SYS: 1 = forget current host (or dongle), A = USB/BLE, X = bootloader, OUT = Caps Lock
OUT: toggle USB / BLE. EC11: volume
```

Connection keys and their codes for VIA: `platforms/nrf/README.md`.
Debugging over USB: `platforms/nrf/DEBUGGING.md`.

VIA: Settings → "Show Design tab", then Design → Load Draft Definition →
`via/we6k.json`.

RGB Matrix: the LEDs switch off after 5 minutes without typing and while the
computer sleeps. In the schematic, RGB4 red and RGB7 green share the same
driver output (see `we6k.c`).

Build:

    qmk compile -kb wekey/we6k -km default

Flash: double-tap reset (or hold `SYS` + `X`), then copy onto the `WEKEY840`
drive: on a new board or after a platform update first the core
`nrf52840_xtal.uf2` (distributed with the core, not part of this
repository), then after another double-tap `wekey_we6k_default.uf2`; for
keymap changes only `wekey_we6k_default.uf2`. The core's own build also makes
`full/wekey_we6k_default.uf2` (core + QMK in one file).
A blank board first needs the WeKey UF2 bootloader, flashed once with a debugger.
