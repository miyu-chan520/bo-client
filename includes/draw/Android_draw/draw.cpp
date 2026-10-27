#include "Android_draw/draw.h"
#include "protect/oxorany.hpp"
#include "../../includes/font/font_data.hpp"

EGLDisplay display = EGL_NO_DISPLAY;
EGLConfig config;
EGLSurface surface = EGL_NO_SURFACE;
EGLContext context = EGL_NO_CONTEXT;

ANativeWindow *native_window;
ImFont* fontBold   = nullptr;
ImFont* fontMedium = nullptr;
ImFont* fontDesc   = nullptr;
ImFont* espFont    = nullptr;

int native_window_screen_x = 0;
int native_window_screen_y = 0;
android::ANativeWindowCreator::DisplayInfo displayInfo{0};
uint32_t orientation = 0;
bool g_Initialized = false;
ImGuiWindow *g_window = nullptr;

// ═══════════════════════════════════════════════════════
// GLOBAL HIDE RECORD — dideklarasikan di main.cpp
// ═══════════════════════════════════════════════════════
extern bool g_hideRecord;

bool initGUI_draw(uint32_t _screen_x, uint32_t _screen_y, bool log) {
    orientation = displayInfo.orientation;

    if (!init_egl(_screen_x, _screen_y, log)) return false;
    if (!ImGui_init()) return false;

    return true;
}

bool init_egl(uint32_t _screen_x, uint32_t _screen_y, bool log) {
    // ── Block untrusted touches (biar overlay bisa disentuh) ──
    FILE* fp = popen("settings put system block_untrusted_touches 0", "r");
    if (fp) {
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), fp) != NULL) { }
        pclose(fp);
    }
    system("settings put global block_untrusted_touches 0 > /dev/null 2>&1");
    system("settings put secure block_untrusted_touches 0 > /dev/null 2>&1");

    // ═══════════════════════════════════════════════════════
    // HIDE RECORD
    //   arg4 (hide)   : true = sembunyikan dari recorder & screenshot
    //                   false = tampil normal
    //   arg5 (secure) : reserved (header baru sudah handle via `hide`)
    // ═══════════════════════════════════════════════════════
    ::native_window = android::ANativeWindowCreator::Create(
        "com.android.systemui",
        _screen_x, _screen_y,
        g_hideRecord,    // ← hide
        false            // ← secure (sudah ditangani header)
    );

    ANativeWindow_acquire(native_window);
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) return false;

    if (eglInitialize(display, 0, 0) != EGL_TRUE) return false;

    EGLint num_config = 0;
    // ── RGBA8888: buffer warna penuh biar font tajam ──
    const EGLint attribList[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_BUFFER_SIZE,     32,
        EGL_DEPTH_SIZE,      16,
        EGL_STENCIL_SIZE,    8,
        EGL_NONE
    };
    const EGLint attrib_list[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE
    };

    if (eglChooseConfig(display, attribList, &config, 1, &num_config) != EGL_TRUE)
        return false;

    EGLint egl_format;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &egl_format);
    ANativeWindow_setBuffersGeometry(native_window, 0, 0, egl_format);

    context = eglCreateContext(display, config, EGL_NO_CONTEXT, attrib_list);
    if (context == EGL_NO_CONTEXT) return false;

    surface = eglCreateWindowSurface(display, config, native_window, nullptr);
    if (surface == EGL_NO_SURFACE) return false;

    if (!eglMakeCurrent(display, surface, surface, context)) return false;

    return true;
}

void screen_config() {
    displayInfo = android::ANativeWindowCreator::GetDisplayInfo();
}

bool ImGui_init() {
    if (g_Initialized) return true;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplAndroid_Init(native_window);
    ImGui_ImplOpenGL3_Init(oxorany("#version 300 es"));

    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;

    // ═══════════════════════════════════════════════════════
    // FONT CUSTOM dari font_data.hpp — tajam untuk HP modern
    // ═══════════════════════════════════════════════════════
    ImFontConfig cfg;
    cfg.FontData             = (void*)font_ttf;
    cfg.FontDataSize         = sizeof(font_ttf);
    cfg.FontDataOwnedByAtlas = false;
    cfg.OversampleH          = 2;
    cfg.OversampleV          = 2;
    cfg.PixelSnapH           = true;
    cfg.RasterizerDensity    = 2.0f;

    // Menu font
    ImFont* mainFont = io.Fonts->AddFontFromMemoryTTF(
        (void*)font_ttf, (int)sizeof(font_ttf), 20.f, &cfg);

    // ESP font
    ImFont* espFontCfg = io.Fonts->AddFontFromMemoryTTF(
        (void*)font_ttf, (int)sizeof(font_ttf), 18.f, &cfg);

    fontBold   = mainFont;
    fontMedium = mainFont;
    fontDesc   = espFontCfg;
    espFont    = espFontCfg;

    io.FontDefault = mainFont;
    io.Fonts->Build();

    ImGui::GetStyle().ScaleAllSizes(1.25f);

    ::g_Initialized = true;
    return true;
}

void drawBegin() {
    screen_config();
    if (::orientation != displayInfo.orientation) {
        ::orientation = displayInfo.orientation;
        touch::update(displayInfo.width, displayInfo.height, displayInfo.orientation);
        if (g_window) {
            g_window->Pos.x = 100;
            g_window->Pos.y = 125;
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(native_window_screen_x, native_window_screen_y);
    ImGui::NewFrame();
}

void drawEnd() {
    ImGui::Render();
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    eglSwapBuffers(display, surface);
}

void shutdown() {
    if (!g_Initialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();

    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
        if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
        eglTerminate(display);
    }
    display = EGL_NO_DISPLAY;
    context = EGL_NO_CONTEXT;
    surface = EGL_NO_SURFACE;

    ANativeWindow_release(native_window);
    android::ANativeWindowCreator::Destroy(native_window);
    ::g_Initialized = false;
}