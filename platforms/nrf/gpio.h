// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "pin_defs.h"
#include "nrf_core.h"

typedef uint8_t pin_t;

/* All GPIO access goes through the core (pin = port * 32 + pin). */

#define gpio_set_pin_input(pin) ((void)nrf_core->gpio_config((pin), NRF_CORE_GPIO_INPUT))
#define gpio_set_pin_input_high(pin) ((void)nrf_core->gpio_config((pin), NRF_CORE_GPIO_INPUT | NRF_CORE_GPIO_PULL_UP))
#define gpio_set_pin_input_low(pin) ((void)nrf_core->gpio_config((pin), NRF_CORE_GPIO_INPUT | NRF_CORE_GPIO_PULL_DOWN))
#define gpio_set_pin_output_push_pull(pin) ((void)nrf_core->gpio_config((pin), NRF_CORE_GPIO_OUTPUT))
#define gpio_set_pin_output_open_drain(pin) ((void)nrf_core->gpio_config((pin), NRF_CORE_GPIO_OUTPUT | NRF_CORE_GPIO_OPEN_DRAIN))
#define gpio_set_pin_output(pin) gpio_set_pin_output_push_pull(pin)

#define gpio_write_pin_high(pin) nrf_core->gpio_write((pin), 1)
#define gpio_write_pin_low(pin) nrf_core->gpio_write((pin), 0)
#define gpio_write_pin(pin, level) nrf_core->gpio_write((pin), (level) ? 1 : 0)

#define gpio_read_pin(pin) ((bool)nrf_core->gpio_read(pin))

#define gpio_toggle_pin(pin) nrf_core->gpio_toggle(pin)
