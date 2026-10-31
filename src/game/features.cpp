#include "features.hpp"
#include "scanner.hpp"
#include "Offsets.h"
#include "data.hpp"
#include "math.hpp"
#include "../shadowhook/shadowhook.h"
#include <android/log.h>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <cmath>
#include <GLES3/gl3.h>

#define LOG_TAG "BoFeatures"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace features {

Config g_cfg;

// Original function pointers
static int (*orig_TakeDamage_9)(void *player, int damage, uint32_t damagerID,
                                void *damageInfo, int hitGroup,
                                Vector3 hitPos, Vector3 hitNormal,
                                void *checkParams, void *weaponInfo, uint32_t vehicleID) = nullptr;

static int (*orig_TakeDamage_4)(void *player, int damage, uint32_t damagerID, void *damageInfo) = nullptr;

static void (*orig_glDrawElements)(GLenum mode, GLsizei count, GLenum type, const void *indices) = nullptr;

// ============================================================================
// HELPER: Target Selection
// ============================================================================
static void* get_best_target(Vector3& outHeadPos) {
    std::lock_guard<std::mutex> lk(data::players_mutex);
    uint64_t localAddr = data::localPlayerAddr.load(std::memory_order_relaxed);
    if (!localAddr) return nullptr;

    void* bestTarget = nullptr;
    float bestFov = g_cfg.aim_fov;
    float bestDist = 99999.0f;

    for (auto& p : data::players) {
        if (!p.addr || p.addr == localAddr) continue;
        if (p.curHP <= 0 || p.isDead) continue;
        if (g_cfg.aim_visible_only && !p.isVisible) continue;

        Vector3 headPos = p.headPos;
        if (headPos == Vector3::Zero()) continue;

        if (g_cfg.silent_aim) {
            // Pick closest in 3D distance or screen FOV
            if (p.distance < bestDist) {
                bestDist = p.distance;
                bestTarget = (void*)p.addr;
                outHeadPos = headPos;
            }
        }
    }
    return bestTarget;
}

// ============================================================================
// SILENT AIM & UNDERKILL HOOK (Player.TakeDamage)
// ============================================================================
static int hook_TakeDamage_9(void *player, int damage, uint32_t damagerID,
                             void *damageInfo, int hitGroup,
                             Vector3 hitPos, Vector3 hitNormal,
                             void *checkParams, void *weaponInfo, uint32_t vehicleID) {
    if (!g_cfg.aim_enable && !g_cfg.silent_aim && !g_cfg.underkill && !g_cfg.autokill) {
        return orig_TakeDamage_9(player, damage, damagerID, damageInfo,
                                hitGroup, hitPos, hitNormal,
                                checkParams, weaponInfo, vehicleID);
    }

    if (!damageInfo) {
        return orig_TakeDamage_9(player, damage, damagerID, damageInfo,
                                hitGroup, hitPos, hitNormal,
                                checkParams, weaponInfo, vehicleID);
    }

    Vector3 targetHead = Vector3::Zero();
    void* target = get_best_target(targetHead);

    if (target && targetHead != Vector3::Zero()) {
        Vector3 aimPoint = targetHead;

        // UNDERKILL: Inject damage from below ground level to bypass all covers
        if (g_cfg.underkill) {
            aimPoint.y -= g_cfg.underkill_depth;
            hitNormal = Vector3(0.0f, 1.0f, 0.0f); // Point upwards
        }

        Vector3 myPos = Vector3::Zero();
        uint64_t localAddr = data::localPlayerAddr.load(std::memory_order_relaxed);
        if (localAddr) {
            uint64_t headNode = *(uint64_t*)(localAddr + scanner::g_Offsets.Player_HeadTF);
            if (headNode) myPos = GetNodePosition(headNode);
        }

        Vector3 direction = (myPos == Vector3::Zero()) ? hitNormal : Vector3::Normalize(aimPoint - myPos);

        // Force Headshot hitgroup = 1
        *(short*)((uintptr_t)damageInfo + Offsets::DmgInfo_HitGroup) = 1;
        *(Vector3*)((uintptr_t)damageInfo + Offsets::DmgInfo_HitLocation) = aimPoint;
        *(Vector3*)((uintptr_t)damageInfo + Offsets::DmgInfo_HitNormal) = direction;
        *(Vector3*)((uintptr_t)damageInfo + Offsets::DmgInfo_RayDir) = direction;

        hitGroup = 1;
        hitPos = aimPoint;
        hitNormal = direction;

        if (g_cfg.autokill) {
            damage = 999;
            *(int*)((uintptr_t)damageInfo + Offsets::DmgInfo_Damage) = 999;
        }

        LOGI("[*] SilentAim/Underkill applied! Headshot redirection @ (%.1f, %.1f, %.1f)",
             aimPoint.x, aimPoint.y, aimPoint.z);
    }

    return orig_TakeDamage_9(player, damage, damagerID, damageInfo,
                            hitGroup, hitPos, hitNormal,
                            checkParams, weaponInfo, vehicleID);
}

