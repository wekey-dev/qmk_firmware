# nRF52 targets running as an app image on top of the nRF Core.
# Select with `"processor": "nrf52840"` (or nrf52833) in keyboard.json,
# or `MCU = nrf52840` in rules.mk.

ifneq ($(findstring nrf52, $(MCU)),)
  # Deferred, per key (DEBOUNCE, default 5 ms): a change is reported once it
  # has been stable that long. Eager debouncing reports the first scan that
  # sees a change, so a single misread scan (a line caught mid-transition)
  # becomes a key press. Over BLE the matrix is scanned once per connection
  # event (7.5-15 ms apart, see platforms/nrf/nrf_power.c), so a key is
  # reported one connection interval later than with eager debouncing.
  # A keyboard can still choose another type.
  DEBOUNCE_TYPE ?= sym_defer_pk
  # Drivers backed by the core (platforms/nrf/drivers): WS2812 on PWM, battery
  # level measured by the core
  WS2812_DRIVER ?= vendor
  BATTERY_DRIVER ?= vendor
endif

ifneq ($(findstring nrf52840, $(MCU)),)
  PLATFORM_KEY := nrf
  PROTOCOL := NRF
  MCU_FAMILY := NRF52
  MCU_SERIES := NRF52840
  NRF_CHIP := 52840
  UF2_FAMILY := 0xADA52840
endif

ifneq ($(findstring nrf52833, $(MCU)),)
  PLATFORM_KEY := nrf
  PROTOCOL := NRF
  MCU_FAMILY := NRF52
  MCU_SERIES := NRF52833
  NRF_CHIP := 52833
  UF2_FAMILY := 0x621E937A
endif
