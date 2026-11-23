// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later

#include "matrix.h"
#include "action.h"
#include "action_util.h"
#include "mousekey.h"
#include "programmable_button.h"
#include "host.h"
#include "suspend.h"
#include "led.h"
#include "wait.h"

void suspend_power_down(void) {
    suspend_power_down_quantum();
    wait_ms(17);
}

void suspend_wakeup_init(void) {
    clear_mods();
    clear_weak_mods();
    clear_keys();
#ifdef MOUSEKEY_ENABLE
    mousekey_clear();
#endif
#ifdef PROGRAMMABLE_BUTTON_ENABLE
    programmable_button_clear();
#endif
#ifdef EXTRAKEY_ENABLE
    host_system_send(0);
    host_consumer_send(0);
#endif
    suspend_wakeup_init_quantum();
}
