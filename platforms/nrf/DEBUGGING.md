# Debugging nrf keyboards over USB

nrf keyboards have a debug console: a virtual serial port that appears next to the keyboard when it is plugged in over USB. You only need the USB cable, with no debugger and no driver to install (Windows 10+, macOS and Linux support it out of the box). Through the console you can:

- read the log, including the end of the previous run after a crash or reset;
- check the state of BLE, USB, battery and charger;
- probe I2C devices and GPIO pins.

The console exists in **debug** firmware builds, which is what the prebuilt firmware in this repository is. Release builds leave it out.

## Opening the console

Use any serial terminal; the baud rate does not matter.

| OS | Port | Open with |
|----|------|-----------|
| macOS | `ls /dev/tty.usbmodem*` | `screen /dev/tty.usbmodemXXXX` (quit: `Ctrl-A`, `K`, `y`) |
| Linux | `/dev/ttyACM0` | `screen /dev/ttyACM0` or `picocom /dev/ttyACM0` (you may need to be in the `dialout` group) |
| Windows | Device Manager → Ports (COM & LPT) → `COMx` | PuTTY (connection type *Serial*, `COMx`) or the Arduino IDE Serial Monitor |

Press Enter to get the `nrf:~$` prompt. `help` lists the top-level commands (`nrf`, `dmesg`, `devmem`, `log`, `kernel`…); `nrf` on its own lists all of its subcommands with their syntax (also `nrf <command> -h`). `Tab` completes commands.

New log lines appear in the terminal as they happen while the console is open.

## Commands

| Command | What it shows / does |
|---------|----------------------|
| `dmesg` | The whole log since boot, **plus the end of the previous run** if the keyboard just restarted (crash, watchdog, reset, `QK_BOOT`). Lost when power is fully removed (USB and battery) and after System OFF. `dmesg -c` clears it |
| `nrf status` | Firmware, whether the keymap is running or in safe mode (crash count), VID/PID, BLE, USB, output, battery, charger |
| `nrf ble` | Current profile or the dongle, paired hosts per profile and the dongle, the connected host or dongle, connection parameters (interval, `(LLPM)` at 1 ms, PHY 1M / 2M, latency, MTU), the dongle bond and how the keyboard advertises for it (directed / waiting to pair), connections and disconnections with the last reason, how often peripheral latency was turned off for VIA, scan timing (reports sent on time or late). `nrf ble clear` resets the counters |
| `nrf usb` | USB state; count and last time of each event (VBUS, suspend, resume, bus reset…); unacknowledged / failed reports |
| `nrf hid` | Output (USB or BLE, automatic or forced), report queue (sent, dropped, highest depth), reports sent / failed per transport, raw HID (VIA): packets, how fast the keyboard answers and how long the host takes for the next request (which side makes VIA slow) |
| `nrf battery` | How the battery is measured, the last reading (pin and battery voltage, %), power source. `nrf battery read` measures now |
| `nrf charger` | Charger enable / status pins, the level read, charge state |
| `nrf bench 3 6 17` | Time the GPIO calls a matrix scan makes, on an **unused** pin (here P0.03), and estimate one scan of a 6 × 17 matrix |
| `nrf i2c scan` | Probe addresses 0x08–0x77: the address if a device answers, `--` if none, `TO` if the bus is stuck (timeout) |
| `nrf i2c dump` | SCL/SDA pin levels, I2C controller registers and whether the bus is marked as faulty |
| `nrf i2c read 0x74 0x00 4` | Read 4 bytes from register 0x00 of device 0x74 |
| `nrf i2c write 0x74 0xFD 0x0B` | Write: first byte is the register, then the data |
| `nrf i2c init 26 6` | Set up I2C on SCL/SDA pins (number = port × 32 + pin, e.g. P0.26 = 26) if the keyboard has not |
| `nrf gpio 36` | Level of one pin (number = port × 32 + pin: P1.04 = 36) |
| `nrf wdt` | Hardware watchdog: timeout, feed count, whether the last reset came from it. `nrf wdt test` hangs the chip on purpose: it must reset after about 8 s |
| `nrf bootloader` (or `nrf dfu`) | Restart into the UF2 bootloader (the `WEKEY840` / `WEKEY833` drive), no key needed |
| `devmem 0x400034C4` | Read any register (here the I2C error source) |
| `kernel uptime`, `kernel reboot cold` | Time since boot, restart |

## Printing from your keymap

Anything QMK prints goes to the same log, so it shows up live in the console and in `dmesg`:

- `rules.mk`: `CONSOLE_ENABLE = yes`, then use `uprintf("...")`. For `dprintf("...")`, also turn on `debug_enable = true` (e.g. in `keyboard_post_init_user()`).
- Without `CONSOLE_ENABLE`, write a line directly:

```c
#include "nrf_core.h"

static const char msg[] = "caps lock on\n";
nrf_core->log_write(msg, sizeof(msg) - 1);
```

QMK Toolbox / `qmk console` are not needed and do not see this log; use the serial console instead.

## Common situations

| Symptom | What to look at |
|---------|-----------------|
| Keyboard stopped typing after a crash, but the console still works | `nrf status` shows **safe mode**: the keymap crashed 3 times in a row and is no longer started. `dmesg` shows why (`QMK app fault` or `QMK app not responding`). Fix the keymap and flash it (double-tap reset). A plain reset does not clear safe mode; flashing or fully removing power (USB **and** battery) does |
| Keyboard restarted by itself | `dmesg`: the end of the previous run is still there. `nrf wdt` tells you whether the watchdog reset it |
| LEDs / OLED / other I2C device dead | `nrf i2c scan` (the device's address must answer), then `nrf i2c dump`. A device that does not answer does not slow typing down: failed I2C transfers are retried every 1–8 s |
| BLE drops or does not connect | `dmesg` around the `connected` / `disconnected (0x..)` lines, and `nrf ble`. `security failed` after forgetting a host on one side: forget the keyboard on the computer / phone too and pair again |
| Keys lag or go missing over BLE | `nrf ble`: count of late reports; `nrf hid`: dropped reports |
| Dongle does not pair or is slow | Keyboard: `nrf ble` (`dongle no bond, advertising to pair` while waiting; once connected `interval 1000 us (LLPM), PHY 2M`). Dongle console: `nrf link`, and its log: `too far` (bring the keyboard within ~1 m), `this dongle has one` (hold its button 3 s), `this dongle is for ... only` (a dongle built with `NRF_DONGLE_LOCK` for another keyboard) |
| VIA lists the keyboard over the dongle but does not react | The keyboard is off or away: the dongle removes its VIA (raw HID) interface after 5 s without a keyboard (dongle `nrf status`), so VIA stops listing it; it comes back when the keyboard connects |
| BLE host lost after choosing USB | Expected: `OU_USB` leaves the wireless link after 5 s (`nrf ble`: `wireless parked`). Pick the profile or the dongle again to reconnect |
| Typing over USB stops for a moment | `nrf usb`: count and time of bus resets and suspends |
| Battery % looks wrong | `nrf battery read` (voltage at the pin and at the battery); check the divider values in `config.h` |
| Not charging | `nrf charger`; `NRF_CHARGER_*` pins in `config.h` |

## Reporting a problem

Paste the full `dmesg` output and the output of `nrf status` together with the commands that match the problem (`nrf ble`, `nrf usb`, `nrf i2c dump`…). Say what you did just before it happened.
