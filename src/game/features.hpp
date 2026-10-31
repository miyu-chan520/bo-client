#pragma once

#include <cstdint>
#include "../other/vector3.h"

namespace features {

// Feature State & Config
struct Config {
    // Aimbot & Silent Aim
    bool aim_enable{false};
    bool silent_aim{false};
    bool aim_visible_only{true};
    float aim_fov{90.0f};
    float aim_smooth{5.0f};
    int aim_target_bone{1}; // 1 = Head, 2 = Chest

    // AutoKill & UnderKill
    bool autokill{false};
    int autokill_delay_ms{150};
    bool underkill{false};
    float underkill_depth{12.0f};

    // Fly Hack (RiverMod)
    bool fly_enable{false};
    bool fly_high{false};
    float fly_speed{15.0f};
    bool fly_car{false};

    // Ghost Mode
    bool ghost_enable{false};
    Vector3 ghost_origin{0, 0, 0};

    // AutoTeleMark
    bool tele_mark_enable{false};
    bool tele_kill{false};

    // Speed Timer
    bool speed_enable{false};
    float speed_multiplier{1.8f};

    // Chams
    bool chams_enable{false};
    int chams_mode{0}; // 0 = Always visible, 1 = Shaded, 2 = Wireframe
    float chams_color[4]{1.0f, 0.2f, 0.2f, 1.0f};
};

extern Config g_cfg;

void install_hooks();
void on_player_update_tick(void* local_player);
void trigger_autokill_cycle();

} // namespace features
