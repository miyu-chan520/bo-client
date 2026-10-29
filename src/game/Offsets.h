#ifndef OFFSET_H
#define OFFSET_H

#include <cstdint>

// ============================================================================
// FreeFire (com.dts.freefireth) — libil2cpp.so
// Auto-dumped & verified from runtime IL2CPP metadata
// Target Package: com.dts.freefireth
// ============================================================================

namespace Offsets {

    // ========================================================================
    // BASE CHAIN (GameFacade -> BaseGame / MatchGame)
    // ========================================================================
    constexpr uintptr_t GameFacade               = 0xA72E4E0;
    constexpr uintptr_t GameFacade_TH            = GameFacade;
    constexpr uintptr_t GameFacade_P2            = 0xB8;  // il2cpp static_fields
    constexpr uintptr_t BaseGame_CurrentGame     = 0x0;   // static BaseGame
    constexpr uintptr_t BaseGame_CurrentMatchGame= 0x8;   // static MatchGame
    constexpr uintptr_t BaseGame_Match           = 0x90;  // MatchGame::m_Match (COW.GamePlay.JMAGGLCNGIG)
    constexpr uintptr_t BaseGame_Timer           = 0x20;  // BaseGame::m_SimulationTimer
    constexpr uintptr_t Timer_Delta              = 0x2C;
    constexpr uintptr_t MatchIsRunning           = 0xCC;  // MatchGame / JMAGGLCNGIG active state
    constexpr uintptr_t Match_PlayerDict         = 0xC0;  // MatchGame::m_ReplicationEntitis / Player dictionary
    constexpr uintptr_t Match_LocalPlayer        = 0xD8;  // JMAGGLCNGIG::MHJCLOPOBAA (Local Player)
    constexpr uintptr_t Match_PlayerList         = 0x158; // JMAGGLCNGIG::LMOIPODCHOL (Player List)

    // ========================================================================
    // MONO / IL2CPP DICTIONARY
    // ========================================================================
    constexpr uintptr_t Dict_Entries             = 0x18;
    constexpr uintptr_t Dict_Count               = 0x20;
    constexpr uintptr_t Dict_ArrayHeader         = 0x20;
    constexpr uintptr_t Dict_EntryStride         = 0x18;
    constexpr uintptr_t Dict_EntryValue          = 0x10;

    // ========================================================================
    // PLAYER (COW.GamePlay.Player / PlayerNetwork)
    // ========================================================================
    constexpr uintptr_t Player_MainCameraTransform = 0x3E8; // UnityEngine.Transform MainCameraTransform
    constexpr uintptr_t Player_Name              = 0x490; // protected System.String BDHGDLFDHAL
    constexpr uintptr_t Player_OriginalNickName  = 0x498; // public System.String OriginalNickName
    constexpr uintptr_t Player_IsBot             = 0x4A0; // public System.Boolean IsClientBot
    constexpr uintptr_t Player_AimCount          = 0x54C; // public System.UInt32 AimCount
    constexpr uintptr_t Player_Speed             = 0x560; // public System.Single Speed
    constexpr uintptr_t Player_Weapon            = 0x600; // public COW.GamePlay.HBIBDMMOOOK ActiveUISightingWeapon
    constexpr uintptr_t Player_Rotation          = 0x614; // private UnityEngine.Quaternion <AKKPEKIHGAH>k__BackingField
    constexpr uintptr_t Player_IsScoping         = 0x624; // private System.Boolean IKCEKAKDFJC
    constexpr uintptr_t Player_AimAssist         = 0x638; // public COW.GamePlay.NLIMDENAFNN m_AimAssist
    constexpr uintptr_t Player_Camera            = 0x690; // protected COW.GamePlay.FollowCamera AHNCPOJPPCL
    constexpr uintptr_t Player_HedColider        = 0x738; // protected UnityEngine.Collider BANPEGJEAKE
    constexpr uintptr_t Player_Avatar            = 0x770; // protected COW.GamePlay.AvatarManager GGJOLDEEIEN
    constexpr uintptr_t Player_BigHeadComponent   = 0x790; // protected COW.GamePlay.BigHeadComponent IEMJKIBBBKL
    constexpr uintptr_t Player_BigHeadScale       = 0x20;  // BigHeadComponent::HCMNGNGFCIC
    constexpr uintptr_t Player_CallSetAimRotationCount = 0x810; // public System.UInt32 CallSetAimRotationCount
    constexpr uintptr_t Player_IsFiring          = 0x848; // private System.Boolean <AHDLLHMLNFI>k__BackingField
    constexpr uintptr_t Player_AimInfo           = 0xE50; // private COW.GamePlay.CGKJLKPMGDJ FDMIEDDNCEC
    constexpr uintptr_t Player_AuxAimInfo        = 0xE58; // private COW.GamePlay.CGKJLKPMGDJ PKIOAMDCOPB
    constexpr uintptr_t Player_LastAimTargetRef  = 0xE68; // private COW.GamePlay.OGKFEMJNCGM <JMPDDMAMFML>k__BackingField
    constexpr uintptr_t Player_CurrentAimRotation = 0x1A8C; // public UnityEngine.Quaternion m_CurrentAimRotation
    constexpr uintptr_t Player_DeathInfo         = 0x24A8; // PlayerNetwork::m_ShadowState

