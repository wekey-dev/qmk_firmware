# Core SDK for QMK (generated)

Exported by the nRF Core on every build. It holds what the QMK app image is
built against, so `qmk_firmware_ble` builds without the core sources:

| File | Content |
|---|---|
| `nrf_core_api.h` | The ABI between core and QMK |
| `nrf_core_abi_hash.h` | `NRF_CORE_ABI_HASH`, hash of `nrf_core_api.h`. Stored in the QMK image header; the core starts only images built against the same header |
| `<soc>/memory.ld` | QMK flash and RAM regions, from the core's devicetree |
| `<soc>/sdk.mk` | Which core version / ABI hash this was exported from |

That is all `qmk_firmware_ble/platforms/nrf/sdk` contains: QMK carries no
prebuilt binaries. A QMK build gives the app image only (`<keyboard>.uf2`),
flashed onto a board that already runs a core with the same ABI hash.

The complete SDK of a core build (its `out/sdk`, also the CI artifact
`nrf-sdk`), distributed on its own, adds:

| File | Content |
|---|---|
| `<soc>/core/rc.bin`, `xtal.bin` (+ `.json`) | Prebuilt cores `nrf/<soc>` (RC oscillator) and `nrf/<soc>/xtal` (32.768 kHz crystal), with board, ABI hash, version and load address |
| `nrf52840/dongle/dongle.bin` (+ `.json`) | Prebuilt WeKey 2.4 GHz USB dongle (nRF52840 Dongle / E104-BT5040U) with an empty preset |
| `tools/nrf_app_full.py`, `tools/nrf_dongle.py` | The scripts that merge them with a QMK image |

The core's build merges them with the QMK images into `full/<keyboard>.uf2`
(the core a keyboard needs, chosen by `NRF_LFCLK_CRYSTAL`, + its QMK image:
first install) and, for keyboards with `NRF_DONGLE = yes` in `rules.mk`,
`dongle/<keyboard>.uf2` (the dongle with the keyboard's identity).

A QMK build can use a complete SDK instead of `platforms/nrf/sdk`: unpack it
anywhere and pass its folder as `NRF_CORE_SDK` (`make <kb>:<km>
NRF_CORE_SDK=<sdk>`, `qmk compile ... -e NRF_CORE_SDK=<sdk>`, or exported in
the environment). The build then uses its ABI headers and memory map, and
writes `<target>_full.uf2` and (with `NRF_DONGLE = yes`)
`<target>_dongle.uf2` next to `<target>.uf2`, with its tools.

**Do not edit.** It is replaced as a whole when the core exports a new SDK;
commit that in qmk_firmware_ble. Core and QMK firmware must come from SDKs
with the same ABI hash.
