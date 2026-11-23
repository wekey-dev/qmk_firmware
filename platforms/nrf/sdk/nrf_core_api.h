/*
 * nrf_core_api.h - ABI between the nRF Core (Zephyr/NCS) and the QMK app image.
 *
 * This header is the single source of truth for both sides:
 *   - the core includes it directly.
 *   - qmk_firmware_ble gets a copy through the QMK SDK the core exports on every
 *     build (cmake/qmk_sdk.cmake -> qmk_firmware_ble/platforms/nrf/sdk),
 *     together with NRF_CORE_ABI_HASH, a hash of this very file.
 *
 * Contents:
 *   1. common constants (chips, pins, return codes)
 *   2. app image header: what the QMK image declares to the core
 *   3. constants used by the API functions
 *   4. struct nrf_core_api: what the core offers to the QMK image
 *   5. compile-time layout checks
 *
 * Usage rules (always):
 *   - Only fixed-width integer types, pointers and function pointers cross
 *     the boundary. No enums (QMK builds with -fshort-enums, Zephyr does not),
 *     no float/double, no bitfields, no structs passed by value, no implicit
 *     padding (add an explicit `reserved` member instead).
 *   - API functions are called from the QMK thread only (the QMK image has no
 *     ISRs). irq_lock()/irq_unlock() are its only way to create a critical
 *     section: cpsid/PRIMASK would block the BLE controller's zero-latency
 *     interrupts.
 *
 * Versioning: NOT RELEASED YET. Until the first keyboard runs this firmware
 * the ABI stays 1.0 and may change freely. Core and QMK image must come from
 * the same version of this header: the image header carries NRF_CORE_ABI_HASH
 * and the core refuses an image with a different hash. From the first release
 * on:
 *   - new API functions / header fields are appended at the end and bump
 *     NRF_CORE_ABI_MINOR; users check api->size / header_size first;
 *   - any incompatible change bumps NRF_CORE_ABI_MAJOR (the core refuses an
 *     app image with a different major version).
 */
#ifndef NRF_CORE_API_H_
#define NRF_CORE_API_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_CORE_ABI_MAJOR 1
#define NRF_CORE_ABI_MINOR 0

/* ========================================================================= */
/* 1. Common constants                                                        */
/* ========================================================================= */

/* Chip identifiers (header.chip, api->chip) */
#define NRF_CORE_CHIP_NRF52833 52833u
#define NRF_CORE_CHIP_NRF52840 52840u

/* Pins are numbered port * 32 + pin: P0.13 = 13, P1.04 = 36 (QMK: P0_13, P1_04). */
#define NRF_CORE_NO_PIN 0xFFu

/* Return codes: negative errno values (newlib / Zephyr numbering) */
#define NRF_CORE_OK 0
#define NRF_CORE_EBUSY (-16)
#define NRF_CORE_EINVAL (-22)
#define NRF_CORE_ENOTCONN (-128)
#define NRF_CORE_ETIMEDOUT (-116)
#define NRF_CORE_ENOTSUP (-134)
#define NRF_CORE_EIO (-5)      /* bus error: NACK, arbitration lost... */

/* ========================================================================= */
/* 2. App image header (at the very beginning of the QMK partition)           */
/* ========================================================================= */

#define NRF_APP_MAGIC 0x414B4D51u /* "QMKA" little endian */

/* Maximum length of identity strings, including the terminating NUL */
#define NRF_APP_STR_MAX 32u

/*
 * Device identity, from the keyboard's keyboard.json (usb.vid, usb.pid,
 * usb.device_version, manufacturer, keyboard_name). The core presents the
 * keyboard with these values on USB (device descriptor, strings) and BLE (GAP
 * name, advertising, Device Information Service). 0 / NULL / "" = not
 * declared: the core uses its Kconfig default (CONFIG_NRF_CORE_DEFAULT_*).
 */
