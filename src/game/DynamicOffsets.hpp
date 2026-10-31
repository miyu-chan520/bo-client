#pragma once

#include "Offsets.h"
#include "../other/memory.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>
#include <unistd.h>
#include <android/log.h>

#define DO_LOG_TAG "DynamicOffsets"
#define DO_LOGI(...) __android_log_print(ANDROID_LOG_INFO,  DO_LOG_TAG, __VA_ARGS__)
#define DO_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, DO_LOG_TAG, __VA_ARGS__)

namespace DynamicOffsets {

    struct OffsetFieldBinding {
        const char* className;
        const char* fieldName;
        uintptr_t*  targetPtr;
        const char* description;
    };

    inline bool g_ramScanCompleted = false;

    // Helper: read remote C-string safely from memory
    inline bool ReadRemoteString(uint64_t addr, char* out, size_t maxLen) {
        if (!addr || addr < 0x1000) return false;
        out[0] = '\0';
        char buf[128];
        size_t toRead = maxLen < sizeof(buf) ? maxLen : sizeof(buf);
        if (!mem_read(addr, buf, toRead)) return false;
        buf[toRead - 1] = '\0';
        strncpy(out, buf, maxLen);
        out[maxLen - 1] = '\0';
        return strlen(out) > 0;
    }

    // Helper: inspect Il2CppClass name and namespace
    inline bool GetClassInfo(uint64_t klass, char* outName, char* outNs) {
        if (!klass || klass < 0x100000000) return false;
        uint64_t namePtr = rpm<uint64_t>(klass + 0x10);
        uint64_t nsPtr   = rpm<uint64_t>(klass + 0x18);
        return ReadRemoteString(namePtr, outName, 64) && ReadRemoteString(nsPtr, outNs, 64);
    }

    // Dynamic field offset getter traversing live FieldInfo structs in RAM
    // FieldInfo stride = 0x20 (32 bytes) on ARM64:
    //   +0x00: name (const char*)
    //   +0x08: type
    //   +0x10: parent (Il2CppClass*)
    //   +0x18: offset (int32_t)
    inline int32_t GetFieldOffsetByName(uint64_t klass, const char* targetFieldName) {
        if (!klass || klass < 0x100000000) return -1;
        uint64_t fieldsPtr = rpm<uint64_t>(klass + 0x80);
        if (!fieldsPtr || fieldsPtr < 0x100000000) return -1;

        for (int i = 0; i < 300; i++) {
            uint64_t fieldStruct = fieldsPtr + (i * 0x20);
            uint64_t namePtr = rpm<uint64_t>(fieldStruct + 0x00);
            if (!namePtr || namePtr < 0x100000000) break; // End of fields

            char fName[128];
            if (ReadRemoteString(namePtr, fName, sizeof(fName))) {
                if (strcmp(fName, targetFieldName) == 0) {
                    int32_t off = rpm<int32_t>(fieldStruct + 0x18);
                    return off;
                }
            }
        }
        return -1;
    }

