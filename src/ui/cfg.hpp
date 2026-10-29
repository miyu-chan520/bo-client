#pragma once
#include "imgui.h"
#include <cstdio>
#include <cstdint>

namespace cfg {
namespace esp {
    // ── Master ──
    inline bool  box             = true;
    inline int   box_type        = 0;
    inline bool  name            = true;
    inline bool  health          = true;
    inline bool  armor           = true;
    inline bool  distance        = true;
    inline bool  weapon          = true;
    inline bool  bot_tag         = true;
    inline bool  skeleton        = false;
    inline bool  tracer          = false;
    inline int   tracer_pos      = 0;      // 0: Bottom, 1: Center, 2: Top
    inline bool  crosshair       = true;
    inline bool  firing_ind      = true;
    inline bool  head_dot        = true;   // Head point indicator
    inline bool  enemy_count     = true;   // Enemy counter HUD badge
    inline bool  offscreen_arrow = true;   // 360 Off-Screen Threat Radar
    inline bool  loot            = false;

    // ── Setting ──
    inline float max_range     = 250.f;
    inline float loot_range    = 150.f;
    inline float box_rounding  = 3.f;
    inline float box_thickness = 2.0f;
    inline float bracket_frac  = 0.28f;

    // ── Warna ──
    inline ImVec4 box_col      = ImVec4(0.0f, 0.90f, 1.0f, 1.0f);
    inline ImVec4 box_visible  = ImVec4(0.30f, 1.0f, 0.55f, 1.0f);
    inline ImVec4 name_col     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 distance_col = ImVec4(0.95f, 0.95f, 0.30f, 1.0f);
    inline ImVec4 weapon_col   = ImVec4(1.0f, 0.85f, 0.30f, 1.0f);
}

namespace aim {
    // ── Master & Triggers ──
    inline bool  enable        = false;  // Aimbot Master Toggle
    inline bool  aim_lock      = true;   // Aim Lock / Aim Assistance
    inline bool  aim_visible   = true;   // Aim Visible Only
    inline bool  aim_fire      = true;   // Aim Fire (Only when firing)
    inline bool  aim_scope     = false;  // Aim Scope (Only when scoping/sighting)
    inline bool  auto_drag     = true;   // Auto Drag Headshot: Body normally, Head on light drag
    inline bool  no_recoil     = false;  // No Recoil (Safe memory zero)
    inline bool  zero_spread   = false;  // Zero Spread [RISK]
    inline bool  silent_aim    = false;  // Silent Aim / Magic Bullet [RISK]
    inline bool  magnet_aim    = false;  // Bullet Magnet / Super Aim Assist [RISK]
    inline bool  big_head      = false;  // Big Head Hitbox Extender [RISK]
    inline float big_head_scale= 1.8f;   // Head Hitbox Scale Multiplier
    inline bool  draw_fov      = true;   // Draw FOV Circle on Screen

    // ── Parameters ──
    inline int   target_bone   = 0;      // 0: Head, 1: Neck, 2: Body
    inline float fov           = 160.f;  // FOV radius in pixels
    inline float max_range     = 250.f;  // Aim range in meters
    inline float aim_smooth    = 0.35f;  // Aim assist smoothing (0.1 to 1.0)
    inline float aim_x_offset  = 0.0f;   // Horizontal parallax trim (-0.30m to +0.30m)
    inline ImVec4 fov_col      = ImVec4(1.0f, 0.35f, 0.35f, 0.45f);
}

namespace misc {
    inline bool  fov_changer   = false;  // Wide FOV [SAFE]
    inline float fov_val       = 1.15f;  // FOV scale factor
    inline bool  dist_warning  = true;   // Proximity threat alert (< 20m) [SAFE]
    inline bool  chams_glow    = false;  // Chams Glow Indicator [SAFE]
}

struct ConfigData {
    uint32_t magic = 0x424F4346; // "BOCF"
    int version = 4;
    bool esp_box;
    int esp_box_type;
    bool esp_name;
    bool esp_health;
    bool esp_armor;
    bool esp_distance;
    bool esp_weapon;
    bool esp_bot_tag;
    bool esp_skeleton;
    bool esp_tracer;
    int esp_tracer_pos;
    bool esp_crosshair;
    bool esp_firing_ind;
    bool esp_head_dot;
    bool esp_enemy_count;
    bool esp_offscreen_arrow;
    bool esp_loot;
    float esp_max_range;
    float esp_loot_range;

    bool aim_enable;
    bool aim_lock;
    bool aim_visible;
    bool aim_fire;
    bool aim_scope;
    bool aim_auto_drag;
    bool aim_no_recoil;
    bool aim_zero_spread;
    bool aim_silent_aim;
    bool aim_magnet_aim;
    bool aim_big_head;
    float aim_big_head_scale;
    bool aim_draw_fov;
    int aim_target_bone;
    float aim_fov;
    float aim_max_range;
    float aim_smooth;
    float aim_x_offset;

