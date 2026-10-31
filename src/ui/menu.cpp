#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "Android_draw/draw.h"
#include "protect/oxorany.hpp"
#include "styles/styles.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <inttypes.h>
#include <unordered_map>
#include "func/esp.hpp"
#include "cfg.hpp"
#include "game/aimbot.hpp"

namespace ui::menu {
    using namespace style;

    // ============================================
    // STATE
    // ============================================
    static float  ma     = 0.f;
    static int    tab    = 0;
    static bool   drag   = false;
    static ImVec2 doff(0, 0);
    static ImVec2 g_menu_pos(-1.f, -1.f);

    static constexpr float R  = 10.f;

    // Typography
    static constexpr float FONT_TITLE   = 16.f;
    static constexpr float FONT_MAIN    = 15.f;
    static constexpr float FONT_SEC     = 13.f;
    static constexpr float FONT_HINT    = 12.f;

    // Status toast for Save / Load
    static std::string s_saveStatus;
    static float s_saveStatusTimer = 0.f;

    // Animation helper
    static std::unordered_map<std::string, float> g_nsAnim;
    static inline float NS_Anim(const std::string& key, float target, float speed = 14.f) {
        float dt = ImGui::GetIO().DeltaTime; if (dt <= 0.f) dt = 1.f/60.f;
        auto it = g_nsAnim.find(key);
        if (it == g_nsAnim.end()) { g_nsAnim[key] = target; return target; }
        float cur = it->second;
        float t = 1.f - std::exp(-speed * dt);
        cur = cur + (target - cur) * t;
        if (std::fabs(cur - target) < 0.001f) cur = target;
        it->second = cur; return cur;
    }
    static inline ImU32 NS_Lerp(ImU32 a, ImU32 b, float t) {
        t = ImClamp(t, 0.f, 1.f);
        int ar=(a>>16)&0xFF, ag=(a>>8)&0xFF, ab=a&0xFF, aa=(a>>24)&0xFF;
        int br=(b>>16)&0xFF, bg=(b>>8)&0xFF, bb=b&0xFF, ba=(b>>24)&0xFF;
        return IM_COL32((int)(ar+(br-ar)*t),(int)(ag+(bg-ag)*t),
                        (int)(ab+(bb-ab)*t),(int)(aa+(ba-aa)*t));
    }

    // Color helpers
    static inline ImU32 C_TextMain()    { return col(UITheme::text,       content_alpha); }
    static inline ImU32 C_TextSec()     { return col(UITheme::text_dim,   content_alpha); }
    static inline ImU32 C_TextDim()     { return col(UITheme::text_dim,   content_alpha * 0.7f); }
    static inline ImU32 C_BgDark()      { return col(UITheme::bg,         content_alpha); }
    static inline ImU32 C_BgSidebar()   { return col(UITheme::sidebar,    content_alpha); }
    static inline ImU32 C_BgCard()      { return col(UITheme::panel,      content_alpha); }
    static inline ImU32 C_BgCardHov()   { return col(UITheme::widget,     content_alpha); }
    static inline ImU32 C_Border()      { return col(UITheme::border,     content_alpha); }
    static inline ImU32 C_Accent()      { return col(UITheme::accent,     content_alpha); }
    static inline ImU32 C_AccentBright(){ return col(UITheme::accent_light, content_alpha); }
    static inline ImU32 C_Success()     { return IM_COL32(52, 199, 89, (int)(content_alpha * 255)); }
    static inline ImU32 C_Danger()      { return IM_COL32(255, 69, 58, (int)(content_alpha * 255)); }

    // Core Components
    static void NS_CardBegin(ImDrawList* dl, ImVec2 pos, float w, float h) {
        dl->AddRectFilled(pos, {pos.x+w, pos.y+h}, C_BgCard(), 8.f);
        dl->AddRect(pos, {pos.x+w, pos.y+h}, C_Border(), 8.f, 0, 1.f);
    }