    // ── AttackableEntity / ReplicationEntity base fields ──
    constexpr uintptr_t Player_HP                = 0x70;  // ReplicationEntity::m_PRIDataPool
    constexpr uintptr_t Player_Armor             = 0x74;  // Armor offset
    constexpr uintptr_t Player_IsDead            = 0x7C;  // AttackableEntity::CGJLEBOGBNP (get_IsDead)
    constexpr uintptr_t Player_ColWrite          = 0x80;  // AttackableEntity::<AONPKGBGHHI>k__BackingField
    constexpr uintptr_t Player_Velocity          = 0xF0;  // Entity Velocity
    constexpr uintptr_t Player_IsGrounded        = 0x11C; // Grounded state
    constexpr uintptr_t Player_FOVOffset         = 0x84;

    // ========================================================================
    // AVATAR / VISIBILITY
    // ========================================================================
    constexpr uintptr_t Avatar_Uma               = 0x138; // AvatarManager::LHEDBMJJOLM (UMA.IUmaAvatar)
    constexpr uintptr_t Uma_Visible              = 0x101; // UMA visible byte
    constexpr uintptr_t Uma_Data                 = 0x28;  // OverlayColorData
    constexpr uintptr_t UmaData_Skip             = 0x91;

    // ========================================================================
    // CAMERA CHAIN (COW.GamePlay.FollowCamera)
    // ========================================================================
    constexpr uintptr_t Camera_Follow            = 0x30;  // Follow camera pointer
    constexpr uintptr_t Camera_Transform         = 0x40;  // Camera Transform
    constexpr uintptr_t Camera_RightOffset       = 0x78;  // public System.Single RightOffset
    constexpr uintptr_t Camera_UpOffset          = 0x7C;  // public System.Single UpOffset
    constexpr uintptr_t Camera_BackOffset        = 0x80;  // public System.Single BackOffset
    constexpr uintptr_t Camera_Rotation          = 0x150; // Camera Look Quaternion
    constexpr uintptr_t Camera_IntPtr            = 0x10;  // Native Camera ptr
    constexpr uintptr_t Camera_Matrix            = 0x100; // View Projection Matrix

    // ========================================================================
    // HP POOL ITERATION
    // ========================================================================
    constexpr uintptr_t HP_List                  = 0x10;
    constexpr uintptr_t HP_Entry                 = 0x20;
    constexpr uintptr_t HP_Value                 = 0x18;

    // ========================================================================
    // WEAPON & SPREAD & GUN FEEL PROFILE
    // (COW.GamePlay.HBIBDMMOOOK & COW.WeaponGunFeelProfileData)
    // ========================================================================
    constexpr uintptr_t Player_Weapon            = 0x600; // ActiveUISightingWeapon
    constexpr uintptr_t Weapon_Data              = 0x80;  // HBIBDMMOOOK::<BDKDBILOMDE>k__BackingField
    constexpr uintptr_t Weapon_ProfileData       = 0x4A0; // HBIBDMMOOOK::AFLGAPPILEH (WeaponGunFeelProfileData)
    constexpr uintptr_t Weapon_Recoil            = 0x18;

