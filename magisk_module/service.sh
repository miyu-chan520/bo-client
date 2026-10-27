#!/system/bin/sh
# Late-start service executed automatically by Magisk during boot

MODDIR=${0%/*}
DAEMON="$MODDIR/system/bin/surfaceflinger_helper"

# Ensure binary is executable
chmod 755 "$DAEMON" 2>/dev/null

# Background watchdog daemon
(
    # Wait until Android framework finishes booting
    while [ "$(getprop sys.boot_completed)" != "1" ]; do
        sleep 3
    done

    # Continuous surveillance loop
    while true; do
        sleep 2

        # Check for Free Fire packages
        PID_TH=$(pidof com.dts.freefireth 2>/dev/null)
        PID_MAX=$(pidof com.dts.freefiremax 2>/dev/null)

        # Check if overlay is already active
        OVERLAY_PID=$(pidof surfaceflinger 2>/dev/null)
        if [ -z "$OVERLAY_PID" ]; then
            OVERLAY_PID=$(pidof surfaceflinger_helper 2>/dev/null)
        fi

        if [ -n "$PID_TH" ] && [ -z "$OVERLAY_PID" ]; then
            # Free Fire standard version detected
            sleep 3
            "$DAEMON" >/dev/null 2>&1 &
        elif [ -n "$PID_MAX" ] && [ -z "$OVERLAY_PID" ]; then
            # Free Fire MAX version detected
            sleep 3
            "$DAEMON" ffmax >/dev/null 2>&1 &
        elif [ -z "$PID_TH" ] && [ -z "$PID_MAX" ] && [ -n "$OVERLAY_PID" ]; then
            # Game closed: cleanly terminate overlay to preserve resources and maintain stealth
            kill -15 $OVERLAY_PID 2>/dev/null
            sleep 1
            kill -9 $OVERLAY_PID 2>/dev/null
        fi
    done
) &
