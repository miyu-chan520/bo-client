#pragma once

#include "Offsets.h"
#include "../other/vector3.h"
#include "../other/memory.hpp"
#include "../other/utils.hpp"
#include "math.hpp"
#include "data.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include <cstdint>
#include <mutex>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <android/log.h>

#define AS_LOG(...) do { \
    __android_log_print(ANDROID_LOG_INFO, "AimSilent", __VA_ARGS__); \
} while(0)

// Global legacy toggle bindings (mirrors cfg::aim)
inline bool AimSilentEnabled = false;
inline bool AimSilentNobita  = false;
static bool g_AimSilentHookInstalled = false;

namespace aim_engine {

    // ── Helper: Find Best Target ──
    struct AimTarget {
        uint64_t addr = 0;
        Vector3  worldPos = Vector3::Zero();
        Vector3  screenPos = Vector3::Zero();
        uint64_t headCollider = 0;
        float    screenDist = 99999.0f;
        float    distance = 99999.0f;
    };

    static inline AimTarget FindBestTarget(const Vector3& localPos) {
        AimTarget best;
        uint64_t localAddr = data::localPlayerAddr.load(std::memory_order_relaxed);
        if (!localAddr) return best;

        float screenCx = g_sw * 0.5f;
        float screenCy = g_sh * 0.5f;

        std::lock_guard<std::mutex> lk(data::players_mutex);
        for (const auto& p : data::players) {
            if (!p.addr || p.addr == localAddr) continue;
            if (p.curHP <= 0) continue;
            if (p.distance > cfg::aim::max_range) continue;

            // Select bone screen coordinate based on configuration
            Vector3 boneScreen = p.Head;
            if (cfg::aim::target_bone == 1 && ok(p.Breast)) boneScreen = p.Breast;
            else if (cfg::aim::target_bone == 2 && ok(p.Hip)) boneScreen = p.Hip;

            if (!ok(boneScreen)) continue;

            float sDist = hypotf(boneScreen.x - screenCx, boneScreen.y - screenCy);

            // FOV check if FOV is active
            if (cfg::aim::draw_fov || cfg::aim::fov > 0.0f) {
                if (sDist > cfg::aim::fov) continue;
            }

            // Prefer closest to crosshair (or closest distance if sDist is equal)
            if (sDist < best.screenDist) {
                // Resolve 3D world position of target bone
                uintptr_t boneOff = Offsets::Bone_Head;
                if (cfg::aim::target_bone == 1) boneOff = Offsets::Bone_Breast;
                else if (cfg::aim::target_bone == 2) boneOff = Offsets::Bone_Hip;

                uint64_t boneNode = rpm<uint64_t>(p.addr + boneOff);
                if (!boneNode) boneNode = rpm<uint64_t>(p.addr + Offsets::Bone_Head);

                Vector3 wPos = boneNode ? GetNodePosition(boneNode) : Vector3::Zero();
                if (wPos == Vector3::Zero()) continue;

                best.addr = p.addr;
                best.worldPos = wPos;
                best.screenPos = boneScreen;
                best.headCollider = rpm<uint64_t>(p.addr + Offsets::Player_HedColider);
                best.screenDist = sDist;
                best.distance = p.distance;
            }
        }
        return best;
    }

    // ── Aim Rotation (Euler to Quaternion) ──
    static inline Vector4 DirectionToQuaternion(const Vector3& dir) {
        float len = Vector3::Magnitude(dir);
        if (len < 0.0001f) return Vector4(0, 0, 0, 1);

        float yaw = atan2f(dir.x, dir.z);
        float pitch = -asinf(dir.y / len);

        float cy = cosf(yaw * 0.5f);
        float sy = sinf(yaw * 0.5f);
        float cp = cosf(pitch * 0.5f);
        float sp = sinf(pitch * 0.5f);

        Vector4 q;
        q.w = cy * cp;
        q.x = cy * sp;
        q.y = sy * cp;
        q.z = -sy * sp;
        return q;
    }