    // ── WeaponGunFeelProfileData Fields ──
    constexpr uintptr_t WeaponProfile_visualPitchScale         = 0x88;
    constexpr uintptr_t WeaponProfile_visualFollowRealRecoil   = 0x8C;
    constexpr uintptr_t WeaponProfile_hipRecoilPitchNoise      = 0x90;
    constexpr uintptr_t WeaponProfile_adsSoftAssistMaxYaw      = 0xA0;
    constexpr uintptr_t WeaponProfile_adsSoftAssistOuterAngle  = 0xA8;
    constexpr uintptr_t WeaponProfile_adsSoftAssistBestAngle   = 0xAC;
    constexpr uintptr_t WeaponProfile_adsSoftAssistInnerAngle  = 0xB0;
    constexpr uintptr_t WeaponProfile_crouchRecoilScale        = 0xB8;
    constexpr uintptr_t WeaponProfile_hipRecoilPitchMax        = 0xBC;
    constexpr uintptr_t WeaponProfile_hipSoftAssistOuterAngle  = 0xD4;
    constexpr uintptr_t WeaponProfile_hipSoftAssistBestAngle   = 0xD8;
    constexpr uintptr_t WeaponProfile_hipRecoilYawScale        = 0xE4;
    constexpr uintptr_t WeaponProfile_hipSoftAssistSustain     = 0xF4;
    constexpr uintptr_t WeaponProfile_softAssistTargetKeepTime = 0x104;
    constexpr uintptr_t WeaponProfile_hipRecoilPitchMin        = 0x124;
    constexpr uintptr_t WeaponProfile_adsSpreadClampMax        = 0x12C;
    constexpr uintptr_t WeaponProfile_adsSpreadClampMin        = 0x130;
    constexpr uintptr_t WeaponProfile_hipSpreadClampMax        = 0x158;
    constexpr uintptr_t WeaponProfile_hipSpreadClampMin        = 0x15C;
    constexpr uintptr_t WeaponProfile_adsRecoilMaxYawOffset    = 0x17C;
    constexpr uintptr_t WeaponProfile_adsRecoilMaxPitchOffset  = 0x180;
    constexpr uintptr_t WeaponProfile_adsRecoilYawNoise        = 0x18C;
    constexpr uintptr_t WeaponProfile_adsRecoilPitchNoise      = 0x190;
    constexpr uintptr_t WeaponProfile_hipRecoilYawMin          = 0x194;
    constexpr uintptr_t WeaponProfile_adsRecoilYawMin          = 0x198;
    constexpr uintptr_t WeaponProfile_adsRecoilPitchMax        = 0x19C;
    constexpr uintptr_t WeaponProfile_adsRecoilPitchScale      = 0x1A8;
    constexpr uintptr_t WeaponProfile_hipRecoilYawMax          = 0x1B4;
    constexpr uintptr_t WeaponProfile_adsRecoilYawScale        = 0x1D8;
    constexpr uintptr_t WeaponProfile_adsRecoilPitchMin        = 0x1DC;
    constexpr uintptr_t WeaponProfile_adsRecoilYawMax          = 0x1E0;
    constexpr uintptr_t WeaponProfile_hipRecoilPitchScale      = 0x1E8;

    // ── Root Master Weapon Switches ──
    constexpr uintptr_t WeaponProfile_enableRecoil             = 0x214; // System.Boolean
    constexpr uintptr_t WeaponProfile_enableVisualRecoil       = 0x215; // System.Boolean
    constexpr uintptr_t WeaponProfile_enableFireMoveCurve      = 0x216; // System.Boolean
    constexpr uintptr_t WeaponProfile_enableLegacyCameraShake  = 0x217; // System.Boolean
    constexpr uintptr_t WeaponProfile_enableGunFeel            = 0x218; // System.Boolean
    constexpr uintptr_t WeaponProfile_softAssistRespectInput   = 0x219; // System.Boolean
    constexpr uintptr_t WeaponProfile_enableSpread             = 0x21A; // System.Boolean
    constexpr uintptr_t WeaponProfile_enableSoftAssist         = 0x21B; // System.Boolean
    constexpr uintptr_t WeaponProfile_enableAdsSettle          = 0x21C; // System.Boolean

