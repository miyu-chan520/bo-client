#pragma once
#include <cstdint>
#include <cstddef>

// ============================================================================
// LIBMAIN REVERSED IL2CPP SYMBOL & HOOK MAP
// Target: libmain.so (ARM64) · Game: Free Fire (com.dts.freefireth)
// ============================================================================

namespace LibmainOffsets {

    struct HookDefinition {
        uint32_t    id;
        const char* assembly;
        const char* namespaze;
        const char* klass;
        const char* method;
        uint32_t    parameters;
        uintptr_t   handler;
        uintptr_t   backup_ptr;
    };

    inline constexpr HookDefinition kHooks[28] = {
        {  1, "Assembly-CSharp.dll", "GCommon",      "MatchGameClient",                    "Send",                             4, 0x13093c, 0x4a6010 },
        {  2, "Assembly-CSharp.dll", "COW",          "UIHudMatchResultItemBaseController", ".ctor",                            0, 0x12fe08, 0x4a5fa8 },
        {  3, "Assembly-CSharp.dll", "COW",          "UIHudMatchResultItemBaseController", "SetMatchResultPlayerInfoData",     4, 0x12fe08, 0x4a5fa8 },
        {  4, "Assembly-CSharp.dll", "",             "EventLoggerBase",                    ".ctor",                            0, 0x12fe08, 0x4a5fa8 },
        {  5, "Assembly-CSharp.dll", "COW",          "EventLogger",                        "SendEventLogJson",                 2, 0x12fe08, 0x4a5fa8 },
        {  6, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "OnFixedUpdate",                    2, 0x12fe10, 0x4a5fe0 },
        {  7, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "LateUpdate",                       0, 0x12fe48, 0x4a5fe8 },
        {  8, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "UpdateBehavior",                   2, 0x12f02c, 0x4a5f18 },
        {  9, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "IsIgnoreHighFalling",              0, 0x12f984, 0x4a5f30 },
        { 10, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "get_IsPoseFallingHigh",            0, 0x12f8f4, 0x4a5f28 },
        { 11, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "get_InFallingState",               0, 0x12fa14, 0x4a5f38 },
        { 12, "Assembly-CSharp.dll", "COW.GamePlay", "HBIBDMMOOOK",                        "EDFCIKAGHON",                      4, 0x12fe08, 0x4a5fa8 },
        { 13, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "TakeDamage",                       9, 0x12fb54, 0x4a5f60 },
        { 14, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "OBEMNJENEFF",                     10, 0x12fbf4, 0x4a5f68 },
        { 15, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "ChangeToGBFootBall",               2, 0x12fe08, 0x4a5fa8 },
        { 16, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "ChangeBackFromGBFootBall",         0, 0x12fe08, 0x4a5fa8 },
        { 17, "Assembly-CSharp.dll", "COW",          "InGameTeamGoalManager",              "CheckTeamGoalOpen",                1, 0x12fd8c, 0x4a5f98 },
        { 18, "Assembly-CSharp.dll", "COW",          "GrenadeLine",                        "Update",                           0, 0x0d1cc0, 0x4a3168 },
        { 19, "Assembly-CSharp.dll", "COW",          "GrenadeLine",                        "set_ShowThrowSkillLine",           1, 0x0d1cc0, 0x4a3168 },
        { 20, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "IsFoldWingGliding",                0, 0x12fc9c, 0x4a5f70 },
        { 21, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "CanSnowSlideSpeedTakeEffect",      0, 0x12fd08, 0x4a5f78 },
        { 22, "Assembly-CSharp.dll", "COW.GamePlay", "Vehicle",                            "TakeDamage",                       4, 0x12fa70, 0x4a5f58 },
        { 23, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "get_InSwapWeaponCD",               0, 0x12fa1c, 0x4a5f48 },
        { 24, "Assembly-CSharp.dll", "COW.GamePlay", "MDPBNKPCGNG",                        "EHAANNKDEEL",                      6, 0x12fa40, 0x4a5f50 },
        { 25, "Assembly-CSharp.dll", "GarenaMSDK",   "GarenaMSDKMgr",                      "Update",                           0, 0x130034, 0x4a5ff8 },
        { 26, "Assembly-CSharp.dll", "COW.GamePlay", "HBIBDMMOOOK",                        "JBHDCDGJDPK",                      0, 0x12fdac, 0x4a5fa0 },
        { 27, "Assembly-CSharp.dll", "COW.GamePlay", "NMOGECBBAOK",                        "MECKAEEFGHA",                      0, 0x12fd50, 0x4a5f90 },
        { 28, "Assembly-CSharp.dll", "COW",          "GameFacade",                         "ClientUsingVersion",               0, 0x12fd2c, 0x4a5f88 }
    };

    namespace EntityPipeline {
        constexpr const char* kContextNamespace   = "COW.GamePlay";
        constexpr const char* kContextClass       = "JMAGGLCNGIG";
        constexpr const char* kContextStaticField = "DKCLHINMMFO";

        constexpr uintptr_t kPlayerArrayOffset    = 0x18;
        constexpr uintptr_t kArrayCountOffset     = 0x18;
        constexpr uintptr_t kArrayDataOffset      = 0x20;

        constexpr const char* kFieldRootTransform = "GBKFHDFCPMD";
        constexpr const char* kFieldHeadTransform = "IIPBIDIBJDK";
        constexpr const char* kMethodIsVisible    = "IsVisible";
        constexpr const char* kMethodIsTeammate   = "IsLocalTeammate";

        constexpr uintptr_t kTransformAccessOffset = 0x10;
    }
}
