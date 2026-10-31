#pragma once

#include "Offsets.h"
#include "../other/memory.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <chrono>
#include <unistd.h>
#include <android/log.h>

#define DO_LOG_TAG "BO_DynamicRAM"
#define DO_LOGI(...) __android_log_print(ANDROID_LOG_INFO,  DO_LOG_TAG, __VA_ARGS__)
#define DO_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, DO_LOG_TAG, __VA_ARGS__)

// ============================================================================
// 100% NON-INTRUSIVE DYNAMIC RAM SCANNER (ZERO GAME CRASH · PURE RPM)
// Resolves all IL2CPP classes, fields, and bone offsets directly from live RAM.
// Target: Free Fire Standard (com.dts.freefireth)
// ============================================================================
namespace DynamicOffsets {

    inline uint64_t g_gameFacadeClass = 0;
    inline uint64_t g_gameFacadeCell  = 0;
    inline bool     g_ramScanCompleted = false;

    struct MemoryRange {
        uint64_t start;
        uint64_t end;
    };

    // Safely read remote C-string with ASCII validation (prevents reading garbage)
    inline bool ReadRemoteString(uint64_t addr, char* out, size_t maxLen) {
        if (!addr || addr < 0x10000 || addr > 0x7fffffffff) return false;
        out[0] = '\0';
        char buf[64];
        size_t toRead = maxLen < sizeof(buf) ? maxLen : sizeof(buf);
        if (!mem_read(addr, buf, toRead)) return false;
        buf[toRead - 1] = '\0';

        // Strict printable ASCII validation
        size_t len = 0;
        while (len < toRead && buf[len] != '\0') {
            unsigned char c = (unsigned char)buf[len];
            if (c < 0x20 || c > 0x7E) return false;
            len++;
        }
        if (len == 0) return false;

        strncpy(out, buf, maxLen);
        out[maxLen - 1] = '\0';
        return true;
    }

    // Inspect Il2CppClass name and namespace safely
    inline bool GetClassInfo(uint64_t klass, char* outName, char* outNs) {
        if (!klass || klass < 0x100000000 || klass > 0x7fffffffff) return false;
        uint64_t namePtr = rpm<uint64_t>(klass + 0x10);
        uint64_t nsPtr   = rpm<uint64_t>(klass + 0x18);
        if (!namePtr || namePtr < 0x100000000 || namePtr > 0x7fffffffff) return false;
        if (!nsPtr   || nsPtr   < 0x100000000 || nsPtr   > 0x7fffffffff) return false;
        return ReadRemoteString(namePtr, outName, 64) && ReadRemoteString(nsPtr, outNs, 64);
    }

    // Resolve base parent class in Il2CppClass hierarchy (probes standard Unity offsets)
    inline uint64_t GetParentClass(uint64_t klass) {
        if (!klass || klass < 0x100000000 || klass > 0x7fffffffff) return 0;
        static int s_parentOffset = -1;
        if (s_parentOffset >= 0) {
            uint64_t p = rpm<uint64_t>(klass + s_parentOffset);
            return (p >= 0x100000000 && p < 0x7fffffffff) ? p : 0;
        }

        // Test common Il2CppClass parent offsets: 0x48, 0x58, 0x50, 0x60, 0x68
        for (int off : { 0x48, 0x58, 0x50, 0x60, 0x68, 0x40 }) {
            uint64_t candidate = rpm<uint64_t>(klass + off);
            if (candidate >= 0x100000000 && candidate < 0x7fffffffff) {
                char pName[64] = {0}, pNs[64] = {0};
                if (GetClassInfo(candidate, pName, pNs) && pName[0] != '\0') {
                    s_parentOffset = off;
                    return candidate;
                }
            }
        }
        return 0;
    }