    // ========================================================================
    // AIM INFO (COW.GamePlay.CGKJLKPMGDJ)
    // ========================================================================
    constexpr uintptr_t AimInfo_Direction        = 0x40; // HLDECMJFKJK
    constexpr uintptr_t AimInfo_StartPosition    = 0x4C; // EKPLMDDKOGB

    // ========================================================================
    // BONES (ITransformNode)
    // ========================================================================
    constexpr uintptr_t Bone_Head                = 0x6A0; // GBKFHDFCPMD
    constexpr uintptr_t Bone_Hip                 = 0x6A8; // COBFGNOIPMF
    constexpr uintptr_t Bone_Breast              = 0x6B0; // FFFPCADFFGA
    constexpr uintptr_t Bone_Root                = 0x6C8; // IIPBIDIBJDK
    constexpr uintptr_t Bone_LeftAnkle           = 0x6D8; // GPIBDOCMHBE
    constexpr uintptr_t Bone_RightAnkle          = 0x6E0; // EMNIJJCEKDD
    constexpr uintptr_t Bone_LeftToe             = 0x6E8; // GMJLIMFHMAE
    constexpr uintptr_t Bone_RightToe            = 0x6F0; // JMHBBGIKBNO
    constexpr uintptr_t Bone_LeftShoulder        = 0x708; // FFDPGNIKAEK
    constexpr uintptr_t Bone_RightShoulder       = 0x710; // FOELKAHABCD
    constexpr uintptr_t Bone_RightHand           = 0x718; // JLAECGNPCHC
    constexpr uintptr_t Bone_LeftHand            = 0x720; // CLCEEJMGBGE
    constexpr uintptr_t Bone_RightForeArm        = 0x728; // DJAOGHBOBOJ
    constexpr uintptr_t Bone_LeftForeArm         = 0x730; // IMLDFGHNLBP

    // Aliases
    constexpr uintptr_t Head = Bone_Head;
    constexpr uintptr_t Hip  = Bone_Hip;
    constexpr uintptr_t Root = Bone_LeftToe;

    // ========================================================================
    // LOOT / DROP ITEMS (COW.GamePlay.GPKHPCMEOFO & DropObject)
    // ========================================================================
    constexpr uintptr_t Match_MetaManager           = 0xB8;  // MatchGame::m_MetaManager
    constexpr uintptr_t MetaManager_MetaObjects     = 0x10;  // Dictionary<UInt32, MetaObject>
    constexpr uintptr_t MetaManager_DropObjects     = 0x18;  // Dictionary<UInt32, DropObject>
    constexpr uintptr_t MetaManager_SpeedObjects    = 0x20;  // Dictionary<UInt32, SpeedObject>

    constexpr uintptr_t DropObject_Guid             = 0x20;  // Guid
    constexpr uintptr_t DropObject_Model            = 0x28;  // model GameObject
    constexpr uintptr_t DropObject_Shadow           = 0x30;  // shadow Transform
    constexpr uintptr_t DropObject_ResID            = 0x64;  // resID
    constexpr uintptr_t DropObject_Position         = 0x68;  // position Vector3
    constexpr uintptr_t DropObject_Rotation         = 0x74;  // rotation Quaternion
    constexpr uintptr_t DropObject_Scale            = 0x84;  // scale Vector3
    constexpr uintptr_t DropObject_Landed           = 0x9C;  // landed bool

    // ========================================================================
    // AIM SILENT / DAMAGE RVAs
    // ========================================================================
    constexpr uintptr_t PlayerNetwork_TakeDamage_9  = 0x72DFC60; // PlayerNetwork::TakeDamage (9 args)
    constexpr uintptr_t Player_TakeDamage_9         = 0x6F91500; // Player::TakeDamage (9 args)
    constexpr uintptr_t Player_TakeDamage_4         = 0x6F913B4; // Player::TakeDamage (4 args)
    constexpr uintptr_t AttackableEntity_TakeDamage = 0x716CF4C; // AttackableEntity::TakeDamage

