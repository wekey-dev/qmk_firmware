/*
 * QMK app image linker script (both SoCs).
 *
 * memory.ld defines APP_FLASH / APP_RAM. It is generated from the
 * core's devicetree and comes with the core SDK
 * (sdk/<mcu>/memory.ld, found through -L), so the image
 * always matches the core's memory split.
 */
INCLUDE memory.ld
INCLUDE nrf_app_common.ld
