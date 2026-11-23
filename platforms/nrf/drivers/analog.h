// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Analog inputs on the core: AIN pins only (P0_02..P0_05, P0_28..P0_31),
// 0..VDD. ADC_RESOLUTION bits (default 10, max 12), as on ChibiOS.
#pragma once

#include <stdint.h>
#include "gpio.h"

#ifndef ADC_RESOLUTION
#    define ADC_RESOLUTION 10
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Reading in ADC_RESOLUTION bits, or -1 if the pin is not an analog pin. */
int16_t analogReadPin(pin_t pin);

#ifdef __cplusplus
}
#endif
