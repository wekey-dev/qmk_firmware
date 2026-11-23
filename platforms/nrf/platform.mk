# Hey Emacs, this is a -*- makefile -*-
##############################################################################
# nRF52 app-image platform (runs on top of the nRF Core)
#
# The resulting image is NOT a standalone firmware. It has no vector table and
# no interrupt handlers; it is linked at the start of the "qmk" flash partition
# and entered by the core through struct nrf_core_api.
#
# Everything needed from the core comes from the core SDK in this
# repository (sdk/, exported by the core's build and
# committed here), so QMK builds on its own:
#   nrf_core_api.h, nrf_core_abi_hash.h       ABI and its hash
#   <mcu>/memory.ld                  QMK flash/RAM regions
#   <mcu>/sdk.mk                     export info
##############################################################################

NRF_PLATFORM_DIR := $(PLATFORM_PATH)/$(PLATFORM_KEY)
NRF_CORE_SDK ?= $(NRF_PLATFORM_DIR)/sdk
NRF_CORE_SDK_MK := $(NRF_CORE_SDK)/$(MCU)/sdk.mk

ifeq ("$(wildcard $(NRF_CORE_SDK_MK))","")
    $(call CATASTROPHIC_ERROR,Missing core SDK,No $(NRF_CORE_SDK_MK): export the core SDK into platforms/nrf/sdk and commit it)
endif
include $(NRF_CORE_SDK_MK)

EXTRAINCDIRS += $(NRF_CORE_SDK)

SRC += $(NRF_PLATFORM_DIR)/nrf_app.c \
       $(NRF_PLATFORM_DIR)/nrf_app_header.c \
       $(NRF_PLATFORM_DIR)/nrf_power.c \
       $(NRF_PLATFORM_DIR)/nrf_keycodes.c

##############################################################################
# Toolchain
##############################################################################
TOOLCHAIN ?= arm-none-eabi-

CC      = $(CC_PREFIX) $(TOOLCHAIN)gcc
OBJCOPY = $(TOOLCHAIN)objcopy
OBJDUMP = $(TOOLCHAIN)objdump
SIZE    = $(TOOLCHAIN)size
AR      = $(TOOLCHAIN)ar
NM      = $(TOOLCHAIN)nm
HEX     = $(OBJCOPY) -O $(FORMAT)
EEP     =
BIN     = $(OBJCOPY) -O binary

# Cortex-M4F, hard float. The ABI never passes float values, so this does not
# need to match the core's float ABI; the core enables the FPU and saves its
# context (CONFIG_FPU_SHARING).
MCUFLAGS = -mcpu=cortex-m4 \
           -mthumb \
           -mfloat-abi=hard \
           -mfpu=fpv4-sp-d16 \
           -fsingle-precision-constant

# nrf_app.ld includes memory.ld (from the SDK, per MCU) and
# nrf_app_common.ld (sections), found through -L.
LDSCRIPT = $(NRF_PLATFORM_DIR)/ld/nrf_app.ld

COMPILEFLAGS += $(MCUFLAGS) \
                -fomit-frame-pointer \
                -ffunction-sections \
                -fdata-sections \
                -fno-common \
                -fshort-wchar \
                -fno-builtin-printf

CFLAGS   += $(COMPILEFLAGS)
CXXFLAGS += $(COMPILEFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  += $(MCUFLAGS)

LDFLAGS += $(MCUFLAGS) \
           -T $(LDSCRIPT) \
           -L $(NRF_CORE_SDK)/$(MCU) \
           -L $(NRF_PLATFORM_DIR)/ld \
           -Wl,--gc-sections \
           -Wl,--no-wchar-size-warning \
           -nostartfiles \
           --specs=nano.specs \
           --specs=nosys.specs

ifeq ("$(shell echo "int main(){}" | $(CC) -shared -Wl,--no-warn-rwx-segments -x c - -o /dev/null 2>&1)", "")
    LDFLAGS += -Wl,--no-warn-rwx-segments
endif

OPT_DEFS += -DPROTOCOL_NRF \
            -DMCU_NRF52 \
            -DNRF_CHIP=$(NRF_CHIP)

CONFIG_H += $(NRF_PLATFORM_DIR)/config.h

##############################################################################
# Outputs
#   .elf  - for debugging (add-symbol-file in GDB together with the core ELF)
#   .bin  - raw image starting at the app header, CRC patched in
#   .hex  - same content at the partition address, for nrfjprog / nrfutil
#   .uf2  - same content for the WeKey UF2 bootloader (drag and drop onto the
#           WEKEY840 / WEKEY833 drive); FIRMWARE_FORMAT, copied to the QMK root
#   .nrf.json - the keyboard's dongle settings for the core's build
#           (NRF_DONGLE / NRF_DONGLE_LOCK in rules.mk)
#
# Prebuilt cores and the dongle are not part of this repository (the core SDK
# here holds only the ABI headers and the memory map). The core's build merges
# them with this image into the full image (core + QMK, first install) and the
# dongle made for this keyboard; built on its own, QMK gives the app image only,
# flashed after a core.
#
# NRF_CORE_SDK=<sdk> (make variable or environment) builds against a complete
# core SDK obtained from the core instead (prebuilt cores, dongle, tools/):
# its ABI headers and memory map are used, and its tools add
#   _full.uf2 / _full.hex - the matching prebuilt core + this image
#   _dongle.uf2 / _dongle.hex - with NRF_DONGLE = yes: the dongle made for this
#           keyboard (NRF_DONGLE_LOCK = yes: it pairs no other keyboard)
# copied next to <target>.uf2 like before.
##############################################################################
NRF_APP_PACK = $(NRF_PLATFORM_DIR)/nrf_app_pack.py
NRF_PYTHON ?= python3

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	@$(SILENT) || printf "$(MSG_FLASH) $@" | $(AWK_CMD)
	$(eval CMD=$(BIN) $< $(BUILD_DIR)/$(TARGET).raw.bin && $(NRF_PYTHON) $(NRF_APP_PACK) $(BUILD_DIR)/$(TARGET).raw.bin --bin $(BUILD_DIR)/$(TARGET).bin --hex $@)
	@$(BUILD_CMD)

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).hex
	@true

