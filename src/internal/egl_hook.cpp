#define IMGUI_DEFINE_MATH_OPERATORS
#include "egl_hook.hpp"
#include "../shadowhook/shadowhook.h"
#include "../includes/internal/ImGui/imgui.h"
#include "../includes/internal/ImGui/imgui_internal.h"
#include "../includes/internal/ImGui/backends/imgui_impl_opengl3.h"
#include "../ui/menu.hpp"
#include "../func/esp.hpp"
#include "../ui/styles/styles.h"
#include "font_data.hpp"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android/input.h>
#include <atomic>

#define LOG_TAG "BoEGLHook"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern float g_sw;
extern float g_sh;
extern std::atomic<bool> menu_visible;
extern float global_anim_time;

namespace egl_hook {

static bool g_init = false;
static int g_width = 0;
static int g_height = 0;

bool is_initialized() { return g_init; }

// ============================================================================
// EGL HOOK
// ============================================================================
static EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = nullptr;

static EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    if (!g_init) {
        eglQuerySurface(dpy, surface, EGL_WIDTH, &g_width);
        eglQuerySurface(dpy, surface, EGL_HEIGHT, &g_height);

        if (g_width > 0 && g_height > 0) {
            g_sw = static_cast<float>(g_width);
            g_sh = static_cast<float>(g_height);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(g_sw, g_sh);

            // Setup default font
            if (sizeof(font_ttf) > 0) {
                ImFontConfig font_cfg;
                font_cfg.FontDataOwnedByAtlas = false;
                io.Fonts->AddFontFromMemoryTTF((void*)font_ttf, sizeof(font_ttf), 14.0f, &font_cfg);
            } else {
                io.Fonts->AddFontDefault();
            }

            ImGui_ImplOpenGL3_Init("#version 300 es");
            style::init_styles();

            g_init = true;
            LOGI("[✓] Internal ImGui + EGL GLES3 initialized (%dx%d)!", g_width, g_height);
        }
    }

    if (g_init) {
        global_anim_time += 0.016f;

        ImGui_ImplOpenGL3_NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(static_cast<float>(g_width), static_cast<float>(g_height));
        ImGui::NewFrame();

        // 1. Draw ESP Elements
        esp::draw_esp();

        // 2. Draw Internal Menu Window
        ui::menu::render();

        ImGui::Render();
        glViewport(0, 0, g_width, g_height);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    return orig_eglSwapBuffers(dpy, surface);
}

// ============================================================================
// TOUCH INPUT HOOK (AInputQueue_getEvent)
// ============================================================================
static int32_t (*orig_AInputQueue_getEvent)(AInputQueue* queue, AInputEvent** outEvent) = nullptr;
static void (*orig_AInputQueue_finishEvent)(AInputQueue* queue, AInputEvent* event, int handled) = nullptr;

static int32_t hook_AInputQueue_getEvent(AInputQueue* queue, AInputEvent** outEvent) {
    int32_t res = orig_AInputQueue_getEvent(queue, outEvent);
    if (res >= 0 && *outEvent) {
        int32_t type = AInputEvent_getType(*outEvent);
        if (type == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(*outEvent);
            float x = AMotionEvent_getRawX(*outEvent, 0);
            float y = AMotionEvent_getRawY(*outEvent, 0);

            if (g_init) {
                ImGuiIO& io = ImGui::GetIO();
                io.AddMousePosEvent(x, y);

                int maskedAction = action & AMOTION_EVENT_ACTION_MASK;
                if (maskedAction == AMOTION_EVENT_ACTION_DOWN) {
                    io.AddMouseButtonEvent(0, true);
                } else if (maskedAction == AMOTION_EVENT_ACTION_UP || maskedAction == AMOTION_EVENT_ACTION_CANCEL) {
                    io.AddMouseButtonEvent(0, false);
                }

                // If menu is open and touches are on ImGui window, intercept to prevent game from firing
                if (menu_visible.load() && io.WantCaptureMouse) {
                    if (orig_AInputQueue_finishEvent) {
                        orig_AInputQueue_finishEvent(queue, *outEvent, 1);
                    }
                    return -1;
                }
            }
        }
    }
    return res;
}

// ============================================================================
// INSTALL HOOKS
// ============================================================================
bool install_hooks() {
    LOGI("[*] Hooking eglSwapBuffers & Touch Events...");

    void* h_egl = shadowhook_hook_sym_name("libEGL.so",
                                          "eglSwapBuffers",
                                          (void*)hook_eglSwapBuffers,
                                          (void**)&orig_eglSwapBuffers);
    LOGI("[+] eglSwapBuffers Hook: %s", h_egl ? "OK" : "FAIL");

    void* h_touch = shadowhook_hook_sym_name("libandroid.so",
                                            "AInputQueue_getEvent",
                                            (void*)hook_AInputQueue_getEvent,
                                            (void**)&orig_AInputQueue_getEvent);
    shadowhook_hook_sym_name("libandroid.so",
                             "AInputQueue_finishEvent",
                             nullptr,
                             (void**)&orig_AInputQueue_finishEvent);
    LOGI("[+] Touch Event Hook: %s", h_touch ? "OK" : "FAIL");

    return (h_egl != nullptr);
}

} // namespace egl_hook
