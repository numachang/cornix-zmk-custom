/*
 * Copyright (c) 2026 numachang
 * SPDX-License-Identifier: MIT
 *
 * DIAG (diag/14-reconnect-heartbeat): dual liveness heartbeat for issue #14
 * ("left central powered off -> right peripheral stops recovering / freezes").
 *
 * Two INDEPENDENT 1 Hz heartbeats, logged over the USB CDC console on BOTH
 * halves (both build with zmk-usb-logging). Capture both halves in one
 * synchronized Tera Term timeline, then kill the central by unplugging the
 * LEFT's USB-C (with the power switch OFF the USB is the left's only power),
 * and read the RIGHT half's log:
 *
 *   "hb sys-wq #N"  -- a k_work_delayable on the SYSTEM workqueue.
 *   "hb thread #N"  -- a dedicated, preemptible k_thread.
 *
 * Interpretation after the central disappears:
 *   sys-wq stops, thread continues    -> the SYSTEM workqueue is wedged: a BLE
 *                                        work item blocks forever and starves
 *                                        every other system-wq job. This is the
 *                                        real #14 deadlock signature.
 *   both stop, preceded by a FATAL    -> a fault (e.g. the split notify thread
 *     "Stack overflow ... Halting"       overflowing its stack: Zephyr #44349 /
 *                                        ZMK #1245). CONFIG_THREAD_NAME +
 *                                        CONFIG_RESET_ON_FATAL_ERROR=n make the
 *                                        dump name the culprit and stay on screen.
 *   both stop, no FATAL dump          -> hard hang or USB-CDC death; dig deeper.
 *   split reconnect logs resume       -> the right was alive and recovered.
 *
 * Unlike the battery-probe census build, this touches NO GPIO, so normal split
 * operation is preserved: the keyboard behaves like the daily firmware plus USB
 * logging -- exactly what #14 must be observed under.
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hb_probe, LOG_LEVEL_INF);

#define HB_PERIOD_MS 1000

/* --- heartbeat 1: SYSTEM workqueue (dies if the system-wq is wedged) --- */
static struct k_work_delayable sys_hb_work;
static uint32_t sys_hb_n;

static void sys_hb_handler(struct k_work *work) {
    ARG_UNUSED(work);
    LOG_INF("hb sys-wq #%u (up %lld ms)", sys_hb_n++, k_uptime_get());
    k_work_reschedule(&sys_hb_work, K_MSEC(HB_PERIOD_MS));
}

/* --- heartbeat 2: dedicated thread (survives a wedged system-wq; dies only
 *     if the scheduler/CPU itself stops, e.g. a fatal fault halt) --- */
static void thread_hb_entry(void *a, void *b, void *c) {
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);
    uint32_t n = 0;
    for (;;) {
        LOG_INF("hb thread #%u (up %lld ms)", n++, k_uptime_get());
        k_msleep(HB_PERIOD_MS);
    }
}
/* Low, preemptible priority so it never disturbs BLE but still runs whenever
 * the system workqueue is merely blocked (not the CPU). 1 KiB stack covers the
 * logging call's own usage. */
K_THREAD_DEFINE(hb_thread_id, 1024, thread_hb_entry, NULL, NULL, NULL, 14, 0, 0);

static int hb_probe_init(void) {
    LOG_INF("ACTIVE: #14 dual heartbeat (sys-wq + thread), 1 Hz, no GPIO");
    k_work_init_delayable(&sys_hb_work, sys_hb_handler);
    k_work_reschedule(&sys_hb_work, K_NO_WAIT);
    return 0;
}
SYS_INIT(hb_probe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
