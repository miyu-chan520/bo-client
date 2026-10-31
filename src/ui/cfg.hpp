#pragma once
#include "imgui.h"
#include "../game/features.hpp"

namespace cfg {

namespace esp {
    // ── Master ──
    inline bool  box          = true;
    inline int   box_type     = 3;
    inline bool  name         = true;
    inline bool  health       = true;
    inline bool  armor        = true;
    inline bool  distance     = true;
    inline bool  weapon       = true;
    inline bool  bot_tag      = true;
    inline bool  skeleton     = true;
    inline bool  tracer       = false;
    inline bool  crosshair    = true;
    inline bool  firing_ind   = true;
    inline bool  loot         = false;

    // ── Setting ──
    inline float max_range     = 300.f;
    inline float loot_range    = 150.f;
    inline float box_rounding  = 3.f;
    inline float box_thickness = 2.0f;
    inline float bracket_frac  = 0.28f;

    // ── Colors ──
    inline ImVec4 box_col      = ImVec4(0.0f, 0.90f, 1.0f, 1.0f);
    inline ImVec4 box_visible  = ImVec4(0.30f, 1.0f, 0.55f, 1.0f);
    inline ImVec4 name_col     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    inline ImVec4 distance_col = ImVec4(0.95f, 0.95f, 0.30f, 1.0f);
    inline ImVec4 weapon_col   = ImVec4(1.0f, 0.85f, 0.30f, 1.0f);
}

namespace combat {
    inline bool&  aim_enable       = features::g_cfg.aim_enable;
    inline bool&  silent_aim       = features::g_cfg.silent_aim;
    inline bool&  aim_visible_only = features::g_cfg.aim_visible_only;
    inline float& aim_fov          = features::g_cfg.aim_fov;
    inline float& aim_smooth       = features::g_cfg.aim_smooth;
    inline bool&  autokill         = features::g_cfg.autokill;
    inline bool&  underkill        = features::g_cfg.underkill;
    inline float& underkill_depth  = features::g_cfg.underkill_depth;
}

namespace movement {
    inline bool&  fly_enable       = features::g_cfg.fly_enable;
    inline bool&  fly_high         = features::g_cfg.fly_high;
    inline float& fly_speed        = features::g_cfg.fly_speed;
    inline bool&  fly_car          = features::g_cfg.fly_car;
    inline bool&  ghost_enable     = features::g_cfg.ghost_enable;
    inline bool&  tele_kill        = features::g_cfg.tele_kill;
    inline bool&  speed_enable     = features::g_cfg.speed_enable;
    inline float& speed_multiplier = features::g_cfg.speed_multiplier;
}

namespace visual {
    inline bool&  chams_enable     = features::g_cfg.chams_enable;
    inline int&   chams_mode       = features::g_cfg.chams_mode;
    inline float* chams_color      = features::g_cfg.chams_color;
}

} // namespace cfg