    static void NS_CardTitle(ImDrawList* dl, ImVec2 pos, const char* title, const char* hint = nullptr) {
        dl->AddRectFilled({pos.x, pos.y+2}, {pos.x+3, pos.y+16}, C_Accent(), 2.f);
        dl->AddText(ImGui::GetFont(), FONT_TITLE, {pos.x+10, pos.y}, C_TextSec(), title);
        if (hint) {
            float hw = ImGui::CalcTextSize(hint).x;
            dl->AddText(ImGui::GetFont(), FONT_SEC,
                {pos.x + ImGui::GetContentRegionAvail().x - hw, pos.y+2},
                C_TextDim(), hint);
        }
    }

    static bool NS_Toggle(const char* id, const char* label, bool* value, const char* desc = nullptr) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = desc ? 44.f : 34.f;

        bool hover = ImGui::IsMouseHoveringRect(pos, {pos.x+avail, pos.y+h});
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            std::string key = std::string(id)+"#t";
            float on = NS_Anim(key+"on", *value ? 1.f : 0.f, 16.f);

            if (hover) dl->AddRectFilled(pos, {pos.x+avail, pos.y+h}, C_BgCardHov(), 4.f);

            dl->AddText(ImGui::GetFont(), FONT_MAIN, {pos.x+8, pos.y+6}, C_TextMain(), label);
            if (desc) {
                dl->AddText(ImGui::GetFont(), FONT_HINT, {pos.x+8, pos.y+24}, C_TextDim(), desc);
            }

            float swW = 38.f, swH = 20.f;
            float swX = pos.x + avail - swW - 10.f;
            float swY = pos.y + (h - swH) * 0.5f;

            ImU32 trackCol = NS_Lerp(IM_COL32(50, 50, 60, (int)(content_alpha*255)), C_Accent(), on);
            dl->AddRectFilled({swX, swY}, {swX+swW, swY+swH}, trackCol, swH*0.5f);