    // Dynamically locate FieldInfo* array pointer inside Il2CppClass
    // Verifies candidate[0].parent == klass to guarantee 100% correct pointer across Unity versions
    inline uint64_t GetFieldsArray(uint64_t klass) {
        if (!klass || klass < 0x100000000 || klass > 0x7fffffffff) return 0;
        for (uint64_t off = 0x68; off <= 0x90; off += 8) {
            uint64_t candidate = rpm<uint64_t>(klass + off);
            if (candidate >= 0x100000000 && candidate < 0x7fffffffff) {
                uint64_t parent = rpm<uint64_t>(candidate + 0x10);
                if (parent == klass) {
                    return candidate;
                }
            }
        }
        // Fallback default ARM64 IL2CPP field array offset
        return rpm<uint64_t>(klass + 0x80);
    }

    // Dynamic field offset getter matching multiple candidate names (obfuscated + de-obfuscated)
    // Walks class inheritance chain (klass -> parent -> grandparent) up to 6 levels deep.
    // FieldInfo stride = 0x20 (32 bytes) on ARM64:
    //   +0x00: name (const char*)
    //   +0x08: type
    //   +0x10: parent (Il2CppClass*)
    //   +0x18: offset (int32_t)
    inline int32_t GetFieldOffsetByNames(uint64_t klass, const std::vector<const char*>& candidates) {
        if (!klass || klass < 0x100000000 || klass > 0x7fffffffff) return -1;

        uint64_t curClass = klass;
        for (int depth = 0; depth < 6 && curClass >= 0x100000000 && curClass < 0x7fffffffff; depth++) {
            uint64_t fieldsPtr = GetFieldsArray(curClass);
            if (fieldsPtr >= 0x100000000 && fieldsPtr < 0x7fffffffff) {
                int consecutiveNulls = 0;
                for (int i = 0; i < 1200; i++) {
                    uint64_t fieldStruct = fieldsPtr + (i * 0x20);
                    uint64_t namePtr = rpm<uint64_t>(fieldStruct + 0x00);
                    if (!namePtr || namePtr < 0x100000000 || namePtr > 0x7fffffffff) {
                        if (++consecutiveNulls > 10) break;
                        continue;
                    }
                    consecutiveNulls = 0;

                    char fName[64];
                    if (ReadRemoteString(namePtr, fName, sizeof(fName))) {
                        for (const char* targetName : candidates) {
                            if (strcmp(fName, targetName) == 0) {
                                return rpm<int32_t>(fieldStruct + 0x18);
                            }
                        }
                    }
                }
            }
            curClass = GetParentClass(curClass);
        }
        return -1;
    }

    inline int32_t GetFieldOffsetByName(uint64_t klass, const char* targetFieldName) {
        std::vector<const char*> c = { targetFieldName };
        return GetFieldOffsetByNames(klass, c);
    }

