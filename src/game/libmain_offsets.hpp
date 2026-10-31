#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <dlfcn.h>
#include <android/log.h>

// ============================================================================
// LIBMAIN REVERSED IL2CPP SYMBOL & HOOK MAP + DYNAMIC METHOD RESOLVER
// Target: libmain.so (ARM64) · Game: Free Fire Standard & MAX (OB55)
// Fully reverse-engineered from P2077kng v9.1.2-Beta
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
        const char* description;
    };

    inline constexpr HookDefinition kHooks[28] = {
        {  1, "Assembly-CSharp.dll", "GCommon",      "MatchGameClient",                    "Send",                             4, 0x13093c, 0x4a6010, "Ghost Mode / Packet Interception" },
        {  2, "Assembly-CSharp.dll", "COW",          "UIHudMatchResultItemBaseController", ".ctor",                            0, 0x12fe08, 0x4a5fa8, "Suppress Match Result" },
        {  3, "Assembly-CSharp.dll", "COW",          "UIHudMatchResultItemBaseController", "SetMatchResultPlayerInfoData",     4, 0x12fe08, 0x4a5fa8, "Hide Modder Profile" },
        {  4, "Assembly-CSharp.dll", "",             "EventLoggerBase",                    ".ctor",                            0, 0x12fe08, 0x4a5fa8, "Suppress Logger Base ctor" },
        {  5, "Assembly-CSharp.dll", "COW",          "EventLogger",                        "SendEventLogJson",                 2, 0x12fe08, 0x4a5fa8, "Bypass Telemetry JSON Log" },
        {  6, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "OnFixedUpdate",                    2, 0x12fe10, 0x4a5fe0, "Aimbot / AimKill Execution" },
        {  7, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "LateUpdate",                       0, 0x12fe48, 0x4a5fe8, "Bone Transform Alignment" },
        {  8, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "UpdateBehavior",                   2, 0x12f02c, 0x4a5f18, "Speed Hack & State Control" },
        {  9, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "IsIgnoreHighFalling",              0, 0x12f984, 0x4a5f30, "No Fall Damage -> return 1" },
        { 10, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "get_IsPoseFallingHigh",            0, 0x12f8f4, 0x4a5f28, "No High Fall Pose -> return 0" },
        { 11, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "get_InFallingState",               0, 0x12fa14, 0x4a5f38, "No Fall State -> return 0" },
        { 12, "Assembly-CSharp.dll", "COW.GamePlay", "HBIBDMMOOOK",                        "EDFCIKAGHON",                      4, 0x12fe08, 0x4a5fa8, "OB55 Security Logic Bypass" },
        { 13, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "TakeDamage",                       9, 0x12fb54, 0x4a5f60, "Silent Damage / Under Kill" },
        { 14, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "OBEMNJENEFF",                     10, 0x12fbf4, 0x4a5f68, "Wall penetration / Cover Kill" },
        { 15, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "ChangeToGBFootBall",               2, 0x12fe08, 0x4a5fa8, "Football Speed Exploit" },
        { 16, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "ChangeBackFromGBFootBall",         0, 0x12fe08, 0x4a5fa8, "Football Speed Cancel" },
        { 17, "Assembly-CSharp.dll", "COW",          "InGameTeamGoalManager",              "CheckTeamGoalOpen",                1, 0x12fd8c, 0x4a5f98, "Unlock CS Team Goal" },
        { 18, "Assembly-CSharp.dll", "COW",          "GrenadeLine",                        "Update",                           0, 0x0d1cc0, 0x4a3168, "Grenade Line Trace" },
        { 19, "Assembly-CSharp.dll", "COW",          "GrenadeLine",                        "set_ShowThrowSkillLine",           1, 0x0d1cc0, 0x4a3168, "Enable Throw Skill Line" },
        { 20, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "IsFoldWingGliding",                0, 0x12fc9c, 0x4a5f70, "Infinite Glide -> return 1" },
        { 21, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "CanSnowSlideSpeedTakeEffect",      0, 0x12fd08, 0x4a5f78, "Snow Slide Speed -> return 1" },
        { 22, "Assembly-CSharp.dll", "COW.GamePlay", "Vehicle",                            "TakeDamage",                       4, 0x12fa70, 0x4a5f58, "Instant Vehicle Hit Damage" },
        { 23, "Assembly-CSharp.dll", "COW.GamePlay", "Player",                             "get_InSwapWeaponCD",               0, 0x12fa1c, 0x4a5f48, "No Weapon Swap CD -> return 0" },
        { 24, "Assembly-CSharp.dll", "COW.GamePlay", "MDPBNKPCGNG",                        "EHAANNKDEEL",                      6, 0x12fa40, 0x4a5f50, "Auto Fire / Trigger Dispatch" },
        { 25, "Assembly-CSharp.dll", "GarenaMSDK",   "GarenaMSDKMgr",                      "Update",                           0, 0x130034, 0x4a5ff8, "Garena MSDK Telemetry Bypass" },
        { 26, "Assembly-CSharp.dll", "COW.GamePlay", "HBIBDMMOOOK",                        "JBHDCDGJDPK",                      0, 0x12fdac, 0x4a5fa0, "Bypass Client Mem Check" },
        { 27, "Assembly-CSharp.dll", "COW.GamePlay", "NMOGECBBAOK",                        "MECKAEEFGHA",                      0, 0x12fd50, 0x4a5f90, "Integrity Hash Check Bypass" },
        { 28, "Assembly-CSharp.dll", "COW",          "GameFacade",                         "ClientUsingVersion",               0, 0x12fd2c, 0x4a5f88, "Client Version Verification" }
    };

    // ========================================================================
    // IL2CPP METHOD RESOLVER ENGINE (Tái tạo hoàn chỉnh cơ chế quét từ libmain.so:0xcff30)
    // ========================================================================
    typedef void* (*il2cpp_domain_get_t)();
    typedef void* (*il2cpp_domain_assembly_open_t)(void* domain, const char* name);
    typedef void* (*il2cpp_assembly_get_image_t)(void* assembly);
    typedef void* (*il2cpp_class_from_name_t)(void* image, const char* namespaze, const char* name);
    typedef void* (*il2cpp_class_get_method_from_name_t)(void* klass, const char* name, int argsCount);

    inline uintptr_t ResolveMethod(const char* assemblyName, const char* namespaze, const char* klassName, const char* methodName, int paramCount) {
        static void* hIl2Cpp = nullptr;
        static il2cpp_domain_get_t                  p_domain_get = nullptr;
        static il2cpp_domain_assembly_open_t        p_assembly_open = nullptr;
        static il2cpp_assembly_get_image_t          p_assembly_get_image = nullptr;
        static il2cpp_class_from_name_t             p_class_from_name = nullptr;
        static il2cpp_class_get_method_from_name_t  p_class_get_method_from_name = nullptr;

        if (!hIl2Cpp) {
            hIl2Cpp = dlopen("libil2cpp.so", RTLD_NOW);
            if (!hIl2Cpp) return 0;
            p_domain_get                 = (il2cpp_domain_get_t)dlsym(hIl2Cpp, "il2cpp_domain_get");
            p_assembly_open              = (il2cpp_domain_assembly_open_t)dlsym(hIl2Cpp, "il2cpp_domain_assembly_open");
            p_assembly_get_image         = (il2cpp_assembly_get_image_t)dlsym(hIl2Cpp, "il2cpp_assembly_get_image");
            p_class_from_name            = (il2cpp_class_from_name_t)dlsym(hIl2Cpp, "il2cpp_class_from_name");
            p_class_get_method_from_name = (il2cpp_class_get_method_from_name_t)dlsym(hIl2Cpp, "il2cpp_class_get_method_from_name");
        }

        if (!p_domain_get || !p_assembly_open || !p_assembly_get_image || !p_class_from_name || !p_class_get_method_from_name) {
            return 0;
        }

        void* domain = p_domain_get();
        if (!domain) return 0;

        void* assembly = p_assembly_open(domain, assemblyName ? assemblyName : "Assembly-CSharp.dll");
        if (!assembly) return 0;

        void* image = p_assembly_get_image(assembly);
        if (!image) return 0;

        void* klass = p_class_from_name(image, namespaze ? namespaze : "", klassName);
        if (!klass) return 0;

        void* method = p_class_get_method_from_name(klass, methodName, paramCount);
        if (!method) return 0;

        // MethodInfo->methodPointer is at offset 0x0
        return *(uintptr_t*)method;
    }
}