struct nrf_app_identity {
    uint16_t    vid;
    uint16_t    pid;
    uint16_t    device_ver;   /* BCD, e.g. 0x0102 = 1.0.2 */
    uint16_t    reserved;     /* 0 */
    const char *manufacturer; /* NUL terminated, inside the image, < NRF_APP_STR_MAX */
    char        product[NRF_APP_STR_MAX]; /* NUL terminated unless full */
};

/*
 * Battery measurement, from the keyboard's config.h (NRF_BATTERY_METHOD and
 * friends, see qmk_firmware_ble/platforms/nrf/nrf_app_header.c). The core sets up the
 * SAADC accordingly, measures periodically and publishes the level (Battery
 * Service, api->battery_level()).
 */
#define NRF_BATTERY_NONE 0u    /* not measured: level unknown (0xFF) */
#define NRF_BATTERY_DIVIDER 1u /* resistor divider into an analog pin (AIN0-7) */
#define NRF_BATTERY_VDDH 2u    /* battery on VDDH (high voltage mode): internal VDDH/5 */
#define NRF_BATTERY_VDD 3u     /* battery directly on VDD (coin cell, 2xAA...) */

struct nrf_app_battery {
    uint8_t  method;        /* NRF_BATTERY_* */
    uint8_t  pin;           /* DIVIDER: divider output, must be an AIN pin */
    uint8_t  en_pin;        /* DIVIDER: switches the divider on while sampling, or NRF_CORE_NO_PIN */
    uint8_t  en_active_low; /* DIVIDER: 1 = en_pin is driven low to enable */
    uint32_t r_top_ohms;    /* DIVIDER: battery -> pin */
    uint32_t r_bottom_ohms; /* DIVIDER: pin -> GND */
    uint16_t settle_us;     /* DIVIDER with en_pin: wait between enabling and sampling */
    uint16_t empty_mv;      /* level 0 %   } both 0: built-in LiPo curve,      */
    uint16_t full_mv;       /* level 100 % } otherwise linear between the two */
    uint16_t reserved;      /* 0 */
};

/*
 * Power and clock hardware of the keyboard, from its config.h
 * (NRF_LFCLK_CRYSTAL, NRF_POWER_DCDC_REG1, NRF_POWER_DCDC_REG0).
 *   lfclk_xtal  selects the core build: a 32.768 kHz crystal needs the core
 *               variant "xtal" (nrf/<soc>/xtal), otherwise nrf/<soc> (RC).
 *               The clock source is fixed when the core is built (the BLE
 *               controller starts with it); the core logs a mismatch.
 *   dcdc_reg1/0 the core switches the regulator to DC/DC at boot. Only with
 *               the inductor fitted (REG1: DCC/DEC4, REG0: DCCH), otherwise
 *               the chip browns out (recover with the bootloader: double reset).
 */
struct nrf_app_power {
    uint8_t lfclk_xtal; /* 1 = 32.768 kHz crystal on P0.00/P0.01 */
    uint8_t dcdc_reg1;  /* 1 = enable DC/DC on REG1 (main 1.3 V regulator) */
    uint8_t dcdc_reg0;  /* 1 = enable DC/DC on REG0 (VDDH input stage) */
    uint8_t reserved;   /* 0 */
};

/*
 * Battery charger, from the keyboard's config.h (NRF_CHARGER_*). The core
 * keeps charging enabled (also in safe mode and System OFF), tracks the
 * charge state and publishes it (api->charge_state(), BAS Battery Level
 * Status). "External power" is USB VBUS, which also powers the charger.
 *   en_pin    enables charging: driven to its active level to charge, left
 *             floating (disconnected) to stop. E.g. MCP73831 PROG through
 *             the program resistor: active low, floating = charge disabled.
 *   stat_pin  read while USB is powered: the level given by
 *             NRF_CHARGER_FLAG_STAT_CHARGING_HIGH (1) or not (0) means charging,
 *             the other level charge complete.
 */
