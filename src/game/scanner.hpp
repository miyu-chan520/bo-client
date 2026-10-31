#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace scanner {

struct RuntimeOffsets {
    uintptr_t il2cpp_base{0};
    
    // GameFacade chain
    uintptr_t GameFacade{0};
    uintptr_t GameFacade_Instance{0};
    uintptr_t BaseGame_Match{0x90};
    uintptr_t BaseGame_Timer{0x20};
    uintptr_t Timer_Delta{0x2C};
    uintptr_t Match_PlayerDict{0xC0};
    uintptr_t Match_LocalPlayer{0xD8};
    uintptr_t MatchIsRunning{0xCC};

    // Player fields
    uintptr_t Player_HP{0x70};
    uintptr_t Player_Armor{0x74};
    uintptr_t Player_IsDead{0x7C};
    uintptr_t Player_Avatar{0x770};
    uintptr_t Player_HeadTF{0x6A0};
    uintptr_t Player_FootTF{0x6E8};
    uintptr_t Player_Camera{0x690};
    uintptr_t Player_Weapon{0x600};
    uintptr_t Player_AimInfo{0xE50};
    uintptr_t Player_Velocity{0xF0};
    uintptr_t Player_IsGrounded{0x11C};
    uintptr_t Player_Rotation{0x614};
    uintptr_t Player_Name{0x490};

    // Player methods (function pointers)
    void* fn_TakeDamage_9{nullptr};
    void* fn_TakeDamage_4{nullptr};
    void* fn_get_HeadCollider{nullptr};
    void* fn_GetHeadTF{nullptr};
    void* fn_GetHipTF{nullptr};
    void* fn_UpdateAiming{nullptr};
    void* fn_StartFiring{nullptr};

    // Unity Core methods
    void* fn_Transform_get_position{nullptr};
    void* fn_Transform_set_position{nullptr};
    void* fn_Transform_get_rotation{nullptr};
    void* fn_Transform_set_rotation{nullptr};
    void* fn_Camera_get_main{nullptr};
    void* fn_Camera_WorldToScreenPoint{nullptr};
    void* fn_Time_set_timeScale{nullptr};
};

extern RuntimeOffsets g_Offsets;

// Scanner API
bool run_auto_scan();
uintptr_t get_module_base(const char* module_name);
uintptr_t pattern_scan(uintptr_t base, size_t size, const char* pattern, const char* mask);
uintptr_t pattern_scan_module(const char* module_name, const char* pattern, const char* mask);

} // namespace scanner
