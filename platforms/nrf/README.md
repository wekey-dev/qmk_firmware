# Platform `nrf` (nRF52840 / nRF52833)

QMK keyboards on nRF52 chips get BLE, USB and a 2.4 GHz USB dongle (the WeKey dongle). Over BLE they can pair with several hosts, one per profile. Keystrokes go to USB while the cable is plugged in and to the wireless link (a BLE profile or the dongle) otherwise.

This page is for people writing keymaps and for people bringing up a new keyboard:

1. [Connection keycodes](#1-connection-keycodes) (BLE / USB), with their codes for VIA
2. [nrf API](#2-nrf-api-nrf_coreh) (`nrf_core.h`)
3. [Platform hooks](#3-platform-hooks)
4. [Settings in `config.h`](#4-settings-in-configh)

Debugging over USB (log, BLE / USB / battery state, I2C probing): [DEBUGGING.md](DEBUGGING.md).

---

## 1. Connection keycodes

These are **standard QMK keycodes** from `quantum/keycodes.h`. Upstream QMK defines them but does not implement the Bluetooth ones yet. On nrf, the platform handles all of them for every keyboard, so a keymap only has to place them: no code needed.

| Keycode   | Code (VIA "Any") | Action |
|-----------|------------------|--------|
| `BT_PRF1` | `0x7793` | Type to the BLE host of profile 1, even with USB plugged in |
| `BT_PRF2` | `0x7794` | … profile 2 |
| `BT_PRF3` | `0x7795` | … profile 3 |
| `BT_PRF4` | `0x7796` | … profile 4 (only if the firmware has at least 4 profiles) |
| `BT_PRF5` | `0x7797` | … profile 5 (only if the firmware has at least 5 profiles) |
| `BT_NEXT` | `0x7790` | Next BLE profile (wraps around to the first) |
| `BT_PREV` | `0x7791` | Previous BLE profile |
| `BT_UNPR` | `0x7792` | Forget the host paired on the current profile (or the dongle, after `OU_2P4G`) and advertise again, ready to pair a new one |
| `OU_AUTO` | `0x7780` | Automatic output: USB while plugged in, wireless (BLE profile or dongle) otherwise |
| `OU_USB`  | `0x7784` | Force USB. After 5 s (core `CONFIG_NRF_CORE_USB_WIRELESS_OFF_MS`) the keyboard also leaves the BLE host or the dongle and stops advertising, so a phone shows its on-screen keyboard again; a wireless key (`BT_PRFn`, `OU_BT`, `OU_2P4G`, `OU_AUTO`) or unplugging USB brings the link back (Android has to be reconnected from its Bluetooth settings) |
| `OU_BT`   | `0x7786` | Force BLE (current profile; leaves the dongle) |
| `OU_2P4G` | `0x7785` | Type to the 2.4 GHz USB dongle, even with USB plugged in. Without a paired dongle the keyboard waits for one to pair (hold the dongle within ~50 cm) |
| `OU_NEXT` | `0x7781` | Toggle USB ↔ wireless |
| `OU_PREV` | `0x7782` | Same as `OU_NEXT` |
| `OU_NONE` | `0x7783` | Does nothing (not supported) |

Notes:

- **Profiles:** there are 3 BLE profiles by default. Selecting a profile beyond that does nothing.
- **Selecting a profile switches to BLE.** Pressing `BT_PRFn` sends keystrokes over BLE to that host, even with USB still plugged in (USB keeps charging). This also applies to the profile already in use. Unplugging and replugging USB goes back to USB (`OU_AUTO`).
- **The dongle** is one more wireless target next to the profiles: `OU_2P4G` switches to it, `BT_PRFn` (or `OU_BT`) back to the BLE hosts. Only one wireless link at a time. The link is BLE with a 1 ms connection interval, so the keyboard scans every 1 ms (`NRF_SCAN_INTERVAL_RF_MS`) and typing feels like USB. To pair another dongle: `OU_2P4G`, then `BT_UNPR`; to make the dongle forget its keyboard, hold its button for 3 s. With `NRF_DONGLE = yes` in the keyboard's `rules.mk`, the build also writes a dongle image made for this keyboard; see [Dongle image](#dongle-image-nrf_dongle).
- **What is remembered:** the output choice is **not** kept across restarts; the keyboard always starts in `OU_AUTO`. The current BLE profile, and whether the dongle is the wireless target, are remembered.
- **Overriding:** the platform handles these keycodes *after* `process_record_kb()` and `process_record_user()`. Return `false` there and the platform leaves the key alone.
- **Don't enable QMK's connection features:** the platform does not use `CONNECTION_ENABLE` or `BLUETOOTH_ENABLE`, because the firmware chooses the output itself. Leave both off in `rules.mk`.

### Building against a complete core SDK (`NRF_CORE_SDK`)

`platforms/nrf/sdk` holds only what the app image is compiled against (ABI headers, memory map): this repository carries no prebuilt binaries, so a plain build gives `<target>.uf2` only, flashed after a core. The core is also distributed as a **complete core SDK** (prebuilt cores, the dongle, and the `tools/` that merge them). Unpack it anywhere and point the build at it:

```sh
make wekey/we65i/v2_6:default NRF_CORE_SDK=/path/to/sdk
qmk compile -kb wekey/we65i/v2_6 -km default -e NRF_CORE_SDK=/path/to/sdk
export NRF_CORE_SDK=/path/to/sdk     # or for every build
```

The build then uses that SDK instead of `platforms/nrf/sdk` and, next to `<target>.uf2`, also writes `<target>_full.uf2` (the core this keyboard needs + its image, for a first install; only if the SDK's ABI hash matches) and, with `NRF_DONGLE = yes`, `<target>_dongle.uf2`. Without `NRF_CORE_SDK` those two files are removed again. `util/docker_build.sh` only mounts this repository: put the SDK inside it to use it there.

### Dongle image (`NRF_DONGLE`)

The dongle firmware is the same for every keyboard (`wekey_dongle.uf2`, distributed with the core): once paired, it takes over the keyboard's VID/PID and name. A keyboard can also get its own dongle image:

```make
# rules.mk of the keyboard
NRF_DONGLE = yes        # the core's build writes dongle/<target>.uf2
NRF_DONGLE_LOCK = yes   # optional: that dongle pairs no other keyboard
```

This repository carries no prebuilt dongle (nor cores), so a plain QMK build only records these settings (`.build/<target>.nrf.json`). A build against a complete core SDK (`NRF_CORE_SDK`, above) writes `<target>_dongle.uf2`; the core's own build saves `dongle/<keyboard>_<keymap>.uf2`. Either way the prebuilt dongle gets this keyboard's VID/PID, version, manufacturer and name in its preset block. Copy it onto the dongle's `WEKEYDONGLE` drive (nrf_bootloader board `wekey_dongle`). Nothing is rebuilt, and the dongle works with nRF52833 keyboards too.

- **Identity from the first plug-in:** the computer and VIA see the keyboard before it is paired. Without the preset, the dongle shows up as "WeKey Dongle" until then.
- **`NRF_DONGLE_LOCK`:** the dongle ignores keyboards that advertise another VID/PID while waiting to pair. If it only finds out after connecting (a keyboard on an older core, or one still bonded to it), it reads the VID/PID from the Device Information Service. On a mismatch it disconnects that keyboard, forgets it, and ignores it until the dongle restarts.
- **The generic image** (`wekey_dongle.uf2`) has an empty preset. Flashing it turns a dongle made for one keyboard back into one that pairs with any keyboard. Bonds and the stored identity are kept; flashing never touches them.

### Using them in VIA

VIA does not know these keycodes yet, so an assigned key shows up as a number such as `0x7793`. To assign one:

1. Select the key on the keyboard view in VIA.
2. In the keycode list, open the **Special** tab and pick **Any**.
3. Enter the code from the table above (e.g. `0x7793` for `BT_PRF1`) and confirm.

Don't list these keycodes under `customKeycodes` in the VIA definition file. That list is only for the keyboard's own keycodes (`QK_KB_0`…), not for these codes.

### Keys QMK has no keycode for

For anything QMK has no standard keycode for, such as forgetting the hosts of **all** profiles, define your own key and call the API from section 2:

```c
#include "nrf_core.h"

enum custom_keycodes { BT_CLR_ALL = SAFE_RANGE };

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case BT_CLR_ALL:
            if (record->event.pressed) nrf_core_ble_clear_all();
            return false;
    }
    return true;
}
```

To give such a key a name in VIA, define it at keyboard level in the `QK_KB_0` range. Then list it under `customKeycodes` in the VIA definition, in the same order.

---

## 2. nrf API (`nrf_core.h`)

Add `#include "nrf_core.h"` to your keymap or keyboard `.c` file. You can call these functions from any keymap or keyboard code: `process_record_*`, housekeeping, indicators and so on.

Functions returning `int32_t` give `NRF_CORE_OK` (0) or a negative `NRF_CORE_E*` error: `EINVAL`, `EBUSY`, `ENOTCONN`, `ETIMEDOUT`, `ENOTSUP` or `EIO`.

### BLE connection

| Function | Action |
|----------|--------|
| `nrf_core_ble_profile_select(n)` | Select profile `n` (counted from **0**) and send over BLE. Keycode `BT_PRF1` is profile 0 |
| `nrf_core_ble_profile_next()` / `_prev()` | Next / previous profile |
| `nrf_core_ble_profile()` | Current profile (0…) |
| `nrf_core_ble_clear_current()` | Forget the host of the current profile, advertise again |
| `nrf_core_ble_clear_all()` | Forget the hosts of all profiles |
| `nrf_core_ble_connected()` | `true` once connected **and** encrypted, i.e. keystrokes reach the host |

### USB / BLE output

| Function | Action |
|----------|--------|
| `nrf_core_output_auto()` | USB while plugged in, wireless otherwise (default) |
| `nrf_core_output_usb()` / `nrf_core_output_ble()` | Force USB / wireless (the BLE profile or the dongle, whichever is selected) |
| `nrf_core_output_rf()` | Type to the 2.4 GHz dongle (pairs if no dongle is bonded) |
| `nrf_core_output_toggle()` | Toggle USB ↔ wireless |
| `nrf_core_output_is_usb()` | `true` if keystrokes currently go to USB |
| `nrf_core_output_is_rf()` | `true` if the wireless link is the dongle, not a BLE profile |

### Battery and charging

| Function | Action |
|----------|--------|
| `nrf_core_battery_level()` | Battery level 0…100 %, `0xFF` if not measured (see `NRF_BATTERY_*` in section 4) |
| `nrf_core_charge_state()` | `NRF_CORE_CHARGE_UNKNOWN` (no charger declared), `_DISCHARGING` (on battery), `_CHARGING`, `_FULL`, `_DISABLED` |
| `nrf_core_charging()` | `true` while charging |
| `nrf_core_charge_enable(on)` | Stop / resume charging (on by default) |

### Raw state and commands

| Function | Action |
|----------|--------|
| `nrf_core_conn_state()` | Bit mask of `NRF_CORE_CONN_*` flags: `BLE_ADVERTISING`, `BLE_CONNECTED`, `BLE_SECURED`, `USB_POWERED`, `USB_READY`, `USB_SUSPENDED`, `OUTPUT_USB`, `RF` (wireless link is the dongle). Bits 24…27 hold the profile |
| `nrf_core_command(cmd, arg)` | Send an `NRF_CORE_CMD_*` command directly (e.g. `NRF_CORE_CMD_BLE_DISCONNECT`) |

Example: turn all LEDs blue while waiting for a BLE host.

```c
bool rgb_matrix_indicators_user(void) {
    if (!nrf_core_output_is_usb() && !nrf_core_ble_connected()) {
        rgb_matrix_set_color_all(RGB_BLUE); // waiting for BLE
    }
    return false;
}
```

### Other functions (`nrf_core->...`)

A few more functions are reachable through the `nrf_core` pointer. QMK features already cover most of them, so keymaps rarely need them:

| Function | Notes |
|----------|-------|
| `nrf_core->log_write(str, len)` | Write a line to the debug log (USB serial console of debug builds, `dmesg`) |
| `nrf_core->reboot(NRF_CORE_REBOOT_NORMAL / _BOOTLOADER)` | Restart / enter the UF2 bootloader (what `QK_BOOT` does) |
| `nrf_core->device_id(id)` | 128-bit chip ID |
| `nrf_core->uptime_ms()` | Milliseconds since boot (QMK's `timer_read32()` is based on it) |
| `nrf_core->chip` | `NRF_CORE_CHIP_NRF52840` / `_NRF52833` |

Peripherals (GPIO, I2C, SPI, WS2812, ADC, PWM, EEPROM) use the standard QMK drivers, as on any other platform: `gpio_*`, `i2c_master.h`, `spi_master.h`, `analogReadPin()`, `ws2812`, `backlight`, `audio`, VIA / dynamic keymap. Pins are named `P0_xx` / `P1_xx`.

---

## 3. Platform hooks

These are weak functions declared in `nrf_core.h`; your keyboard or keymap can override them:

| Hook | Use |
|------|-----|
| `bool nrf_idle_allowed_kb(void)` / `_user` | Return `false` to keep scanning instead of going idle (e.g. while an RGB animation runs). Default: `true` |
| `void nrf_power_off_kb(void)` / `_user` | Called right before System OFF (after `NRF_POWER_OFF_TIMEOUT_MS`). Switch off external hardware here, e.g. the shutdown pin of an LED driver. A `_kb` version must call `nrf_power_off_user()` |
| `bool nrf_matrix_wake_prepare(void)` | Set up the matrix so a key press can wake the chip, then arm the wake pins (`nrf_core->gpio_wake_config`). The default handles COL2ROW / ROW2COL / DIRECT_PINS matrices and encoders. A custom matrix that cannot wake the chip should return `false`: the keyboard then keeps scanning and never goes idle or into System OFF |
| `void nrf_matrix_wake_finish(void)` | Put the matrix back into its normal scanning state |

Example (we6k): shut the IS31FL3731 down before System OFF.

```c
void nrf_power_off_kb(void) {
    gpio_write_pin_low(IS31FL3731_SDB_PIN);
    nrf_power_off_user();
}
```

---

## 4. Settings in `config.h`

### Hardware

| Define | Meaning |
|--------|---------|
| `NRF_LFCLK_CRYSTAL` | 32.768 kHz crystal fitted on P0.00/P0.01 (without it, the internal RC oscillator is used) |
| `NRF_POWER_DCDC_REG1` | Inductor on DCC: enable the REG1 DC/DC converter. Only declare it if the inductor is really there |
| `NRF_POWER_DCDC_REG0` | Inductor on DCCH: enable the REG0 DC/DC converter |
| `NRF_BATTERY_METHOD` | `NRF_BATTERY_NONE` (default), `_DIVIDER`, `_VDDH`, `_VDD` |
| `NRF_BATTERY_PIN`, `NRF_BATTERY_R_TOP`, `NRF_BATTERY_R_BOTTOM` | Voltage divider (required with `_DIVIDER`); the pin must be an analog (AIN) pin |
| `NRF_BATTERY_EN_PIN`, `NRF_BATTERY_EN_ACTIVE_LOW`, `NRF_BATTERY_SETTLE_US` | Optional pin that powers the divider only while measuring |
| `NRF_BATTERY_EMPTY_MV`, `NRF_BATTERY_FULL_MV` | Linear 0…100 % between these; default: LiPo curve 3.3–4.2 V |
| `NRF_CHARGER_EN_PIN`, `NRF_CHARGER_EN_ACTIVE_LOW` | Charger enable pin |
| `NRF_CHARGER_STAT_PIN`, `NRF_CHARGER_STAT_PULLUP` / `_PULLDOWN`, `NRF_CHARGER_STAT_CHARGING_LEVEL` | Charger status pin |

See `nrf_app_header.c` in this folder for examples and details.

### Scanning and power saving (defaults in `platforms/nrf/config.h`, override in your keyboard's `config.h`)

| Define | Default | Meaning |
|--------|---------|---------|
| `MATRIX_IO_DELAY` | 5 | Wait after each matrix line, for the others to rise back (µs; QMK's default is 30). Raise it if keys show up that were not pressed (ghost keys) |
| `NRF_SCAN_INTERVAL_MS` | 1 | Scan period while sending over USB (ms) |
| `NRF_SCAN_INTERVAL_BLE_MS` | 3 | Scan period while sending over BLE (ms) |
| `NRF_SCAN_INTERVAL_RF_MS` | 1 | Scan period while sending to the 2.4 GHz dongle (ms) |
| `NRF_BLE_SYNC_ENABLE` | 1 | Over BLE, scan right before each connection event (low latency, fewer wake-ups) |
| `NRF_BLE_SYNC_TIMEOUT_MS` | 16 | Scan anyway if no connection event was signalled for this long (ms) |
| `NRF_IDLE_TIMEOUT_MS` | 500 | Stop scanning and wait for a key interrupt after this long without matrix activity (ms; 0 = never) |
| `NRF_HELD_IDLE_TIMEOUT_MS` | 10000 | Go idle even with keys held if nothing changed for this long (ms) |
| `NRF_IDLE_WAKE_INTERVAL_MS` | 1000 | While idle, wake at least this often for QMK housekeeping (ms) |
| `NRF_POWER_OFF_TIMEOUT_MS` | 30 min | Enter System OFF after this long unused on battery (never while on USB power); a key press turns the keyboard back on (0 = never) |
| `EEPROM_SIZE` | 4096 | Emulated EEPROM size (VIA keymap, settings); must not exceed 4096 |
