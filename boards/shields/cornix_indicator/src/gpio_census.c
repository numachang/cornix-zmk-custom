/*
 * Copyright (c) 2026 numachang
 * SPDX-License-Identifier: MIT
 *
 * DIAG (diag/battery-vddh-probe): full GPIO census.
 *
 * Reads EVERY GPIO on P0 (0-31) and P1 (0-15) and classifies each by probing
 * it twice — once with an internal pull-up, once with a pull-down:
 *
 *   up=1 dn=0 -> FLOAT  (nothing external; only the internal pull wins)
 *   up=1 dn=1 -> HIGH   (driven/tied high externally)
 *   up=0 dn=0 -> LOW    (driven/tied low externally, e.g. GND or active-low)
 *   up=0 dn=1 -> WEAK   (shouldn't normally happen)
 *
 * The full census is logged at boot, then every CENSUS_PERIOD only the pins
 * whose class CHANGED are logged. Plug / unplug each half's USB-C and watch:
 * any pin that flips (e.g. FLOAT->LOW on charging) is part of the charge / VBUS
 * subsystem. A charge-IC STAT line (open-drain, active-low) reads FLOAT when
 * idle and LOW while charging, so it shows up as a FLOAT<->LOW pin. This also
 * tells connected-vs-floating for every pin at once, on both halves.
 *
 * SAFETY: two pins are excluded from reconfiguration:
 *   P0.18 -> nRESET (reconfiguring risks resetting the chip)
 *   P0.05 -> charger-control output (driven low by pinmux.c; releasing it would
 *            stop charging and invalidate the STAT observation)
 * Reconfiguring the rest is harmless and fully undone by reflashing. While this
 * runs, normal functions (typing, LED, etc.) are disrupted by design — this is
 * a read-only-electrical diagnostic, not a usable keyboard build.
 *
 * Own "gpio_census" log module at INFO so it is always visible on the USB CDC
 * console (Tera Term) on either half.
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_census, LOG_LEVEL_INF);

#define CENSUS_PERIOD_MS 1500

enum cls { CLS_FLOAT, CLS_HIGH, CLS_LOW, CLS_WEAK, CLS_ERR };
static const char *const CLS_NAME[] = {"FLOAT", "HIGH", "LOW", "WEAK", "ERR"};

struct port_info {
    const struct device *dev;
    const char *name;
    uint8_t npins;
};

static const struct device *const gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
static const struct device *const gpio1 = DEVICE_DT_GET(DT_NODELABEL(gpio1));
static struct port_info ports[2];

/* last class per pin: [port][pin]; ports are P0 (32) and P1 (16). */
static int8_t last_cls[2][32];

static struct k_work_delayable census_work;
static bool first_pass = true;

/* SAFETY exclusions: {port_index, pin}. P0.18=reset, P0.05=charger ctrl. */
static bool excluded(int p, int pin) {
    return (p == 0 && pin == 18) || (p == 0 && pin == 5);
}

static enum cls classify(const struct device *dev, int pin) {
    int up, dn;
    if (gpio_pin_configure(dev, pin, GPIO_INPUT | GPIO_PULL_UP) < 0) return CLS_ERR;
    up = gpio_pin_get(dev, pin);
    if (gpio_pin_configure(dev, pin, GPIO_INPUT | GPIO_PULL_DOWN) < 0) return CLS_ERR;
    dn = gpio_pin_get(dev, pin);
    if (up < 0 || dn < 0) return CLS_ERR;
    if (up && !dn) return CLS_FLOAT;
    if (up && dn) return CLS_HIGH;
    if (!up && !dn) return CLS_LOW;
    return CLS_WEAK;
}

static void census_handler(struct k_work *work) {
    ARG_UNUSED(work);
    int changed = 0;
    for (int p = 0; p < 2; p++) {
        struct port_info *pi = &ports[p];
        if (!pi->dev) continue;
        for (int pin = 0; pin < pi->npins; pin++) {
            if (excluded(p, pin)) continue;
            enum cls c = classify(pi->dev, pin);
            if (first_pass) {
                LOG_INF("%s.%02d = %s", pi->name, pin, CLS_NAME[c]);
            } else if (c != last_cls[p][pin]) {
                LOG_INF("CHANGE %s.%02d: %s -> %s", pi->name, pin,
                        CLS_NAME[(int)last_cls[p][pin]], CLS_NAME[c]);
                changed++;
            }
            last_cls[p][pin] = (int8_t)c;
        }
    }
    if (first_pass) {
        LOG_INF("--- boot census done; now logging CHANGES only (plug/unplug USB) ---");
        first_pass = false;
    } else if (changed == 0) {
        LOG_INF("hb (no change)");
    }
    k_work_reschedule(&census_work, K_MSEC(CENSUS_PERIOD_MS));
}

static int gpio_census_init(void) {
    ports[0] = (struct port_info){gpio0, "P0", 32};
    ports[1] = (struct port_info){gpio1, "P1", 16};
    if (!device_is_ready(gpio0) || !device_is_ready(gpio1)) {
        LOG_ERR("gpio0/gpio1 not ready");
        return -ENODEV;
    }
    LOG_INF("ACTIVE: GPIO census (pull-up vs pull-down). excl P0.18(reset) P0.05(chg)");
    k_work_init_delayable(&census_work, census_handler);
    k_work_reschedule(&census_work, K_NO_WAIT);
    return 0;
}
SYS_INIT(gpio_census_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