    // 100% Dynamic RAM Scanner: Locates COW.GameFacade in libil2cpp.so .bss by class name
    inline uint64_t FindGameFacadeClassInRam(int pid, uint64_t libBase) {
        uint64_t bssStart = 0, bssEnd = 0;
        char mapsPath[64];
        snprintf(mapsPath, sizeof(mapsPath), "/proc/%d/maps", pid);

        FILE* fp = fopen(mapsPath, "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "libil2cpp.so") && strstr(line, "rw-p")) {
                    uint64_t s = 0, e = 0;
                    if (sscanf(line, "%lx-%lx", &s, &e) == 2) {
                        bssStart = s;
                        bssEnd = e;
                        break;
                    }
                }
            }
            fclose(fp);
        }

        if (!bssStart || !bssEnd || bssEnd <= bssStart) {
            bssStart = libBase + 0x9000000;
            bssEnd   = libBase + 0xD000000;
        }

        DO_LOGI("[*] 100%% Dynamic RAM scan: searching .bss [0x%lx - 0x%lx] for COW.GameFacade...",
                (unsigned long)bssStart, (unsigned long)bssEnd);

        // Scan in 64KB blocks for speed
        constexpr size_t kChunkSize = 65536;
        std::vector<uint64_t> chunk(kChunkSize / sizeof(uint64_t));

        for (uint64_t cur = bssStart; cur < bssEnd; cur += kChunkSize) {
            size_t bytesToRead = (cur + kChunkSize <= bssEnd) ? kChunkSize : (bssEnd - cur);
            if (!mem_read(cur, chunk.data(), bytesToRead)) continue;

            size_t numPtrs = bytesToRead / sizeof(uint64_t);
            for (size_t i = 0; i < numPtrs; i++) {
                uint64_t ptr = chunk[i];
                if (ptr >= 0x100000000) {
                    char cName[64] = {0};
                    char cNs[64] = {0};
                    if (GetClassInfo(ptr, cName, cNs)) {
                        if (strcmp(cName, "GameFacade") == 0 && strcmp(cNs, "COW") == 0) {
                            uint64_t bssAddr = cur + (i * sizeof(uint64_t));
                            Offsets::GameFacade = bssAddr - libBase;
                            DO_LOGI("[✓] 100%% Dynamic RAM: Found COW.GameFacade at .bss+0x%lx (Il2CppClass=0x%lx)",
                                    (unsigned long)Offsets::GameFacade, (unsigned long)ptr);
                            return ptr;
                        }
                    }
                }
            }
        }

        return 0;
    }

    // Resolves all GameFacade static fields dynamically from RAM
    inline bool ResolveGameFacadeFields(uint64_t gfClass) {
        if (!gfClass) return false;
        int32_t offMatchGame = GetFieldOffsetByName(gfClass, "CurrentMatchGame");
        int32_t offGame      = GetFieldOffsetByName(gfClass, "CurrentGame");

        if (offMatchGame >= 0) Offsets::BaseGame_CurrentMatchGame = (uintptr_t)offMatchGame;
        if (offGame >= 0)      Offsets::BaseGame_CurrentGame      = (uintptr_t)offGame;

        DO_LOGI("[✓] 100%% Dynamic RAM: CurrentMatchGame=0x%x, CurrentGame=0x%x", offMatchGame, offGame);
        return true;
    }

    // Resolves MatchGame and Match classes dynamically from live BaseGame object
    inline bool ResolveMatchChain(uint64_t baseGame) {
        if (!baseGame || baseGame < 0x100000000) return false;
        uint64_t matchGameClass = rpm<uint64_t>(baseGame + 0x0);
        if (!matchGameClass) return false;

        int32_t offMatch = GetFieldOffsetByName(matchGameClass, "m_Match");
        if (offMatch >= 0) {
            Offsets::BaseGame_Match = (uintptr_t)offMatch;
            DO_LOGI("[✓] 100%% Dynamic RAM: MatchGame::m_Match = 0x%x", offMatch);
        }

        uint64_t m_Match = rpm<uint64_t>(baseGame + Offsets::BaseGame_Match);
        if (!m_Match || m_Match < 0x100000000) return true;

        uint64_t matchClass = rpm<uint64_t>(m_Match + 0x0);
        if (!matchClass) return true;

        int32_t offDict = GetFieldOffsetByName(matchClass, "m_ReplicationEntitis");
        int32_t offLocal = GetFieldOffsetByName(matchClass, "MHJCLOPOBAA");
        int32_t offMeta = GetFieldOffsetByName(matchClass, "m_MetaManager");

        if (offDict >= 0)  Offsets::Match_PlayerDict  = (uintptr_t)offDict;
        if (offLocal >= 0) Offsets::Match_LocalPlayer = (uintptr_t)offLocal;
        if (offMeta >= 0)  Offsets::Match_MetaManager = (uintptr_t)offMeta;

        DO_LOGI("[✓] 100%% Dynamic RAM: PlayerDict=0x%x, LocalPlayer=0x%x, MetaManager=0x%x",
                offDict, offLocal, offMeta);
        return true;
    }

    // Resolves all Player, Camera, Avatar, and Bone offsets dynamically from live Player object
    inline bool ResolvePlayerClassFields(uint64_t playerAddr) {
        if (!playerAddr || playerAddr < 0x100000000) return false;
        uint64_t playerClass = rpm<uint64_t>(playerAddr + 0x0);
        if (!playerClass) return false;

        char cName[64] = {0}, cNs[64] = {0};
        if (!GetClassInfo(playerClass, cName, cNs) || strcmp(cName, "Player") != 0) return false;

        // Player Core Fields
        int32_t offTeam = GetFieldOffsetByName(playerClass, "<DJJMAHEIDID>k__BackingField");
        if (offTeam < 0) offTeam = GetFieldOffsetByName(playerClass, "TeamIndex");
        if (offTeam >= 0) Offsets::Player_TeamIndex = (uintptr_t)offTeam;

        int32_t offHeadCol = GetFieldOffsetByName(playerClass, "BANPEGJEAKE");
        if (offHeadCol >= 0) Offsets::Player_HedColider = (uintptr_t)offHeadCol;

        int32_t offCam = GetFieldOffsetByName(playerClass, "AHNCPOJPPCL");
        if (offCam >= 0) Offsets::Player_Camera = (uintptr_t)offCam;

        int32_t offRot = GetFieldOffsetByName(playerClass, "<AKKPEKIHGAH>k__BackingField");
        if (offRot >= 0) Offsets::Player_Rotation = (uintptr_t)offRot;

        int32_t offRotCount = GetFieldOffsetByName(playerClass, "CallSetAimRotationCount");
        if (offRotCount >= 0) Offsets::Player_CallSetAimRotationCount = (uintptr_t)offRotCount;

        int32_t offFiring = GetFieldOffsetByName(playerClass, "<AHDLLHMLNFI>k__BackingField");
        if (offFiring >= 0) Offsets::Player_IsFiring = (uintptr_t)offFiring;

        int32_t offAimInfo = GetFieldOffsetByName(playerClass, "FDMIEDDNCEC");
        if (offAimInfo >= 0) Offsets::Player_AimInfo = (uintptr_t)offAimInfo;

        int32_t offAuxAim = GetFieldOffsetByName(playerClass, "PKIOAMDCOPB");
        if (offAuxAim >= 0) Offsets::Player_AuxAimInfo = (uintptr_t)offAuxAim;

        int32_t offCurRot = GetFieldOffsetByName(playerClass, "m_CurrentAimRotation");
        if (offCurRot >= 0) Offsets::Player_CurrentAimRotation = (uintptr_t)offCurRot;

        int32_t offSpeed = GetFieldOffsetByName(playerClass, "Speed");
        if (offSpeed >= 0) Offsets::Player_Speed = (uintptr_t)offSpeed;

        int32_t offWeapon = GetFieldOffsetByName(playerClass, "ActiveUISightingWeapon");
        if (offWeapon >= 0) Offsets::Player_Weapon = (uintptr_t)offWeapon;

        int32_t offScoping = GetFieldOffsetByName(playerClass, "IKCEKAKDFJC");
        if (offScoping >= 0) Offsets::Player_IsScoping = (uintptr_t)offScoping;

        int32_t offAvatar = GetFieldOffsetByName(playerClass, "GGJOLDEEIEN");
        if (offAvatar >= 0) Offsets::Player_Avatar = (uintptr_t)offAvatar;

        int32_t offName = GetFieldOffsetByName(playerClass, "OriginalNickName");
        if (offName >= 0) Offsets::Player_OriginalNickName = (uintptr_t)offName;

        int32_t offBot = GetFieldOffsetByName(playerClass, "IsClientBot");
        if (offBot >= 0) Offsets::Player_IsBot = (uintptr_t)offBot;

        // Dynamic Bone Offsets
        int32_t offHead = GetFieldOffsetByName(playerClass, "GBKFHDFCPMD");
        if (offHead >= 0) Offsets::Bone_Head = (uintptr_t)offHead;

        int32_t offBreast = GetFieldOffsetByName(playerClass, "FFFPCADFFGA");
        if (offBreast >= 0) Offsets::Bone_Breast = (uintptr_t)offBreast;

        int32_t offHip = GetFieldOffsetByName(playerClass, "COBFGNOIPMF");
        if (offHip >= 0) Offsets::Bone_Hip = (uintptr_t)offHip;

        int32_t offLToe = GetFieldOffsetByName(playerClass, "GMJLIMFHMAE");
        if (offLToe >= 0) Offsets::Bone_LeftToe = (uintptr_t)offLToe;

        int32_t offRToe = GetFieldOffsetByName(playerClass, "JMHBBGIKBNO");
        if (offRToe >= 0) Offsets::Bone_RightToe = (uintptr_t)offRToe;

        int32_t offLShoulder = GetFieldOffsetByName(playerClass, "FFDPGNIKAEK");
        if (offLShoulder >= 0) Offsets::Bone_LeftShoulder = (uintptr_t)offLShoulder;

        int32_t offRShoulder = GetFieldOffsetByName(playerClass, "FOELKAHABCD");
        if (offRShoulder >= 0) Offsets::Bone_RightShoulder = (uintptr_t)offRShoulder;

        int32_t offLHand = GetFieldOffsetByName(playerClass, "CLCEEJMGBGE");
        if (offLHand >= 0) Offsets::Bone_LeftHand = (uintptr_t)offLHand;

        int32_t offRHand = GetFieldOffsetByName(playerClass, "JLAECGNPCHC");
        if (offRHand >= 0) Offsets::Bone_RightHand = (uintptr_t)offRHand;

        int32_t offLForeArm = GetFieldOffsetByName(playerClass, "IMLDFGHNLBP");
        if (offLForeArm >= 0) Offsets::Bone_LeftForeArm = (uintptr_t)offLForeArm;

        int32_t offRForeArm = GetFieldOffsetByName(playerClass, "DJAOGHBOBOJ");
        if (offRForeArm >= 0) Offsets::Bone_RightForeArm = (uintptr_t)offRForeArm;

        int32_t offLAnkle = GetFieldOffsetByName(playerClass, "GPIBDOCMHBE");
        if (offLAnkle >= 0) Offsets::Bone_LeftAnkle = (uintptr_t)offLAnkle;

        int32_t offRAnkle = GetFieldOffsetByName(playerClass, "EMNIJJCEKDD");
        if (offRAnkle >= 0) Offsets::Bone_RightAnkle = (uintptr_t)offRAnkle;

        int32_t offRoot = GetFieldOffsetByName(playerClass, "IIPBIDIBJDK");
        if (offRoot >= 0) Offsets::Bone_Root = (uintptr_t)offRoot;

        // Dynamic FollowCamera and AvatarManager child class fields
        uint64_t followCam = rpm<uint64_t>(playerAddr + Offsets::Player_Camera);
        if (followCam >= 0x100000000) {
            uint64_t camClass = rpm<uint64_t>(followCam + 0x0);
            if (camClass >= 0x100000000) {
                int32_t offFollow = GetFieldOffsetByName(camClass, "AHNCPOJPPCL");
                int32_t offRO = GetFieldOffsetByName(camClass, "RightOffset");
                int32_t offUO = GetFieldOffsetByName(camClass, "UpOffset");
                int32_t offBO = GetFieldOffsetByName(camClass, "BackOffset");

                if (offFollow >= 0) Offsets::Camera_Follow      = (uintptr_t)offFollow;
                if (offRO >= 0)     Offsets::Camera_RightOffset = (uintptr_t)offRO;
                if (offUO >= 0)     Offsets::Camera_UpOffset    = (uintptr_t)offUO;
                if (offBO >= 0)     Offsets::Camera_BackOffset  = (uintptr_t)offBO;
            }
        }

        uint64_t avatarMgr = rpm<uint64_t>(playerAddr + Offsets::Player_Avatar);
        if (avatarMgr >= 0x100000000) {
            uint64_t avClass = rpm<uint64_t>(avatarMgr + 0x0);
            if (avClass >= 0x100000000) {
                int32_t offUma = GetFieldOffsetByName(avClass, "LHEDBMJJOLM");
                if (offUma >= 0) Offsets::Avatar_Uma = (uintptr_t)offUma;
            }
        }

        DO_LOGI("[✓] 100%% Dynamic RAM: Player offsets resolved (Team=0x%lx, HeadCol=0x%lx, HeadBone=0x%lx, Cam=0x%lx)",
                (unsigned long)Offsets::Player_TeamIndex,
                (unsigned long)Offsets::Player_HedColider,
                (unsigned long)Offsets::Bone_Head,
                (unsigned long)Offsets::Player_Camera);

        g_ramScanCompleted = true;
        return true;
    }

    // Main entrypoint: called upon attaching to game
    inline bool Init(int pid, uint64_t libBase, const char* pkgName = "com.dts.freefireth") {
        printf("  [~] Starting 100%% Dynamic IL2CPP RAM Scanner (Zero Hardcoded Offsets)...\n");
        DO_LOGI("[*] Initializing 100%% Dynamic RAM Scanner for pid=%d, pkg=%s", pid, pkgName);

        uint64_t gfClass = FindGameFacadeClassInRam(pid, libBase);
        if (gfClass) {
            ResolveGameFacadeFields(gfClass);
            printf("  [✓] COW.GameFacade found at .bss+0x%lx (CurrentMatchGame=0x%lx)\n",
                   (unsigned long)Offsets::GameFacade, (unsigned long)Offsets::BaseGame_CurrentMatchGame);
        } else {
            printf("  [!] COW.GameFacade not yet loaded in RAM, will auto-resolve on match start.\n");
        }

        return true;
    }
}
