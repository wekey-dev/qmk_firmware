// Copyright 2026 WeKey
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Start-up code of the QMK app image.
//
// The core validates the image header (nrf_app_header.c), then calls
// nrf_app_entry() in its "qmk" thread. There is no vector table and no reset handler: the core owns
// the CPU, the interrupts and the stack.

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>
#include "nrf_core.h"

/* Linker symbols (ld/nrf_app_common.ld) */
extern uint32_t __data_load[], __data_start[], __data_end[];
extern uint32_t __bss_start[], __bss_end[];
extern uint8_t  __heap_start[], __heap_end[];
extern void (*__preinit_array_start[])(void);
extern void (*__preinit_array_end[])(void);
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

int main(void);
void nrf_app_entry(const struct nrf_core_api *api) __attribute__((noreturn, used));

const struct nrf_core_api *nrf_core;

void nrf_app_entry(const struct nrf_core_api *api) {
    /* .data / .bss first: `nrf_core` itself lives in .bss. Plain loops so that
     * nothing from libc runs before its data is initialised. */
    volatile uint32_t *src = __data_load;
    volatile uint32_t *dst = __data_start;
    while (dst < __data_end) {
        *dst++ = *src++;
    }
    for (dst = __bss_start; dst < __bss_end;) {
        *dst++ = 0;
    }

    nrf_core = api;

    /* The core already rejects a different major version; also refuse a core
     * built from a different nrf_core_api.h (core and app must match). */
    if (api->abi_major != NRF_CORE_ABI_MAJOR || api->size != sizeof(struct nrf_core_api)) {
        for (;;) {
            api->sleep_ms(1000);
        }
    }

    for (void (**fn)(void) = __preinit_array_start; fn < __preinit_array_end; fn++) {
        (*fn)();
    }
    for (void (**fn)(void) = __init_array_start; fn < __init_array_end; fn++) {
        (*fn)();
    }

    main();

    for (;;) {
        api->sleep_ms(1000);
    }
}

/* ---- newlib-nano system calls ------------------------------------------- */

void *_sbrk(ptrdiff_t incr) {
    static uint8_t *brk = __heap_start;

    if (incr < 0 || (size_t)(__heap_end - brk) < (size_t)incr) {
        errno = ENOMEM;
        return (void *)-1;
    }
    uint8_t *prev = brk;
    brk += incr;
    return prev;
}

int _write(int fd, const char *buf, int len) {
    (void)fd;
    if (nrf_core != NULL && len > 0) {
        nrf_core->log_write(buf, (uint32_t)len);
    }
    return len;
}

int _read(int fd, char *buf, int len) {
    (void)fd;
    (void)buf;
    (void)len;
    return 0;
}

int _close(int fd) {
    (void)fd;
    return -1;
}

int _fstat(int fd, struct stat *st) {
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd) {
    (void)fd;
    return 1;
}

int _lseek(int fd, int ptr, int dir) {
    (void)fd;
    (void)ptr;
    (void)dir;
    return 0;
}

int _getpid(void) {
    return 1;
}

int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status) {
    (void)status;
    for (;;) {
        if (nrf_core != NULL) {
            nrf_core->sleep_ms(1000);
        }
    }
}
