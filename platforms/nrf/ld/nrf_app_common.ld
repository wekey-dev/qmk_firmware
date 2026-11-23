/*
 * Common section layout of a QMK app image for the nRF Core.
 * Included by nrf_app.ld after memory.ld (MEMORY, from the core SDK).
 *
 * Image layout in flash (starts at ORIGIN(APP_FLASH)):
 *   struct nrf_app_header   (.app_header, must be first)
 *   .text / .rodata / init arrays / exidx
 *   .data load image
 * __app_image_end marks the end of what the CRC covers.
 */
ENTRY(nrf_app_entry)

SECTIONS
{
    .text : ALIGN(4)
    {
        __app_flash_start = .;
        KEEP(*(.app_header))
        . = ALIGN(4);
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.glue_7) *(.glue_7t) *(.vfp11_veneer) *(.v4_bx)
        . = ALIGN(4);

        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN(__preinit_array_end = .);

        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN(__init_array_end = .);
        . = ALIGN(4);
    } > APP_FLASH

    .ARM.extab : ALIGN(4)
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > APP_FLASH

    .ARM.exidx : ALIGN(4)
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > APP_FLASH

    .data : ALIGN(4)
    {
        __data_start = .;
        *(.data .data.*)
        *(.ramtext .ramtext.*)
        . = ALIGN(4);
        __data_end = .;
    } > APP_RAM AT > APP_FLASH

    __data_load = LOADADDR(.data);
    __app_image_end = LOADADDR(.data) + SIZEOF(.data);

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > APP_RAM

    /* Everything left in APP_RAM is heap (malloc via _sbrk). The stack of the
     * QMK thread is owned by the core, so none is reserved here. */
    .heap (NOLOAD) : ALIGN(8)
    {
        __heap_start = .;
        end = .;
        _end = .;
    } > APP_RAM

    __heap_end = ORIGIN(APP_RAM) + LENGTH(APP_RAM);
    __app_ram_start = ORIGIN(APP_RAM);
    __app_ram_end = ORIGIN(APP_RAM) + LENGTH(APP_RAM);

    ASSERT(__heap_end - __heap_start >= 1024, "QMK app: less than 1 KB of heap left in APP_RAM")
}