# Explicit rule: QMK's generic %.uf2 rule converts the ELF, which lacks the
# image size and CRC that nrf_app_pack.py writes into the header.
NRF_DONGLE_JSON = {"dongle": $(if $(filter yes,$(strip $(NRF_DONGLE))),true,false), "dongle_lock": $(if $(filter yes,$(strip $(NRF_DONGLE_LOCK))),true,false)}

$(BUILD_DIR)/$(TARGET).uf2: $(BUILD_DIR)/$(TARGET).hex
	@$(SILENT) || printf "Creating UF2 file for the WeKey bootloader: $@" | $(AWK_CMD)
	$(eval CMD=$(NRF_PYTHON) $(UF2CONV) $< --convert --family $(UF2_FAMILY) --output $@ >/dev/null)
	@$(BUILD_CMD)

# On every build (rules.mk or NRF_CORE_SDK may change without the image
# changing): the dongle settings for the core's build, and with a complete core
# SDK (tools/ in it) the full and dongle images, copied next to <target>.uf2.
NRF_SDK_TOOLS := $(NRF_CORE_SDK)/tools
NRF_FULL = $(BUILD_DIR)/$(TARGET)_full
NRF_DONGLE_IMG = $(BUILD_DIR)/$(TARGET)_dongle

cpfirmware: cpfirmware_nrf
cpfirmware_nrf: cpfirmware_qmk
	@echo '$(NRF_DONGLE_JSON)' > $(BUILD_DIR)/$(TARGET).nrf.json
	@rm -f $(NRF_FULL).hex $(NRF_FULL).uf2 $(NRF_DONGLE_IMG).hex $(NRF_DONGLE_IMG).uf2 \
		$(TARGET)_full.uf2 $(TARGET)_dongle.uf2
ifneq ($(wildcard $(NRF_SDK_TOOLS)/nrf_app_full.py),)
	@$(NRF_PYTHON) -B $(NRF_SDK_TOOLS)/nrf_app_full.py $(BUILD_DIR)/$(TARGET).hex \
		--sdk $(NRF_CORE_SDK) --out $(NRF_FULL)
	@if [ -f $(NRF_FULL).uf2 ]; then \
		$(SILENT) || printf "Copying $(TARGET)_full.uf2 (core + QMK) to qmk_firmware folder" | $(AWK_CMD); \
		$(COPY) $(NRF_FULL).uf2 $(TARGET)_full.uf2 && $(PRINT_OK); \
	fi
endif
ifeq ($(strip $(NRF_DONGLE)), yes)
ifneq ($(wildcard $(NRF_SDK_TOOLS)/nrf_dongle.py),)
	@$(NRF_PYTHON) -B $(NRF_SDK_TOOLS)/nrf_dongle.py $(BUILD_DIR)/$(TARGET).hex \
		--sdk $(NRF_CORE_SDK) --out $(NRF_DONGLE_IMG) \
		$(if $(filter yes,$(strip $(NRF_DONGLE_LOCK))),--lock)
	@if [ -f $(NRF_DONGLE_IMG).uf2 ]; then \
		$(SILENT) || printf "Copying $(TARGET)_dongle.uf2 (USB dongle) to qmk_firmware folder" | $(AWK_CMD); \
		$(COPY) $(NRF_DONGLE_IMG).uf2 $(TARGET)_dongle.uf2 && $(PRINT_OK); \
	fi
endif
endif
.PHONY: cpfirmware_nrf

bin: $(BUILD_DIR)/$(TARGET).bin sizeafter
	$(COPY) $(BUILD_DIR)/$(TARGET).bin $(TARGET).bin;

DEBUG = gdb
