// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include "analog.h"
#include "nrf_core.h"

_Static_assert(ADC_RESOLUTION >= 1 && ADC_RESOLUTION <= 12, "ADC_RESOLUTION: 1..12 bits on nRF52");

int16_t analogReadPin(pin_t pin) {
    int32_t raw = nrf_core->adc_read(pin); /* 12 bit */
    if (raw < 0) {
        return -1;
    }
    return (int16_t)(raw >> (12 - ADC_RESOLUTION));
}
