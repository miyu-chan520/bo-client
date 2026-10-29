#pragma once
#include "imgui.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>

namespace cfg {
namespace esp {
    // ── Master ──
    inline bool  box             = true;
    inline int   box_type        = 0;      // 0: Corner, 1: Full, 2: Glow
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
    inline bool  loot            = false;

    // ── Setting ──
    inline float max_range     = 250.f;
    inline float loot_range    = 150.f;
    inline float box_rounding  = 3.f;
    inline float box_thickness = 2.0f;
    inline float bracket_frac  = 0.28f;

    // ── Colors ──
    inline ImVec4 box_col      = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
    inline ImVec4 box_visible  = ImVec4(0.2f, 0.9f, 0.3f, 1.0f);
    inline ImVec4 name_col     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 distance_col = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    inline ImVec4 weapon_col   = ImVec4(1.0f, 0.85f, 0.2f, 1.0f);
}

namespace aim {
    inline bool  enable        = false;
    inline bool  aim_lock      = true;
    inline bool  silent_aim    = false;   // Silent Aim [RISK]
    inline bool  no_recoil     = true;    // Working No Recoil
    inline bool  no_spread     = true;    // Working Zero Spread
    inline bool  hitbox_extender = false; // Big Head / Hitbox Extender
    inline float hitbox_size   = 2.5f;    // Head scale factor (1.0f - 8.0f)
    inline bool  auto_drag     = true;    // Auto Drag Headshot
    inline bool  aim_visible   = true;
    inline bool  aim_fire      = true;
    inline bool  aim_scope     = false;
    inline bool  draw_fov      = true;
    inline int   target_bone   = 0;       // 0: Head, 1: Neck, 2: Body
    inline float fov           = 160.f;
    inline float max_range     = 250.f;
    inline float aim_smooth    = 0.35f;
    inline float aim_x_offset  = 0.0f;    // Parallax compensation horizontal fine-trim
    inline ImVec4 fov_col      = ImVec4(1.0f, 1.0f, 1.0f, 0.6f);
}

// ── Binary Config Serialization ──
struct ConfigData {
    uint32_t magic = 0x424F4346; // 'BOCF'
    uint32_t version = 3;

    // ESP
    bool esp_box;
    int  esp_box_type;
    bool esp_name;
    bool esp_health;
    bool esp_armor;
    bool esp_distance;
    bool esp_weapon;
    bool esp_bot_tag;
    bool esp_skeleton;
    bool esp_tracer;
    int  esp_tracer_pos;
    bool esp_crosshair;
    bool esp_firing_ind;
    bool esp_loot;
    float esp_max_range;
    float esp_loot_range;

    // Aim
    bool aim_enable;
    bool aim_lock;
    bool aim_silent;
    bool aim_no_recoil;
    bool aim_no_spread;
    bool aim_hitbox_extender;
    float aim_hitbox_size;
    bool aim_auto_drag;
    bool aim_visible;
    bool aim_fire;
    bool aim_scope;
    bool aim_draw_fov;
    int  aim_target_bone;
    float aim_fov;
    float aim_max_range;
    float aim_smooth;
    float aim_x_offset;
};

inline bool save(const char* path = "/data/local/tmp/.bo_client.cfg") {
    FILE* fp = fopen(path, "wb");
    if (!fp) return false;
    ConfigData d{};
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
    d.esp_loot = esp::loot;
    d.esp_max_range = esp::max_range;
    d.esp_loot_range = esp::loot_range;

    d.aim_enable = aim::enable;
    d.aim_lock = aim::aim_lock;
    d.aim_silent = aim::silent_aim;
    d.aim_no_recoil = aim::no_recoil;
    d.aim_no_spread = aim::no_spread;
    d.aim_hitbox_extender = aim::hitbox_extender;
    d.aim_hitbox_size = aim::hitbox_size;
    d.aim_auto_drag = aim::auto_drag;
    d.aim_visible = aim::aim_visible;
    d.aim_fire = aim::aim_fire;
    d.aim_scope = aim::aim_scope;
    d.aim_draw_fov = aim::draw_fov;
    d.aim_target_bone = aim::target_bone;
    d.aim_fov = aim::fov;
    d.aim_max_range = aim::max_range;
    d.aim_smooth = aim::aim_smooth;
    d.aim_x_offset = aim::aim_x_offset;

    fwrite(&d, sizeof(d), 1, fp);
    fclose(fp);
    return true;
}

inline bool load(const char* path = "/data/local/tmp/.bo_client.cfg") {
    FILE* fp = fopen(path, "rb");
    if (!fp) return false;
    ConfigData d{};
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
    esp::loot = d.esp_loot;
    esp::max_range = d.esp_max_range;
    esp::loot_range = d.esp_loot_range;

    aim::enable = d.aim_enable;
    aim::aim_lock = d.aim_lock;
    aim::silent_aim = d.aim_silent;
    aim::no_recoil = d.aim_no_recoil;
    aim::no_spread = d.aim_no_spread;
    aim::hitbox_extender = d.aim_hitbox_extender;
    aim::hitbox_size = (d.aim_hitbox_size > 0.1f) ? d.aim_hitbox_size : 2.5f;
    aim::auto_drag = d.aim_auto_drag;
    aim::aim_visible = d.aim_visible;
    aim::aim_fire = d.aim_fire;
    aim::aim_scope = d.aim_scope;
    aim::draw_fov = d.aim_draw_fov;
    aim::target_bone = d.aim_target_bone;
    aim::fov = d.aim_fov;
    aim::max_range = d.aim_max_range;
    aim::aim_smooth = d.aim_smooth;
    aim::aim_x_offset = d.aim_x_offset;

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
    esp::loot = false;
    esp::max_range = 250.f;
    esp::loot_range = 150.f;

    aim::enable = false;
    aim::aim_lock = true;
    aim::silent_aim = false;
    aim::no_recoil = true;
    aim::no_spread = true;
    aim::hitbox_extender = false;
    aim::hitbox_size = 2.5f;
    aim::auto_drag = true;
    aim::aim_visible = true;
    aim::aim_fire = true;
    aim::aim_scope = false;
    aim::draw_fov = true;
    aim::target_bone = 0;
    aim::fov = 160.f;
    aim::max_range = 250.f;
    aim::aim_smooth = 0.35f;
    aim::aim_x_offset = 0.0f;
}
}
