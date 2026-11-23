// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

/* Audio on the core needs no driver settings beyond AUDIO_PIN (and
 * AUDIO_PIN_ALT + AUDIO_PIN_ALT_AS_NEGATIVE): the core's PWM generator 1
 * produces the tone on any pin. See audio_pwm_hardware.c. */
