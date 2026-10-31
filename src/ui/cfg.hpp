#pragma once
#include "imgui.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>

// ============================================================================
// BO Client — Configuration Registry (All Features Default: OFF)
// Full English · External Memory Architecture
// ============================================================================
namespace cfg {

namespace esp {
    inline bool  box             = false;  // Default OFF
    inline int   box_type        = 0;      // 0: Corner, 1: Full
    inline bool  name            = false;  // Default OFF
    inline bool  health          = false;  // Default OFF
    inline bool  armor           = false;  // Default OFF
    inline bool  distance        = false;  // Default OFF
    inline bool  weapon          = false;  // Default OFF
    inline bool  bot_tag         = false;  // Default OFF
    inline bool  skeleton        = false;  // Default OFF
    inline bool  tracer          = false;  // Default OFF
    inline int   tracer_pos      = 0;      // 0: Bottom, 1: Center
    inline bool  crosshair       = false;  // Default OFF
    inline bool  firing_ind      = false;  // Default OFF
    inline bool  loot            = false;  // Default OFF
    inline bool  enemy_count     = false;  // Default OFF
    inline bool  offscreen_radar = false;  // Default OFF
    inline bool  head_dot        = false;  // Default OFF

    inline float max_range       = 250.f;
    inline float loot_range      = 150.f;
    inline float box_rounding    = 3.f;
    inline float box_thickness   = 2.0f;
    inline float bracket_frac    = 0.28f;

    inline ImVec4 box_col        = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
    inline ImVec4 box_visible    = ImVec4(0.2f, 0.9f, 0.3f, 1.0f);
    inline ImVec4 name_col       = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 distance_col   = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    inline ImVec4 weapon_col     = ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
}

namespace bypass {
    inline bool enable             = false; // Default OFF
    inline bool libc_stealth       = false; // Default OFF
    inline bool telemetry_nullify  = false; // Default OFF
}

namespace aim {
    inline bool  enable            = false; // Default OFF
    inline bool  aim_lock          = false; // Default OFF
    inline bool  aim_visible       = true;  // Filter: Visible Only
    inline bool  ignore_knocked    = true;  // Filter: Ignore Knocked
    inline int   target_bone       = 0;     // 0: Head, 1: Neck, 2: Chest
    inline float fov               = 160.f;
    inline bool  draw_fov          = false; // Default OFF
    inline ImVec4 fov_col          = ImVec4(1.0f, 0.25f, 0.25f, 0.7f);

    constexpr float kMaxRange       = 350.0f;
    constexpr float kFastSmoothness = 0.85f;
}

namespace aimkill {
    inline bool  enable            = false; // Default OFF
    constexpr int kDelayLimitMs    = 10;    // 10ms execution delay clamp
}

namespace underkill {
    inline bool  enable            = false; // Default OFF
    inline float depth_offset      = 2.2f;  // Underground vertical offset
}

namespace ghost {
    inline bool  enable            = false; // Default OFF
}

namespace speedtime {
    inline bool  enable            = false; // Default OFF
    inline float scale             = 2.0f;  // Timescale multiplier
}

namespace nonrecoil {
    inline bool  enable            = false; // Default OFF
}

namespace telemark {
    inline bool  enable            = false; // Default OFF
}

namespace floating_hud {
    // Individual toggles in the main menu to show/hide each floating action button
    inline bool  show_ghost        = false; // Default OFF
    inline bool  show_aimkill      = false; // Default OFF
    inline bool  show_telemark     = false; // Default OFF
    inline bool  show_speedtime    = false; // Default OFF

    inline float pos_ghost_x       = 80.f,  pos_ghost_y       = 240.f;
    inline float pos_aimkill_x     = 80.f,  pos_aimkill_y     = 310.f;
    inline float pos_telemark_x    = 80.f,  pos_telemark_y    = 380.f;
    inline float pos_speedtime_x   = 80.f,  pos_speedtime_y   = 450.f;
}

// ── Binary Config Serialization ──
struct ConfigData {
    uint32_t magic = 0x424F3236; // 'BO26'
    uint32_t version = 6;

    // ESP
    bool esp_box;
    int  esp_box_type;
    bool esp_name;
    bool esp_health;
    bool esp_armor;
    bool esp_distance;
    bool esp_weapon;
    bool esp_tracer;
    bool esp_crosshair;
    bool esp_head_dot;
    bool esp_offscreen_radar;
    float esp_max_range;

