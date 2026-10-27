#pragma once

#include "Offsets.h"
#include "../other/vector3.h"
#include "../other/memory.hpp"
#include "../other/utils.hpp"
#include "math.hpp"
#include "data.hpp"
#include "../protect/oxorany.hpp"
#include <cstdint>
#include <mutex>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <android/log.h>

// ===================== AIM SILENT (MANUAL HOOK) =====================
#define AS_LOG(...) do { \
    __android_log_print(ANDROID_LOG_INFO, "AimSilent", __VA_ARGS__); \
    printf("[AimSilent] " __VA_ARGS__); \
    printf("\n"); \
    fflush(stdout); \
} while(0)

// ==================== GLOBAL TOGGLES ====================
inline bool AimSilentEnabled = false;
inline bool AimSilentNobita  = false;
static bool g_AimSilentHookInstalled = false;

// ==================== HELPER FUNCTIONS ====================

static void *GetHeadColliderSilent(void *player) {
    if (!player) return nullptr;
    void *(*_GetHead)(void *) = (void *(*)(void *))((uintptr_t)proc::lib + Offsets::Player_get_HeadCollider);
    return _GetHead(player);
}

static void *GetHeadTFSilent(void *player) {
    if (!player) return nullptr;
    void *(*_GetHead)(void *) = (void *(*)(void *))((uintptr_t)proc::lib + Offsets::Player_GetHeadTF);
    return _GetHead(player);
}

static Vector3 GetHeadPositionSilent(void *player) {
    if (!player) return Vector3::Zero();
    void *headTF = GetHeadTFSilent(player);
    if (!headTF) return Vector3::Zero();
    return GetNodePosition((uint64_t)headTF);
}

static void *GetClosestEnemySilent() {
    std::lock_guard<std::mutex> lk(data::players_mutex);

    void *bestTarget = nullptr;
    float bestDist = 99999.0f;

    uint64_t localAddr = data::localPlayerAddr.load(std::memory_order_relaxed);
    if (!localAddr) return nullptr;

    for (auto& p : data::players) {
        if (!p.addr) continue;
        if (p.addr == localAddr) continue;
        if (p.curHP <= 0) continue;
        if (p.distance > 1000.0f) continue;

        if (p.distance < bestDist) {
            bestDist = p.distance;
            bestTarget = (void*)p.addr;
        }
    }

    return bestTarget;
}

// ==================== TEST WRITE ====================
static inline bool TestWrite(uintptr_t addr) {
    uint32_t orig = rpm<uint32_t>(addr);
    AS_LOG("Test read @0x%lx = 0x%08x", (unsigned long)addr, orig);

    bool wr = wpm<uint32_t>(addr, orig);
    AS_LOG("Test write @0x%lx = %d", (unsigned long)addr, wr);
    return wr;
}

// ==================== HOOK IMPLEMENTATION ====================
// Hook Player.TakeDamage (9-param)
static int (*orig_OnCalcDamage_9)(void *player, int damage, uint damagerID,
                         void *damageInfo, int hitGroup,
                         Vector3 hitPos, Vector3 hitNormal,
                         void *checkParams, void *weaponInfo, uint vehicleID) = nullptr;

static int hook_OnCalcDamage_9(void *player, int damage, uint damagerID,
                      void *damageInfo, int hitGroup,
                      Vector3 hitPos, Vector3 hitNormal,
                      void *checkParams, void *weaponInfo, uint vehicleID) {

    static int callCount = 0;
    if (callCount++ < 30) {
        AS_LOG("HOOK9 #%d | player=%p dmgInfo=%p hitGroup=%d dmg=%d",
               callCount, player, damageInfo, hitGroup, damage);
    }

    if (!AimSilentEnabled && !AimSilentNobita) {
        return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                                 hitGroup, hitPos, hitNormal,
                                 checkParams, weaponInfo, vehicleID);
    }

    if (!player || !damageInfo) {
        return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                                 hitGroup, hitPos, hitNormal,
                                 checkParams, weaponInfo, vehicleID);
    }

    AS_LOG("AIM ACTIVE (9)! player=%p dmgInfo=%p", player, damageInfo);

    void *target = GetClosestEnemySilent();
    if (!target) {
        return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                                 hitGroup, hitPos, hitNormal,
                                 checkParams, weaponInfo, vehicleID);
    }

    Vector3 enemyHeadPos = GetHeadPositionSilent(target);
    if (enemyHeadPos == Vector3::Zero()) {
        return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                                 hitGroup, hitPos, hitNormal,
                                 checkParams, weaponInfo, vehicleID);
    }

    Vector3 myPos = Vector3::Zero();
    {
        uint64_t localAddr = data::localPlayerAddr.load(std::memory_order_relaxed);
        if (localAddr) {
            uint64_t headNode = rpm<uint64_t>(localAddr + Offsets::Bone_Head);
            if (headNode) myPos = GetNodePosition(headNode);
        }
    }

    Vector3 direction = (myPos == Vector3::Zero())
                        ? hitPos
                        : Vector3::Normalize(enemyHeadPos - myPos);

    AS_LOG("MODIFY: headPos=(%.1f,%.1f,%.1f)",
           enemyHeadPos.x, enemyHeadPos.y, enemyHeadPos.z);

    *(short *)((uintptr_t)damageInfo + Offsets::DmgInfo_HitGroup) = 1;
    *(Vector3 *)((uintptr_t)damageInfo + Offsets::DmgInfo_HitLocation) = enemyHeadPos;
    *(Vector3 *)((uintptr_t)damageInfo + Offsets::DmgInfo_HitNormal) = direction;
    *(Vector3 *)((uintptr_t)damageInfo + Offsets::DmgInfo_RayDir) = direction;

    hitGroup = 1;

    return orig_OnCalcDamage_9(player, damage, damagerID, damageInfo,
                              hitGroup, enemyHeadPos, direction,
                              checkParams, weaponInfo, vehicleID);
}

// ==================== HOOK INIT (MANUAL) ====================
static inline void InitAimSilentHook() {
    if (g_AimSilentHookInstalled) return;

    printf("[AimSilent] === Init start (manual hook) ===\n");
    fflush(stdout);

    uintptr_t base = (uintptr_t)proc::lib;
    printf("[AimSilent] base=0x%lx\n", (unsigned long)base);
    fflush(stdout);

    if (!base) {
        printf("[AimSilent] ERROR: base=0!\n");
        fflush(stdout);
        return;
    }

    uintptr_t addr9 = base + Offsets::Player_TakeDamage_9;
    printf("[AimSilent] Hook9 addr=0x%lx\n", (unsigned long)addr9);
    fflush(stdout);

    // ── TEST WRITE ──
    if (!TestWrite(addr9)) {
        printf("[AimSilent] ERROR: mem_write GAGAL! Tidak bisa hook.\n");
        fflush(stdout);
        return;
    }

    // ── MANUAL HOOK ──
    // Catatan: ini butuh trampoline untuk ARM64.
    // Untuk sekarang, kita cuma test apakah write bisa.
    // Kalau write bisa, nanti kita bikin trampoline.

    printf("[AimSilent] mem_write OK! Bisa lanjut manual hook.\n");
    fflush(stdout);

    g_AimSilentHookInstalled = true;
}