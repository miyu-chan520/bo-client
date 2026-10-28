#ifndef OFFSET_H
#define OFFSET_H

#include <cstdint>

// ============================================================================
// FreeFire (com.dts.freefireth) — libil2cpp.so
// OB55 / 1.132.1
// ============================================================================

namespace Offsets {

    // ========================================================================
    // BASE CHAIN (GameFacade → BaseGame)
    // ========================================================================
    constexpr uintptr_t GameFacade = 0xA72E4E0;
    constexpr uintptr_t GameFacade_P2            = 0xB8;
    constexpr uintptr_t BaseGame_Match           = 0x90;
    constexpr uintptr_t BaseGame_Timer           = 0x20;
    constexpr uintptr_t Timer_Delta              = 0x2C;
    constexpr uintptr_t MatchIsRunning           = 0xCC;
    constexpr uintptr_t Match_PlayerDict         = 0xC0;
    constexpr uintptr_t Match_LocalPlayer        = 0xD8;

    // ========================================================================
    // DICTIONARY
    // ========================================================================
    constexpr uintptr_t Dict_Entries             = 0x18;
    constexpr uintptr_t Dict_Count               = 0x20;
    constexpr uintptr_t Dict_ArrayHeader         = 0x20;
    constexpr uintptr_t Dict_EntryStride         = 0x18;
    constexpr uintptr_t Dict_EntryValue          = 0x10;

    // ========================================================================
    // PLAYER — MAIN FIELDS
    // ========================================================================
    constexpr uintptr_t Player_Rotation          = 0x614;
    constexpr uintptr_t Player_HeadTF            = 0x6A0;
    constexpr uintptr_t Player_FootTF            = 0x6E8;
    constexpr uintptr_t Player_Camera            = 0x690;
    constexpr uintptr_t Player_IsFiring          = 0x848;
    constexpr uintptr_t Player_HP                = 0x70;
    constexpr uintptr_t Player_Armor             = 0x74;
    constexpr uintptr_t Player_IsDead            = 0x7C;
    constexpr uintptr_t Player_Avatar            = 0x770;
    constexpr uintptr_t Player_HedColider        = 0x738;
    constexpr uintptr_t Player_ColWrite          = 0x80;
    constexpr uintptr_t Player_DeathInfo         = 0x24A8;
    constexpr uintptr_t Player_IsBot             = 0x4A0;
    constexpr uintptr_t Player_Name              = 0x490;
    constexpr uintptr_t Player_AimInfo           = 0xE50;
    constexpr uintptr_t Player_Velocity          = 0xF0;
    constexpr uintptr_t Player_IsGrounded        = 0x11C;
    constexpr uintptr_t Player_FOVOffset         = 0x84;

    // ========================================================================
    // AVATAR / VISIBLE
    // ========================================================================
    constexpr uintptr_t Avatar_Uma               = 0x138;
    constexpr uintptr_t Uma_Visible              = 0x101;
    constexpr uintptr_t Uma_Data                 = 0x28;
    constexpr uintptr_t UmaData_Skip             = 0x91;

    // ========================================================================
    // CAMERA CHAIN
    // ========================================================================
    constexpr uintptr_t Camera_Follow            = 0x28;
    constexpr uintptr_t Camera_IntPtr            = 0x10;
    constexpr uintptr_t Camera_Matrix            = 0x100;

    // ========================================================================
    // HP POOL
    // ========================================================================
    constexpr uintptr_t HP_List                  = 0x10;
    constexpr uintptr_t HP_Entry                 = 0x20;
    constexpr uintptr_t HP_Value                 = 0x18;

    // ========================================================================
    // WEAPON
    // ========================================================================
    constexpr uintptr_t Player_Weapon            = 0x600;
    constexpr uintptr_t Weapon_Data              = 0x80;
    constexpr uintptr_t Weapon_Recoil            = 0x18;

    // ========================================================================
    // AIM INFO
    // ========================================================================
    constexpr uintptr_t AimInfo_Direction        = 0x40;
    constexpr uintptr_t AimInfo_StartPosition    = 0x4C;

    // ========================================================================
    // BONES (ITransformNode)
    // ========================================================================
    constexpr uintptr_t Bone_Head                = 0x6A0;
    constexpr uintptr_t Bone_Hip                 = 0x6A8;
    constexpr uintptr_t Bone_Breast              = 0x6B0;
    constexpr uintptr_t Bone_Root                = 0x6C8;
    constexpr uintptr_t Bone_LeftShoulder        = 0x708;
    constexpr uintptr_t Bone_RightShoulder       = 0x710;
    constexpr uintptr_t Bone_LeftAnkle           = 0x6D8;
    constexpr uintptr_t Bone_RightAnkle          = 0x6E0;
    constexpr uintptr_t Bone_LeftToe             = 0x6E8;
    constexpr uintptr_t Bone_RightToe            = 0x6F0;
    constexpr uintptr_t Bone_RightForeArm        = 0x728;
    constexpr uintptr_t Bone_LeftForeArm         = 0x730;
    constexpr uintptr_t Bone_RightHand           = 0x718;
    constexpr uintptr_t Bone_LeftHand            = 0x720;