    // Bypass
    bool bypass_enable;
    bool bypass_libc_stealth;
    bool bypass_telemetry;

    // Aim
    bool aim_enable;
    bool aim_lock;
    bool aim_visible;
    bool aim_ignore_knocked;
    int  aim_target_bone;
    float aim_fov;
    bool aim_draw_fov;

    // Core Exploits
    bool aimkill_enable;
    bool underkill_enable;
    float underkill_depth;
    bool ghost_enable;
    bool speedtime_enable;
    float speedtime_scale;
    bool nonrecoil_enable;
    bool telemark_enable;

    // Floating Buttons Toggles & Positions
    bool hud_show_ghost;
    bool hud_show_aimkill;
    bool hud_show_telemark;
    bool hud_show_speedtime;
    float pos_ghost_x, pos_ghost_y;
    float pos_aimkill_x, pos_aimkill_y;
    float pos_telemark_x, pos_telemark_y;
    float pos_speedtime_x, pos_speedtime_y;
};

inline bool save(const char* path = "/data/local/tmp/.bo_client.cfg") {
    FILE* fp = fopen(path, "wb");
    if (!fp) return false;
    ConfigData d{};

    // ESP
    d.esp_box              = esp::box;
    d.esp_box_type         = esp::box_type;
    d.esp_name             = esp::name;
    d.esp_health           = esp::health;
    d.esp_armor            = esp::armor;
    d.esp_distance         = esp::distance;
    d.esp_weapon           = esp::weapon;
    d.esp_tracer           = esp::tracer;
    d.esp_crosshair        = esp::crosshair;
    d.esp_head_dot         = esp::head_dot;
    d.esp_offscreen_radar  = esp::offscreen_radar;
    d.esp_max_range        = esp::max_range;

    // Bypass
    d.bypass_enable        = bypass::enable;
    d.bypass_libc_stealth  = bypass::libc_stealth;
    d.bypass_telemetry     = bypass::telemetry_nullify;

    // Aim
    d.aim_enable           = aim::enable;
    d.aim_lock             = aim::aim_lock;
    d.aim_visible          = aim::aim_visible;
    d.aim_ignore_knocked   = aim::ignore_knocked;
    d.aim_target_bone      = aim::target_bone;
    d.aim_fov              = aim::fov;
    d.aim_draw_fov         = aim::draw_fov;

    // Exploits
    d.aimkill_enable       = aimkill::enable;
    d.underkill_enable     = underkill::enable;
    d.underkill_depth      = underkill::depth_offset;
    d.ghost_enable         = ghost::enable;
    d.speedtime_enable     = speedtime::enable;
    d.speedtime_scale      = speedtime::scale;
    d.nonrecoil_enable     = nonrecoil::enable;
    d.telemark_enable      = telemark::enable;

    // Floating HUD Buttons
    d.hud_show_ghost       = floating_hud::show_ghost;
    d.hud_show_aimkill     = floating_hud::show_aimkill;
    d.hud_show_telemark    = floating_hud::show_telemark;
    d.hud_show_speedtime   = floating_hud::show_speedtime;
    d.pos_ghost_x          = floating_hud::pos_ghost_x;
    d.pos_ghost_y          = floating_hud::pos_ghost_y;
    d.pos_aimkill_x        = floating_hud::pos_aimkill_x;
    d.pos_aimkill_y        = floating_hud::pos_aimkill_y;
    d.pos_telemark_x       = floating_hud::pos_telemark_x;
    d.pos_telemark_y       = floating_hud::pos_telemark_y;
    d.pos_speedtime_x      = floating_hud::pos_speedtime_x;
    d.pos_speedtime_y      = floating_hud::pos_speedtime_y;

    fwrite(&d, sizeof(d), 1, fp);
    fclose(fp);
    return true;
}

inline bool load(const char* path = "/data/local/tmp/.bo_client.cfg") {
    FILE* fp = fopen(path, "rb");
    if (!fp) return false;
    ConfigData d{};
    if (fread(&d, sizeof(d), 1, fp) != 1 || d.magic != 0x424F3236) {
        fclose(fp);
        return false;
    }

    // ESP
    esp::box               = d.esp_box;
    esp::box_type          = d.esp_box_type;
    esp::name              = d.esp_name;
    esp::health            = d.esp_health;
    esp::armor             = d.esp_armor;
    esp::distance          = d.esp_distance;
    esp::weapon            = d.esp_weapon;
    esp::tracer            = d.esp_tracer;
    esp::crosshair         = d.esp_crosshair;
    esp::head_dot          = d.esp_head_dot;
    esp::offscreen_radar   = d.esp_offscreen_radar;
    esp::max_range         = (d.esp_max_range > 10.f) ? d.esp_max_range : 250.f;

    // Bypass
    bypass::enable         = d.bypass_enable;
    bypass::libc_stealth   = d.bypass_libc_stealth;
    bypass::telemetry_nullify = d.bypass_telemetry;

    // Aim
    aim::enable            = d.aim_enable;
    aim::aim_lock          = d.aim_lock;
    aim::aim_visible       = d.aim_visible;
    aim::ignore_knocked    = d.aim_ignore_knocked;
    aim::target_bone       = d.aim_target_bone;
    aim::fov               = (d.aim_fov > 10.0f) ? d.aim_fov : 160.0f;
    aim::draw_fov          = d.aim_draw_fov;

    // Exploits
    aimkill::enable        = d.aimkill_enable;
    underkill::enable      = d.underkill_enable;
    underkill::depth_offset= (d.underkill_depth > 0.1f) ? d.underkill_depth : 2.2f;
    ghost::enable          = d.ghost_enable;
    speedtime::enable      = d.speedtime_enable;
    speedtime::scale       = (d.speedtime_scale >= 1.0f) ? d.speedtime_scale : 2.0f;
    nonrecoil::enable      = d.nonrecoil_enable;
    telemark::enable       = d.telemark_enable;

    // Floating HUD Buttons
    floating_hud::show_ghost      = d.hud_show_ghost;
    floating_hud::show_aimkill    = d.hud_show_aimkill;
    floating_hud::show_telemark   = d.hud_show_telemark;
    floating_hud::show_speedtime  = d.hud_show_speedtime;
    floating_hud::pos_ghost_x     = d.pos_ghost_x;
    floating_hud::pos_ghost_y     = d.pos_ghost_y;
    floating_hud::pos_aimkill_x   = d.pos_aimkill_x;
    floating_hud::pos_aimkill_y   = d.pos_aimkill_y;
    floating_hud::pos_telemark_x  = d.pos_telemark_x;
    floating_hud::pos_telemark_y  = d.pos_telemark_y;
    floating_hud::pos_speedtime_x = d.pos_speedtime_x;
    floating_hud::pos_speedtime_y = d.pos_speedtime_y;

    fclose(fp);
    return true;
}

inline void reset_defaults() {
    // All features default OFF
    esp::box               = false;
    esp::box_type          = 0;
    esp::name              = false;
    esp::health            = false;
    esp::armor             = false;
    esp::distance          = false;
    esp::weapon            = false;
    esp::tracer            = false;
    esp::crosshair         = false;
    esp::head_dot          = false;
    esp::offscreen_radar   = false;
    esp::max_range         = 250.f;

    bypass::enable         = false;
    bypass::libc_stealth   = false;
    bypass::telemetry_nullify = false;

    aim::enable            = false;
    aim::aim_lock          = false;
    aim::aim_visible       = true;
    aim::ignore_knocked    = true;
    aim::target_bone       = 0;
    aim::fov               = 160.f;
    aim::draw_fov          = false;

    aimkill::enable        = false;
    underkill::enable      = false;
    underkill::depth_offset= 2.2f;
    ghost::enable          = false;
    speedtime::enable      = false;
    speedtime::scale       = 2.0f;
    nonrecoil::enable      = false;
    telemark::enable       = false;

    floating_hud::show_ghost      = false;
    floating_hud::show_aimkill    = false;
    floating_hud::show_telemark   = false;
    floating_hud::show_speedtime  = false;
    floating_hud::pos_ghost_x     = 80.f;  floating_hud::pos_ghost_y     = 240.f;
    floating_hud::pos_aimkill_x   = 80.f;  floating_hud::pos_aimkill_y   = 310.f;
    floating_hud::pos_telemark_x  = 80.f;  floating_hud::pos_telemark_y  = 380.f;
    floating_hud::pos_speedtime_x = 80.f;  floating_hud::pos_speedtime_y = 450.f;
}

} // namespace cfg