            float knobR = (swH - 4.f) * 0.5f;
            float knobX = swX + 2.f + knobR + on * (swW - swH);
            float knobY = swY + swH * 0.5f;
            dl->AddCircleFilled({knobX, knobY}, knobR, IM_COL32(255, 255, 255, (int)(content_alpha*255)));
        }

        ImGui::SetCursorScreenPos(pos);
        if (ImGui::InvisibleButton("tog", {avail, h})) {
            *value = !*value;
            ImGui::PopID();
            return true;
        }
        ImGui::PopID();
        return false;
    }

    static bool NS_SliderFloat(const char* id, const char* label, float* value, float minV, float maxV, const char* fmt = "%.1f") {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        if (dl) {
            dl->AddText(ImGui::GetFont(), FONT_MAIN, {pos.x, pos.y+2}, C_TextMain(), label);
            char valStr[32]; snprintf(valStr, sizeof(valStr), fmt, *value);
            float valW = ImGui::CalcTextSize(valStr).x + 16.f;
            float vx = pos.x + avail - valW;
            dl->AddRectFilled({vx, pos.y}, {vx+valW, pos.y+20}, C_BgDark(), 4.f);
            dl->AddRect({vx, pos.y}, {vx+valW, pos.y+20}, C_Border(), 4.f, 0, 1.f);
            dl->AddText(ImGui::GetFont(), FONT_SEC, {vx+8, pos.y+2}, C_TextMain(), valStr);

            float ty = pos.y + 28.f, tw = avail;
            float rawPct = (maxV > minV) ? ImClamp((*value-minV)/(maxV-minV), 0.f, 1.f) : 0.f;
            float pct = NS_Anim(std::string(id)+"#pct", rawPct, 20.f);
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw, ty+4}, IM_COL32(40,40,48,(int)(content_alpha*255)), 2.f);
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw*pct, ty+4}, C_Accent(), 2.f);
            dl->AddCircleFilled({pos.x+tw*pct, ty+2.f}, 6.f, IM_COL32(255,255,255,255));
            dl->AddCircle({pos.x+tw*pct, ty+2.f}, 6.f, C_Accent(), 16, 1.5f);
            ImGui::SetCursorScreenPos({pos.x, ty-6});
            ImGui::SetNextItemWidth(tw);
        }
        bool changed = ImGui::SliderFloat("##s", value, minV, maxV, "");
        ImGui::Dummy(ImVec2(0, 12));
        ImGui::PopID();
        return changed;
    }

    static bool NS_Button(const char* id, const char* label, ImVec2 size, bool accent = true) {
        ImGui::PushID(id);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        bool hover = ImGui::IsMouseHoveringRect(pos, {pos.x+size.x, pos.y+size.y});
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            std::string key = std::string(id)+"#b";
            float hov = NS_Anim(key+"hov", hover?1.f:0.f, 20.f);
            ImVec2 a = pos, b = {pos.x+size.x, pos.y+size.y};
            ImU32 bg = accent ? NS_Lerp(C_Accent(), C_AccentBright(), hov)
                              : NS_Lerp(C_BgCard(), C_BgCardHov(), hov);
            dl->AddRectFilled(a, b, bg, 6.f);
            if (!accent) dl->AddRect(a, b, C_Border(), 6.f, 0, 1.f);
            float lw = ImGui::CalcTextSize(label).x;
            float lh = ImGui::CalcTextSize(label).y;
            dl->AddText(ImGui::GetFont(), FONT_MAIN,
                {pos.x + (size.x-lw)*0.5f, pos.y + (size.y-lh)*0.5f},
                accent ? IM_COL32(255,255,255,255) : C_TextMain(), label);
        }
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("btn", size);
        bool clicked = ImGui::IsItemClicked();
        ImGui::Dummy(size);
        ImGui::PopID();
        return clicked;
    }

    // ============================================
    // FLOATING ACTION HUD BUTTONS (Interactive On-Screen Controls)
    // ============================================
    static void RenderFloatingHud() {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;

        struct HudBtn {
            const char* id;
            const char* title;
            bool* toggleFeature;
            bool* showInHud;
            float* px;
            float* py;
            ImU32 activeCol;
        };

        HudBtn buttons[] = {
            { "f_ghost",    "GHOST",      &cfg::ghost::enable,     &cfg::floating_hud::show_ghost,     &cfg::floating_hud::pos_ghost_x,     &cfg::floating_hud::pos_ghost_y,     IM_COL32(52, 199, 89, 230) },
            { "f_aimkill",  "AIMKILL",    &cfg::aimkill::enable,   &cfg::floating_hud::show_aimkill,   &cfg::floating_hud::pos_aimkill_x,   &cfg::floating_hud::pos_aimkill_y,   IM_COL32(255, 69, 58, 230) },
            { "f_telemark", "TELEMARK",   &cfg::telemark::enable,  &cfg::floating_hud::show_telemark,  &cfg::floating_hud::pos_telemark_x,  &cfg::floating_hud::pos_telemark_y,  IM_COL32(0, 122, 255, 230) },
            { "f_speed",    "SPEEDTIME",  &cfg::speedtime::enable, &cfg::floating_hud::show_speedtime, &cfg::floating_hud::pos_speedtime_x, &cfg::floating_hud::pos_speedtime_y, IM_COL32(255, 149, 0, 230) }
        };

        const float btnW = 96.f;
        const float btnH = 40.f;
        ImVec2 mp = ImGui::GetMousePos();

        for (auto& b : buttons) {
            // Only draw if enabled from Main Menu
            if (!*b.showInHud) continue;

            ImVec2 pos(*b.px, *b.py);
            pos.x = ImClamp(pos.x, 10.f, g_sw - btnW - 10.f);
            pos.y = ImClamp(pos.y, 10.f, g_sh - btnH - 10.f);
            *b.px = pos.x;
            *b.py = pos.y;

            bool hover = (mp.x >= pos.x && mp.x <= pos.x + btnW &&
                          mp.y >= pos.y && mp.y <= pos.y + btnH);

            static std::string s_activeDrag = "";
            static ImVec2 s_dragOffset(0, 0);

            if (hover && ImGui::IsMouseClicked(0) && s_activeDrag.empty()) {
                s_activeDrag = b.id;
                s_dragOffset = { mp.x - pos.x, mp.y - pos.y };
            }

            if (s_activeDrag == b.id) {
                if (ImGui::IsMouseDown(0)) {
                    *b.px = ImClamp(mp.x - s_dragOffset.x, 10.f, g_sw - btnW - 10.f);
                    *b.py = ImClamp(mp.y - s_dragOffset.y, 10.f, g_sh - btnH - 10.f);
                    pos = ImVec2(*b.px, *b.py);
                } else {
                    s_activeDrag = "";
                }
            }

            ImGui::SetCursorScreenPos(pos);
            ImGui::PushID(b.id);
            if (ImGui::InvisibleButton("btn_action", {btnW, btnH})) {
                *b.toggleFeature = !*b.toggleFeature;
            }
            ImGui::PopID();

            bool active = *b.toggleFeature;
            ImU32 bgCol = active ? b.activeCol : IM_COL32(28, 28, 36, 220);
            ImU32 borderCol = active ? IM_COL32(255, 255, 255, 240) : IM_COL32(65, 65, 80, 180);

            dl->AddRectFilled(pos, {pos.x + btnW, pos.y + btnH}, bgCol, 8.f);
            dl->AddRect(pos, {pos.x + btnW, pos.y + btnH}, borderCol, 8.f, 0, 1.5f);

            float tw = ImGui::CalcTextSize(b.title).x;
            float th = ImGui::CalcTextSize(b.title).y;
            dl->AddText(ImGui::GetFont(), FONT_MAIN,
                {pos.x + (btnW - tw)*0.5f, pos.y + (btnH - th)*0.5f - 1.f},
                IM_COL32(255, 255, 255, 255), b.title);

            ImU32 dotCol = active ? IM_COL32(255, 255, 255, 255) : IM_COL32(120, 120, 130, 180);
            dl->AddCircleFilled({pos.x + btnW - 8.f, pos.y + 8.f}, 3.5f, dotCol);
        }
    }

    // ============================================
    // TAB 0: COMBAT (Aimbot & AimKill)
    // ============================================
    static void tab_combat(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 220.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "Aimbot", "Targeting Engine");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        NS_Toggle("aim_en", "Enable Aimbot", &cfg::aim::enable, "Master combat targeting override");
        NS_Toggle("aim_lock", "Camera Aim Lock", &cfg::aim::aim_lock, "Align player view angle towards target");
        NS_Toggle("aim_vis", "Visible Check Only", &cfg::aim::aim_visible, "Skip targets obstructed by walls");
        NS_Toggle("aim_knk", "Ignore Knocked Enemies", &cfg::aim::ignore_knocked, "Skip knocked down / dying players");

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+14});
        ImGui::Dummy({0,0});

        float card2H = 240.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, "AimKill & Under Kill", "Exploits");
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        NS_Toggle("ak_en", "AimKill (10ms Limit)", &cfg::aimkill::enable, "Instant lethal head hit dispatch (clamped to 10ms)");
        NS_Toggle("uk_en", "Under Kill", &cfg::underkill::enable, "Adjust bullet trajectory below ground level");
        NS_SliderFloat("uk_depth", "Under Kill Depth", &cfg::underkill::depth_offset, 1.0f, 5.0f, "%.1fm");
        NS_SliderFloat("aim_fov", "Aim FOV Radius", &cfg::aim::fov, 30.f, 360.f, "%.0f px");

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+14});
        ImGui::Dummy({0, 30});
    }

    // ============================================
    // TAB 1: ESP (BO Client Visuals Preserved)
    // ============================================
    static void tab_esp(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 260.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "Player ESP", "Visuals");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        NS_Toggle("esp_box", "2D Bounding Box", &cfg::esp::box, "Draw player bounding box");
        NS_Toggle("esp_name", "Player Name", &cfg::esp::name, "Display player nicknames");
        NS_Toggle("esp_hp", "Health & Armor Bar", &cfg::esp::health, "Display HP bar (alive players only)");
        NS_Toggle("esp_dist", "Distance", &cfg::esp::distance, "Display distance in meters");
        NS_Toggle("esp_tracer", "Snapline / Tracer", &cfg::esp::tracer, "Draw tracer line to player");

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+14});
        ImGui::Dummy({0,0});

        float card2H = 190.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, "Radar & Settings", "HUD");
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        NS_Toggle("esp_radar", "360 Threat Radar", &cfg::esp::offscreen_radar, "Off-screen threat directional indicators");
        NS_Toggle("esp_dot", "Head Dot Point", &cfg::esp::head_dot, "Draw dot indicator on player head");
        NS_SliderFloat("esp_range", "Maximum Range", &cfg::esp::max_range, 50.f, 400.f, "%.0fm");

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+14});
        ImGui::Dummy({0, 30});
    }

    // ============================================
    // TAB 2: EXPLOITS & MOVEMENT
    // ============================================
    static void tab_exploits(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 260.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "Movement & Weapon Exploits", "Mechanics");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        NS_Toggle("gh_en", "Ghost Mode", &cfg::ghost::enable, "Desync player state / packet freeze");
        NS_Toggle("nr_en", "Non-Recoil", &cfg::nonrecoil::enable, "Zero bullet spread and weapon recoil");
        NS_Toggle("tm_en", "Auto TeleMark", &cfg::telemark::enable, "Teleport directly behind target player");
        NS_Toggle("st_en", "Speed Time", &cfg::speedtime::enable, "Engine timescale velocity acceleration");
        NS_SliderFloat("st_scale", "Speed Multiplier", &cfg::speedtime::scale, 1.0f, 5.0f, "%.1fx");

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+14});
        ImGui::Dummy({0, 30});
    }

    // ============================================
    // TAB 3: FLOATING ACTION HUD BUTTONS ("Nút Nổi")
    // ============================================
    static void tab_floating_hud(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 260.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "On-Screen Action Buttons", "Floating HUD");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        NS_Toggle("hud_gh", "Show [GHOST] Button", &cfg::floating_hud::show_ghost, "Display floating Ghost button on HUD");
        NS_Toggle("hud_ak", "Show [AIMKILL] Button", &cfg::floating_hud::show_aimkill, "Display floating AimKill button on HUD");
        NS_Toggle("hud_tm", "Show [TELEMARK] Button", &cfg::floating_hud::show_telemark, "Display floating TeleMark button on HUD");
        NS_Toggle("hud_st", "Show [SPEEDTIME] Button", &cfg::floating_hud::show_speedtime, "Display floating SpeedTime button on HUD");

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+14});
        ImGui::Dummy({0,0});

        float card2H = 110.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, "Position Reset", "Controls");
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        if (NS_Button("rst_pos", "Reset Button Positions", {200, 34}, false)) {
            cfg::floating_hud::pos_ghost_x     = 80.f;  cfg::floating_hud::pos_ghost_y     = 240.f;
            cfg::floating_hud::pos_aimkill_x   = 80.f;  cfg::floating_hud::pos_aimkill_y   = 310.f;
            cfg::floating_hud::pos_telemark_x  = 80.f;  cfg::floating_hud::pos_telemark_y  = 380.f;
            cfg::floating_hud::pos_speedtime_x = 80.f;  cfg::floating_hud::pos_speedtime_y = 450.f;
        }

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+14});
        ImGui::Dummy({0, 30});
    }

    // ============================================
    // TAB 4: BYPASS & SYSTEM
    // ============================================
    static void tab_bypass(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 220.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "Anti-Cheat Protection", "Bypass");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        NS_Toggle("bp_en", "Master Bypass Engine", &cfg::bypass::enable, "External anti-cheat filter");
        NS_Toggle("bp_libc", "Libc Cloaking (/proc/self/maps)", &cfg::bypass::libc_stealth, "Mask module memory segments");
        NS_Toggle("bp_tele", "Suppress Telemetry Logs", &cfg::bypass::telemetry_nullify, "Block SendEventLogJson & MSDK");

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+14});
        ImGui::Dummy({0, 30});
    }

    // ============================================
    // MAIN RENDER LOOP
    // ============================================
    void render() {
        float dt = ImGui::GetIO().DeltaTime;
        aimbot::TickTimer(dt > 0.f ? dt : 0.016f);

        // Draw BO Client ESP
        func::esp::draw();

        // Render On-Screen Floating Action HUD Buttons
        RenderFloatingHud();

        // Overlay Navigation Trigger Bar
        bar::render();

        ma = ma + ((bar::g_open ? 1.f : 0.f) - ma) * ImClamp(12.f*dt, 0.f, 1.f);
        if (ma > 0.01f) {
            int da = (int)(200*ma*bar::game_alpha());
            ImGui::GetBackgroundDrawList()->AddRectFilled({0,0},{g_sw,g_sh},IM_COL32(0,0,0,da));
        }
        if (ma < 0.01f) {
            drag = false;
            return;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ma);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 22.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_Border, {0,0,0,0});

        float targetW = ImMin(880.f, (g_sw > 300.f) ? (g_sw - 30.f) : 880.f);
        float targetH = ImMin(600.f, (g_sh > 300.f) ? (g_sh - 30.f) : 600.f);
        ImVec2 wsz(targetW, targetH);
        if (g_menu_pos.x < 0.f || g_menu_pos.y < 0.f) {
            g_menu_pos = {(g_sw - wsz.x) * 0.5f, (g_sh - wsz.y) * 0.5f};
        }
        ImGui::SetNextWindowSize(wsz, ImGuiCond_Always);
        ImGui::SetNextWindowPos(g_menu_pos, ImGuiCond_Always);

        ImGui::Begin("##m", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        ImVec2 mp = ImGui::GetMousePos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImDrawList* bg = ImGui::GetBackgroundDrawList();

        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::bg, ma), R);
        bg->AddRect(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::border, ma), R);

        const float headerH = 56.f;
        const float sidebarW = 160.f;
        const float footerH = 48.f;

        // Header
        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+headerH}, col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersTop);
        bg->AddLine({wp.x, wp.y+headerH}, {wp.x+ws.x, wp.y+headerH}, col(UITheme::border, ma), 1.f);

        dl->AddText(ImGui::GetFont(), FONT_TITLE + 1.f, {wp.x + 20.f, wp.y + 16.f}, col(UITheme::text, ma), "BO CLIENT · VIP EXTERNAL");

        // Close button
        float closeW = 32.f;
        float closeX = wp.x + ws.x - closeW - 14.f;
        float closeY = wp.y + (headerH - closeW) * 0.5f;
        if (ImGui::IsMouseHoveringRect({closeX, closeY}, {closeX+closeW, closeY+closeW})) {
            bg->AddRectFilled({closeX, closeY}, {closeX+closeW, closeY+closeW}, col(UITheme::danger, ma), 5.f);
        }
        ImGui::SetCursorScreenPos({closeX, closeY});
        if (ImGui::InvisibleButton("hide", {closeW, closeW})) bar::g_open = false;

        // Sidebar Navigation
        float sbY = wp.y + headerH;
        float sbBot = wp.y + ws.y - footerH;
        bg->AddRectFilled({wp.x, sbY}, {wp.x+sidebarW, sbBot}, col(UITheme::sidebar, ma));
        bg->AddLine({wp.x+sidebarW, sbY}, {wp.x+sidebarW, sbBot}, col(UITheme::border, ma), 1.f);

        const char* tabTitles[] = { "Combat / Aim", "Visuals / ESP", "Exploits", "Floating HUD", "Bypass" };
        for (int i = 0; i < 5; i++) {
            float ny = sbY + 14.f + i * 44.f;
            bool act = (tab == i);
            if (act) {
                bg->AddRectFilled({wp.x, ny}, {wp.x+sidebarW, ny+38.f}, col(UITheme::accent, ma * 0.25f));
                bg->AddRectFilled({wp.x, ny}, {wp.x+4.f, ny+38.f}, col(UITheme::accent, ma));
            }
            dl->AddText(ImGui::GetFont(), FONT_MAIN, {wp.x+18.f, ny+10.f},
                        act ? col(UITheme::text, ma) : col(UITheme::text_dim, ma), tabTitles[i]);
            ImGui::SetCursorScreenPos({wp.x, ny});
            ImGui::PushID(i+500);
            if (ImGui::InvisibleButton("nav", {sidebarW, 38.f})) tab = i;
            ImGui::PopID();
        }

        // Content Area
        float contentX = wp.x + sidebarW + 20.f;
        float contentY = wp.y + headerH + 16.f;
        float contentW = ws.x - sidebarW - 40.f;
        float contentH = ws.y - headerH - footerH - 32.f;

        ImGui::SetCursorScreenPos({contentX, contentY});
        ImGui::BeginChild("content", {contentW, contentH}, false, ImGuiWindowFlags_NoBackground);
        content_w = ImGui::GetContentRegionAvail().x;
        content_alpha = ma;

        switch (tab) {
            case 0: tab_combat(ma); break;
            case 1: tab_esp(ma); break;
            case 2: tab_exploits(ma); break;
            case 3: tab_floating_hud(ma); break;
            case 4: tab_bypass(ma); break;
        }

        ImGui::EndChild();

        // Footer
        float fy = wp.y + ws.y - footerH;
        bg->AddRectFilled({wp.x, fy}, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersBottom);
        bg->AddLine({wp.x, fy}, {wp.x+ws.x, fy}, col(UITheme::border, ma), 1.f);

        ImGui::SetCursorScreenPos({wp.x + 18.f, fy + 8.f});
        if (NS_Button("f_save", "Save Config", {95, 32}, true)) {
            cfg::save();
            s_saveStatus = "Config Saved!";
            s_saveStatusTimer = 3.f;
        }
        ImGui::SameLine(0, 8);
        if (NS_Button("f_load", "Load Config", {95, 32}, false)) {
            cfg::load();
            s_saveStatus = "Config Loaded!";
            s_saveStatusTimer = 3.f;
        }
        ImGui::SameLine(0, 8);
        if (NS_Button("f_reset", "Reset All", {90, 32}, false)) {
            cfg::reset_defaults();
            s_saveStatus = "Defaults Reset!";
            s_saveStatusTimer = 3.f;
        }

        if (s_saveStatusTimer > 0.f) {
            ImGui::SameLine(0, 12);
            dl->AddText(ImGui::GetFont(), FONT_MAIN, {wp.x + 320.f, fy + 16.f}, C_Success(), s_saveStatus.c_str());
            s_saveStatusTimer -= dt;
        }

        // Window drag handling
        if (mp.x >= wp.x && mp.x <= (wp.x + ws.x - closeW - 20.f) && mp.y >= wp.y && mp.y <= (wp.y + headerH)) {
            if (ImGui::IsMouseClicked(0)) {
                drag = true;
                doff = {mp.x - wp.x, mp.y - wp.y};
            }
        }
        if (drag) {
            if (ImGui::IsMouseDown(0)) {
                g_menu_pos = { mp.x - doff.x, mp.y - doff.y };
            } else {
                drag = false;
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(4);
    }
}
