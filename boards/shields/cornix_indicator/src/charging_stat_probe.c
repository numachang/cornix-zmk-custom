/*
 * Copyright (c) 2026 numachang
 * SPDX-License-Identifier: MIT
 *
 * DIAG (diag/battery-vddh-probe): charge-IC STAT pin probe.
 *
 * Replaces the normal indicators (central.c / peripheral.c / charging.c are NOT
 * compiled when this probe is active; see CMakeLists.txt) so the strip is free
 * to show the raw state of the candidate charge-STAT GPIO.
 *
 * Ghidra (rev-rmk Phase 2d/f, gpio-pinmap.md) identified P0.01 as the stock RMK
 * firmware's charge-STAT input (GPIOTE event) on the LEFT half only. P0.01 is
 * XL2 but Cornix runs the LF clock from the internal RC (K32SRC_RC), so the pin
 * is free for GPIO. This probe reads P0.01 (input, pull-up) on BOTH halves to
 * answer on real hardware:
 *   LEFT  : does P0.01 track charging? (confirm the Ghidra finding)
 *   RIGHT : does P0.01 track charging too (wired but unused by RMK), or stay
 *           fixed (not connected to STAT on the right)?
 *
 * Display, per half (SOLID, so it is clearly NOT the old slow green blink):
 *   inner pixel (P0.01 / STAT) : GREEN = level LOW, RED = level HIGH
 *   outer pixel (VBUS)         : BLUE = USB powered, OFF = not powered
 *
 * Plug this half's USB-C and watch its inner pixel and the log:
 *   inner RED -> GREEN (and "P0.01=1 -> 0" in the log) when charging
 *               => P0.01 is wired to STAT on this half.
 *   inner stays RED / "P0.01" never changes while outer is BLUE
 *               => P0.01 is NOT the STAT line here.
 * Exact polarity does not matter; what matters is whether P0.01 CHANGES with
 * charge state. Logged on its own "stat_probe" module at INFO so it is always
 * visible on the USB CDC console (Tera Term), independent of other log levels.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/usb.h>

#include <cornix_rgb_indicator/widget.h>

LOG_MODULE_REGISTER(stat_probe, LOG_LEVEL_INF);

/* Candidate charge-STAT GPIO: P0.01 (port 0, pin 1). */
#define STAT_PORT 0
#define STAT_PIN  1

#define PROBE_PERIOD_MS 1000
#define PROBE_SHOW_MS   3000 /* >> period so the pixel stays solid between samples */

static const struct led_rgb COL_GREEN = {.g = CORNIX_RGB_LEVEL};
static const struct led_rgb COL_RED   = {.r = CORNIX_RGB_LEVEL};
static const struct led_rgb COL_BLUE  = {.b = CORNIX_RGB_LEVEL};
static const struct led_rgb COL_OFF   = {0};

static const struct device *const gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static struct k_work_delayable probe_work;

static int last_stat = -2;
static int last_usb = -2;

static void probe_handler(struct k_work *work) {
    ARG_UNUSED(work);

    int stat = gpio_pin_get(gpio0, STAT_PIN); /* 0 = low, 1 = high, <0 = error */
    bool usb = zmk_usb_is_powered();

    if (stat != last_stat || (int)usb != last_usb) {
        LOG_INF("CHANGE  P0.01=%d  usb_powered=%d", stat, (int)usb);
        last_stat = stat;
        last_usb = (int)usb;
    } else {
        LOG_INF("hb      P0.01=%d  usb_powered=%d", stat, (int)usb); /* heartbeat */
    }

    cornix_rgb_show_once(CORNIX_RGB_PIX_INNER, stat == 0 ? COL_GREEN : COL_RED,
                         PROBE_SHOW_MS);
    cornix_rgb_show_once(CORNIX_RGB_PIX_OUTER, usb ? COL_BLUE : COL_OFF,
                         PROBE_SHOW_MS);

    k_work_reschedule(&probe_work, K_MSEC(PROBE_PERIOD_MS));
}

static int stat_probe_init(void) {
    if (!device_is_ready(gpio0)) {
        LOG_ERR("gpio0 not ready");
        return -ENODEV;
    }
    int ret = gpio_pin_configure(gpio0, STAT_PIN, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        LOG_ERR("configure P0.01 failed (%d)", ret);
        return ret;
    }
    LOG_INF("ACTIVE: probing P0.01 (input pull-up). inner=STAT(G=low,R=high) outer=VBUS");
    k_work_init_delayable(&probe_work, probe_handler);
    k_work_reschedule(&probe_work, K_NO_WAIT);
    return 0;
}
SYS_INIT(stat_probe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