    // ── Main Aim Engine Tick (called from data thread) ──
    inline void Update(uint64_t localAddr, const Vector3& localPos) {
        if (!localAddr) return;

        // Synchronize legacy flags with config
        bool isSilentActive = cfg::aim::silent || AimSilentEnabled;
        bool isNobitaActive = cfg::aim::nobita || AimSilentNobita;
        bool isAutoHeadActive = cfg::aim::auto_headshot;
        bool isAimlockActive = cfg::aim::enable;

        bool anyActive = isSilentActive || isNobitaActive || isAutoHeadActive || isAimlockActive;
        if (!anyActive) return;

        // Check isFiring if on_fire_only is enabled
        bool isFiring = rpm<bool>(localAddr + Offsets::Player_IsFiring);
        if (cfg::aim::on_fire_only && !isFiring) {
            return;
        }

        // 1. Nobita Mode: Write head collider to ALL valid enemies in range
        if (isNobitaActive) {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            for (const auto& p : data::players) {
                if (p.addr && p.addr != localAddr && p.curHP > 0 && p.distance <= cfg::aim::max_range) {
                    uint64_t hc = rpm<uint64_t>(p.addr + Offsets::Player_HedColider);
                    if (hc) {
                        wpm<uint64_t>(p.addr + Offsets::Player_ColWrite, hc);
                    }
                }
            }
        }

        // 2. Target acquisition for Aimlock / Silent Aim / Target Headshot
        AimTarget target = FindBestTarget(localPos);
        if (!target.addr || target.worldPos == Vector3::Zero()) return;

        // Auto Headshot: Write target's head collider to its active collision receiver
        if (isAutoHeadActive || isSilentActive || isNobitaActive) {
            if (target.headCollider) {
                wpm<uint64_t>(target.addr + Offsets::Player_ColWrite, target.headCollider);
            }
        }

        // Silent Aim: Overwrite CGKJLKPMGDJ (Player_AimInfo) on local player
        if (isSilentActive || isNobitaActive) {
            uint64_t aimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AimInfo);
            if (aimInfo) {
                Vector3 myHead = localPos;
                Vector3 dir = (myHead == Vector3::Zero())
                              ? Vector3(0.f, 0.f, 1.f)
                              : Vector3::Normalize(target.worldPos - myHead);

                // Write HitLocation (0x28), RayDir (0x40), HitNormal (0x34)
                wpm<Vector3>(aimInfo + Offsets::HitInfo_HitLocation, target.worldPos);
                wpm<Vector3>(aimInfo + Offsets::HitInfo_RayDir, dir);
                wpm<Vector3>(aimInfo + Offsets::HitInfo_HitNormal, Vector3(0.f, 1.f, 0.f));

                // Write HitCollider (0x20)
                if (target.headCollider) {
                    wpm<uint64_t>(aimInfo + Offsets::HitInfo_HitCollider, target.headCollider);
                }

                // Force Headshot hitgroup & maximum multiplier
                wpm<int>(aimInfo + Offsets::HitInfo_HitGroup, 1);
                wpm<int16_t>(aimInfo + Offsets::HitInfo_HitGroupShort, 1);
                wpm<float>(aimInfo + Offsets::HitInfo_DamageRatio, 1.0f);
                wpm<bool>(aimInfo + Offsets::HitInfo_IgnoreHappens, false);
                wpm<bool>(aimInfo + Offsets::HitInfo_ViewBlocked, false);

                // Update weapon last aiming target reference (0xE68)
                wpm<uint64_t>(localAddr + Offsets::Player_LastAimTargetRef, target.addr);

                // AuxAimInfo (0xE58)
                uint64_t auxAim = rpm<uint64_t>(localAddr + Offsets::Player_AuxAimInfo);
                if (auxAim) {
                    wpm<Vector3>(auxAim + Offsets::HitInfo_HitLocation, target.worldPos);
                    wpm<Vector3>(auxAim + Offsets::HitInfo_RayDir, dir);
                    if (target.headCollider) {
                        wpm<uint64_t>(auxAim + Offsets::HitInfo_HitCollider, target.headCollider);
                    }
                    wpm<int>(auxAim + Offsets::HitInfo_HitGroup, 1);
                    wpm<int16_t>(auxAim + Offsets::HitInfo_HitGroupShort, 1);
                    wpm<bool>(auxAim + Offsets::HitInfo_IgnoreHappens, false);
                    wpm<bool>(auxAim + Offsets::HitInfo_ViewBlocked, false);
                }
            }
        }

        // Aimlock: Calculate rotation facing target and write to Player_Rotation (0x614)
        if (isAimlockActive) {
            Vector3 myHead = localPos;
            if (myHead != Vector3::Zero()) {
                Vector3 dir = Vector3::Normalize(target.worldPos - myHead);
                Vector4 quat = DirectionToQuaternion(dir);

                wpm<Vector4>(localAddr + Offsets::Player_Rotation, quat);
                wpm<Vector4>(localAddr + Offsets::Player_CurrentAimRotation, quat);
            }
        }
    }
}

// ── Hook init backwards compatibility ──
static inline void InitAimSilentHook() {
    g_AimSilentHookInstalled = true;
    AS_LOG("Aim Engine initialized (kernel memory read/write mode)");
}