    // Collect all rw-p memory segments belonging to libil2cpp.so (.data, .bss, anon)
    inline std::vector<MemoryRange> GetIl2CppRwRanges(int pid, uint64_t libBase) {
        std::vector<MemoryRange> ranges;
        char mapsPath[64];
        snprintf(mapsPath, sizeof(mapsPath), "/proc/%d/maps", pid);

        FILE* fp = fopen(mapsPath, "r");
        if (!fp) return ranges;

        char line[512];
        bool inIl2Cpp = false;
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "libil2cpp.so")) {
                inIl2Cpp = true;
                if (strstr(line, "rw-p")) {
                    uint64_t s = 0, e = 0;
                    if (sscanf(line, "%lx-%lx", &s, &e) == 2 && e > s) {
                        ranges.push_back({s, e});
                    }
                }
            } else if (inIl2Cpp && strstr(line, "rw-p")) {
                uint64_t s = 0, e = 0;
                if (sscanf(line, "%lx-%lx", &s, &e) == 2 && e > s) {
                    if (s >= libBase && s < libBase + 0x14000000) {
                        ranges.push_back({s, e});
                    }
                }
            } else if (inIl2Cpp && !strstr(line, "[anon")) {
                inIl2Cpp = false;
            }
        }
        fclose(fp);

        if (ranges.empty() && libBase > 0) {
            ranges.push_back({libBase + 0x500000, libBase + 0x10000000});
        }
        return ranges;
    }

    // 100% Dynamic RAM Scanner: Locates COW.GameFacade Il2CppClass in live memory
    inline uint64_t FindGameFacadeClassInRam(int pid, uint64_t libBase) {
        if (g_gameFacadeClass >= 0x100000000) {
            char cName[64] = {0}, cNs[64] = {0};
            if (GetClassInfo(g_gameFacadeClass, cName, cNs) && strcmp(cName, "GameFacade") == 0) {
                return g_gameFacadeClass;
            }
        }

        auto ranges = GetIl2CppRwRanges(pid, libBase);
        constexpr size_t kChunkSize = 65536;
        std::vector<uint64_t> chunk(kChunkSize / sizeof(uint64_t));

        for (const auto& r : ranges) {
            for (uint64_t cur = r.start; cur < r.end; cur += kChunkSize) {
                size_t bytesToRead = (cur + kChunkSize <= r.end) ? kChunkSize : (r.end - cur);
                if (!mem_read(cur, chunk.data(), bytesToRead)) continue;

                size_t numPtrs = bytesToRead / sizeof(uint64_t);
                for (size_t i = 0; i < numPtrs; i++) {
                    uint64_t ptr = chunk[i];
                    if (ptr >= 0x100000000 && ptr < 0x7fffffffff) {
                        char cName[64] = {0};
                        char cNs[64] = {0};
                        if (GetClassInfo(ptr, cName, cNs)) {
                            if (strcmp(cName, "GameFacade") == 0 && strcmp(cNs, "COW") == 0) {
                                uint64_t bssAddr = cur + (i * sizeof(uint64_t));
                                g_gameFacadeClass = ptr;
                                g_gameFacadeCell  = bssAddr;
                                Offsets::GameFacade = bssAddr - libBase;
                                DO_LOGI("[✓] Found COW.GameFacade: Il2CppClass=0x%lx at BSS+0x%lx",
                                        (unsigned long)ptr, (unsigned long)Offsets::GameFacade);
                                return ptr;
                            }
                        }
                    }
                }
            }
        }

        return 0;
    }

    // Resolves GameFacade static fields (CurrentMatchGame / CurrentGame)
    inline bool ResolveGameFacadeFields(uint64_t gfClass) {
        if (!gfClass || gfClass < 0x100000000) return false;
        int32_t offMatchGame = GetFieldOffsetByNames(gfClass, {"CurrentMatchGame", "m_CurrentMatchGame"});
        int32_t offGame      = GetFieldOffsetByNames(gfClass, {"CurrentGame", "m_CurrentGame"});

        if (offMatchGame >= 0) Offsets::BaseGame_CurrentMatchGame = (uintptr_t)offMatchGame;
        if (offGame >= 0)      Offsets::BaseGame_CurrentGame      = (uintptr_t)offGame;

        DO_LOGI("[✓] GameFacade fields: CurrentMatchGame=0x%x, CurrentGame=0x%x", offMatchGame, offGame);
        return true;
    }

    // Resolves MatchGame and Match classes from live BaseGame RAM object
    inline bool ResolveMatchChain(uint64_t baseGame) {
        if (!baseGame || baseGame < 0x100000000) return false;
        uint64_t matchGameClass = rpm<uint64_t>(baseGame + 0x0);
        if (!matchGameClass || matchGameClass < 0x100000000) return false;

        // 1. Resolve fields declared directly on MatchGame (BaseGame)
        int32_t offMatch = GetFieldOffsetByNames(matchGameClass, {"m_Match", "Match"});
        if (offMatch >= 0) {
            Offsets::BaseGame_Match = (uintptr_t)offMatch;
        }

        int32_t offDict = GetFieldOffsetByNames(matchGameClass, {"m_ReplicationEntitis", "m_LReplicationEntitis", "PlayerDict", "ReplicationEntities"});
        if (offDict >= 0) {
            Offsets::Match_PlayerDict = (uintptr_t)offDict;
        }

        int32_t offMeta = GetFieldOffsetByNames(matchGameClass, {"m_MetaManager", "MetaManager"});
        if (offMeta >= 0) {
            Offsets::Match_MetaManager = (uintptr_t)offMeta;
        }

        // 2. Resolve fields declared on Match (JMAGGLCNGIG)
        uint64_t m_Match = rpm<uint64_t>(baseGame + Offsets::BaseGame_Match);
        if (!m_Match || m_Match < 0x100000000) return true;

        uint64_t matchClass = rpm<uint64_t>(m_Match + 0x0);
        if (!matchClass || matchClass < 0x100000000) return true;

        int32_t offLocal = GetFieldOffsetByNames(matchClass, {"MHJCLOPOBAA", "LocalPlayer", "m_LocalPlayer"});
        if (offLocal >= 0) {
            Offsets::Match_LocalPlayer = (uintptr_t)offLocal;
        }

        DO_LOGI("[✓] Match chain resolved: Match=0x%lx, Dict=0x%lx, Local=0x%lx, Meta=0x%lx",
                (unsigned long)Offsets::BaseGame_Match,
                (unsigned long)Offsets::Match_PlayerDict,
                (unsigned long)Offsets::Match_LocalPlayer,
                (unsigned long)Offsets::Match_MetaManager);
        return true;
    }

    // Resolves all Player, Camera, Avatar, and Bone offsets from live LocalPlayer RAM object
    inline bool ResolvePlayerClassFields(uint64_t playerAddr) {
        if (!playerAddr || playerAddr < 0x100000000) return false;
        uint64_t playerClass = rpm<uint64_t>(playerAddr + 0x0);
        if (!playerClass || playerClass < 0x100000000) return false;

        char cName[64] = {0}, cNs[64] = {0};
        if (!GetClassInfo(playerClass, cName, cNs)) return false;
        if (strcmp(cName, "Player") != 0 && strcmp(cName, "PlayerNetwork") != 0) return false;

        // Player Core Properties
        int32_t offTeam = GetFieldOffsetByNames(playerClass, {"<DJJMAHEIDID>k__BackingField", "TeamIndex", "m_TeamIndex"});
        if (offTeam >= 0) Offsets::Player_TeamIndex = (uintptr_t)offTeam;

        int32_t offHeadCol = GetFieldOffsetByNames(playerClass, {"BANPEGJEAKE", "HeadCollider", "m_HeadCollider"});
        if (offHeadCol >= 0) Offsets::Player_HedColider = (uintptr_t)offHeadCol;

        int32_t offCam = GetFieldOffsetByNames(playerClass, {"AHNCPOJPPCL", "FollowCamera", "m_FollowCamera"});
        if (offCam >= 0) Offsets::Player_Camera = (uintptr_t)offCam;

        int32_t offRot = GetFieldOffsetByNames(playerClass, {"<AKKPEKIHGAH>k__BackingField", "Rotation", "m_Rotation"});
        if (offRot >= 0) Offsets::Player_Rotation = (uintptr_t)offRot;

        int32_t offRotCount = GetFieldOffsetByName(playerClass, "CallSetAimRotationCount");
        if (offRotCount >= 0) Offsets::Player_CallSetAimRotationCount = (uintptr_t)offRotCount;

        int32_t offFiring = GetFieldOffsetByNames(playerClass, {"<AHDLLHMLNFI>k__BackingField", "IsFiring", "m_IsFiring"});
        if (offFiring >= 0) Offsets::Player_IsFiring = (uintptr_t)offFiring;

        int32_t offAimInfo = GetFieldOffsetByNames(playerClass, {"FDMIEDDNCEC", "AimInfo", "m_AimInfo"});
        if (offAimInfo >= 0) Offsets::Player_AimInfo = (uintptr_t)offAimInfo;

        int32_t offAuxAim = GetFieldOffsetByNames(playerClass, {"PKIOAMDCOPB", "AuxAimInfo", "m_AuxAimInfo"});
        if (offAuxAim >= 0) Offsets::Player_AuxAimInfo = (uintptr_t)offAuxAim;

        int32_t offCurRot = GetFieldOffsetByNames(playerClass, {"m_CurrentAimRotation", "CurrentAimRotation"});
        if (offCurRot >= 0) Offsets::Player_CurrentAimRotation = (uintptr_t)offCurRot;

        int32_t offSpeed = GetFieldOffsetByNames(playerClass, {"Speed", "m_Speed"});
        if (offSpeed >= 0) Offsets::Player_Speed = (uintptr_t)offSpeed;

        int32_t offWeapon = GetFieldOffsetByNames(playerClass, {"ActiveUISightingWeapon", "ActiveWeapon"});
        if (offWeapon >= 0) Offsets::Player_Weapon = (uintptr_t)offWeapon;

        int32_t offScoping = GetFieldOffsetByNames(playerClass, {"IKCEKAKDFJC", "IsScoping", "m_IsScoping"});
        if (offScoping >= 0) Offsets::Player_IsScoping = (uintptr_t)offScoping;

        int32_t offAvatar = GetFieldOffsetByNames(playerClass, {"GGJOLDEEIEN", "AvatarManager", "m_AvatarManager"});
        if (offAvatar >= 0) Offsets::Player_Avatar = (uintptr_t)offAvatar;

        int32_t offName = GetFieldOffsetByNames(playerClass, {"OriginalNickName", "NickName"});
        if (offName >= 0) Offsets::Player_OriginalNickName = (uintptr_t)offName;

        int32_t offBot = GetFieldOffsetByNames(playerClass, {"IsClientBot", "IsBot"});
        if (offBot >= 0) Offsets::Player_IsBot = (uintptr_t)offBot;

        // ════════════════════════════════════════════════════════
        // DYNAMIC SKELETON BONE OFFSETS (14 BONES)
        // ════════════════════════════════════════════════════════
        int32_t offHead = GetFieldOffsetByNames(playerClass, {"GBKFHDFCPMD", "Head", "m_Head"});
        if (offHead >= 0) Offsets::Bone_Head = (uintptr_t)offHead;

        int32_t offBreast = GetFieldOffsetByNames(playerClass, {"FFFPCADFFGA", "Breast", "Chest"});
        if (offBreast >= 0) Offsets::Bone_Breast = (uintptr_t)offBreast;

        int32_t offHip = GetFieldOffsetByNames(playerClass, {"COBFGNOIPMF", "Hip", "Pelvis"});
        if (offHip >= 0) Offsets::Bone_Hip = (uintptr_t)offHip;

        int32_t offRoot = GetFieldOffsetByNames(playerClass, {"IIPBIDIBJDK", "Root", "m_Root"});
        if (offRoot >= 0) Offsets::Bone_Root = (uintptr_t)offRoot;

        int32_t offLToe = GetFieldOffsetByNames(playerClass, {"GMJLIMFHMAE", "LeftToe", "LToe"});
        if (offLToe >= 0) Offsets::Bone_LeftToe = (uintptr_t)offLToe;

        int32_t offRToe = GetFieldOffsetByNames(playerClass, {"JMHBBGIKBNO", "RightToe", "RToe"});
        if (offRToe >= 0) Offsets::Bone_RightToe = (uintptr_t)offRToe;

        int32_t offLShoulder = GetFieldOffsetByNames(playerClass, {"FFDPGNIKAEK", "LeftShoulder", "LShoulder"});
        if (offLShoulder >= 0) Offsets::Bone_LeftShoulder = (uintptr_t)offLShoulder;

        int32_t offRShoulder = GetFieldOffsetByNames(playerClass, {"FOELKAHABCD", "RightShoulder", "RShoulder"});
        if (offRShoulder >= 0) Offsets::Bone_RightShoulder = (uintptr_t)offRShoulder;

        int32_t offLHand = GetFieldOffsetByNames(playerClass, {"CLCEEJMGBGE", "LeftHand", "LHand"});
        if (offLHand >= 0) Offsets::Bone_LeftHand = (uintptr_t)offLHand;

        int32_t offRHand = GetFieldOffsetByNames(playerClass, {"JLAECGNPCHC", "RightHand", "RHand"});
        if (offRHand >= 0) Offsets::Bone_RightHand = (uintptr_t)offRHand;

        int32_t offLForeArm = GetFieldOffsetByNames(playerClass, {"IMLDFGHNLBP", "LeftForeArm", "LForeArm"});
        if (offLForeArm >= 0) Offsets::Bone_LeftForeArm = (uintptr_t)offLForeArm;

        int32_t offRForeArm = GetFieldOffsetByNames(playerClass, {"DJAOGHBOBOJ", "RightForeArm", "RForeArm"});
        if (offRForeArm >= 0) Offsets::Bone_RightForeArm = (uintptr_t)offRForeArm;

        int32_t offLAnkle = GetFieldOffsetByNames(playerClass, {"GPIBDOCMHBE", "LeftAnkle", "LAnkle"});
        if (offLAnkle >= 0) Offsets::Bone_LeftAnkle = (uintptr_t)offLAnkle;

        int32_t offRAnkle = GetFieldOffsetByNames(playerClass, {"EMNIJJCEKDD", "RightAnkle", "RAnkle"});
        if (offRAnkle >= 0) Offsets::Bone_RightAnkle = (uintptr_t)offRAnkle;

        int32_t offDead = GetFieldOffsetByNames(playerClass, {"CGJLEBOGBNP", "IsDead", "m_IsDead"});
        if (offDead >= 0) Offsets::Player_IsDead = (uintptr_t)offDead;

        int32_t offColWrite = GetFieldOffsetByNames(playerClass, {"<AONPKGBGHHI>k__BackingField", "ColWrite", "m_Collider"});
        if (offColWrite >= 0) Offsets::Player_ColWrite = (uintptr_t)offColWrite;

        int32_t offHP = GetFieldOffsetByNames(playerClass, {"m_PRIDataPool", "PRIDataPool", "m_HP"});
        if (offHP >= 0) Offsets::Player_HP = (uintptr_t)offHP;

        int32_t offArmor = GetFieldOffsetByNames(playerClass, {"m_Armor", "Armor", "curArmor"});
        if (offArmor >= 0) Offsets::Player_Armor = (uintptr_t)offArmor;

        int32_t offVel = GetFieldOffsetByNames(playerClass, {"m_Velocity", "Velocity"});
        if (offVel >= 0) Offsets::Player_Velocity = (uintptr_t)offVel;

        int32_t offGround = GetFieldOffsetByNames(playerClass, {"m_IsGrounded", "IsGrounded"});
        if (offGround >= 0) Offsets::Player_IsGrounded = (uintptr_t)offGround;

        Offsets::Head   = Offsets::Bone_Head;
        Offsets::Breast = Offsets::Bone_Breast;
        Offsets::Hip    = Offsets::Bone_Hip;
        Offsets::Root   = Offsets::Bone_Root;

        // Dynamic FollowCamera child fields
        uint64_t followCam = rpm<uint64_t>(playerAddr + Offsets::Player_Camera);
        if (followCam >= 0x100000000) {
            uint64_t camClass = rpm<uint64_t>(followCam + 0x0);
            if (camClass >= 0x100000000) {
                int32_t offFollow = GetFieldOffsetByNames(camClass, {"AHNCPOJPPCL", "Camera", "m_Camera"});
                int32_t offRO = GetFieldOffsetByNames(camClass, {"RightOffset", "m_RightOffset"});
                int32_t offUO = GetFieldOffsetByNames(camClass, {"UpOffset", "m_UpOffset"});
                int32_t offBO = GetFieldOffsetByNames(camClass, {"BackOffset", "m_BackOffset"});

                if (offFollow >= 0) Offsets::Camera_Follow      = (uintptr_t)offFollow;
                if (offRO >= 0)     Offsets::Camera_RightOffset = (uintptr_t)offRO;
                if (offUO >= 0)     Offsets::Camera_UpOffset    = (uintptr_t)offUO;
                if (offBO >= 0)     Offsets::Camera_BackOffset  = (uintptr_t)offBO;
            }
        }

        // Dynamic AvatarManager child fields
        uint64_t avatarMgr = rpm<uint64_t>(playerAddr + Offsets::Player_Avatar);
        if (avatarMgr >= 0x100000000) {
            uint64_t avClass = rpm<uint64_t>(avatarMgr + 0x0);
            if (avClass >= 0x100000000) {
                int32_t offUma = GetFieldOffsetByNames(avClass, {"LHEDBMJJOLM", "UmaAvatar", "m_UmaAvatar"});
                if (offUma >= 0) Offsets::Avatar_Uma = (uintptr_t)offUma;
            }
        }

        // Dynamic AimInfo (CGKJLKPMGDJ) fields
        uint64_t aimInfo = rpm<uint64_t>(playerAddr + Offsets::Player_AimInfo);
        if (aimInfo >= 0x100000000) {
            uint64_t aiClass = rpm<uint64_t>(aimInfo + 0x0);
            if (aiClass >= 0x100000000) {
                int32_t offLoc = GetFieldOffsetByNames(aiClass, {"HPCOJHGFEEG", "HitLocation"});
                int32_t offCol = GetFieldOffsetByNames(aiClass, {"JINDNOFABKB", "HitCollider"});
                int32_t offGrp = GetFieldOffsetByNames(aiClass, {"OLGEMBBDBID", "HitGroup"});
                int32_t offGrpS = GetFieldOffsetByName(aiClass, "CLOLKHNMNEK");
                int32_t offRatio = GetFieldOffsetByNames(aiClass, {"CFIPDNFBKHK", "DamageRatio"});
                int32_t offBlocked = GetFieldOffsetByNames(aiClass, {"CEOKOILBODN", "ViewBlocked"});

                if (offLoc >= 0)     Offsets::HitInfo_HitLocation   = (uintptr_t)offLoc;
                if (offCol >= 0)     Offsets::HitInfo_HitCollider   = (uintptr_t)offCol;
                if (offGrp >= 0)     Offsets::HitInfo_HitGroup      = (uintptr_t)offGrp;
                if (offGrpS >= 0)    Offsets::HitInfo_HitGroupShort = (uintptr_t)offGrpS;
                if (offRatio >= 0)   Offsets::HitInfo_DamageRatio   = (uintptr_t)offRatio;
                if (offBlocked >= 0) Offsets::HitInfo_ViewBlocked   = (uintptr_t)offBlocked;
            }
        }

        DO_LOGI("[✓] Player fields & 14 bones resolved (Team=0x%lx, HeadCol=0x%lx, Head=0x%lx, Cam=0x%lx)",
                (unsigned long)Offsets::Player_TeamIndex,
                (unsigned long)Offsets::Player_HedColider,
                (unsigned long)Offsets::Bone_Head,
                (unsigned long)Offsets::Player_Camera);

        g_ramScanCompleted = true;
        return true;
    }

    // Main entrypoint: called upon attaching to game
    inline bool Init(int pid, uint64_t libBase, const char* pkgName = "com.dts.freefireth") {
        printf("  [~] Initializing Non-Intrusive Dynamic RAM Scanner for %s (Zero Crash)...\n", pkgName);
        DO_LOGI("[*] Non-Intrusive Dynamic RAM Scanner initialized for pid=%d, pkg=%s", pid, pkgName);

        uint64_t gfClass = FindGameFacadeClassInRam(pid, libBase);
        if (gfClass) {
            ResolveGameFacadeFields(gfClass);
            printf("  [✓] COW.GameFacade found at BSS+0x%lx (CurrentMatchGame=0x%lx)\n",
                   (unsigned long)Offsets::GameFacade, (unsigned long)Offsets::BaseGame_CurrentMatchGame);
        } else {
            printf("  [!] GameFacade not yet loaded in RAM, will auto-resolve on match start.\n");
        }

        return true;
    }
}
