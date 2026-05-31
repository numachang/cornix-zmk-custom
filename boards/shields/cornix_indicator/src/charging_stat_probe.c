/*
 * Copyright (c) 2026 numachang
 * SPDX-License-Identifier: MIT
 *
 * DIAG (diag/battery-vddh-probe): charge-IC STAT pin probe.
 *
 * Replaces the normal indicators (central.c / peripheral.c / charging.c are
 * NOT compiled when CONFIG_CORNIX_RGB_STAT_PROBE=y; see CMakeLists.txt) so the
 * strip is free to show the raw state of the candidate charge-STAT GPIO.
 *
 * Ghidra (rev-rmk Phase 2d/f, gpio-pinmap.md) identified P0.01 as the stock
 * RMK firmware's charge-STAT input (GPIOTE event) on the LEFT half only. P0.01
 * is XL2 but Cornix runs the LF clock from the internal RC (K32SRC_RC), so the
 * pin is free for GPIO. This probe reads P0.01 (input, pull-up) on BOTH halves
 * and shows it, so we can answer on real hardware:
 *   - LEFT  : does P0.01 track charging? (confirm the Ghidra finding)
 *   - RIGHT : does P0.01 track charging too (wired but unused by RMK), or stay
 *             fixed (not connected to STAT on the right)?
 *
 * Display, per half:
 *   inner pixel (P0.01 / STAT) : GREEN = level LOW  (e.g. charging, active-low)
 *                                RED   = level HIGH (idle / pulled-up)
 *   outer pixel (VBUS)         : BLUE  = USB powered, OFF = not powered
 *
 * Plug this half's USB-C and watch the inner pixel:
 *   inner RED -> GREEN when charging  => P0.01 is wired to STAT on this half.
 *   inner stays RED while outer is BLUE => P0.01 is NOT the STAT line here.
 * The exact polarity does not matter for the connected/not-connected question;
 * what matters is whether the inner pixel CHANGES with charge state. The raw
 * level is also logged (visible on the left's USB CDC console / Tera Term).
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/usb.h>

#include <cornix_rgb_indicator/widget.h>

LOG_MODULE_DECLARE(cornix_rgb, CONFIG_ZMK_LOG_LEVEL);

/* Candidate charge-STAT GPIO: P0.01 (port 0, pin 1). */
#define STAT_PORT 0
#define STAT_PIN  1

#define PROBE_PERIOD_MS 300
#define PROBE_SHOW_MS   500 /* > period so the pixel stays lit between samples */

static const struct led_rgb COL_GREEN = {.g = CORNIX_RGB_LEVEL};
static const struct led_rgb COL_RED   = {.r = CORNIX_RGB_LEVEL};
static const struct led_rgb COL_BLUE  = {.b = CORNIX_RGB_LEVEL};
static const struct led_rgb COL_OFF   = {0};

static const struct device *const gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static struct k_work_delayable probe_work;

static int last_stat = -1;
static int last_usb = -1;

static void probe_handler(struct k_work *work) {
    ARG_UNUSED(work);

    int stat = gpio_pin_get(gpio0, STAT_PIN); /* 0 = low, 1 = high, <0 = error */
    bool usb = zmk_usb_is_powered();

    if (stat != last_stat || (int)usb != last_usb) {
        LOG_INF("stat_probe: P0.01=%d usb_powered=%d", stat, (int)usb);
        last_stat = stat;
        last_usb = (int)usb;
    }

    cornix_rgb_show_once(CORNIX_RGB_PIX_INNER, stat == 0 ? COL_GREEN : COL_RED,
                         PROBE_SHOW_MS);
    cornix_rgb_show_once(CORNIX_RGB_PIX_OUTER, usb ? COL_BLUE : COL_OFF,
                         PROBE_SHOW_MS);

    k_work_reschedule(&probe_work, K_MSEC(PROBE_PERIOD_MS));
}

static int stat_probe_init(void) {
    if (!device_is_ready(gpio0)) {
        LOG_ERR("stat_probe: gpio0 not ready");
        return -ENODEV;
    }
    int ret = gpio_pin_configure(gpio0, STAT_PIN, GPIO_INPUT | GPIO_PULL_UP);
    if (ret < 0) {
        LOG_ERR("stat_probe: configure P0.01 failed (%d)", ret);
        return ret;
    }
    LOG_INF("stat_probe: probing P0.01 (input pull-up); inner=STAT outer=VBUS");
    k_work_init_delayable(&probe_work, probe_handler);
    k_work_reschedule(&probe_work, K_NO_WAIT);
    return 0;
}
SYS_INIT(stat_probe_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
