/*
 * Copyright (c) 2026 numachang
 * SPDX-License-Identifier: MIT
 *
 * Experiment D (diag/14-D-btctlr-capture): isolate CONFIG_BT_CTLR_ASSERT_HANDLER
 * as the ONLY change from the pre-watchdog daily main source -- no USB logging,
 * no record/report, nothing else that could itself perturb the bug. Tests the
 * hypothesis that merely enabling the controller assert handler makes the split
 * peripheral reconnect instead of hard-hanging when the central abruptly loses
 * power (issue #14).
 *
 * Enabling CONFIG_BT_CTLR_ASSERT_HANDLER changes how every controller LL_ASSERT
 * expands (it calls this handler with __FILE__/__LINE__ instead of going through
 * BT_ASSERT -> k_oops -> the default fatal handler that arch_system_halt()s =
 * the freeze). The logs show the handler never actually fires, so the effect is
 * a deterministic build/codegen difference, not a caught fault -- this build
 * checks whether that difference alone is enough to keep the half alive.
 *
 * The handler body is reached only if some controller invariant ever breaks;
 * recover by rebooting rather than halting. No logging: confirm "reconnect"
 * behaviourally (the half keeps typing / its RGB indicator stays live).
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

void bt_ctlr_assert_handle(char *file, uint32_t line) {
    ARG_UNUSED(file);
    ARG_UNUSED(line);
    sys_reboot(SYS_REBOOT_COLD);
    for (;;) {
        /* unreachable */
    }
}