#define NRF_CHARGER_FLAG_EN_ACTIVE_LOW 0x01u      /* en_flags */
#define NRF_CHARGER_FLAG_STAT_PULLUP 0x01u         /* stat_flags: enable the pin's pull-up */
#define NRF_CHARGER_FLAG_STAT_PULLDOWN 0x02u       /* stat_flags: enable the pin's pull-down */
#define NRF_CHARGER_FLAG_STAT_CHARGING_HIGH 0x04u  /* stat_flags: high level = charging */

struct nrf_app_charger {
    uint8_t en_pin;     /* NRF_CORE_NO_PIN if charging cannot be controlled */
    uint8_t en_flags;   /* NRF_CHARGER_FLAG_EN_* */
    uint8_t stat_pin;   /* NRF_CORE_NO_PIN if the state cannot be read */
    uint8_t stat_flags; /* NRF_CHARGER_FLAG_STAT_* */
};

struct nrf_core_api;
typedef void (*nrf_app_entry_t)(const struct nrf_core_api *api);

struct nrf_app_header {
    /* --- image (checked by the core before starting the app) --- */
    uint32_t        magic;       /* NRF_APP_MAGIC */
    uint16_t        abi_major;   /* NRF_CORE_ABI_MAJOR the app was built against */
    uint16_t        abi_minor;   /* NRF_CORE_ABI_MINOR the app was built against */
    uint32_t        header_size; /* sizeof(struct nrf_app_header) */
    uint32_t        chip;        /* NRF_CORE_CHIP_* the app was built for */
    uint32_t        load_addr;   /* flash address the image is linked at (== header address) */
    uint32_t        image_size;  /* bytes from load_addr to end of initialised data */
    uint32_t        ram_start;   /* RAM region used by the app (.data/.bss/heap) */
    uint32_t        ram_end;     /* exclusive */
    nrf_app_entry_t entry;       /* called once, in the QMK thread, never returns */
    uint32_t        crc32;       /* CRC-32/ISO-HDLC over [load_addr + header_size, load_addr + image_size) */
    uint32_t        abi_hash;    /* NRF_CORE_ABI_HASH (nrf_core_abi_hash.h) the app was built with */

    /* --- keyboard description (used by the core before the app starts) --- */
    struct nrf_app_identity identity;
    struct nrf_app_battery  battery;
    struct nrf_app_power    power;
    struct nrf_app_charger  charger;
};

/* Offsets used by the post-build packer (platforms/nrf/nrf_app_pack.py) */
#define NRF_APP_HEADER_OFF_HEADER_SIZE 8u
#define NRF_APP_HEADER_OFF_LOAD_ADDR 16u
#define NRF_APP_HEADER_OFF_IMAGE_SIZE 20u
#define NRF_APP_HEADER_OFF_CRC32 36u
#define NRF_APP_HEADER_SIZE 116u

/* ========================================================================= */
/* 3. Constants used by the API functions                                     */
/* ========================================================================= */

/* gpio_config() flags (combine) */
#define NRF_CORE_GPIO_DISCONNECT 0x0000u
#define NRF_CORE_GPIO_INPUT 0x0001u
#define NRF_CORE_GPIO_OUTPUT 0x0002u
#define NRF_CORE_GPIO_PULL_UP 0x0010u
#define NRF_CORE_GPIO_PULL_DOWN 0x0020u
#define NRF_CORE_GPIO_OPEN_DRAIN 0x0040u
#define NRF_CORE_GPIO_INIT_HIGH 0x0100u
#define NRF_CORE_GPIO_INIT_LOW 0x0200u

/* gpio_wake_config() modes. A triggered wake pin is automatically disabled by the core. */
#define NRF_CORE_WAKE_DISABLE 0u
#define NRF_CORE_WAKE_LEVEL_LOW 1u
#define NRF_CORE_WAKE_LEVEL_HIGH 2u

