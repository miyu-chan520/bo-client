#pragma once
#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <unordered_map>
#include <cmath>
#include <algorithm>

inline float g_sw, g_sh;

namespace UITheme {
    // BACKGROUND
    inline ImVec4 bg           = ImVec4(8/255.f, 8/255.f, 10/255.f, 1.f);
    inline ImVec4 bg_two       = ImVec4(12/255.f, 12/255.f, 15/255.f, 1.f);
    inline ImVec4 sidebar      = ImVec4(12/255.f, 12/255.f, 15/255.f, 1.f);
    inline ImVec4 panel        = ImVec4(18/255.f, 18/255.f, 22/255.f, 1.f);
    inline ImVec4 widget       = ImVec4(26/255.f, 26/255.f, 32/255.f, 1.f);

    // ACCENT — MERAH
    inline ImVec4 accent       = ImVec4(180/255.f, 30/255.f, 30/255.f, 1.f);
    inline ImVec4 accent_light = ImVec4(220/255.f, 40/255.f, 40/255.f, 1.f);
    inline ImVec4 accent_dark  = ImVec4(120/255.f, 20/255.f, 20/255.f, 1.f);

    // SAKURA THEME (From PS RiverMod Sakura UI: #B30E0E0E & #FF4B8B / #FF758C)
    inline ImVec4 sakura_bg        = ImVec4(14/255.f, 14/255.f, 18/255.f, 0.92f);
    inline ImVec4 sakura_panel     = ImVec4(24/255.f, 20/255.f, 28/255.f, 0.90f);
    inline ImVec4 sakura_widget    = ImVec4(34/255.f, 28/255.f, 40/255.f, 0.95f);
    inline ImVec4 sakura_accent    = ImVec4(255/255.f, 75/255.f, 139/255.f, 1.f);   // #FF4B8B
    inline ImVec4 sakura_light     = ImVec4(255/255.f, 117/255.f, 140/255.f, 1.f);  // #FF758C
    inline ImVec4 sakura_dark      = ImVec4(180/255.f, 40/255.f, 95/255.f, 1.f);
    inline ImVec4 sakura_purple    = ImVec4(155/255.f, 89/255.f, 182/255.f, 1.f);

    // TEXT
    inline ImVec4 text         = ImVec4(230/255.f, 230/255.f, 235/255.f, 1.f);
    inline ImVec4 text_light   = ImVec4(230/255.f, 230/255.f, 235/255.f, 1.f);
    inline ImVec4 text_dim     = ImVec4(140/255.f, 140/255.f, 150/255.f, 1.f);

    // STATUS
    inline ImVec4 success      = ImVec4(52/255.f, 199/255.f, 89/255.f, 1.f);
    inline ImVec4 warning      = ImVec4(255/255.f, 180/255.f, 40/255.f, 1.f);
    inline ImVec4 danger       = ImVec4(255/255.f, 69/255.f, 58/255.f, 1.f);

    // BORDER
    inline ImVec4 border       = ImVec4(35/255.f, 35/255.f, 42/255.f, 1.f);
    inline ImVec4 border_light = ImVec4(50/255.f, 50/255.f, 60/255.f, 1.f);
    inline ImVec4 border_dark  = ImVec4(0/255.f, 0/255.f, 0/255.f, 1.f);
    inline ImVec4 subtab_bg    = ImVec4(12/255.f, 12/255.f, 15/255.f, 1.f);

    // BUTTON
    inline ImVec4 ButtonColor   = ImVec4(180/255.f, 30/255.f, 30/255.f, 1.f);
    inline ImVec4 ButtonHovered = ImVec4(220/255.f, 40/255.f, 40/255.f, 1.f);
    inline ImVec4 ButtonActive  = ImVec4(120/255.f, 20/255.f, 20/255.f, 1.f);

    inline void ButtonStyle() {
        ImGui::PushStyleColor(ImGuiCol_Button, ButtonColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ButtonHovered);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ButtonActive);
    }
    inline void PopButtonStyle() {
        ImGui::PopStyleColor(3);
    }
}


namespace style {
    inline std::unordered_map<std::string, float> anims;
    inline std::unordered_map<std::string, ImVec4> anim_colors;
    inline float content_w = 0.f;
    inline float content_alpha = 1.f;
    inline bool popup_open = false;
    inline std::string active_popup = "";
    inline constexpr float S = 2.5f;

    static float dt = 0.016f;
    static bool clicked = false;

    static inline float lrp(float a, float b, float t) { return a + (b - a) * t; }
    static inline ImVec4 lrp_col(const ImVec4& a, const ImVec4& b, float t) {
        return ImVec4(lrp(a.x, b.x, t), lrp(a.y, b.y, t), lrp(a.z, b.z, t), lrp(a.w, b.w, t));
    }

    inline void tick() {
        clicked = false;
        static float lt = 0.f;
        float ct = ImGui::GetTime();
        if (lt > 0.f) { dt = ct - lt; dt = ImClamp(dt, 0.001f, 0.1f); }
        lt = ct;
    }

    inline float anim(const std::string& id, float tgt, float spd = 12.f) {
        auto it = anims.find(id);
        if (it == anims.end()) { anims[id] = tgt; return tgt; }
        float t = ImClamp(spd * dt, 0.f, 1.f);
        it->second = lrp(it->second, tgt, t);
        if (fabsf(it->second - tgt) < 0.001f) it->second = tgt;
        return it->second;
    }

    inline ImVec4 anim_col(const std::string& id, const ImVec4& tgt, float spd = 12.f) {
        auto it = anim_colors.find(id);
        if (it == anim_colors.end()) { anim_colors[id] = tgt; return tgt; }
        float t = ImClamp(spd * dt, 0.f, 1.f);
        it->second = lrp_col(it->second, tgt, t);
        return it->second;
    }

    inline ImU32 col(const ImVec4& c, float a = 1.f) {
        return IM_COL32((int)(c.x * 255), (int)(c.y * 255), (int)(c.z * 255), (int)(c.w * 255 * a * content_alpha));
    }

    inline bool popup() { return popup_open || !active_popup.empty(); }
    inline void close_popup() { active_popup = ""; popup_open = false; }

    inline void popups() {
        if (active_popup.empty()) return;
        float pa = anim(active_popup + "_pa", 1.f, 18.f);
        if (pa < 0.01f) { active_popup = ""; return; }
        if (pa > 0.15f && ImGui::IsMouseClicked(0) && !clicked) { active_popup = ""; popup_open = false; }
    }
}