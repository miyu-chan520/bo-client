#include "bar.hpp"
#include "styles/styles.h"
#include "nexora_logo.h"        // Logo header
#include "imgui.h"
#include "imgui_internal.h"
#include <GLES3/gl3.h>           // OpenGL ES 3 texture support
#include <cmath>
#include <cstdio>

namespace ui::bar {
    using namespace style;

    static float lrp(float a, float b, float t) { return a + (b - a) * t; }
    static float ease(float t) { return -(cosf(3.14159265f * t) - 1.f) / 2.f; }

    void set_game_alpha(float a) { g_game_alpha = a; }
    float game_alpha() { return g_game_alpha; }

    // ============================================
    // LOGO TEXTURE
    // ============================================
    static GLuint g_logo_tex = 0;
    static bool   g_logo_loaded = false;

    static void load_logo_texture_once() {
        if (g_logo_loaded) return;
        if (logo_width <= 0 || logo_height <= 0) return;

        glGenTextures(1, &g_logo_tex);
        glBindTexture(GL_TEXTURE_2D, g_logo_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     logo_width, logo_height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, logo_data);
        glBindTexture(GL_TEXTURE_2D, 0);

        g_logo_loaded = true;
    }

    // ============================================
    // ICON FPS
    // ============================================
    static float  g_fps_x = -1.f;
    static float  g_fps_y = -1.f;
    static bool   g_fps_drag = false;
    static bool   g_fps_potential = false;
    static ImVec2 g_fps_off(0, 0);
    static ImVec2 g_fps_down(0, 0);
    static constexpr float FPS_DRAG_THRESHOLD = 10.f;

    static void get_fps_pos(float w, float h) {
        if (g_fps_x < 0.f || g_fps_y < 0.f) {
            g_fps_x = g_sw - w - 25.f;
            g_fps_y = g_sh - h - 25.f;
        }
        if (g_sw > 0) g_fps_x = ImMax(0.f, ImMin(g_fps_x, g_sw - w));
        if (g_sh > 0) g_fps_y = ImMax(0.f, ImMin(g_fps_y, g_sh - h));
    }

    static void render_fps_icon(float alpha) {
        load_logo_texture_once();      // ← ensure texture loaded

        const float w = 128.f, h = 56.f;
        get_fps_pos(w, h);

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 mp = ImGui::GetMousePos();
        bool hov = (mp.x >= g_fps_x && mp.x <= g_fps_x + w &&
                    mp.y >= g_fps_y && mp.y <= g_fps_y + h);

        // Detect drag start
        if (hov && ImGui::IsMouseClicked(0)) {
            g_fps_potential = true;
            g_fps_down = mp;
            g_fps_off = ImVec2(mp.x - g_fps_x, mp.y - g_fps_y);
        }

        // Drag
        if (g_fps_potential && ImGui::IsMouseDown(0)) {
            float dx = mp.x - g_fps_down.x;
            float dy = mp.y - g_fps_down.y;
            if (g_fps_drag || (dx*dx + dy*dy) > (FPS_DRAG_THRESHOLD * FPS_DRAG_THRESHOLD)) {
                g_fps_drag = true;
                g_fps_x = mp.x - g_fps_off.x;
                g_fps_y = mp.y - g_fps_off.y;
                get_fps_pos(w, h);
            }
        }

        // Release → click = buka menu
        if (ImGui::IsMouseReleased(0)) {
            if (g_fps_potential && !g_fps_drag) {
                g_open = true;
            }
            g_fps_potential = false;
            g_fps_drag = false;
        }

        float fps = ImGui::GetIO().Framerate;

        ImU32 bg_col     = IM_COL32(8, 8, 10, (int)(230 * alpha));
        ImU32 border_col = g_fps_drag ? col(UITheme::accent_light, alpha)
                                      : col(UITheme::border, alpha);

        // Background pill
        float pillR = h * 0.5f;
        dl->AddRectFilled({g_fps_x + 2, g_fps_y + 3}, {g_fps_x + w + 2, g_fps_y + h + 3},
                          IM_COL32(0, 0, 0, (int)(60 * alpha)), pillR);
        dl->AddRectFilled({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h},
                          bg_col, pillR);
        dl->AddRect({g_fps_x, g_fps_y}, {g_fps_x + w, g_fps_y + h},
                    border_col, pillR, 0, g_fps_drag ? 2.f : 1.2f);

        // Accent strip kiri (merah BS)
        dl->AddRectFilled({g_fps_x + 4.f, g_fps_y + 4.f},
                          {g_fps_x + 7.f, g_fps_y + h - 4.f},
                          col(UITheme::accent, alpha), 2.f);

        // ============================================
        // LOGO (texture, bukan font)
        // ============================================
        float cx = g_fps_x + 34.f, cy = g_fps_y + h * 0.5f, radius = 18.f;

        if (g_logo_loaded && g_logo_tex != 0) {
            // Gambar logo dari texture
            dl->AddImageRounded(
                (ImTextureID)(intptr_t)g_logo_tex,
                {cx - radius, cy - radius},
                {cx + radius, cy + radius},
                {0, 0}, {1, 1},
                IM_COL32(255, 255, 255, (int)(255 * alpha)),
                radius
            );
        } else {
            // Fallback: red accent container with badge
            dl->AddCircleFilled({cx, cy}, radius, col(UITheme::accent, alpha));
        }

        // ============================================
        // FPS indicator text
        // ============================================
        // Simplified geometric placeholder rendering
        // Standard ImGui font fallback
        // Text rendering configuration in draw.cpp

        int fpsInt = (int)(fps + 0.5f);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d FPS", fpsInt);

        ImFont* font = ImGui::GetFont();
        if (font) {
            ImU32 text_col = fps >= 50.f ? IM_COL32(52, 199, 89, 255)
                            : fps >= 30.f ? IM_COL32(255, 180, 40, 255)
                                          : IM_COL32(255, 69, 58, 255);
            dl->AddText(font, 18.f,
                        {g_fps_x + 58.f, g_fps_y + (h - 18.f) * 0.5f},
                        text_col, buf);
        }
    }

    // ============================================
    // Render FPS status widget only when main menu is collapsed
    // ============================================
    void render() {
        static float prog = 0.f;
        float dt = ImGui::GetIO().DeltaTime;

        float tgt = g_open ? 1.f : 0.f;
        float spd = 12.f * dt;
        if (prog < tgt) prog = ImMin(prog + spd, tgt);
        else if (prog > tgt) prog = ImMax(prog - spd, tgt);
        prog = ImClamp(prog, 0.f, 1.f);

        g_alpha = ease(prog);

        if (g_game_alpha > 0.001f && !g_open) {
            render_fps_icon(1.f);
        }
    }
}