    // ========================================================================
    // LOOT / DROP ITEMS
    // ========================================================================
    constexpr uintptr_t Match_MetaManager           = 0xB8;
    constexpr uintptr_t MetaManager_DropObjects     = 0x18;
    constexpr uintptr_t MetaManager_MetaObjects     = 0x10;
    constexpr uintptr_t MetaManager_SpeedObjects    = 0x20;

    constexpr uintptr_t DropObject_Guid             = 0x20;
    constexpr uintptr_t DropObject_Model            = 0x28;
    constexpr uintptr_t DropObject_Shadow           = 0x30;
    constexpr uintptr_t DropObject_ResID            = 0x64;
    constexpr uintptr_t DropObject_Position         = 0x68;
    constexpr uintptr_t DropObject_Rotation         = 0x74;
    constexpr uintptr_t DropObject_Scale            = 0x84;
    constexpr uintptr_t DropObject_Landed           = 0x9C;

    // ========================================================================
    // ALIAS
    // ========================================================================
    constexpr uintptr_t Head = Bone_Head;
    constexpr uintptr_t Hip  = Bone_Hip;
    constexpr uintptr_t Root = Bone_LeftToe;

    // ========================================================================
    // ========================================================================
    // AIM SILENT — RVA VERSION
    // ========================================================================
    // ========================================================================

    // ── Hook target ──
    constexpr uintptr_t Player_TakeDamage_9 = 0x6F91500;
    constexpr uintptr_t Player_TakeDamage_4 = 0x6F913B4;

    // ── Helper: Get Head Collider / Transform ──
    constexpr uintptr_t Player_get_HeadCollider = 0x6F2ACB0;
    constexpr uintptr_t Player_GetHeadTF       = 0x6FAE37C;
    constexpr uintptr_t Player_GetHipTF        = 0x6FAE508;

    // ── Helper: Aim Assist ──
    constexpr uintptr_t Player_GetHeuristicTarget   = 0x6F8E764;
    constexpr uintptr_t Player_UpdateAiming         = 0x6F8EE08;
    constexpr uintptr_t Player_StartFiring          = 0x6F988AC;
    constexpr uintptr_t Player_GetLastAimingInfo    = 0x6F1C238;
    constexpr uintptr_t Player_GetLastAimingTarget  = 0x6F8E3B4;

    // ── Helper: Get GameObject ──
    constexpr uintptr_t Component_get_gameObject = 0x9FD760C;

    // ── Field HitObjectInfo (CGKJLKPMGDJ) ──
    constexpr uintptr_t HitInfo_HitObject      = 0x18;
    constexpr uintptr_t HitInfo_HitCollider    = 0x20;
    constexpr uintptr_t HitInfo_HitLocation    = 0x28;
    constexpr uintptr_t HitInfo_HitNormal      = 0x34;
    constexpr uintptr_t HitInfo_RayDir         = 0x40;
    constexpr uintptr_t HitInfo_HitGroup       = 0x58;
    constexpr uintptr_t HitInfo_DamageRatio    = 0x5C;
    constexpr uintptr_t HitInfo_IgnoreHappens  = 0x70;
    constexpr uintptr_t HitInfo_ViewBlocked    = 0x71;
    constexpr uintptr_t HitInfo_HitGroupShort  = 0x80;

    // ── Additional Player Aim Fields ──
    constexpr uintptr_t Player_AuxAimInfo         = 0xE58;
    constexpr uintptr_t Player_LastAimTargetRef   = 0xE68;
    constexpr uintptr_t Player_CurrentAimRotation = 0x1A8C;

    // ── Field DamageInfo (HNLIMBLIANL) ──
    constexpr uintptr_t DmgInfo_Damage         = 0x10;
    constexpr uintptr_t DmgInfo_DamageType     = 0x14;
    constexpr uintptr_t DmgInfo_Damager        = 0x28;
    constexpr uintptr_t DmgInfo_HitLocation    = 0x4C;
    constexpr uintptr_t DmgInfo_HitNormal      = 0x58;
    constexpr uintptr_t DmgInfo_RayDir         = 0x64;
    constexpr uintptr_t DmgInfo_HitGroup       = 0x70;

}

#endif // OFFSET_H