# Build test of the nrf peripheral drivers (I2C, SPI, WS2812, PWM backlight
# and audio, analog, battery) through stock QMK features. Not meant for a
# real board: the pins in config.h are arbitrary free pins of the nRF52840 /
# nRF52833 DK.
OLED_ENABLE = yes                 # SSD1306 over I2C
RGBLIGHT_ENABLE = yes             # WS2812 (WS2812_DRIVER = vendor)
POINTING_DEVICE_ENABLE = yes
POINTING_DEVICE_DRIVER = pmw3360  # SPI
QUANTUM_PAINTER_ENABLE = yes
QUANTUM_PAINTER_DRIVERS += st7789_spi
BATTERY_ENABLE = yes              # BATTERY_DRIVER = vendor (core battery level)
ANALOG_DRIVER_REQUIRED = yes
BACKLIGHT_ENABLE = yes            # BACKLIGHT_DRIVER = pwm (core PWM1)
AUDIO_ENABLE = yes                # AUDIO_DRIVER = pwm_hardware (core PWM2)