/* wait_event() result bits */
#define NRF_CORE_EVT_TIMEOUT 0x00000000u
#define NRF_CORE_EVT_GPIO_WAKE 0x00000001u /* an armed wake pin triggered */
#define NRF_CORE_EVT_HOST_LEDS 0x00000002u /* host LED state (or active transport) changed */
#define NRF_CORE_EVT_CONN 0x00000004u      /* conn_state() changed */
#define NRF_CORE_EVT_RAW_HID 0x00000008u   /* raw HID packet waiting (hid_raw_recv) */
#define NRF_CORE_EVT_BLE_SYNC 0x00000010u  /* BLE connection event soon (NRF_CORE_CMD_BLE_SYNC) */
#define NRF_CORE_EVT_BATTERY 0x00000020u   /* battery_level() or charge_state() changed */

/* charge_state() values */
#define NRF_CORE_CHARGE_UNKNOWN 0u     /* no charger declared */
#define NRF_CORE_CHARGE_DISCHARGING 1u /* no external power: running on the battery */
#define NRF_CORE_CHARGE_CHARGING 2u    /* external power, charging */
#define NRF_CORE_CHARGE_FULL 3u        /* external power, charge complete */
#define NRF_CORE_CHARGE_DISABLED 4u    /* external power, charging switched off (command) */

/* hid_send() report types */
#define NRF_CORE_HID_KEYBOARD 1u /* 8 bytes: mods, reserved, keys[6] */
#define NRF_CORE_HID_MOUSE 2u    /* struct nrf_core_mouse_report (7 bytes) */
#define NRF_CORE_HID_SYSTEM 3u   /* 2 bytes: usage (LE) */
#define NRF_CORE_HID_CONSUMER 4u /* 2 bytes: usage (LE) */
#define NRF_CORE_HID_NKRO 6u     /* 1 + NRF_CORE_NKRO_BYTES bytes: mods, bitmap */
#define NRF_CORE_HID_RAW 0x20u   /* NRF_CORE_RAW_HID_SIZE bytes (VIA / raw HID): to the transport of the last request */

#define NRF_CORE_NKRO_BYTES 30u
#define NRF_CORE_RAW_HID_SIZE 32u

struct nrf_core_mouse_report {
    uint8_t buttons;
    int16_t x;
    int16_t y;
    int8_t  v; /* wheel */
    int8_t  h; /* pan */
} __attribute__((packed));

/* conn_state() bits */
#define NRF_CORE_CONN_BLE_ADVERTISING 0x0001u
#define NRF_CORE_CONN_BLE_CONNECTED 0x0002u
#define NRF_CORE_CONN_BLE_SECURED 0x0004u /* encrypted: reports are delivered */
#define NRF_CORE_CONN_USB_POWERED 0x0010u
#define NRF_CORE_CONN_USB_READY 0x0020u
#define NRF_CORE_CONN_USB_SUSPENDED 0x0040u
#define NRF_CORE_CONN_OUTPUT_USB 0x0100u /* reports currently go to USB (else the wireless link) */
/* The wireless link is the 2.4 GHz receiver (USB dongle, NRF_CORE_CMD_OUTPUT_RF),
 * not a BLE profile; BLE_ADVERTISING / _CONNECTED / _SECURED describe that link. */
#define NRF_CORE_CONN_RF 0x0200u
#define NRF_CORE_CONN_PROFILE_SHIFT 24u  /* bits 24..27: active BLE profile */
#define NRF_CORE_CONN_PROFILE_MASK 0x0F000000u