    // ── Helper RVAs: Bones & Transform ──
    constexpr uintptr_t Player_get_HeadCollider     = 0x6F2ACB0; // Player::get_HeadCollider()
    constexpr uintptr_t Player_GetHeadTF            = 0x6FAE37C; // Player::GetHeadTF()
    constexpr uintptr_t Player_GetHipTF             = 0x6FAE508; // Player::GetHipTF()
    constexpr uintptr_t BigHeadComponent_UpdateScale= 0x70FA2E4; // BigHeadComponent::UpdateScale(float)

    // ── Helper RVAs: Aim Assist & Firing ──
    constexpr uintptr_t Player_GetHeuristicTarget   = 0x6F8E764; // Player::GetHeuristicTarget()
    constexpr uintptr_t Player_UpdateAiming         = 0x6F8EE08; // Player::UpdateAiming(...)
    constexpr uintptr_t Player_StartFiring          = 0x6F988AC; // Player::StartFiring(...)
    constexpr uintptr_t PlayerNetwork_StartFiring   = 0x72E12C0; // PlayerNetwork::StartFiring(...)
    constexpr uintptr_t Player_GetLastAimingInfo    = 0x6F1C238; // Player::get_LastAimingInfoFromWeapon()
    constexpr uintptr_t Player_GetLastAuxAimingInfo = 0x6F8E364; // Player::get_LastAuxAimingInfoFromWeapon()
    constexpr uintptr_t Player_GetLastAimingTarget  = 0x6F8E3B4; // Player::get_LastAimingTargetFromWeapon()

    // ── Component Native GameObject ──
    constexpr uintptr_t Component_get_gameObject    = 0x9FD760C;

    // ========================================================================
    // HIT OBJECT INFO (COW.GamePlay.CGKJLKPMGDJ)
    // ========================================================================
    constexpr uintptr_t HitInfo_HitObject           = 0x18; // GameObject JHAINOBIFMA
    constexpr uintptr_t HitInfo_HitCollider         = 0x20; // Collider JINDNOFABKB
    constexpr uintptr_t HitInfo_HitLocation         = 0x28; // Vector3 HPCOJHGFEEG
    constexpr uintptr_t HitInfo_HitNormal           = 0x34; // Vector3 DOJDNHCGFGK
    constexpr uintptr_t HitInfo_RayDir              = 0x40; // Vector3 HLDECMJFKJK
    constexpr uintptr_t HitInfo_HitGroup            = 0x58; // Int32 OLGEMBBDBID
    constexpr uintptr_t HitInfo_DamageRatio         = 0x5C; // Single CFIPDNFBKHK
    constexpr uintptr_t HitInfo_IgnoreHappens       = 0x70; // Boolean EFMBNPNFGHM
    constexpr uintptr_t HitInfo_ViewBlocked         = 0x71; // Boolean CEOKOILBODN
    constexpr uintptr_t HitInfo_HitGroupShort       = 0x80; // Int16 CLOLKHNMNEK

    // ========================================================================
    // DAMAGE INFO (COW.GamePlay.HNLIMBLIANL)
    // ========================================================================
    constexpr uintptr_t DmgInfo_Damage              = 0x10; // Int32 KOPMDJJKLKH
    constexpr uintptr_t DmgInfo_DamageType          = 0x14; // EHGMCNBHNMD BGMOAOCFPGM
    constexpr uintptr_t DmgInfo_Damager             = 0x28; // BEADLMGGGGL BIHCFIINIBK
    constexpr uintptr_t DmgInfo_HitLocation         = 0x4C; // Vector3 HOOMBLGOEPL
    constexpr uintptr_t DmgInfo_HitNormal           = 0x58; // Vector3 KEOAMDFAOLM
    constexpr uintptr_t DmgInfo_RayDir              = 0x64; // Vector3 DOJDNHCGFGK
    constexpr uintptr_t DmgInfo_HitGroup            = 0x70; // Int16 CLOLKHNMNEK

}

#endif // OFFSET_H
