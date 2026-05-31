/*
 * Copyright (c) 2026 numachang
 * SPDX-License-Identifier: MIT
 *
 * DIAG (diag/14-reconnect-heartbeat): root-cause capture + auto-recovery for the
 * #14 hard freeze (central abruptly powered off -> peripheral halts, no fault
 * dump, never recovers).
 *
 * Two mechanisms, no debugger needed:
 *
 *  1. Controller assert sink. With CONFIG_BT_CTLR_ASSERT_HANDLER=y, every
 *     LL_ASSERT() in the Zephyr SW link-layer controller routes here on a
 *     violated invariant (this is INDEPENDENT of CONFIG_ASSERT, so we leave the
 *     global assert off to avoid surprising asserts elsewhere). The handler
 *     can't reliably flush USB CDC if we're deep in the radio ISR, so instead it
 *     records the file:line into __noinit RAM (survives a warm reset) and cold-
 *     reboots. The next boot prints ">>> PREV LL_ASSERT @ file:line" = the exact
 *     controller location that fails when the central vanishes.
 *
 *  2. Hardware watchdog. If the freeze is a pure interrupt-locked spin with NO
 *     LL_ASSERT (nothing routes to the sink), the WDT counter (clocked off the
 *     32 kHz LFCLK, independent of the CPU) still fires after WDT_TIMEOUT_MS and
 *     resets the SoC. The next boot sees reset_cause = WATCHDOG with no recorded
 *     assert => "silent hard hang". Either way the right half AUTO-RECOVERS
 *     (reboots + re-advertises) instead of needing a manual power cycle, which
 *     is also a practical mitigation for #14.
 *
 * The heartbeats in hb_probe.c still run, so the log just before a reset shows
 * whether only the system workqueue wedged (sys-wq hb stops, thread hb lives) or
 * the whole CPU halted (both stop) right up to the reset.
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/reboot.h>
#include <string.h>

LOG_MODULE_REGISTER(diag_root, LOG_LEVEL_INF);

/* --- retained crash record (in .noinit: kept across a warm/WDT reset, lost
 *     only on real power-off) --- */
#define CRASH_MAGIC 0x4C4C4153u /* 'LLAS' */
struct crash_info {
    uint32_t magic;
    uint32_t line;
    char file[64];
};
static __noinit struct crash_info crash;

/* Controller assertion sink (CONFIG_BT_CTLR_ASSERT_HANDLER=y). Runs at the point
 * an LL invariant breaks -- record where, then cold-reboot so the next boot can
 * print it (USB CDC can't be trusted to flush from inside the radio ISR). */
void bt_ctlr_assert_handle(char *file, uint32_t line) {
    crash.magic = CRASH_MAGIC;
    crash.line = line;
    size_t i = 0;
    if (file != NULL) {
        for (; i < sizeof(crash.file) - 1U && file[i] != '\0'; i++) {
            crash.file[i] = file[i];
        }
    }
    crash.file[i] = '\0';
    sys_reboot(SYS_REBOOT_COLD);
    for (;;) {
        /* unreachable */
    }
}

/* --- hardware watchdog: recover from a silent interrupt-locked hard hang.
 * #if-guarded so the config bisect can build with WATCHDOG=n (assert sink only)
 * to isolate which change suppresses the #14 freeze. --- */
#if IS_ENABLED(CONFIG_WATCHDOG)
#define WDT_TIMEOUT_MS 4000
static const struct device *const wdt_dev = DEVICE_DT_GET(DT_NODELABEL(wdt0));
static int wdt_ch = -1;

static void wdt_feed_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(wdt_feed_work, wdt_feed_handler);

static void wdt_feed_handler(struct k_work *work) {
    ARG_UNUSED(work);
    if (wdt_ch >= 0) {
        (void)wdt_feed(wdt_dev, wdt_ch);
    }
    k_work_reschedule(&wdt_feed_work, K_MSEC(WDT_TIMEOUT_MS / 4));
}
#endif /* CONFIG_WATCHDOG */

/* The boot report (reset cause + any recorded LL_ASSERT) must NOT be logged from
 * SYS_INIT: that runs before the USB CDC console is up, so those lines are
 * dropped. Stash the values at init and have a delayable work re-print them a
 * few times once logging is live, so Tera Term reliably catches them. */
static uint32_t boot_cause;
static bool boot_had_assert;
static uint32_t boot_assert_line;
static char boot_assert_file[sizeof(crash.file)];
static int report_n;

static void report_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(report_work, report_handler);

static void report_handler(struct k_work *work) {
    ARG_UNUSED(work);
    LOG_INF("=== DIAG ROOT report #%d: prev reset_cause=0x%08x [%s%s%s%s] ===", report_n,
            boot_cause, (boot_cause & RESET_POR) ? "POR " : "",
            (boot_cause & RESET_PIN) ? "PIN " : "", (boot_cause & RESET_SOFTWARE) ? "SOFT " : "",
            (boot_cause & RESET_WATCHDOG) ? "WDT " : "");
    if (boot_had_assert) {
        LOG_ERR(">>> PREV LL_ASSERT @ %s:%u <<< (controller invariant violated -- ROOT CAUSE)",
                boot_assert_file, boot_assert_line);
    } else if (boot_cause & RESET_WATCHDOG) {
        LOG_ERR(">>> PREV BOOT: WATCHDOG reset, NO LL_ASSERT = silent hard hang (irq-locked) <<<");
    } else {
        LOG_INF("(prev boot was clean: no LL_ASSERT, no watchdog)");
    }
    if (++report_n < 5) {
        k_work_reschedule(&report_work, K_SECONDS(2));
    }
}

static int diag_root_init(void) {
    (void)hwinfo_get_reset_cause(&boot_cause);
    (void)hwinfo_clear_reset_cause();
    if (crash.magic == CRASH_MAGIC) {
        boot_had_assert = true;
        boot_assert_line = crash.line;
        (void)strncpy(boot_assert_file, crash.file, sizeof(boot_assert_file) - 1U);
        boot_assert_file[sizeof(boot_assert_file) - 1U] = '\0';
        crash.magic = 0u;
    }
    /* print once logging is live, then repeat (see report_handler) */
    k_work_reschedule(&report_work, K_SECONDS(3));

#if IS_ENABLED(CONFIG_WATCHDOG)
    if (!device_is_ready(wdt_dev)) {
        LOG_ERR("WDT device not ready; auto-recovery disabled");
        return 0;
    }
    struct wdt_timeout_cfg cfg = {
        .flags = WDT_FLAG_RESET_SOC,
        .window = {.min = 0u, .max = WDT_TIMEOUT_MS},
        .callback = NULL,
    };
    wdt_ch = wdt_install_timeout(wdt_dev, &cfg);
    if (wdt_ch < 0) {
        LOG_ERR("wdt_install_timeout failed: %d", wdt_ch);
        return 0;
    }
    int rc = wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (rc != 0) {
        LOG_ERR("wdt_setup failed: %d", rc);
        return 0;
    }
    k_work_reschedule(&wdt_feed_work, K_NO_WAIT);
    LOG_INF("WDT armed %d ms (fed every %d ms from system-wq)", WDT_TIMEOUT_MS, WDT_TIMEOUT_MS / 4);
#else
    LOG_INF("WDT disabled for this bisect build (assert sink only)");
#endif /* CONFIG_WATCHDOG */
    return 0;
}
SYS_INIT(diag_root_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
