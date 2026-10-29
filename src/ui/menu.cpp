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
    static constexpr float Rt = 6.f;

    // ============================================
    // TYPOGRAPHY (SCALED FOR MOBILE / HIGH-DPI)
    // ============================================
    static constexpr float FONT_TITLE   = 16.f;
    static constexpr float FONT_MAIN    = 15.f;
    static constexpr float FONT_SEC     = 13.f;
    static constexpr float FONT_HINT    = 12.f;

    // Toast status for Save/Load/Reset operations
    static std::string s_saveStatus;
    static float s_saveStatusTimer = 0.f;

    // ============================================
    // ANIMATION
    // ============================================
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

    // ============================================
    // COLOR HELPERS
    // ============================================
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
    static inline ImU32 C_AccentDim()   { return col(UITheme::accent, content_alpha * 0.25f); }
    static inline ImU32 C_Success()     { return IM_COL32(52, 199, 89, (int)(content_alpha * 255)); }
    static inline ImU32 C_Danger()      { return IM_COL32(255, 69, 58, (int)(content_alpha * 255)); }

    // ============================================
    // CORE COMPONENTS
    // ============================================
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

    static bool NS_Toggle(const char* id, const char* label, bool* value) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = 34.f;
        bool pressed = ImGui::InvisibleButton("btn", ImVec2(avail, h));
        bool hover = ImGui::IsItemHovered();
        bool clicked = pressed && !ImGui::IsMouseDragging(0, 5.0f);
        if (clicked) *value = !*value;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            std::string key = std::string(id) + "#t";
            float on = NS_Anim(key+"on", *value ? 1.f : 0.f, 16.f);
            float hov = NS_Anim(key+"hov", hover ? 1.f : 0.f, 18.f);
            float lh = ImGui::CalcTextSize(label).y;
            ImU32 lblCol = NS_Lerp(C_TextSec(), C_TextMain(), ImMax(on, hov*0.5f));
            dl->AddText(ImGui::GetFont(), FONT_MAIN,
                {pos.x, pos.y+(h-lh)*0.5f}, lblCol, label);
            float tw = 40.f, th = 22.f;
            float tx = pos.x + avail - tw, ty = pos.y + (h - th) * 0.5f;
            dl->AddRectFilled({tx, ty}, {tx+tw, ty+th},
                NS_Lerp(IM_COL32(42,42,50,(int)(content_alpha*255)), C_Accent(), on), th*0.5f);
            if (on > 0.5f) {
                dl->AddRect({tx, ty}, {tx+tw, ty+th},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)(80*on) << 24),
                    th*0.5f, 0, 1.f);
            }
            float knobR = th*0.5f - 2.5f;
            float knobX = tx + 2.5f + (tw - th) * on;
            dl->AddCircleFilled({knobX+knobR, ty+th*0.5f}, knobR,
                on > 0.5f ? IM_COL32(255,255,255,255) : IM_COL32(200,200,210,255));
        }
        ImGui::PopID();
        return clicked;
    }

    static bool NS_Slider(const char* id, const char* label, float* value,
                          float minV, float maxV, const char* suffix = "") {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            char valStr[32]; snprintf(valStr, sizeof(valStr), "%.2f%s", *value, suffix);
            float valW = ImGui::CalcTextSize(valStr).x + 18.f;
            dl->AddText(ImGui::GetFont(), FONT_MAIN, {pos.x, pos.y+2}, C_TextSec(), label);
            float vx = pos.x + avail - valW;
            dl->AddRectFilled({vx, pos.y}, {vx+valW, pos.y+22}, C_BgDark(), 4.f);
            dl->AddRect({vx, pos.y}, {vx+valW, pos.y+22}, C_Border(), 4.f, 0, 1.f);
            dl->AddText(ImGui::GetFont(), FONT_SEC,
                {vx + 8, pos.y + 4}, C_TextMain(), valStr);

            float ty = pos.y + 30.f, tw2 = avail;
            float rawPct = (maxV > minV) ? ImClamp((*value-minV)/(maxV-minV),0.f,1.f) : 0.f;
            float pct = NS_Anim(std::string(id)+"#pct", rawPct, 22.f);
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2, ty+4},
                IM_COL32(42,42,50,(int)(content_alpha*255)), 2.f);
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2*pct, ty+4}, C_Accent(), 2.f);
            if (pct > 0.01f) {
                dl->AddRect({pos.x, ty}, {pos.x+tw2*pct, ty+4},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)90 << 24), 2.f, 0, 1.f);
            }
            dl->AddCircleFilled({pos.x+tw2*pct, ty+2.f}, 7.f, IM_COL32(255,255,255,255));
            dl->AddCircle({pos.x+tw2*pct, ty+2.f}, 7.f, C_Accent(), 16, 1.5f);
            ImGui::SetCursorScreenPos({pos.x, ty-6});
            ImGui::SetNextItemWidth(tw2);
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
            if (accent && hov > 0.1f) {
                dl->AddRect(a, b, (C_AccentBright() & 0x00FFFFFF) | ((ImU32)(120*hov) << 24),
                            6.f, 0, 1.f);
            }
            if (!accent) {
                dl->AddRect(a, b, C_Border(), 6.f, 0, 1.f);
            }
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

    static void NS_InfoRow(const char* label, const char* value, ImU32 valueColor = 0) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float avail = ImGui::GetContentRegionAvail().x;
        float rh = 32.f;
        bool hov = ImGui::IsMouseHoveringRect(pos, {pos.x+avail, pos.y+rh});
        if (dl) {
            if (hov) {
                dl->AddRectFilled({pos.x, pos.y}, {pos.x+avail, pos.y+rh}, C_BgCardHov(), 4.f);
                dl->AddRectFilled({pos.x, pos.y+6}, {pos.x+2, pos.y+rh-6}, C_Accent(), 1.f);
            }
            dl->AddText(ImGui::GetFont(), FONT_MAIN, {pos.x+10, pos.y+7}, C_TextDim(), label);
            float vw = ImGui::CalcTextSize(value).x;
            ImU32 fc = valueColor ? valueColor : C_TextMain();
            dl->AddText(ImGui::GetFont(), FONT_MAIN, {pos.x+avail-vw-10, pos.y+7}, fc, value);
        }
        ImGui::Dummy(ImVec2(0, rh+2));
    }

    static bool NS_ComboRow(const char* id, const char* label, int* value,
                            const char** items, int itemCount) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = 38.f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            dl->AddRectFilled(pos, {pos.x+avail, pos.y+h}, C_BgDark(), 6.f);
            dl->AddRect(pos, {pos.x+avail, pos.y+h}, C_Border(), 6.f, 0, 1.f);
            dl->AddText(ImGui::GetFont(), FONT_MAIN, {pos.x+12, pos.y+10}, C_TextDim(), label);
            int cur = *value;
            const char* curText = (cur>=0 && cur<itemCount) ? items[cur] : "?";
            float cw = ImGui::CalcTextSize(curText).x;
            dl->AddText(ImGui::GetFont(), FONT_MAIN,
                {pos.x+avail-cw-32, pos.y+10}, C_TextMain(), curText);
            dl->AddText(ImGui::GetFont(), FONT_SEC,
                {pos.x+avail-20, pos.y+11}, C_TextDim(), "\u25BC");
        }
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,  IM_COL32(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_PopupBg,        C_BgCard());
        ImGui::PushStyleColor(ImGuiCol_Header,         C_Accent());
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,  C_AccentBright());
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,   C_AccentBright());
        ImGui::PushStyleColor(ImGuiCol_Text,           C_TextMain());
        ImGui::SetCursorScreenPos({pos.x+4, pos.y+4});
        ImGui::SetNextItemWidth(avail-8);
        bool changed = ImGui::Combo("##combo", value, items, itemCount);
        ImGui::PopStyleColor(8);
        ImGui::PopID();
        ImGui::Dummy(ImVec2(0, 4));
        return changed;
    }

    static void NS_ColorEdit(const char* id, const char* label, ImVec4* color) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            dl->AddText(ImGui::GetFont(), FONT_MAIN, {pos.x, pos.y+8}, C_TextSec(), label);
            ImU32 colU32 = ImGui::ColorConvertFloat4ToU32(*color);
            float sw = 26.f;
            ImVec2 sp = {pos.x + avail - sw - 8.f, pos.y + 4.f};
            dl->AddRectFilled(sp, {sp.x+sw, sp.y+sw}, colU32, 4.f);
            dl->AddRect(sp, {sp.x+sw, sp.y+sw}, C_Border(), 4.f, 0, 1.f);
        }
        ImGui::SetCursorScreenPos({pos.x + avail - 70.f, pos.y + 4.f});
        ImGui::ColorEdit4("##ce_edit", &color->x,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
            ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview);
        ImGui::Dummy({0, 4.f});
        ImGui::PopID();
    }

    // ============================================
    // TAB 0: DASHBOARD
    // ============================================
    static void tab_dashboard(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 190.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("Information"), "Info");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        NS_InfoRow(oxorany("Client"), oxorany("BO CLIENT"), C_Accent());
        NS_InfoRow(oxorany("Version"), oxorany("2.0 VIP"));
        NS_InfoRow(oxorany("Dev"), oxorany("@bodz520"));
        char libBuf[64];
        snprintf(libBuf, sizeof(libBuf), "0x%lx", proc::lib);
        NS_InfoRow(oxorany("Library"), libBuf);

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        float card2H = 110.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("Live Stats"));
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        char fpsT[16]; snprintf(fpsT, sizeof(fpsT), "%.0f", ImGui::GetIO().Framerate);
        ImU32 fc = ImGui::GetIO().Framerate > 55 ? C_Success()
                 : (ImGui::GetIO().Framerate > 30 ? IM_COL32(255,180,40,(int)(content_alpha*255)) : C_Danger());
        dl->AddText(ImGui::GetFont(), 30.f, {cp2.x+20, cp2.y+50}, fc, fpsT);
        dl->AddText(ImGui::GetFont(), FONT_SEC,
            {cp2.x+24+ImGui::CalcTextSize(fpsT).x * 30.f / ImGui::GetFontSize(), cp2.y+66},
            C_TextDim(), "FPS");

        char scr[32];
        snprintf(scr, sizeof(scr), "%.0f x %.0f", g_sw, g_sh);
        float sx = cp2.x + cw*0.5f;
        dl->AddText(ImGui::GetFont(), FONT_SEC, {sx, cp2.y+52}, C_TextDim(), "SCREEN");
        dl->AddText(ImGui::GetFont(), FONT_MAIN, {sx, cp2.y+70}, C_TextMain(), scr);

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0,0});

        float card3H = 65.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("Match Status"));
        bool inM = data::inMatch.load();
        ImU32 mCol = inM ? C_Success() : C_TextDim();
        dl->AddText(ImGui::GetFont(), FONT_MAIN,
            {cp3.x+16, cp3.y+42}, mCol,
            inM ? "In Match" : "Not in Match");

        char pbuf[32];
        snprintf(pbuf, sizeof(pbuf), "%zu players", data::players.size());
        float pw = ImGui::CalcTextSize(pbuf).x;
        dl->AddText(ImGui::GetFont(), FONT_SEC,
            {cp3.x+cw-pw-16, cp3.y+42}, C_TextSec(), pbuf);

        ImGui::SetCursorScreenPos({cp3.x, cp3.y+card3H+12});
        ImGui::Dummy({0, 40});
    }

    // ============================================
    // TAB 1: AIMBOT (DEDICATED SECTION)
    // ============================================
    static void tab_aim(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;
        float colW = (cw - 32 - 24) * 0.5f;

        // ── Card 1: Master & Triggers ──
        float card1H = 210.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("Aimbot & Aim Lock"), "Master");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        ImGui::PushID("aim_cols_m");
        ImGui::Columns(2, "aim_cols_id", false);
        ImGui::SetColumnWidth(0, colW + 12);
        ImGui::SetColumnWidth(1, colW + 12);

        NS_Toggle("aim_enable",   oxorany("Aimbot Master"),    &cfg::aim::enable);
        NS_Toggle("aim_lock",     oxorany("Aim Lock"),         &cfg::aim::aim_lock);
        NS_Toggle("aim_vis",      oxorany("Aim Visible"),      &cfg::aim::aim_visible);

        ImGui::NextColumn();

        NS_Toggle("aim_fire",     oxorany("Aim Fire"),         &cfg::aim::aim_fire);
        NS_Toggle("aim_scope",    oxorany("Aim Scope"),        &cfg::aim::aim_scope);
        NS_Toggle("aim_recoil",   oxorany("No Recoil"),        &cfg::aim::no_recoil);

        ImGui::Columns(1);
        ImGui::PopID();

        // Status pill
        {
            bool active = cfg::aim::enable;
            char st[64];
            snprintf(st, sizeof(st), "%s", active ? "ACTIVE" : "INACTIVE");
            ImU32 stCol = active ? C_Success() : C_TextDim();
            float sw = ImGui::CalcTextSize(st).x;
            dl->AddText(ImGui::GetFont(), FONT_MAIN,
                {cp.x + cw - sw - 16, cp.y + 14}, stCol, st);
        }

        dl->AddText(ImGui::GetFont(), FONT_HINT,
            {cp.x+16, cp.y+180}, C_TextDim(),
            oxorany("* Aim Lock engages aim assistance when crosshair is moved over designated target."));

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        // ── Card 2: Targeting & Auto Drag ──
        float card2H = 180.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("Targeting & Bone Selection"), "Bone");
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        const char* target_bones[] = { "Aim Head", "Aim Neck", "Aim Body" };
        NS_ComboRow("aim_bone", oxorany("Target Bone"), &cfg::aim::target_bone, target_bones, 3);
        NS_Toggle("aim_auto_drag", oxorany("Auto Drag Headshot"), &cfg::aim::auto_drag);

        if (cfg::aim::auto_drag) {
            dl->AddText(ImGui::GetFont(), FONT_SEC,
                {cp2.x+16, cp2.y+138}, C_AccentBright(),
                oxorany("* Body aim normal | Light upward swipe locks to Head"));
        }

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0,0});

        // ── Card 3: FOV & Smoothness Tuning ──
        float card3H = 240.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("Field Of View & Smoothness"), "Tuning");
        ImGui::SetCursorScreenPos({cp3.x+16, cp3.y+44});

        NS_Toggle("aim_draw_fov", oxorany("Show FOV Circle"), &cfg::aim::draw_fov);
        NS_Slider("aim_fov",      oxorany("FOV Radius"),      &cfg::aim::fov,        30.f, 400.f, "px");
        NS_Slider("aim_range",    oxorany("Aim Range"),       &cfg::aim::max_range,  30.f, 400.f, "m");
        NS_Slider("aim_smooth",   oxorany("Aim Smoothness"),  &cfg::aim::aim_smooth, 0.05f, 0.85f, "");

        ImGui::SetCursorScreenPos({cp3.x, cp3.y+card3H+12});
        ImGui::Dummy({0, 40});
    }

    // ============================================
    // TAB 2: ESP (DEDICATED SECTION)
    // ============================================
    static void tab_esp(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float colW = (cw - 32 - 24) * 0.5f;
        float card1H = 300.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("ESP Visuals"), "Master");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        ImGui::PushID("esp_cols");
        ImGui::Columns(2, "esp_cols_id", false);
        ImGui::SetColumnWidth(0, colW + 12);
        ImGui::SetColumnWidth(1, colW + 12);

        NS_Toggle("esp_box",     oxorany("Box"),       &cfg::esp::box);
        NS_Toggle("esp_name",    oxorany("Name"),      &cfg::esp::name);
        NS_Toggle("esp_health",  oxorany("Health"),    &cfg::esp::health);
        NS_Toggle("esp_armor",   oxorany("Armor"),     &cfg::esp::armor);
        NS_Toggle("esp_dist",    oxorany("Distance"),  &cfg::esp::distance);
        NS_Toggle("esp_bot",     oxorany("Bot Tag"),   &cfg::esp::bot_tag);

        ImGui::NextColumn();

        NS_Toggle("esp_tracer",  oxorany("Tracer"),    &cfg::esp::tracer);
        NS_Toggle("esp_skel",    oxorany("Skeleton"),  &cfg::esp::skeleton);
        NS_Toggle("esp_cross",   oxorany("Crosshair"), &cfg::esp::crosshair);
        NS_Toggle("esp_weapon",  oxorany("Weapon"),    &cfg::esp::weapon);
        NS_Toggle("esp_fire",    oxorany("Firing"),    &cfg::esp::firing_ind);
        NS_Toggle("esp_loot",    oxorany("Loot"),      &cfg::esp::loot);

        ImGui::Columns(1);
        ImGui::PopID();

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        // ── Card 2: Box Settings ──
        float card2H = 220.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("Box Settings"));
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        const char* box_types[] = { oxorany("Corner"), oxorany("Full"), oxorany("Glow") };
        NS_ComboRow("box_type", oxorany("Box Type"), &cfg::esp::box_type, box_types, 3);
        NS_Slider("box_th", oxorany("Thickness"),  &cfg::esp::box_thickness, 0.5f, 5.0f, "");
        NS_Slider("box_rd", oxorany("Rounding"),   &cfg::esp::box_rounding,  0.f,  10.f, "");
        NS_Slider("box_rg", oxorany("Max Range"),  &cfg::esp::max_range,     50.f, 500.f, "m");

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0,0});

        // ── Card 3: Loot Settings ──
        float card3H = 110.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("Loot Settings"));
        ImGui::SetCursorScreenPos({cp3.x+16, cp3.y+44});

        NS_Slider("loot_rg", oxorany("Loot Range"), &cfg::esp::loot_range, 30.f, 300.f, "m");

        ImGui::SetCursorScreenPos({cp3.x, cp3.y+card3H+12});
        ImGui::Dummy({0,0});

        // ── Card 4: Colors ──
        float card4H = 200.f;
        ImVec2 cp4 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp4, cw, card4H);
        NS_CardTitle(dl, {cp4.x+16, cp4.y+14}, oxorany("Colors"));
        ImGui::SetCursorScreenPos({cp4.x+16, cp4.y+44});
        NS_ColorEdit("col_box",  oxorany("Box"),       &cfg::esp::box_col);
        NS_ColorEdit("col_vis",  oxorany("Visible"),   &cfg::esp::box_visible);
        NS_ColorEdit("col_name", oxorany("Name"),      &cfg::esp::name_col);
        NS_ColorEdit("col_dist", oxorany("Distance"),  &cfg::esp::distance_col);

        ImGui::SetCursorScreenPos({cp4.x, cp4.y+card4H+12});
        ImGui::Dummy({0, 40});
    }

    // ============================================
    // TAB 3: SETTINGS
    // ============================================
    static void tab_settings(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        // ── Card 1: Display ──
        float card1H = 120.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, oxorany("Display"));
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        char buf[64];
        snprintf(buf, sizeof(buf), "%.0f x %.0f", g_sw, g_sh);
        NS_InfoRow(oxorany("Screen"), buf);
        snprintf(buf, sizeof(buf), "%.1f", ImGui::GetIO().Framerate);
        NS_InfoRow(oxorany("FPS"), buf);

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        // ── Card 2: System ──
        float card2H = 130.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, oxorany("System"));
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        snprintf(buf, sizeof(buf), "0x%lx", proc::lib);
        NS_InfoRow(oxorany("Library"), buf);
        snprintf(buf, sizeof(buf), "%d", proc::pid);
        NS_InfoRow(oxorany("PID"), buf);

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0,0});

        // ── Card 3: Configuration Management (Save/Load/Reset Clarification) ──
        float card3H = 180.f;
        ImVec2 cp3 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp3, cw, card3H);
        NS_CardTitle(dl, {cp3.x+16, cp3.y+14}, oxorany("Configuration Management"), "Storage");
        ImGui::SetCursorScreenPos({cp3.x+16, cp3.y+42});

        dl->AddText(ImGui::GetFont(), FONT_SEC, {cp3.x+16, cp3.y+44}, C_TextMain(),
            oxorany("• Save: Writes all ESP, Aim Lock, FOV & Tuning to /data/local/tmp/.bo_client.cfg"));
        dl->AddText(ImGui::GetFont(), FONT_SEC, {cp3.x+16, cp3.y+66}, C_TextDim(),
            oxorany("• Load: Restores your saved configuration from device storage."));
        dl->AddText(ImGui::GetFont(), FONT_SEC, {cp3.x+16, cp3.y+88}, C_TextDim(),
            oxorany("• Reset: Reverts all options back to clean defaults."));

        ImGui::SetCursorScreenPos({cp3.x+16, cp3.y+120});
        if (NS_Button("set_save", oxorany("Save Config"), {110, 34}, true)) {
            bool ok = cfg::save();
            s_saveStatus = ok ? "Config saved successfully!" : "Failed to write config!";
            s_saveStatusTimer = 3.5f;
        }
        ImGui::SameLine(0, 10);
        if (NS_Button("set_load", oxorany("Load Config"), {110, 34}, false)) {
            bool ok = cfg::load();
            s_saveStatus = ok ? "Config loaded from disk!" : "No config file found!";
            s_saveStatusTimer = 3.5f;
        }
        ImGui::SameLine(0, 10);
        if (NS_Button("set_reset", oxorany("Reset Defaults"), {110, 34}, false)) {
            cfg::reset_defaults();
            s_saveStatus = "Settings reset to defaults!";
            s_saveStatusTimer = 3.5f;
        }

        ImGui::SetCursorScreenPos({cp3.x, cp3.y+card3H+12});
        ImGui::Dummy({0,0});

        // ── Card 4: Actions ──
        float card4H = 96.f;
        ImVec2 cp4 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp4, cw, card4H);
        NS_CardTitle(dl, {cp4.x+16, cp4.y+14}, oxorany("Actions"));
        ImGui::SetCursorScreenPos({cp4.x+16, cp4.y+44});

        if (NS_Button("unload", oxorany("Unload Service"), {cw-32, 34}, true)) {
            exit(0);
        }

        ImGui::SetCursorScreenPos({cp4.x, cp4.y+card4H+12});
        ImGui::Dummy({0, 40});
    }

    // ============================================
    // MAIN RENDER
    // ============================================
    void render() {
        float dt = ImGui::GetIO().DeltaTime;
        aimbot::TickTimer(dt > 0.f ? dt : 0.016f);
        if (ImGui::GetIO().MouseDown[0]) {
            aimbot::NotifyTouch(true, ImGui::GetIO().MouseDelta.y);
        }

        func::esp::draw();
        bar::render();

        ma = ma + ((bar::g_open ? 1.f : 0.f) - ma) * ImClamp(12.f*dt, 0.f, 1.f);

        if (ma > 0.01f) {
            int da = (int)(200*ma*bar::game_alpha());
            ImGui::GetBackgroundDrawList()->AddRectFilled({0,0},{g_sw,g_sh},IM_COL32(0,0,0,da));
        }
        if (ma < 0.01f) return;

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ma);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0,0});
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 22.f);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 6.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 36.f);
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 12.f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_Border, {0,0,0,0});

        float targetW = ImMin(980.f, (g_sw > 300.f) ? (g_sw - 30.f) : 980.f);
        float targetH = ImMin(650.f, (g_sh > 300.f) ? (g_sh - 30.f) : 650.f);
        ImVec2 wsz(targetW, targetH);
        if (g_menu_pos.x < 0.f || g_menu_pos.y < 0.f) {
            g_menu_pos = {(g_sw - wsz.x) * 0.5f, (g_sh - wsz.y) * 0.5f};
        }
        if (g_menu_pos.x < 10.f) g_menu_pos.x = 10.f;
        if (g_menu_pos.y < 10.f) g_menu_pos.y = 10.f;
        ImGui::SetNextWindowSize(wsz, ImGuiCond_Always);
        ImGui::SetNextWindowPos(g_menu_pos, ImGuiCond_Always);

        ImGui::Begin("##m", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        ImVec2 mp = ImGui::GetMousePos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImDrawList* bg = ImGui::GetBackgroundDrawList();

        bg->AddRectFilled({wp.x-6, wp.y-6}, {wp.x+ws.x+6, wp.y+ws.y+6},
                          IM_COL32(0,0,0,(int)(35*ma)), R+5.f);
        bg->AddRectFilled({wp.x-12, wp.y-12}, {wp.x+ws.x+12, wp.y+ws.y+12},
                          IM_COL32(0,0,0,(int)(15*ma)), R+10.f);

        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::bg, ma), R);
        bg->AddRect(wp, {wp.x+ws.x, wp.y+ws.y}, col(UITheme::border, ma), R);

        const float headerH = 60.f;
        const float sidebarW = 150.f;
        const float footerH = 50.f;

        bg->AddRectFilled(wp, {wp.x+ws.x, wp.y+headerH},
                          col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersTop);
        bg->AddLine({wp.x, wp.y+headerH}, {wp.x+ws.x, wp.y+headerH},
                    col(UITheme::border, ma), 1.f);

        float logoSize = 34.f;
        float logoX = wp.x + 18.f;
        float logoY = wp.y + (headerH - logoSize) * 0.5f;
        bg->AddRectFilled({logoX, logoY}, {logoX+logoSize, logoY+logoSize},
                          col(UITheme::accent, ma), 7.f);
        ImVec2 ls = ImGui::CalcTextSize("N");
        bg->AddText({logoX + (logoSize-ls.x)*0.5f, logoY + (logoSize-ls.y)*0.5f},
                    IM_COL32(255,255,255,255), "N");

        dl->AddText(ImGui::GetFont(), FONT_TITLE + 1.f, {logoX + logoSize + 14.f, wp.y + 12.f},
                    col(UITheme::text, ma), "BO CLIENT");
        dl->AddText(ImGui::GetFont(), FONT_HINT, {logoX + logoSize + 14.f, wp.y + 34.f},
                    col(UITheme::text_dim, ma), "FREE FIRE");

        bg->AddLine({logoX + logoSize + 115.f, wp.y+18.f},
                    {logoX + logoSize + 115.f, wp.y+42.f},
                    col(UITheme::border, ma), 1.f);

        const char* tabTitles[] = {"Dashboard", "Aimbot", "ESP", "Settings"};
        bg->AddText(ImGui::GetFont(), FONT_SEC,
            {logoX + logoSize + 130.f, wp.y + 24.f},
            col(UITheme::text_dim, ma), "/");
        bg->AddText(ImGui::GetFont(), FONT_TITLE,
            {logoX + logoSize + 144.f, wp.y + 22.f},
            col(UITheme::text, ma), tabTitles[tab]);

        float closeW = 32.f;
        float closeX = wp.x + ws.x - closeW - 14.f;
        float closeY = wp.y + (headerH - closeW) * 0.5f;
        bool hoverClose = ImGui::IsMouseHoveringRect({closeX, closeY},
                                                      {closeX+closeW, closeY+closeW});
        if (hoverClose) bg->AddRectFilled({closeX, closeY},
                                          {closeX+closeW, closeY+closeW},
                                          col(UITheme::danger, ma), 5.f);
        float cX = closeX+16, cY = closeY+16, cr = 5.f;
        bg->AddLine({cX-cr,cY-cr},{cX+cr,cY+cr},
            hoverClose ? IM_COL32(255,255,255,255) : col(UITheme::text_dim, ma), 1.8f);
        bg->AddLine({cX-cr,cY+cr},{cX+cr,cY-cr},
            hoverClose ? IM_COL32(255,255,255,255) : col(UITheme::text_dim, ma), 1.8f);

        ImGui::SetCursorScreenPos({closeX, closeY});
        ImGui::PushID(9999);
        if (ImGui::InvisibleButton("hide", {closeW, closeW})) bar::g_open = false;
        ImGui::PopID();

        float sbY = wp.y + headerH;
        float sbBot = wp.y + ws.y - footerH;
        bg->AddRectFilled({wp.x, sbY}, {wp.x+sidebarW, sbBot},
                          col(UITheme::sidebar, ma));
        bg->AddLine({wp.x+sidebarW, sbY}, {wp.x+sidebarW, sbBot},
                    col(UITheme::border, ma), 1.f);

        float navStartY = sbY + 16.f;
        float navItemH = 42.f;
        for (int i = 0; i < 4; i++) {
            float ny = navStartY + i * (navItemH + 4);
            bool act = (tab == i);
            bool hov = ImGui::IsMouseHoveringRect({wp.x, ny},
                                                   {wp.x+sidebarW, ny+navItemH});
            if (act) {
                bg->AddRectFilled({wp.x, ny}, {wp.x+sidebarW, ny+navItemH},
                                  col(UITheme::accent, ma * 0.25f));
                bg->AddRectFilled({wp.x, ny}, {wp.x+4, ny+navItemH},
                                  col(UITheme::accent, ma));
            } else if (hov) {
                bg->AddRectFilled({wp.x, ny}, {wp.x+sidebarW, ny+navItemH},
                                  col(UITheme::widget, ma));
            }
            char numBuf[4]; snprintf(numBuf, sizeof(numBuf), "%02d", i+1);
            dl->AddText(ImGui::GetFont(), FONT_SEC,
                {wp.x+14, ny+14}, act ? col(UITheme::accent, ma) : col(UITheme::text_dim, ma), numBuf);
            ImU32 txtCol = act ? col(UITheme::text, ma) :
                          (hov ? col(UITheme::text, ma) : col(UITheme::text_dim, ma));
            dl->AddText(ImGui::GetFont(), FONT_MAIN,
                {wp.x+40, ny+12}, txtCol, tabTitles[i]);
            ImGui::SetCursorScreenPos({wp.x, ny});
            ImGui::PushID(i+500);
            if (ImGui::InvisibleButton("nav", {sidebarW, navItemH})) tab = i;
            ImGui::PopID();
        }

        float sFootY = sbBot - 44.f;
        bg->AddLine({wp.x+12, sFootY}, {wp.x+sidebarW-12, sFootY},
                    col(UITheme::border, ma), 1.f);
        dl->AddText(ImGui::GetFont(), FONT_SEC, {wp.x+14, sFootY+10},
                    col(UITheme::text_dim, ma), "BO CLIENT v2.0");
        dl->AddText(ImGui::GetFont(), FONT_HINT, {wp.x+14, sFootY+26},
                    col(UITheme::accent, ma), "@bodz520");

        float contentX = wp.x + sidebarW + 20.f;
        float contentY = wp.y + headerH + 18.f;
        float contentW = ws.x - sidebarW - 40.f;
        float contentH = ws.y - headerH - footerH - 54.f;

        ImGui::SetCursorScreenPos({contentX, contentY});

        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg,           IM_COL32(24, 24, 32, (int)(ma * 160)));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab,         col(UITheme::accent, ma * 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered,  col(UITheme::accent_light, ma));
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive,   col(UITheme::accent_light, ma));

        ImGui::BeginChild("content", {contentW, contentH}, false,
                          ImGuiWindowFlags_NoBackground);

        // Mobile touch swipe scrolling
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows) && ImGui::IsMouseDragging(0, 2.5f)) {
            float dy = ImGui::GetIO().MouseDelta.y;
            ImGui::SetScrollY(ImGui::GetScrollY() - dy);
        }

        dl = ImGui::GetWindowDrawList();
        content_w = ImGui::GetContentRegionAvail().x;
        content_alpha = ma;

        switch (tab) {
            case 0: tab_dashboard(ma); break;
            case 1: tab_aim(ma); break;
            case 2: tab_esp(ma); break;
            case 3: tab_settings(ma); break;
        }

        ImGui::EndChild();

        ImGui::PopStyleColor(4);

        float fy = wp.y + ws.y - footerH;
        bg->AddRectFilled({wp.x, fy}, {wp.x+ws.x, wp.y+ws.y},
                          col(UITheme::sidebar, ma), R, ImDrawFlags_RoundCornersBottom);
        bg->AddLine({wp.x, fy}, {wp.x+ws.x, fy}, col(UITheme::border, ma), 1.f);

        float fbX = wp.x + 18.f;
        float fbY = fy + 8.f;
        ImGui::SetCursorScreenPos({fbX, fbY});
        if (NS_Button("f_save", "Save", {85, 34}, true)) {
            bool ok = cfg::save();
            s_saveStatus = ok ? "Config Saved!" : "Save Error!";
            s_saveStatusTimer = 3.5f;
        }
        ImGui::SameLine(0, 8);
        if (NS_Button("f_load", "Load", {85, 34}, false)) {
            bool ok = cfg::load();
            s_saveStatus = ok ? "Config Loaded!" : "No File!";
            s_saveStatusTimer = 3.5f;
        }
        ImGui::SameLine(0, 8);
        if (NS_Button("f_reset", "Reset", {80, 34}, false)) {
            cfg::reset_defaults();
            s_saveStatus = "Reset to Defaults!";
            s_saveStatusTimer = 3.5f;
        }

        if (s_saveStatusTimer > 0.f) {
            ImGui::SameLine(0, 14);
            dl->AddText(ImGui::GetFont(), FONT_MAIN,
                {wp.x + 295.f, fy + 16.f},
                C_Success(), s_saveStatus.c_str());
        }

        const char* fh = "@bodz520";
        float fhw = ImGui::CalcTextSize(fh).x;
        dl->AddText(ImGui::GetFont(), FONT_SEC,
            {wp.x + ws.x - fhw - 90, fy + 17}, col(UITheme::accent, ma), fh);
        const char* fv = "V2.0 VIP";
        float fvw = ImGui::CalcTextSize(fv).x;
        dl->AddText(ImGui::GetFont(), FONT_SEC,
            {wp.x + ws.x - fvw - 16, fy + 17}, col(UITheme::text_dim, ma), fv);

        if ((mp.x>=wp.x && mp.x<=wp.x+ws.x && mp.y>=wp.y && mp.y<=wp.y+headerH)) {
            if (ImGui::IsMouseClicked(0)) {
                drag = true;
                doff = {mp.x-wp.x, mp.y-wp.y};
            }
        }
        if (drag) {
            if (ImGui::IsMouseDown(0)) {
                ImVec2 np{mp.x-doff.x, mp.y-doff.y};
                np.x = ImClamp(np.x, 0.f, g_sw-ws.x);
                np.y = ImClamp(np.y, 0.f, g_sh-ws.y);
                g_menu_pos = np;
            } else {
                drag = false;
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(12);
    }
}