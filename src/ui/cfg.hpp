#pragma once
#include "imgui.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>

// ============================================================================
// BO Client — Streamlined Config Registry
// Features: Bypass, Aimbot, AimKill (10ms limit), Under Kill, Ghost,
//           SpeedTime, NonRecoil, Auto TeleMark, Floating Action HUD
// ============================================================================
namespace cfg {

namespace esp {
    inline bool  box             = true;
    inline int   box_type        = 0;
    inline bool  name            = true;
    inline bool  health          = true;
    inline bool  armor           = true;
    inline bool  distance        = true;
    inline bool  weapon          = false;
    inline bool  bot_tag         = true;
    inline bool  skeleton        = false;
    inline bool  tracer          = false;
    inline int   tracer_pos      = 0;
    inline bool  crosshair       = false;
    inline bool  firing_ind      = false;
    inline bool  loot            = false;
    inline bool  enemy_count     = false;
    inline bool  offscreen_radar = false;
    inline bool  head_dot        = false;
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
    inline bool enable             = true;
    inline bool libc_stealth       = true; // Hide /proc/self/maps, block process_vm_readv
    inline bool telemetry_nullify  = true; // Suppress SendEventLogJson & MSDK telemetry
}

namespace aim {
    inline bool  enable            = true;
    inline bool  aim_lock          = true;
    inline bool  aim_visible       = true; // Check visible
    inline bool  ignore_knocked    = true; // Bỏ qua kẻ địch gục
    inline int   target_bone       = 0;    // 0: Head, 1: Neck, 2: Chest
    inline float fov               = 160.f;
    inline bool  draw_fov          = true;
    inline ImVec4 fov_col          = ImVec4(1.0f, 0.25f, 0.25f, 0.7f);

    constexpr float kMaxRange       = 350.0f;
    constexpr float kFastSmoothness = 0.85f;
}

namespace aimkill {
    inline bool  enable            = false; // AimKill switch
    constexpr int kDelayLimitMs    = 10;    // Giới hạn 10ms tick rate clamp
}

namespace underkill {
    inline bool  enable            = false; // Under Kill (bắn ngầm xuyên địa hình)
    inline float depth_offset      = 2.2f;  // Độ sâu dìm vector xuống dưới mặt đất
}

namespace ghost {
    inline bool  enable            = false; // Ghost mode (desync movement packets)
}

namespace speedtime {
    inline bool  enable            = false; // Speed time (timescale engine)
    inline float scale             = 2.0f;  // Hệ số timescale (1.0x - 5.0x)
}

namespace nonrecoil {
    inline bool  enable            = true;  // Triệt tiêu độ giật và độ tản đạn
}

namespace telemark {
    inline bool  enable            = false; // Auto telemark / teleport tới mục tiêu
}

namespace floating_hud {
    inline bool  show_buttons      = true;  // Nút nổi điều khiển nhanh như của lib
    inline float pos_ghost_x       = 80.f,  pos_ghost_y       = 240.f;
    inline float pos_aimkill_x     = 80.f,  pos_aimkill_y     = 310.f;
    inline float pos_telemark_x    = 80.f,  pos_telemark_y    = 380.f;
    inline float pos_speedtime_x   = 80.f,  pos_speedtime_y   = 450.f;
}

// ── Binary Config Serialization ──
struct ConfigData {
    uint32_t magic = 0x424F3235; // 'BO25'
    uint32_t version = 5;

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

    // Floating Buttons Positions
    bool hud_show_buttons;
    float pos_ghost_x, pos_ghost_y;
    float pos_aimkill_x, pos_aimkill_y;
    float pos_telemark_x, pos_telemark_y;
    float pos_speedtime_x, pos_speedtime_y;
};

inline bool save(const char* path = "/data/local/tmp/.bo_client.cfg") {
    FILE* fp = fopen(path, "wb");
    if (!fp) return false;
    ConfigData d{};
    d.bypass_enable        = bypass::enable;
    d.bypass_libc_stealth  = bypass::libc_stealth;
    d.bypass_telemetry     = bypass::telemetry_nullify;

    d.aim_enable           = aim::enable;
    d.aim_lock             = aim::aim_lock;
    d.aim_visible          = aim::aim_visible;
    d.aim_ignore_knocked   = aim::ignore_knocked;
    d.aim_target_bone      = aim::target_bone;
    d.aim_fov              = aim::fov;
    d.aim_draw_fov         = aim::draw_fov;

    d.aimkill_enable       = aimkill::enable;
    d.underkill_enable     = underkill::enable;
    d.underkill_depth      = underkill::depth_offset;
    d.ghost_enable         = ghost::enable;
    d.speedtime_enable     = speedtime::enable;
    d.speedtime_scale      = speedtime::scale;
    d.nonrecoil_enable     = nonrecoil::enable;
    d.telemark_enable      = telemark::enable;

    d.hud_show_buttons     = floating_hud::show_buttons;
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
    if (fread(&d, sizeof(d), 1, fp) != 1 || d.magic != 0x424F3235) {
        fclose(fp);
        return false;
    }

    bypass::enable         = d.bypass_enable;
    bypass::libc_stealth   = d.bypass_libc_stealth;
    bypass::telemetry_nullify = d.bypass_telemetry;

    aim::enable            = d.aim_enable;
    aim::aim_lock          = d.aim_lock;
    aim::aim_visible       = d.aim_visible;
    aim::ignore_knocked    = d.aim_ignore_knocked;
    aim::target_bone       = d.aim_target_bone;
    aim::fov               = (d.aim_fov > 10.0f) ? d.aim_fov : 160.0f;
    aim::draw_fov          = d.aim_draw_fov;

    aimkill::enable        = d.aimkill_enable;
    underkill::enable      = d.underkill_enable;
    underkill::depth_offset= (d.underkill_depth > 0.1f) ? d.underkill_depth : 2.2f;
    ghost::enable          = d.ghost_enable;
    speedtime::enable      = d.speedtime_enable;
    speedtime::scale       = (d.speedtime_scale >= 1.0f) ? d.speedtime_scale : 2.0f;
    nonrecoil::enable      = d.nonrecoil_enable;
    telemark::enable       = d.telemark_enable;

    floating_hud::show_buttons = d.hud_show_buttons;
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
    bypass::enable         = true;
    bypass::libc_stealth   = true;
    bypass::telemetry_nullify = true;

    aim::enable            = true;
    aim::aim_lock          = true;
    aim::aim_visible       = true;
    aim::ignore_knocked    = true;
    aim::target_bone       = 0;
    aim::fov               = 160.f;
    aim::draw_fov          = true;

    aimkill::enable        = false;
    underkill::enable      = false;
    underkill::depth_offset= 2.2f;
    ghost::enable          = false;
    speedtime::enable      = false;
    speedtime::scale       = 2.0f;
    nonrecoil::enable      = true;
    telemark::enable       = false;

    floating_hud::show_buttons = true;
    floating_hud::pos_ghost_x     = 80.f;  floating_hud::pos_ghost_y     = 240.f;
    floating_hud::pos_aimkill_x   = 80.f;  floating_hud::pos_aimkill_y   = 310.f;
    floating_hud::pos_telemark_x  = 80.f;  floating_hud::pos_telemark_y  = 380.f;
    floating_hud::pos_speedtime_x = 80.f;  floating_hud::pos_speedtime_y = 450.f;
}

} // namespace cfg
