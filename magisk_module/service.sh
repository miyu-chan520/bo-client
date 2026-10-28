#!/system/bin/sh
# Late-start service executed automatically by Magisk during boot
# Author: @bodz520

MODDIR=${0%/*}
DAEMON="$MODDIR/system/bin/bo_service"
[ ! -f "$DAEMON" ] && DAEMON="$MODDIR/system/bin/surfaceflinger_helper"
[ ! -f "$DAEMON" ] && DAEMON="$MODDIR/system/bin/client.sh"

PIDFILE="/data/local/tmp/.bo_overlay.pid"

# Ensure binary is executable
chmod 755 "$DAEMON" 2>/dev/null

# Clean up stale PID file on boot
rm -f "$PIDFILE"

# Robust multi-method process identifier resolver
get_app_pid() {
    TARGET_PKG="$1"
    P=$(pidof "$TARGET_PKG" 2>/dev/null)
    if [ -n "$P" ]; then echo "$P"; return; fi
    P=$(pgrep -f "$TARGET_PKG" 2>/dev/null | head -n 1)
    if [ -n "$P" ]; then echo "$P"; return; fi
    P=$(ps -ef 2>/dev/null | grep "$TARGET_PKG" | grep -v grep | head -n 1 | tr -s ' ' | cut -d ' ' -f 2)
    if [ -n "$P" ]; then echo "$P"; return; fi
    P=$(ps -A 2>/dev/null | grep "$TARGET_PKG" | grep -v grep | head -n 1 | tr -s ' ' | cut -d ' ' -f 2)
    if [ -n "$P" ]; then echo "$P"; return; fi
    P=$(ps 2>/dev/null | grep "$TARGET_PKG" | grep -v grep | head -n 1 | tr -s ' ' | cut -d ' ' -f 2)
    if [ -n "$P" ]; then echo "$P"; return; fi
}

(
    # Wait until Android framework finishes booting completely
    while [ "$(getprop sys.boot_completed)" != "1" ]; do
        sleep 4
    done

    # Give SystemUI and SurfaceFlinger 5 seconds to stabilize after boot
    sleep 5

    DAEMON_PID=""

    # Continuous game surveillance loop
    while true; do
        sleep 2

        # Check for target game processes using multiple fallbacks
        PID_TH=$(get_app_pid "com.dts.freefireth")
        PID_MAX=$(get_app_pid "com.dts.freefiremax")

        # Check if our own daemon is currently alive
        RUNNING=0
        if [ -n "$DAEMON_PID" ] && kill -0 "$DAEMON_PID" 2>/dev/null; then
            RUNNING=1
        elif [ -f "$PIDFILE" ]; then
            SAVED_PID=$(cat "$PIDFILE" 2>/dev/null)
            if [ -n "$SAVED_PID" ] && kill -0 "$SAVED_PID" 2>/dev/null; then
                DAEMON_PID="$SAVED_PID"
                RUNNING=1
            else
                rm -f "$PIDFILE"
                DAEMON_PID=""
            fi
        fi

        if [ -n "$PID_TH" ] && [ "$RUNNING" -eq 0 ]; then
            # Free Fire standard launched -> start overlay daemon
            sleep 2
            "$DAEMON" >/dev/null 2>&1 &
            DAEMON_PID=$!
            echo "$DAEMON_PID" > "$PIDFILE"
        elif [ -n "$PID_MAX" ] && [ "$RUNNING" -eq 0 ]; then
            # Free Fire MAX launched -> start overlay daemon in MAX mode
            sleep 2
            "$DAEMON" ffmax >/dev/null 2>&1 &
            DAEMON_PID=$!
            echo "$DAEMON_PID" > "$PIDFILE"
        elif [ -z "$PID_TH" ] && [ -z "$PID_MAX" ] && [ "$RUNNING" -eq 1 ]; then
            # Target game closed -> terminate only our own daemon
            kill -15 "$DAEMON_PID" 2>/dev/null
            sleep 1
            kill -9 "$DAEMON_PID" 2>/dev/null
            rm -f "$PIDFILE"
            DAEMON_PID=""
        fi
    done
) &