/* command() ids: BLE */
#define NRF_CORE_CMD_BLE_PROFILE_SELECT 1u /* arg = profile index */
#define NRF_CORE_CMD_BLE_PROFILE_NEXT 2u
#define NRF_CORE_CMD_BLE_PROFILE_PREV 3u
#define NRF_CORE_CMD_BLE_CLEAR_CURRENT 4u /* forget bond of current profile (or the dongle), re-advertise */
#define NRF_CORE_CMD_BLE_CLEAR_ALL 5u     /* all BLE profiles; the dongle bond stays */
#define NRF_CORE_CMD_BLE_ADV_START 6u
#define NRF_CORE_CMD_BLE_DISCONNECT 7u
/* arg 1 = post NRF_CORE_EVT_BLE_SYNC shortly before every connection event,
 * arg 0 = stop. NRF_CORE_ENOTSUP if the core cannot (built without
 * CONFIG_NRF_CORE_BLE_SYNC, or registration failed): keep timed scanning. */
#define NRF_CORE_CMD_BLE_SYNC 8u
/* command() ids: output selection */
#define NRF_CORE_CMD_OUTPUT_AUTO 10u /* USB when ready, otherwise BLE */
#define NRF_CORE_CMD_OUTPUT_USB 11u
#define NRF_CORE_CMD_OUTPUT_BLE 12u
#define NRF_CORE_CMD_OUTPUT_TOGGLE 13u
/* Type to the 2.4 GHz receiver (USB dongle), also with USB plugged in, like a
 * BLE profile. Without a dongle bond the keyboard waits for a dongle to pair
 * (NRF_CORE_CMD_BLE_CLEAR_CURRENT forgets it). BLE profile commands switch
 * back to the BLE hosts. NRF_CORE_ENOTSUP if the core has no dongle support. */
#define NRF_CORE_CMD_OUTPUT_RF 14u
/* command() ids: power */
#define NRF_CORE_CMD_CHARGE_ENABLE 20u /* arg 1 = charge (default), 0 = stop charging */

/* spi_config() mode (CPOL/CPHA) */
#define NRF_CORE_SPI_MODE_0 0u
#define NRF_CORE_SPI_MODE_1 1u
#define NRF_CORE_SPI_MODE_2 2u
#define NRF_CORE_SPI_MODE_3 3u
#define NRF_CORE_SPI_LSB_FIRST 0x10u /* spi_config() flag: least significant bit first */

/* PWM: channel = generator * NRF_CORE_PWM_CHANNELS_PER_GEN + output */
#define NRF_CORE_PWM_GENERATORS 2u
#define NRF_CORE_PWM_CHANNELS_PER_GEN 4u
#define NRF_CORE_PWM_GEN_BACKLIGHT 0u /* convention: channels 0-3 */
#define NRF_CORE_PWM_GEN_AUDIO 1u     /* convention: channels 4-7 */
#define NRF_CORE_PWM_CHANNEL(gen, out) ((gen) * NRF_CORE_PWM_CHANNELS_PER_GEN + (out))
#define NRF_CORE_PWM_ACTIVE_LOW 0x01u /* flag: output low while "on" (duty counts low time) */

/* reboot() modes */
#define NRF_CORE_REBOOT_NORMAL 0u
#define NRF_CORE_REBOOT_BOOTLOADER 1u

/* ========================================================================= */
/* 4. API table passed to the app entry point                                 */
/* ========================================================================= */

struct nrf_core_api {
    uint32_t size; /* sizeof(struct nrf_core_api) as built by the core */
    uint16_t abi_major;
    uint16_t abi_minor;
    uint32_t chip;        /* NRF_CORE_CHIP_* */
    uint32_t eeprom_size; /* bytes available through eeprom_read/write */

    /* --- time --- */
    uint32_t (*uptime_ms)(void);
    void (*sleep_ms)(uint32_t ms);     /* yields the CPU (low power) */
    void (*busy_wait_us)(uint32_t us); /* spins, does not yield */
    /* Sleep until uptime_ms() reaches deadline_ms (wrap-safe, returns at once if
     * it already has): a fixed scan rate that does not drift with scan time. */
    void (*sleep_until_ms)(uint32_t deadline_ms);

