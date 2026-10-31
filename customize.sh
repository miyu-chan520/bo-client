SKIPUNZIP=0

ui_print "****************************************"
ui_print "*        Bo-Client Zygisk VIP          *"
ui_print "*      Free Fire & Free Fire MAX       *"
ui_print "*     Internal Multi-Feature Module    *"
ui_print "****************************************"

# Verify architecture
if [ "$ARCH" != "arm64" ]; then
    abort "! Unsupported architecture: $ARCH (Requires 64-bit arm64-v8a)"
fi

# Create zygisk directories
mkdir -p "$MODPATH/zygisk"

# Extract zygisk shared library
ui_print "- Installing Zygisk arm64 binary..."
if [ -f "$ZIPFILE" ]; then
    unzip -o "$ZIPFILE" 'zygisk/*' -d "$MODPATH" >/dev/null 2>&1
fi

set_perm_recursive "$MODPATH" 0 0 0755 0644
set_perm "$MODPATH/zygisk/arm64-v8a.so" 0 0 0755

ui_print "- Installation successful!"
ui_print "- Please reboot your device to activate."
