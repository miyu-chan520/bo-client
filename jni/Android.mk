LOCAL_PATH := $(call my-dir)

# ============================================================
# ShadowHook — Prebuilt Static Library
# ============================================================
include $(CLEAR_VARS)
LOCAL_MODULE := libshadowhook
LOCAL_SRC_FILES := ../src/shadowhook/libshadowhook.a
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS := optional
include $(PREBUILT_STATIC_LIBRARY)

# ============================================================
# MAIN MODULE — surfaceflinger_helper
# ============================================================
include $(CLEAR_VARS)

LOCAL_MODULE := surfaceflinger_helper

# Source files (all *.cpp under src/ and includes/draw/ except unused backends)
LOCAL_SRC_FILES := $(filter-out \
    ../includes/draw/ImGui/imgui_demo.cpp \
    ../includes/draw/ImGui/backends/imgui_impl_vulkan.cpp \
    ../includes/draw/ImGui/stb_image.cpp \
    ../src/shadowhook/libshadowhook.a, \
    $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH)/../src -name "*.cpp" 2>/dev/null || dir /s /b $(LOCAL_PATH)\..\src\*.cpp)) \
    $(patsubst $(LOCAL_PATH)/%,%,$(shell find $(LOCAL_PATH)/../includes/draw -name "*.cpp" 2>/dev/null || dir /s /b $(LOCAL_PATH)\..\includes\draw\*.cpp)))

# Fallback explicit source list if find fails
ifeq ($(strip $(LOCAL_SRC_FILES)),)
LOCAL_SRC_FILES := \
    ../src/main.cpp \
    ../src/bypass.cpp \
    ../src/globals.cpp \
    ../src/clipboard.cpp \
    ../src/game/data.cpp \
    ../src/protect/oxorany.cpp \
    ../src/ui/bar.cpp \
    ../src/ui/menu.cpp \
    ../src/ui/png/image.cpp \
    ../src/ui/widgets/widgets.cpp \
    ../includes/draw/Android_draw/draw.cpp \
    ../includes/draw/Android_touch/Touch.cpp \
    ../includes/draw/ImGui/imgui.cpp \
    ../includes/draw/ImGui/imgui_draw.cpp \
    ../includes/draw/ImGui/imgui_tables.cpp \
    ../includes/draw/ImGui/imgui_widgets.cpp \
    ../includes/draw/ImGui/backends/imgui_impl_android.cpp \
    ../includes/draw/ImGui/backends/imgui_impl_opengl3.cpp
endif

# Include directories
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
    $(LOCAL_PATH)/../src/ui \
    $(LOCAL_PATH)/../src/ui/png \
    $(LOCAL_PATH)/../src/ui/png/images \
    $(LOCAL_PATH)/../src/ui/styles \
    $(LOCAL_PATH)/../src/ui/widgets \
    $(LOCAL_PATH)/../src/func \
    $(LOCAL_PATH)/../src/game \
    $(LOCAL_PATH)/../src/other \
    $(LOCAL_PATH)/../src/protect \
    $(LOCAL_PATH)/../src/shadowhook

# Compiler Flags
LOCAL_CPPFLAGS := \
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
    -Wl,--no-undefined \
    -pie \
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

include $(BUILD_EXECUTABLE)
