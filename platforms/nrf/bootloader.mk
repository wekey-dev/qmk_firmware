# The QMK image is not a standalone firmware: it is started by the nRF Core.
# QK_BOOT asks the core to reboot into the WeKey UF2 bootloader
# (nrf_bootloader, GPREGRET = 0x57); double-tapping reset does the same.
BOOTLOADER ?= nrf-core

ifeq ($(strip $(BOOTLOADER)), nrf-core)
    OPT_DEFS += -DBOOTLOADER_NRF_CORE
    BOOTLOADER_TYPE = nrf_core
endif

ifeq ($(strip $(BOOTLOADER)), custom)
    OPT_DEFS += -DBOOTLOADER_CUSTOM
    BOOTLOADER_TYPE = nrf_core
endif

# Output: UF2 for drag and drop onto the bootloader drive. The .hex (same
# content, for a debugger) stays in the build directory.
FIRMWARE_FORMAT ?= uf2
