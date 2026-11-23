# WeKey nRF DK test keyboard

Four-key test "keyboard" using the buttons of the Nordic development kits, to
bring up the QMK app image on top of the nRF Core before real hardware
exists.

| Button | Pin   | Key                                  |
|--------|-------|--------------------------------------|
| BTN1   | P0.11 | `A`                                  |
| BTN2   | P0.12 | Caps Lock (host LED state is logged) |
| BTN3   | P0.24 | Volume up                            |
| BTN4   | P0.25 | Forget bonds and advertise again     |

Battery: the DKs have none, so `config.h` measures VDD
(`NRF_BATTERY_VDD`, 1.8-3.0 V mapped to 0-100 %) just to exercise the battery
path; the level shows up in the host's Bluetooth settings.

Variants:

* `wekey/nrf_dk/nrf52840` - nRF52840 DK (PCA10056)
* `wekey/nrf_dk/nrf52833` - nRF52833 DK (PCA10100)

Build this keyboard on its own, like any QMK keyboard (the core's ABI and
memory map come from `platforms/nrf/sdk`):

    qmk compile -kb wekey/nrf_dk/nrf52840 -km default
    util/docker_build.sh wekey/nrf_dk/nrf52840:default   # official QMK image

The core's own build can also build bootloader, core and this keyboard
together; that also refreshes `sdk`.

Flashing:

1. Once, with a debugger: erase the chip and write the WeKey UF2 bootloader
   (`out/bootloader/wekey_nrf52840.hex`, or `wekey_nrf52833.hex`). The DK's
   nRF USB port then shows a `WEKEY840` / `WEKEY833` drive.
2. Copy the core onto it: `nrf52840_xtal.uf2` (`nrf52833_xtal.uf2`),
   distributed with the core, not part of this repository. Or, from the
   core's own build, the full image `full/wekey_nrf_dk_nrf52840_default.uf2`
   (core + QMK in one file) instead of steps 2 and 3.
3. Double-tap reset and copy `wekey_nrf_dk_nrf52840_default.uf2` (this QMK
   build). Later keymap changes: only this file.