    /* --- critical sections --- */
    uint32_t (*irq_lock)(void);
    void (*irq_unlock)(uint32_t key);

    /* --- gpio --- */
    int32_t (*gpio_config)(uint32_t pin, uint32_t flags);
    void (*gpio_write)(uint32_t pin, uint32_t level);
    uint32_t (*gpio_read)(uint32_t pin);
    void (*gpio_toggle)(uint32_t pin);
    int32_t (*gpio_wake_config)(uint32_t pin, uint32_t mode);

    /* --- power --- */
    /* Block until an event (NRF_CORE_EVT_*) or timeout. Returns and clears the pending event bits. */
    uint32_t (*wait_event)(uint32_t timeout_ms);
    /* System OFF. Configure wake pins first. Does not return (wakes through reset). */
    void (*power_off)(void);
    void (*reboot)(uint32_t mode);
    uint32_t (*battery_level)(void); /* 0..100, or 0xFF if unknown */
    uint32_t (*charge_state)(void);  /* NRF_CORE_CHARGE_* */

    /* --- HID --- */
    /* Queue a report (copied) and return at once: transmission happens in the
     * core's hid_tx thread, in order. Returns NRF_CORE_OK, NRF_CORE_EINVAL, or
     * NRF_CORE_EBUSY if the queue stayed full (link stalled) for ~100 ms. */
    int32_t (*hid_send)(uint32_t type, const void *data, uint32_t len);
    uint32_t (*hid_leds)(void);
    /* Copy one received raw HID packet into buf. Returns length, 0 if none. */
    int32_t (*hid_raw_recv)(void *buf, uint32_t maxlen);

    /* --- connection management --- */
    uint32_t (*conn_state)(void);
    int32_t (*command)(uint32_t cmd, uint32_t arg);

    /* --- persistent storage (EEPROM emulation, RAM mirrored, flushed lazily) --- */
    int32_t (*eeprom_read)(uint32_t addr, void *buf, uint32_t len);
    int32_t (*eeprom_write)(uint32_t addr, const void *buf, uint32_t len);
    void (*eeprom_flush)(void);

    /* --- peripherals (pins chosen by the app at run time) ---
     * One bus of each kind, blocking calls with a timeout; buffers may be in
     * flash (copied by the core) or RAM. Return NRF_CORE_OK or a negative
     * NRF_CORE_E* code (EINVAL, EIO = NACK / bus error, ETIMEDOUT). */
    /* I2C master (TWIM): pins and bus speed (100000, 250000 or 400000 Hz). */
    int32_t (*i2c_init)(uint32_t scl_pin, uint32_t sda_pin, uint32_t freq_hz);
    /* Write tx (if tx_len), then read rx (if rx_len) with a repeated start.
     * timeout_ms is capped at 1000 (0xFFFF does not wait forever). After a
     * timeout the bus counts as faulted and calls fail at once (EIO while
     * SCL/SDA is held low, ETIMEDOUT otherwise) until a retry, after 1 s
     * then up to 8 s, completes; a device that never answers costs no time. */
    int32_t (*i2c_xfer)(uint32_t addr7, const void *tx, uint32_t tx_len, void *rx, uint32_t rx_len,
                        uint32_t timeout_ms);
    /* SPI master (SPIM): pins (NRF_CORE_NO_PIN if unused). The app drives CS itself. */
    int32_t (*spi_init)(uint32_t sck_pin, uint32_t mosi_pin, uint32_t miso_pin);
    /* Clock, mode and bit order for the next transfers: call it BEFORE
     * asserting CS, it may move SCK to the mode's idle level. flags:
     * NRF_CORE_SPI_MODE_* | NRF_CORE_SPI_LSB_FIRST; freq rounded down to
     * 125 kHz..8 MHz. */
    int32_t (*spi_config)(uint32_t freq_hz, uint32_t flags);
    /* Full duplex: clocks max(tx_len, rx_len) bytes (0xFF sent past tx_len). */
    int32_t (*spi_xfer)(const void *tx, uint32_t tx_len, void *rx, uint32_t rx_len);
    /* WS2812 / SK6812 data stream on any pin (PWM, 800 kHz): len bytes sent
     * as is, in the LED's byte order, followed by the reset gap. */
    int32_t (*ws2812_write)(uint32_t pin, const void *data, uint32_t len);
    /* Analog input (AIN pin): 12-bit reading, 0..4095 = 0..VDD. Negative on error. */
    int32_t (*adc_read)(uint32_t pin);
    /* PWM outputs (backlight, buzzer): NRF_CORE_PWM_GENERATORS generators of
     * NRF_CORE_PWM_CHANNELS_PER_GEN channels, any pins; the channels of a
     * generator share its frequency. pin NRF_CORE_NO_PIN frees a channel; a
     * (re)configured channel starts off. */
    int32_t (*pwm_config)(uint32_t channel, uint32_t pin, uint32_t flags);
    /* 4 Hz .. 8 MHz (default 1000 Hz); resolution = 16 MHz / freq steps,
     * at most 32767. Duties (as fractions) are kept. */
    int32_t (*pwm_frequency)(uint32_t generator, uint32_t freq_hz);
    /* 0 = off .. 0xFFFF = always on. 0 and 0xFFFF hold the pin at a fixed
     * level; a generator with no channel in between stops (no extra current). */
    int32_t (*pwm_duty)(uint32_t channel, uint32_t duty);

