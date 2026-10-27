#!/bin/sh
cd /storage/emulated/0/ZqwClient

echo "═══ FOLDER UTAMA ═══"
ls -la

echo ""
echo "═══ FOLDER JNI ═══"
ls -la jni/

echo ""
echo "═══ FOLDER SRC ═══"
ls -la src/

echo ""
echo "═══ FOLDER LOGIN ═══"
ls -la src/Login/

echo ""
echo "═══ FOLDER INCLUDES ═══"
ls -la includes/

echo ""
echo "═══ SEMUA FILE CPP/H ═══"
find . -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) | sort