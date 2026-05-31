# Attach to the app target (caksoylar style) so widget code shares ZMK's
# include search path — including app/include/drivers/ext_power.h.
# central.c and peripheral.c each guard themselves with #if so it's safe
# to list both unconditionally; the inactive one compiles to nothing.
# DIAG (diag/14-reconnect-heartbeat): HARD-CODED to the #14 dual heartbeat,
# which logs two independent 1 Hz liveness markers (system workqueue + a
# dedicated thread) over USB CDC on both halves and touches NO GPIO, so normal
# split operation is preserved while we observe the right half losing its central.
# The normal indicators are dropped so nothing else logs/contends meanwhile.
# (The battery-probe census build used src/gpio_census.c here instead.)
# To restore normal indicators, revert to:
#   target_sources(app PRIVATE src/widget.c src/central.c src/peripheral.c src/charging.c)
target_sources(app PRIVATE src/hb_probe.c src/diag_root.c)
zephyr_include_directories(include)