    bool misc_fov_changer;
    float misc_fov_val;
    bool misc_dist_warning;
    bool misc_chams_glow;
};

inline bool save(const char* path = "/data/local/tmp/.bo_client.cfg") {
    FILE* fp = fopen(path, "wb");
    if (!fp) return false;
    ConfigData d;
    d.esp_box = esp::box;
    d.esp_box_type = esp::box_type;
    d.esp_name = esp::name;
    d.esp_health = esp::health;
    d.esp_armor = esp::armor;
    d.esp_distance = esp::distance;
    d.esp_weapon = esp::weapon;
    d.esp_bot_tag = esp::bot_tag;
    d.esp_skeleton = esp::skeleton;
    d.esp_tracer = esp::tracer;
    d.esp_tracer_pos = esp::tracer_pos;
    d.esp_crosshair = esp::crosshair;
    d.esp_firing_ind = esp::firing_ind;
    d.esp_head_dot = esp::head_dot;
    d.esp_enemy_count = esp::enemy_count;
    d.esp_offscreen_arrow = esp::offscreen_arrow;
    d.esp_loot = esp::loot;
    d.esp_max_range = esp::max_range;
    d.esp_loot_range = esp::loot_range;

    d.aim_enable = aim::enable;
    d.aim_lock = aim::aim_lock;
    d.aim_visible = aim::aim_visible;
    d.aim_fire = aim::aim_fire;
    d.aim_scope = aim::aim_scope;
    d.aim_auto_drag = aim::auto_drag;
    d.aim_no_recoil = aim::no_recoil;
    d.aim_zero_spread = aim::zero_spread;
    d.aim_silent_aim = aim::silent_aim;
    d.aim_magnet_aim = aim::magnet_aim;
    d.aim_big_head = aim::big_head;
    d.aim_big_head_scale = aim::big_head_scale;
    d.aim_draw_fov = aim::draw_fov;
    d.aim_target_bone = aim::target_bone;
    d.aim_fov = aim::fov;
    d.aim_max_range = aim::max_range;
    d.aim_smooth = aim::aim_smooth;
    d.aim_x_offset = aim::aim_x_offset;

    d.misc_fov_changer = misc::fov_changer;
    d.misc_fov_val = misc::fov_val;
    d.misc_dist_warning = misc::dist_warning;
    d.misc_chams_glow = misc::chams_glow;

    fwrite(&d, sizeof(d), 1, fp);
    fclose(fp);
    return true;
}

inline bool load(const char* path = "/data/local/tmp/.bo_client.cfg") {
    FILE* fp = fopen(path, "rb");
    if (!fp) return false;
    ConfigData d;
    if (fread(&d, sizeof(d), 1, fp) != 1 || d.magic != 0x424F4346) {
        fclose(fp);
        return false;
    }
    esp::box = d.esp_box;
    esp::box_type = d.esp_box_type;
    esp::name = d.esp_name;
    esp::health = d.esp_health;
    esp::armor = d.esp_armor;
    esp::distance = d.esp_distance;
    esp::weapon = d.esp_weapon;
    esp::bot_tag = d.esp_bot_tag;
    esp::skeleton = d.esp_skeleton;
    esp::tracer = d.esp_tracer;
    esp::tracer_pos = d.esp_tracer_pos;
    esp::crosshair = d.esp_crosshair;
    esp::firing_ind = d.esp_firing_ind;
    esp::head_dot = d.esp_head_dot;
    esp::enemy_count = d.esp_enemy_count;
    esp::offscreen_arrow = d.esp_offscreen_arrow;
    esp::loot = d.esp_loot;
    esp::max_range = d.esp_max_range;
    esp::loot_range = d.esp_loot_range;

    aim::enable = d.aim_enable;
    aim::aim_lock = d.aim_lock;
    aim::aim_visible = d.aim_visible;
    aim::aim_fire = d.aim_fire;
    aim::aim_scope = d.aim_scope;
    aim::auto_drag = d.aim_auto_drag;
    aim::no_recoil = d.aim_no_recoil;
    aim::zero_spread = d.aim_zero_spread;
    aim::silent_aim = d.aim_silent_aim;
    aim::magnet_aim = d.aim_magnet_aim;
    aim::big_head = d.aim_big_head;
    aim::big_head_scale = d.aim_big_head_scale;
    aim::draw_fov = d.aim_draw_fov;
    aim::target_bone = d.aim_target_bone;
    aim::fov = d.aim_fov;
    aim::max_range = d.aim_max_range;
    aim::aim_smooth = d.aim_smooth;
    aim::aim_x_offset = d.aim_x_offset;

    misc::fov_changer = d.misc_fov_changer;
    misc::fov_val = d.misc_fov_val;
    misc::dist_warning = d.misc_dist_warning;
    misc::chams_glow = d.misc_chams_glow;

    fclose(fp);
    return true;
}

inline void reset_defaults() {
    esp::box = true;
    esp::box_type = 0;
    esp::name = true;
    esp::health = true;
    esp::armor = true;
    esp::distance = true;
    esp::weapon = true;
    esp::bot_tag = true;
    esp::skeleton = false;
    esp::tracer = false;
    esp::tracer_pos = 0;
    esp::crosshair = true;
    esp::firing_ind = true;
    esp::head_dot = true;
    esp::enemy_count = true;
    esp::offscreen_arrow = true;
    esp::loot = false;
    esp::max_range = 250.f;
    esp::loot_range = 150.f;

    aim::enable = false;
    aim::aim_lock = true;
    aim::aim_visible = true;
    aim::aim_fire = true;
    aim::aim_scope = false;
    aim::auto_drag = true;
    aim::no_recoil = false;
    aim::zero_spread = false;
    aim::silent_aim = false;
    aim::magnet_aim = false;
    aim::big_head = false;
    aim::big_head_scale = 1.8f;
    aim::draw_fov = true;
    aim::target_bone = 0;
    aim::fov = 160.f;
    aim::max_range = 250.f;
    aim::aim_smooth = 0.35f;
    aim::aim_x_offset = 0.0f;

    misc::fov_changer = false;
    misc::fov_val = 1.15f;
    misc::dist_warning = true;
    misc::chams_glow = false;
}
}
