#!/bin/bash
set -e

echo "=== Building Bo-Client Zygisk Module (arm64-v8a) ==="
cd "$(dirname "$0")/jni"

if command -v ndk-build >/dev/null 2>&1; then
    ndk-build -B
elif [ -n "$ANDROID_NDK_ROOT" ]; then
    "$ANDROID_NDK_ROOT/ndk-build" -B
elif [ -n "$NDK" ]; then
    "$NDK/ndk-build" -B
else
    echo "Error: ndk-build not found in PATH or ANDROID_NDK_ROOT!"
    exit 1
fi

echo "=== Assembling Magisk / KernelSU / APatch Module Zip ==="
cd ..
mkdir -p package/zygisk
cp libs/arm64-v8a/libbo_client.so package/zygisk/arm64-v8a.so
cp module.prop package/
cp customize.sh package/
cd package
zip -r ../bo-client-zygisk-v2.0.zip .
cd ..
rm -rf package

echo "=== Build Complete: bo-client-zygisk-v2.0.zip ==="
