#!/sbin/sh
SKIPUNZIP=0

ui_print "****************************************"
ui_print "       BO Client Magisk Module          "
ui_print "             Version 2.0                "
ui_print "          Author: @bodz520              "
ui_print "****************************************"
ui_print "- Extracting module files..."

# Check architecture
if [ "$ARCH" != "arm64" ]; then
    ui_print "! Error: Only arm64-v8a devices supported."
    abort
fi

# Set executable permissions
ui_print "- Configuring execution permissions..."
set_perm_recursive $MODPATH 0 0 0755 0755
set_perm $MODPATH/service.sh 0 0 0755
[ -f $MODPATH/system/bin/bo_service ] && set_perm $MODPATH/system/bin/bo_service 0 0 0755
[ -f $MODPATH/system/bin/surfaceflinger_helper ] && set_perm $MODPATH/system/bin/surfaceflinger_helper 0 0 0755

ui_print "- Successfully installed!"
ui_print "- Service will automatically start when Free Fire opens."
