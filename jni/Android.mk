LOCAL_PATH := $(call my-dir)

# ============================================================
# ShadowHook — static
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libshadowhook
LOCAL_SRC_FILES := ../src/shadowhook/libshadowhook.a
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS := optional
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# ZYGISK INTERNAL MODULE — libbo_client.so
# ============================================================
include $(CLEAR_VARS)

LOCAL_MODULE := bo_client

# Collect sources
LOCAL_SRC_FILES := \
    ../src/zygisk/zygisk_main.cpp \
    ../src/game/scanner.cpp \
    ../src/bypass/bypass.cpp \
    ../src/game/features.cpp \
    ../src/internal/egl_hook.cpp \
    ../src/globals.cpp \
    ../src/clipboard.cpp \
    ../src/game/data.cpp \
    ../src/protect/oxorany.cpp \
    ../src/ui/bar.cpp \
    ../src/ui/menu.cpp \
    ../src/ui/widgets/widgets.cpp \
    ../src/ui/png/image.cpp \
    ../src/Login/Jni_Stuff.cpp \
    ../includes/draw/ImGui/imgui.cpp \
    ../includes/draw/ImGui/imgui_draw.cpp \
    ../includes/draw/ImGui/imgui_tables.cpp \
    ../includes/draw/ImGui/imgui_widgets.cpp \
    ../includes/draw/ImGui/backends/imgui_impl_android.cpp \
    ../includes/draw/ImGui/backends/imgui_impl_opengl3.cpp

# Include Directories
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../includes \
    $(LOCAL_PATH)/../includes/font \
    $(LOCAL_PATH)/../includes/internal \
    $(LOCAL_PATH)/../includes/internal/ImGui \
    $(LOCAL_PATH)/../includes/internal/ImGui/backends \
    $(LOCAL_PATH)/../includes/internal/Android_draw \
    $(LOCAL_PATH)/../includes/internal/Android_touch \
    $(LOCAL_PATH)/../includes/internal/native_surface \
    $(LOCAL_PATH)/../includes/draw/ImGui \
    $(LOCAL_PATH)/../includes/draw/ImGui/backends \
    $(LOCAL_PATH)/../includes/protect \
    $(LOCAL_PATH)/../src \
    $(LOCAL_PATH)/../src/zygisk \
    $(LOCAL_PATH)/../src/bypass \
    $(LOCAL_PATH)/../src/internal \
    $(LOCAL_PATH)/../src/ui \
    $(LOCAL_PATH)/../src/ui/png \
    $(LOCAL_PATH)/../src/ui/png/images \
    $(LOCAL_PATH)/../src/ui/styles \
    $(LOCAL_PATH)/../src/ui/widgets \
    $(LOCAL_PATH)/../src/func \
    $(LOCAL_PATH)/../src/game \
    $(LOCAL_PATH)/../src/other \
    $(LOCAL_PATH)/../src/protect \
    $(LOCAL_PATH)/../src/shadowhook \
    $(LOCAL_PATH)/../src/Login

# Compiler Flags
LOCAL_CPPFLAGS := \
    -DZYGISK_MODULE \
    -DIMGUI_DEFINE_MATH_OPERATORS \
    -std=c++17 \
    -fexceptions \
    -frtti \
    -fvisibility=hidden \
    -fvisibility-inlines-hidden \
    -Oz \
    -ffunction-sections \
    -fdata-sections \
    -fomit-frame-pointer \
    -Wno-error=format-security \
    -fno-color-diagnostics \
    -fmerge-all-constants \
    -fno-ident

# Linker Flags
LOCAL_LDFLAGS := \
    -Wl,--gc-sections \
    -Wl,--strip-all \
    -Wl,--build-id=none \
    -Wl,-z,relro \
    -Wl,-z,now \
    -Wl,-z,noexecstack

# Static Libraries
LOCAL_STATIC_LIBRARIES := \
    libshadowhook

# System Libraries
LOCAL_LDLIBS := \
    -llog \
    -landroid \
    -lEGL \
    -lGLESv3 \
    -lz \
    -ldl

include $(BUILD_SHARED_LIBRARY)
