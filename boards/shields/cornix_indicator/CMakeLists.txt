# Attach to the app target (caksoylar style) so widget code shares ZMK's
# include search path — including app/include/drivers/ext_power.h.
# central.c and peripheral.c each guard themselves with #if so it's safe
# to list both unconditionally; the inactive one compiles to nothing.
target_sources(app PRIVATE src/widget.c)
if(CONFIG_CORNIX_RGB_STAT_PROBE)
    # DIAG (diag/battery-vddh-probe): replace the normal indicators with the
    # P0.01 charge-STAT probe. widget.c (strip worker) is kept; central.c /
    # peripheral.c / charging.c are dropped so the probe owns the strip.
    target_sources(app PRIVATE src/charging_stat_probe.c)
else()
    target_sources(app PRIVATE
        src/central.c
        src/peripheral.c
        src/charging.c
    )
endif()
zephyr_include_directories(include)
