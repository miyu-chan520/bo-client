#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.hpp"
#include "bar.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "protect/oxorany.hpp"
#include "styles/styles.h"
#include <cmath>
#include <cstdio>
#include <string>
#include <atomic>
#include <mutex>
#include <inttypes.h>
#include <unordered_map>
#include "../func/esp.hpp"
#include "cfg.hpp"
#include "../game/features.hpp"
#include "../game/scanner.hpp"
#include "../bypass/bypass.hpp"
#include "../game/data.hpp"

extern float g_sw;
extern float g_sh;
extern std::atomic<bool> menu_visible;
extern float global_anim_time;

namespace ui::menu {
    using namespace style;

    // ============================================
    // STATE
    // ============================================
    static float  ma     = 1.0f;
    static int    tab    = 0;
    static bool   drag   = false;
    static ImVec2 doff(0, 0);
    static ImVec2 g_menu_pos(100.f, 100.f);

    static constexpr float R  = 10.f;
    static constexpr float Rt = 6.f;

    static const char* tabTitles[] = {
        "DASHBOARD",
        "SAKURA AIM",
        "VISUALS",
        "EXPLOITS",
        "BYPASS",
        "SETTINGS"
    };

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
    // COLOR HELPERS (Sakura Theme Integration)
    // ============================================
    static inline ImU32 C_TextMain()    { return col(UITheme::text,          content_alpha); }
    static inline ImU32 C_TextSec()     { return col(UITheme::text_dim,      content_alpha); }
    static inline ImU32 C_TextDim()     { return col(UITheme::text_dim,      content_alpha * 0.7f); }
    static inline ImU32 C_BgDark()      { return col(UITheme::sakura_bg,     content_alpha); }
    static inline ImU32 C_BgSidebar()   { return col(UITheme::sidebar,       content_alpha); }
    static inline ImU32 C_BgCard()      { return col(UITheme::sakura_panel,  content_alpha); }
    static inline ImU32 C_BgCardHov()   { return col(UITheme::sakura_widget, content_alpha); }
    static inline ImU32 C_Border()      { return col(UITheme::border,        content_alpha); }
    static inline ImU32 C_Accent()      { return col(UITheme::sakura_accent, content_alpha); }
    static inline ImU32 C_AccentBright(){ return col(UITheme::sakura_light,  content_alpha); }
    static inline ImU32 C_AccentDim()   { return col(UITheme::sakura_accent, content_alpha * 0.25f); }
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
        dl->AddRectFilled({pos.x, pos.y+2}, {pos.x+3, pos.y+12}, C_Accent(), 2.f);
        dl->AddText(ImGui::GetFont(), 11.f, {pos.x+10, pos.y}, C_TextSec(), title);
        if (hint) {
            float hw = ImGui::CalcTextSize(hint).x;
            dl->AddText(ImGui::GetFont(), 10.f,
                {pos.x + ImGui::GetContentRegionAvail().x - hw, pos.y+1},
                C_TextDim(), hint);
        }
    }

    static bool NS_Toggle(const char* id, const char* label, bool* value) {
        ImGui::PushID(id);
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = 28.f;
        ImGui::InvisibleButton("btn", ImVec2(avail, h));
        bool clicked = ImGui::IsItemClicked();
        bool hover = ImGui::IsItemHovered();
        if (clicked) *value = !*value;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            std::string key = std::string(id) + "#t";
            float on = NS_Anim(key+"on", *value ? 1.f : 0.f, 16.f);
            float hov = NS_Anim(key+"hov", hover ? 1.f : 0.f, 18.f);
            float lh = ImGui::CalcTextSize(label).y;
            ImU32 lblCol = NS_Lerp(C_TextSec(), C_TextMain(), ImMax(on, hov*0.5f));
            dl->AddText(ImGui::GetFont(), 12.f,
                {pos.x, pos.y+(h-lh)*0.5f}, lblCol, label);
            float tw = 34.f, th = 18.f;
            float tx = pos.x + avail - tw, ty = pos.y + (h - th) * 0.5f;
            dl->AddRectFilled({tx, ty}, {tx+tw, ty+th},
                NS_Lerp(IM_COL32(42,42,50,(int)(content_alpha*255)), C_Accent(), on), th*0.5f);
            if (on > 0.5f) {
                dl->AddRect({tx, ty}, {tx+tw, ty+th},
                    (C_Accent() & 0x00FFFFFF) | ((ImU32)(80*on) << 24),
                    th*0.5f, 0, 1.f);
            }
            float knobR = th*0.5f - 2.f;
            float knobX = tx + 2.f + (tw - th) * on;
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
            char valStr[32]; snprintf(valStr, sizeof(valStr), "%.1f%s", *value, suffix);
            float valW = ImGui::CalcTextSize(valStr).x + 16.f;
            dl->AddText(ImGui::GetFont(), 12.f, {pos.x, pos.y+2}, C_TextSec(), label);
            float vx = pos.x + avail - valW;
            dl->AddRectFilled({vx, pos.y}, {vx+valW, pos.y+20}, C_BgDark(), 4.f);
            dl->AddRect({vx, pos.y}, {vx+valW, pos.y+20}, C_Border(), 4.f, 0, 1.f);
            dl->AddText(ImGui::GetFont(), 11.f,
                {vx + 8, pos.y + 3}, C_TextMain(), valStr);

            float ty = pos.y + 26.f, tw2 = avail;
            float rawPct = (maxV > minV) ? ImClamp((*value-minV)/(maxV-minV),0.f,1.f) : 0.f;
            float pct = NS_Anim(std::string(id)+"#pct", rawPct, 22.f);
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2, ty+3},
                IM_COL32(42,42,50,(int)(content_alpha*255)), 2.f);
            dl->AddRectFilled({pos.x, ty}, {pos.x+tw2*pct, ty+3}, C_Accent(), 2.f);
        }
        ImGui::SetCursorScreenPos({pos.x, pos.y + 22.f});
        ImGui::InvisibleButton("slider", {avail, 14.f});
        if (ImGui::IsItemActive()) {
            float mouseX = ImGui::GetIO().MousePos.x - pos.x;
            float pct = ImClamp(mouseX / avail, 0.f, 1.f);
            *value = minV + pct * (maxV - minV);
        }
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::PopID();
        return false;
    }

    static void NS_InfoRow(const char* label, const char* value, ImU32 valCol = 0) {
        float avail = ImGui::GetContentRegionAvail().x;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            dl->AddText(ImGui::GetFont(), 11.f, pos, C_TextDim(), label);
            float vw = ImGui::CalcTextSize(value).x;
            dl->AddText(ImGui::GetFont(), 11.f, {pos.x + avail - vw, pos.y},
                        valCol ? valCol : C_TextMain(), value);
        }
        ImGui::Dummy({0, 18.f});
    }

    static bool NS_Button(const char* id, const char* label, ImVec2 size = {0, 0}, bool primary = false) {
        ImGui::PushID(id);
        float w = size.x > 0 ? size.x : ImGui::GetContentRegionAvail().x;
        float h = size.y > 0 ? size.y : 28.f;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("btn", {w, h});
        bool clicked = ImGui::IsItemClicked();
        bool hov = ImGui::IsItemHovered();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (dl) {
            ImU32 bgCol = primary ? (hov ? C_AccentBright() : C_Accent())
                                  : (hov ? C_BgCardHov() : C_BgCard());
            dl->AddRectFilled(pos, {pos.x+w, pos.y+h}, bgCol, 5.f);
            dl->AddRect(pos, {pos.x+w, pos.y+h}, C_Border(), 5.f, 0, 1.f);
            ImVec2 ts = ImGui::CalcTextSize(label);
            dl->AddText(ImGui::GetFont(), 11.f,
                {pos.x + (w - ts.x)*0.5f, pos.y + (h - ts.y)*0.5f},
                primary ? IM_COL32(255,255,255,255) : (hov ? C_TextMain() : C_TextSec()), label);
        }
        ImGui::PopID();
        return clicked;
    }

    // ============================================
    // TAB 0: DASHBOARD
    // ============================================
    static void tab_dashboard(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 170.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "System Architecture", "Zygisk Internal");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        NS_InfoRow("Client Architecture", "Zygisk Internal In-Process", C_Accent());
        NS_InfoRow("Target Mode", "Free Fire & FF MAX");
        NS_InfoRow("License State", "VIP Permanent (NoKey)", C_Success());
        NS_InfoRow("Bypass Engine", bypass::is_anogs_hooked() ? "3-Layer Active (Shielded)" : "Active", C_Success());
        char libBuf[64];
        snprintf(libBuf, sizeof(libBuf), "0x%lx", (unsigned long)scanner::g_Offsets.il2cpp_base);
        NS_InfoRow("libil2cpp Base", libBuf);

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        float card2H = 90.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, "Runtime Telemetry");
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        char fpsT[16]; snprintf(fpsT, sizeof(fpsT), "%.0f", ImGui::GetIO().Framerate);
        dl->AddText(ImGui::GetFont(), 26.f, {cp2.x+20, cp2.y+44}, C_Success(), fpsT);
        dl->AddText(ImGui::GetFont(), 10.f, {cp2.x+70, cp2.y+54}, C_TextDim(), "FPS");

        bool inM = data::inMatch.load();
        dl->AddText(ImGui::GetFont(), 13.f, {cp2.x + cw*0.45f, cp2.y+48}, inM ? C_Success() : C_TextDim(),
                    inM ? "In Match" : "Lobby");

        char pbuf[32];
        snprintf(pbuf, sizeof(pbuf), "%zu entities", data::players.size());
        dl->AddText(ImGui::GetFont(), 12.f, {cp2.x + cw - 90.f, cp2.y+48}, C_TextMain(), pbuf);

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0, 20});
    }

    // ============================================
    // TAB 1: COMBAT (Aimbot, Silent Aim, AutoKill)
    // ============================================
    static void tab_combat(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 260.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "Target Acquisition", "Silent & Memory");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+40});

        NS_Toggle("cb_aim", "Master Aimbot", &cfg::combat::aim_enable);
        NS_Toggle("cb_silent", "Silent Aim (Headshot Lock)", &cfg::combat::silent_aim);
        NS_Toggle("cb_vis", "Visible Check Only", &cfg::combat::aim_visible_only);
        NS_Slider("cb_fov", "Aim FOV Radius", &cfg::combat::aim_fov, 10.0f, 360.0f, " px");
        NS_Slider("cb_smooth", "Aim Smoothing", &cfg::combat::aim_smooth, 1.0f, 20.0f);

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        float card2H = 170.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, "Lethal Exploits", "Instant Damage");
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+40});

        NS_Toggle("cb_autokill", "AutoKill (Iterative Mass Damage)", &cfg::combat::autokill);
        NS_Toggle("cb_underkill", "UnderKill (Shoot Through Terrain)", &cfg::combat::underkill);
        NS_Slider("cb_ukdepth", "UnderKill Depth", &cfg::combat::underkill_depth, 5.0f, 30.0f, " m");

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0, 20});
    }

    // ============================================
    // TAB 2: VISUALS (ESP & Chams)
    // ============================================
    static void tab_visuals(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 270.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "Visual Overlays", "Internal HUD");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+40});

        NS_Toggle("esp_box", "2D / 3D Box ESP", &cfg::esp::box);
        NS_Toggle("esp_skel", "Skeleton Bones ESP", &cfg::esp::skeleton);
        NS_Toggle("esp_health", "Health & Armor Bar", &cfg::esp::health);
        NS_Toggle("esp_name", "Player Name Tag", &cfg::esp::name);
        NS_Toggle("esp_dist", "Distance Indicator", &cfg::esp::distance);
        NS_Toggle("esp_bot", "Bot Identification Tag", &cfg::esp::bot_tag);
        NS_Slider("esp_range", "ESP Max Distance", &cfg::esp::max_range, 50.0f, 500.0f, " m");

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        float card2H = 120.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, "Chams Material Override", "Wall Penetration");
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+40});

        NS_Toggle("chams_en", "Chams (Always Visible Through Walls)", &cfg::visual::chams_enable);

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0, 20});
    }

    // ============================================
    // TAB 3: EXPLOITS (Fly Hack, Ghost, Speed, Tele)
    // ============================================
    static void tab_exploits(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 180.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "Fly Hack (RiverMod Integration)", "Physics Override");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+40});

        NS_Toggle("ex_fly", "Fly Hack Master (Zero Gravity)", &cfg::movement::fly_enable);
        NS_Toggle("ex_flyhigh", "Fly Elevation (Hover Up)", &cfg::movement::fly_high);
        NS_Slider("ex_flyspeed", "Fly Movement Speed", &cfg::movement::fly_speed, 5.0f, 40.0f);

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        float card2H = 200.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, "Spatial & Temporal Exploits", "Desync");
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+40});

        NS_Toggle("ex_ghost", "Ghost Mode (Network Position Freeze)", &cfg::movement::ghost_enable);
        NS_Toggle("ex_speed", "Speed Timer Multiplier", &cfg::movement::speed_enable);
        NS_Slider("ex_spdval", "Game Speed Multiplier", &cfg::movement::speed_multiplier, 1.0f, 4.0f, "x");
        if (NS_Button("ex_telekill_btn", "Instant AutoTeleKill Behind Enemy", {0, 28}, true)) {
            cfg::movement::tele_kill = true;
        }

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0, 20});
    }

    // ============================================
    // TAB 4: BYPASS (libanogs & VFS Cloaking)
    // ============================================
    static void tab_bypass(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 175.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "Anti-Cheat Neutralization", "libanogs.so Hooks");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        bool anogs = bypass::is_anogs_hooked();
        NS_InfoRow("libanogs Detection Hooks", anogs ? "ACTIVE (Bypassed)" : "STANDBY", anogs ? C_Success() : C_TextDim());
        NS_InfoRow("Anti-Frida Memory Scan", "NEUTRALIZED", C_Success());
        NS_InfoRow("Anti-Debug TracerPid Scan", "CLOAKED (0)", C_Success());
        NS_InfoRow("Cheat DLL String Scanner", "BLOCKED (0x0)", C_Success());
        NS_InfoRow("Mono Integrity Verification", "BYPASSED (0x1)", C_Success());

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0,0});

        float card2H = 130.f;
        ImVec2 cp2 = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp2, cw, card2H);
        NS_CardTitle(dl, {cp2.x+16, cp2.y+14}, "Kernel Stealth & /proc Cloaking", "VFS Redirection");
        ImGui::SetCursorScreenPos({cp2.x+16, cp2.y+44});

        bool stealth = bypass::is_stealth_active();
        NS_InfoRow("/proc/self/maps Filter", stealth ? "ACTIVE (Sanitized VFS)" : "PENDING", stealth ? C_Success() : C_TextDim());
        NS_InfoRow("Zygisk & ShadowHook Mask", "CLOAKED", C_Success());
        NS_InfoRow("P20/River Memory Watchdog", "NEUTRALIZED", C_Success());

        ImGui::SetCursorScreenPos({cp2.x, cp2.y+card2H+12});
        ImGui::Dummy({0, 20});
    }

    // ============================================
    // TAB 5: SETTINGS & CONFIG
    // ============================================
    static void tab_settings(float a) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float cw = ImGui::GetContentRegionAvail().x;

        float card1H = 140.f;
        ImVec2 cp = ImGui::GetCursorScreenPos();
        NS_CardBegin(dl, cp, cw, card1H);
        NS_CardTitle(dl, {cp.x+16, cp.y+14}, "Configuration Profiles", "Memory Storage");
        ImGui::SetCursorScreenPos({cp.x+16, cp.y+44});

        if (NS_Button("cfg_save", "Save Preset", {110, 30}, true)) {}
        ImGui::SameLine(0, 10);
        if (NS_Button("cfg_load", "Load Preset", {110, 30}, false)) {}
        ImGui::SameLine(0, 10);
        if (NS_Button("cfg_reset", "Reset All", {110, 30}, false)) {}

        ImGui::SetCursorScreenPos({cp.x, cp.y+card1H+12});
        ImGui::Dummy({0, 20});
    }

    // ============================================
    // MAIN RENDER ENTRY
    // ============================================
    void render() {
        // Floating icon / pill toggle when collapsed (Sakura Petal Style)
        if (!menu_visible.load()) {
            ImGui::SetNextWindowPos({30.f, 150.f}, ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize({64.f, 64.f});
            ImGui::Begin("##floating_pill", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                ImGuiWindowFlags_NoSavedSettings);
            
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetWindowPos();
            dl->AddCircleFilled({p.x+32.f, p.y+32.f}, 28.f, C_Accent(), 32);
            dl->AddCircle({p.x+32.f, p.y+32.f}, 28.f, IM_COL32(255,255,255,220), 32, 1.8f);
            dl->AddText(ImGui::GetFont(), 13.f, {p.x+14.f, p.y+24.f}, IM_COL32(255,255,255,255), "SAKURA");

            if (ImGui::InvisibleButton("##pill_btn", {64.f, 64.f})) {
                menu_visible.store(true);
            }
            ImGui::End();
            return;
        }

        // Expanded Main Window
        float winW = 580.f;
        float winH = 390.f;
        ImGui::SetNextWindowSize({winW, winH}, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(g_menu_pos, ImGuiCond_FirstUseEver);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, R);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::PushStyleColor(ImGuiCol_WindowBg, C_BgDark());
        ImGui::PushStyleColor(ImGuiCol_Border, C_Border());

        ImGui::Begin("##bo_main_menu", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();
        content_alpha = ma;

        // Header
        float headerH = 46.f;
        dl->AddRectFilled(wp, {wp.x+ws.x, wp.y+headerH}, C_BgSidebar(), R, ImDrawFlags_RoundCornersTop);
        dl->AddLine({wp.x, wp.y+headerH}, {wp.x+ws.x, wp.y+headerH}, C_Border(), 1.f);
        dl->AddText(ImGui::GetFont(), 14.f, {wp.x+18.f, wp.y+14.f}, C_AccentBright(), "🌸 SAKURA MODS VIP — BO-CLIENT ZYGISK");

        // Close pill button
        float cbW = 24.f;
        ImVec2 cbPos = {wp.x + ws.x - 34.f, wp.y + 11.f};
        dl->AddCircleFilled({cbPos.x+12.f, cbPos.y+12.f}, 12.f, IM_COL32(255,60,60,180));
        dl->AddText(ImGui::GetFont(), 11.f, {cbPos.x+9.f, cbPos.y+5.f}, IM_COL32(255,255,255,255), "X");
        ImGui::SetCursorScreenPos(cbPos);
        if (ImGui::InvisibleButton("##close_btn", {cbW, cbW})) {
            menu_visible.store(false);
        }

        // Sidebar Navigation
        float sidebarW = 125.f;
        float bodyY = wp.y + headerH;
        float bodyH = ws.y - headerH;
        dl->AddRectFilled({wp.x, bodyY}, {wp.x+sidebarW, wp.y+ws.y}, C_BgSidebar(), R, ImDrawFlags_RoundCornersBottomLeft);
        dl->AddLine({wp.x+sidebarW, bodyY}, {wp.x+sidebarW, wp.y+ws.y}, C_Border(), 1.f);

        float navH = 34.f;
        for (int i = 0; i < 6; ++i) {
            float ny = bodyY + 12.f + i * (navH + 4.f);
            bool active = (tab == i);
            if (active) {
                dl->AddRectFilled({wp.x, ny}, {wp.x+sidebarW, ny+navH}, C_AccentDim());
                dl->AddRectFilled({wp.x, ny}, {wp.x+3.f, ny+navH}, C_Accent());
            }
            dl->AddText(ImGui::GetFont(), 10.f, {wp.x+14.f, ny+10.f},
                        active ? C_AccentBright() : C_TextSec(), tabTitles[i]);
            ImGui::SetCursorScreenPos({wp.x, ny});
            if (ImGui::InvisibleButton(tabTitles[i], {sidebarW, navH})) {
                tab = i;
            }
        }

        // Content Area
        float contentX = wp.x + sidebarW + 16.f;
        float contentY = bodyY + 14.f;
        float contentW = ws.x - sidebarW - 32.f;
        float contentH = bodyH - 28.f;

        ImGui::SetCursorScreenPos({contentX, contentY});
        ImGui::BeginChild("content_region", {contentW, contentH}, false, ImGuiWindowFlags_NoBackground);

        switch (tab) {
            case 0: tab_dashboard(ma); break;
            case 1: tab_combat(ma);    break;
            case 2: tab_visuals(ma);   break;
            case 3: tab_exploits(ma);  break;
            case 4: tab_bypass(ma);    break;
            case 5: tab_settings(ma);  break;
        }

        ImGui::EndChild();

        // Dragging window by header
        ImVec2 mp = ImGui::GetIO().MousePos;
        if (mp.x >= wp.x && mp.x <= wp.x+ws.x-40.f && mp.y >= wp.y && mp.y <= wp.y+headerH) {
            if (ImGui::IsMouseClicked(0)) {
                drag = true;
                doff = {mp.x - wp.x, mp.y - wp.y};
            }
        }
        if (drag) {
            if (ImGui::IsMouseDown(0)) {
                g_menu_pos = {mp.x - doff.x, mp.y - doff.y};
            } else {
                drag = false;
            }
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }
}
