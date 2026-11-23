// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Critical sections MUST go through the core: Zephyr locks interrupts with
// BASEPRI so the BLE controller's zero-latency IRQs keep running. Masking with
// PRIMASK (cpsid i) here would break BLE timing.
#pragma once

#include <stdint.h>
#include "nrf_core.h"

static __inline__ void __nrf_interrupt_restore__(const uint32_t *key) {
    nrf_core->irq_unlock(*key);
    __asm__ volatile("" ::: "memory");
}

/* irq_lock() nests, so FORCEON and RESTORESTATE behave the same. */
#define ATOMIC_BLOCK(type) for (type, __ToDo = 1; __ToDo; __ToDo = 0)
#define ATOMIC_RESTORESTATE uint32_t __nrf_key __attribute__((__cleanup__(__nrf_interrupt_restore__))) = nrf_core->irq_lock()
#define ATOMIC_FORCEON ATOMIC_RESTORESTATE

#define ATOMIC_BLOCK_RESTORESTATE ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
#define ATOMIC_BLOCK_FORCEON ATOMIC_BLOCK(ATOMIC_FORCEON)
