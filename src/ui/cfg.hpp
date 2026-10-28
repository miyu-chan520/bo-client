#pragma once
#include "imgui.h"

namespace cfg {
namespace esp {
    // ── Master ──
    inline bool  box          = true;
    inline int   box_type     = 0;
    inline bool  name         = true;
    inline bool  health       = true;
    inline bool  armor        = true;
    inline bool  distance     = true;
    inline bool  weapon       = true;
    inline bool  bot_tag      = true;
    inline bool  skeleton     = false;
    inline bool  tracer       = false;
    inline bool  crosshair    = true;
    inline bool  firing_ind   = true;
    inline bool  loot         = false;

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
    inline bool  aim_lock      = true;   // Aim Lock (View Angle Lock)
    inline bool  aim_visible   = true;   // Aim Visible Only
    inline bool  aim_fire      = true;   // Aim Fire (Only when firing)
    inline bool  aim_scope     = false;  // Aim Scope (Only when scoping/sighting)
    inline bool  auto_drag     = true;   // Auto Drag Headshot: Body normally, Head on light drag
    inline bool  no_recoil     = false;  // No Recoil (Safe memory zero)
    inline bool  draw_fov      = true;   // Draw FOV Circle on Screen

    // ── Parameters ──
    inline int   target_bone   = 0;      // 0: Auto Drag, 1: Head, 2: Neck, 3: Body
    inline float fov           = 160.f;  // FOV radius in pixels
    inline float max_range     = 250.f;  // Aim range in meters
    inline ImVec4 fov_col      = ImVec4(1.0f, 0.35f, 0.35f, 0.45f);
}
}