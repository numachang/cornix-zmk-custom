# Attach to the app target (caksoylar style) so widget code shares ZMK's
# include search path — including app/include/drivers/ext_power.h.
# central.c and peripheral.c each guard themselves with #if so it's safe
# to list both unconditionally; the inactive one compiles to nothing.
target_sources(app PRIVATE src/widget.c)
# DIAG (diag/battery-vddh-probe): HARD-CODED to the P0.01 charge-STAT probe.
# The CONFIG_CORNIX_RGB_STAT_PROBE Kconfig gate did not take effect in the
# first attempt (the build took the else-branch, so charging.c's green blink
# ran and the probe never compiled). Hard-code it here for the diag so the
# probe definitely owns the strip. widget.c (strip worker) is kept; central.c /
# peripheral.c / charging.c are intentionally dropped.
# To restore normal indicators, revert to:
#   target_sources(app PRIVATE src/central.c src/peripheral.c src/charging.c)
target_sources(app PRIVATE src/charging_stat_probe.c)
zephyr_include_directories(include)