    /* --- misc --- */
    void (*device_id)(uint32_t id[4]);
    void (*log_write)(const char *str, uint32_t len);
};

/* ========================================================================= */
/* 5. Compile-time layout checks (identical results required on both sides)   */
/* ========================================================================= */
#ifdef __cplusplus
#    define NRF_CORE_STATIC_ASSERT static_assert
#else
#    define NRF_CORE_STATIC_ASSERT _Static_assert
#endif

NRF_CORE_STATIC_ASSERT(sizeof(void *) == 4, "nrf_core ABI is 32-bit only");

NRF_CORE_STATIC_ASSERT(sizeof(struct nrf_app_identity) == 44, "nrf_app_identity layout changed");
NRF_CORE_STATIC_ASSERT(sizeof(struct nrf_app_battery) == 20, "nrf_app_battery layout changed");
NRF_CORE_STATIC_ASSERT(sizeof(struct nrf_app_header) == NRF_APP_HEADER_SIZE, "nrf_app_header size changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_app_header, header_size) == NRF_APP_HEADER_OFF_HEADER_SIZE, "header layout changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_app_header, load_addr) == NRF_APP_HEADER_OFF_LOAD_ADDR, "header layout changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_app_header, image_size) == NRF_APP_HEADER_OFF_IMAGE_SIZE, "header layout changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_app_header, crc32) == NRF_APP_HEADER_OFF_CRC32, "header layout changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_app_header, abi_hash) == 40, "header layout changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_app_header, identity) == 44, "header layout changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_app_header, battery) == 88, "header layout changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_app_header, power) == 108, "header layout changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_app_header, charger) == 112, "header layout changed");
NRF_CORE_STATIC_ASSERT(sizeof(struct nrf_app_charger) == 4, "nrf_app_charger layout changed");
NRF_CORE_STATIC_ASSERT(sizeof(struct nrf_app_power) == 4, "nrf_app_power layout changed");

NRF_CORE_STATIC_ASSERT(sizeof(struct nrf_core_mouse_report) == 7, "mouse report size changed");

NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_core_api, uptime_ms) == 16, "nrf_core_api layout changed");
NRF_CORE_STATIC_ASSERT(offsetof(struct nrf_core_api, log_write) == 16 + 4 * 35, "nrf_core_api layout changed");

#ifdef __cplusplus
}
#endif

#endif /* NRF_CORE_API_H_ */
