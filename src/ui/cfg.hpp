#pragma once
#include "imgui.h"

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
    // ── Master ──
    inline bool  enable        = false;  // Aimlock / Legit Aimbot
    inline bool  silent        = false;  // Silent Aim (Bullet Redirection)
    inline bool  auto_headshot = false;  // Head Collider Injection
    inline bool  nobita        = false;  // Nobita Mode (All Enemy Headshot + Pen)
    inline bool  on_fire_only  = false;  // Trigger only when firing
    inline bool  draw_fov      = false;  // Draw FOV Circle on Screen

    // ── Parameters ──
    inline float fov           = 180.f;  // FOV radius in pixels
    inline float max_range     = 250.f;  // Aim range in meters
    inline int   target_bone   = 0;      // 0: Head, 1: Chest, 2: Hip
    inline ImVec4 fov_col      = ImVec4(1.0f, 1.0f, 1.0f, 0.35f);
}
}