// ============================================================================
// CHAMS HOOK (GLES3 Depth Override)
// ============================================================================
static void hook_glDrawElements_Chams(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    if (g_cfg.chams_enable && count > 500) {
        // Player body mesh depth override
        glDepthRangef(1.0f, 0.0f); // Z-Test Always pass (visible through walls)
        glEnable(GL_BLEND);
        glBlendColor(g_cfg.chams_color[0], g_cfg.chams_color[1], g_cfg.chams_color[2], g_cfg.chams_color[3]);
        glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR);

        orig_glDrawElements(mode, count, type, indices);

        // Restore default depth test
        glDepthRangef(0.0f, 1.0f);
        glDisable(GL_BLEND);
        return;
    }
    orig_glDrawElements(mode, count, type, indices);
}

// ============================================================================
// FLY HACK, GHOST MODE, TELEPORT & SPEED TIMER LOOP
// ============================================================================
void on_player_update_tick(void* local_player) {
    if (!local_player) return;

    // 1. SPEED TIMER ACCELERATION
    if (g_cfg.speed_enable && scanner::g_Offsets.fn_Time_set_timeScale) {
        typedef void (*set_timeScale_t)(float);
        ((set_timeScale_t)scanner::g_Offsets.fn_Time_set_timeScale)(g_cfg.speed_multiplier);
    }

    // 2. FLY HACK (RiverMod)
    if (g_cfg.fly_enable) {
        typedef void (*get_pos_t)(void*, Vector3*);
        typedef void (*set_pos_t)(void*, Vector3);

        uint64_t headNode = *(uint64_t*)((uintptr_t)local_player + scanner::g_Offsets.Player_HeadTF);
        if (headNode && scanner::g_Offsets.fn_Transform_get_position && scanner::g_Offsets.fn_Transform_set_position) {
            Vector3 pos;
            ((get_pos_t)scanner::g_Offsets.fn_Transform_get_position)((void*)headNode, &pos);

            // Cancel gravity & elevate height
            if (g_cfg.fly_high) {
                pos.y += g_cfg.fly_speed * 0.05f;
            }

            // Zero out ground velocity
            *(bool*)((uintptr_t)local_player + scanner::g_Offsets.Player_IsGrounded) = false;
            Vector3* vel = (Vector3*)((uintptr_t)local_player + scanner::g_Offsets.Player_Velocity);
            if (vel) {
                vel->y = 0.0f; // Gravity cancel
            }

            ((set_pos_t)scanner::g_Offsets.fn_Transform_set_position)((void*)headNode, pos);
        }
    }

    // 3. GHOST MODE
    if (g_cfg.ghost_enable) {
        if (g_cfg.ghost_origin == Vector3::Zero()) {
            // Save origin point
            uint64_t headNode = *(uint64_t*)((uintptr_t)local_player + scanner::g_Offsets.Player_HeadTF);
            if (headNode) g_cfg.ghost_origin = GetNodePosition(headNode);
        }
    } else {
        g_cfg.ghost_origin = Vector3::Zero();
    }

    // 4. AUTOTELEMARK / TELEKILL
    if (g_cfg.tele_kill) {
        Vector3 targetHead = Vector3::Zero();
        void* target = get_best_target(targetHead);
        if (target && targetHead != Vector3::Zero()) {
            uint64_t localHead = *(uint64_t*)((uintptr_t)local_player + scanner::g_Offsets.Player_HeadTF);
            if (localHead && scanner::g_Offsets.fn_Transform_set_position) {
                // Teleport directly behind target enemy
                Vector3 telePos = targetHead;
                telePos.z -= 1.5f;
                typedef void (*set_pos_t)(void*, Vector3);
                ((set_pos_t)scanner::g_Offsets.fn_Transform_set_position)((void*)localHead, telePos);
                g_cfg.tele_kill = false; // Trigger once per activation
                LOGI("[+] AutoTeleKill jumped behind enemy @ (%.1f, %.1f, %.1f)", telePos.x, telePos.y, telePos.z);
            }
        }
    }
}

// ============================================================================
// INSTALL HOOKS
// ============================================================================
void install_hooks() {
    LOGI("[*] Installing Game Feature Hooks via ShadowHook...");

    // TakeDamage Hook (Silent Aim, Underkill, Autokill)
    if (scanner::g_Offsets.fn_TakeDamage_9) {
        void* h = shadowhook_hook_func_addr(scanner::g_Offsets.fn_TakeDamage_9,
                                           (void*)hook_TakeDamage_9,
                                           (void**)&orig_TakeDamage_9);
        LOGI("[+] TakeDamage_9 Hook installed: %s", h ? "OK" : "FAIL");
    }

    // GLES3 Chams Hook
    void* h_chams = shadowhook_hook_sym_name("libGLESv3.so",
                                            "glDrawElements",
                                            (void*)hook_glDrawElements_Chams,
                                            (void**)&orig_glDrawElements);
    if (!h_chams) {
        shadowhook_hook_sym_name("libGLESv2.so",
                                 "glDrawElements",
                                 (void*)hook_glDrawElements_Chams,
                                 (void**)&orig_glDrawElements);
    }
    LOGI("[+] GLES3 Chams Hook installed!");

    // Background Feature Worker Thread (AutoKill cycle & Tick)
    std::thread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            uint64_t localAddr = data::localPlayerAddr.load(std::memory_order_relaxed);
            if (localAddr) {
                on_player_update_tick((void*)localAddr);
            }
        }
    }).detach();
}

} // namespace features
