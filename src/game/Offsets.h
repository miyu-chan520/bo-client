#pragma once

#include <cstdint>

// ============================================================================
// BO Client — Dynamic Runtime Offset Registry (100% RAM Scanned)
// Target: Free Fire Standard (com.dts.freefireth)
// Clean, streamlined offset storage populated directly from live RAM.
// ============================================================================
namespace Offsets {

    // ── Base Chain (Discovered dynamically in libil2cpp.so .bss) ──
    inline uintptr_t GameFacade                = 0;
    inline uintptr_t GameFacade_P2             = 0xB8;  // static_fields
    inline uintptr_t BaseGame_CurrentGame      = 0x0;
    inline uintptr_t BaseGame_CurrentMatchGame = 0x8;
    inline uintptr_t BaseGame_Match            = 0x90;
    inline uintptr_t MatchIsRunning            = 0xCC;
    inline uintptr_t Match_PlayerDict          = 0xC0;
    inline uintptr_t Match_LocalPlayer         = 0xD8;
    inline uintptr_t Match_MetaManager         = 0xB8;

    // ── Player & Combat (Resolved from live Player Il2CppClass) ──
    inline uintptr_t Player_MainCameraTransform= 0x3E8;
    inline uintptr_t Player_TeamIndex          = 0x430;
    inline uintptr_t Player_Name               = 0x490;
    inline uintptr_t Player_OriginalNickName   = 0x498;
    inline uintptr_t Player_IsBot              = 0;
    inline uintptr_t Player_Speed              = 0;
    inline uintptr_t Player_Weapon             = 0;
    inline uintptr_t Player_Rotation           = 0;
    inline uintptr_t Player_IsScoping          = 0;
    inline uintptr_t Player_Camera             = 0x690;
    inline uintptr_t Player_HedColider         = 0x738;
    inline uintptr_t Player_Avatar             = 0x770;
    inline uintptr_t Player_CallSetAimRotationCount = 0;
    inline uintptr_t Player_IsFiring           = 0;
    inline uintptr_t Player_AimInfo            = 0;
    inline uintptr_t Player_AuxAimInfo         = 0;
    inline uintptr_t Player_LastAimTargetRef   = 0xE68;
    inline uintptr_t Player_CurrentAimRotation = 0;
    inline uintptr_t Player_HP                 = 0x70;
    inline uintptr_t Player_Armor              = 0x74;
    inline uintptr_t Player_IsDead             = 0x7C;
    inline uintptr_t Player_ColWrite           = 0x80;
    inline uintptr_t Player_Velocity           = 0xF0;
    inline uintptr_t Player_IsGrounded         = 0x11C;

    // ── Avatar & Camera (Resolved from live child classes) ──
    inline uintptr_t Avatar_Uma                = 0x28;
    inline uintptr_t Uma_Visible               = 0x101;
    inline uintptr_t Uma_Data                  = 0x28;
    inline uintptr_t UmaData_Skip              = 0x91;

    inline uintptr_t Camera_Follow             = 0x30;
    inline uintptr_t Camera_Transform          = 0x40;
    inline uintptr_t Camera_RightOffset        = 0;
    inline uintptr_t Camera_UpOffset           = 0;
    inline uintptr_t Camera_BackOffset         = 0;
    inline uintptr_t Camera_Rotation           = 0x150;
    inline uintptr_t Camera_IntPtr             = 0x10;
    inline uintptr_t Camera_Matrix             = 0x100;

    // ── HP Pool Iteration ──
    inline uintptr_t HP_List                   = 0x10;
    inline uintptr_t HP_Entry                  = 0x20;
    inline uintptr_t HP_Value                  = 0x18;

    // ── Weapon Data ──
    inline uintptr_t Weapon_Data               = 0x80;

    // ── 14 Bones (Resolved from live Player Il2CppClass) ──
    inline uintptr_t Bone_Head                 = 0x6A0;
    inline uintptr_t Bone_Breast               = 0x6B0;
    inline uintptr_t Bone_Hip                  = 0x6A8;
    inline uintptr_t Bone_Root                 = 0x6C8;
    inline uintptr_t Bone_LeftToe              = 0x6E8;
    inline uintptr_t Bone_RightToe             = 0x6F0;
    inline uintptr_t Bone_LeftShoulder         = 0x708;
    inline uintptr_t Bone_RightShoulder        = 0x710;
    inline uintptr_t Bone_LeftHand             = 0x720;
    inline uintptr_t Bone_RightHand            = 0x718;
    inline uintptr_t Bone_LeftForeArm          = 0x730;
    inline uintptr_t Bone_RightForeArm         = 0x728;
    inline uintptr_t Bone_LeftAnkle            = 0x6D8;
    inline uintptr_t Bone_RightAnkle           = 0x6E0;

    // Aliases
    inline uintptr_t Head                      = 0x6A0;
    inline uintptr_t Breast                    = 0x6B0;
    inline uintptr_t Hip                       = 0x6A8;
    inline uintptr_t Root                      = 0x6C8;

    // ── Loot ──
    inline uintptr_t MetaManager_DropObjects   = 0x18;
    inline uintptr_t DropObject_Guid           = 0x20;
    inline uintptr_t DropObject_Position       = 0x68;
    inline uintptr_t DropObject_ResID          = 0x64;
    inline uintptr_t DropObject_Landed         = 0x9C;

    // ── HitInfo (Resolved from live AimInfo Il2CppClass) ──
    inline uintptr_t HitInfo_HitLocation       = 0x28;
    inline uintptr_t HitInfo_HitCollider       = 0x20;
    inline uintptr_t HitInfo_RayDir            = 0x40;
    inline uintptr_t HitInfo_HitGroup          = 0x58;
    inline uintptr_t HitInfo_HitGroupShort     = 0x80;
    inline uintptr_t HitInfo_DamageRatio       = 0x5C;
    inline uintptr_t HitInfo_ViewBlocked       = 0x71;
}
