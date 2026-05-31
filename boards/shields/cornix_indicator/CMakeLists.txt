# Attach to the app target (caksoylar style) so widget code shares ZMK's
# include search path — including app/include/drivers/ext_power.h.
# central.c and peripheral.c each guard themselves with #if so it's safe
# to list both unconditionally; the inactive one compiles to nothing.
# DIAG (diag/battery-vddh-probe): HARD-CODED to the full GPIO census, which
# scans every P0/P1 pin (pull-up vs pull-down) on both halves and logs it.
# It subsumes the P0.01 charge-STAT probe (P0.01 is one of the scanned pins).
# widget.c and the normal indicators (central/peripheral/charging) are dropped
# so nothing else touches the GPIOs while we census them.
# To restore normal indicators, revert to:
#   target_sources(app PRIVATE src/widget.c src/central.c src/peripheral.c src/charging.c)
target_sources(app PRIVATE src/gpio_census.c)
zephyr_include_directories(include)
