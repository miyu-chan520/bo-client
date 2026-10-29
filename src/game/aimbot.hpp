#pragma once

#include "Offsets.h"
#include "../other/vector3.h"
#include "../other/memory.hpp"
#include "../other/utils.hpp"
#include "math.hpp"
#include "data.hpp"
#include "../ui/cfg.hpp"
#include "../ui/bar.hpp"
#include "../ui/styles/styles.h"
#include "imgui.h"
#include <cstdint>
#include <mutex>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <atomic>
#include <algorithm>
#include <android/log.h>

namespace aimbot {

    static inline bool is_screen_valid(const Vector3& v) {
        return v.x > 0.0f && v.y > 0.0f;
    }

    struct TargetInfo {
        uint64_t addr = 0;
        Vector3  worldPos = Vector3::Zero();
        Vector3  screenPos = Vector3::Zero();
        float    screenDist = 99999.0f;
        float    distance = 99999.0f;
        bool     isHeadTarget = false;
    };

    // Thread-safe auto-drag state
    inline std::atomic<float> s_drag_boost_timer{0.0f};

    // Called on render frame to track touch and swipe events
    inline void NotifyTouch(bool isDown, float deltaY) {
        if (ui::bar::g_open) return; // Ignore touch notifications when menu is open

        // Dragging lightly upwards (negative deltaY in screen coordinates)
        if (isDown && deltaY < -0.4f) {
            s_drag_boost_timer.store(0.6f, std::memory_order_relaxed);
        }
    }

    inline void TickTimer(float dt) {
        float cur = s_drag_boost_timer.load(std::memory_order_relaxed);
        if (cur > 0.0f) {
            float next = cur - dt;
            s_drag_boost_timer.store(next > 0.0f ? next : 0.0f, std::memory_order_relaxed);
        }
    }

    static inline void Init() {
        // Aimbot initialized
    }

    // Main Aimbot Tick — executed on each background memory cycle
    inline void Update(uint64_t localAddr, const Vector3& localPos, const Vector3& cameraPos) {
        // If menu is open, never hijack aim or camera
        if (ui::bar::g_open) return;
        if (!localAddr) return;

        // ════════════════════════════════════════════════════════════════════
        // 1. HITBOX EXTENDER / BIG HEAD (Applied to all enemies)
        // ════════════════════════════════════════════════════════════════════
        if (cfg::aim::hitbox_extender) {
            float targetScale = cfg::aim::hitbox_size;
            if (targetScale < 1.0f) targetScale = 1.0f;
            if (targetScale > 8.0f) targetScale = 8.0f;

            std::lock_guard<std::mutex> lk(data::players_mutex);
            for (const auto& p : data::players) {
                if (!p.addr || p.addr == localAddr || p.curHP <= 0) continue;
                uint64_t bhComp = rpm<uint64_t>(p.addr + Offsets::Player_BigHeadComponent);
                if (bhComp) {
                    float curScale = rpm<float>(bhComp + Offsets::Player_BigHeadScale);
                    if (std::fabs(curScale - targetScale) > 0.05f) {
                        wpm<float>(bhComp + Offsets::Player_BigHeadScale, targetScale);
                    }
                }
            }
        }

        // ════════════════════════════════════════════════════════════════════
        // 2. WORKING WEAPON MODS: NO RECOIL & NO SPREAD
        // ════════════════════════════════════════════════════════════════════
        uint64_t w = rpm<uint64_t>(localAddr + Offsets::Player_Weapon);
        if (w) {
            uint64_t wd = rpm<uint64_t>(w + Offsets::Weapon_ProfileData);
            if (wd) {
                // Working No Recoil
                if (cfg::aim::no_recoil) {
                    wpm<bool>(wd + Offsets::WeaponProfile_enableRecoil, false);
                    wpm<bool>(wd + Offsets::WeaponProfile_enableVisualRecoil, false);
                    wpm<bool>(wd + Offsets::WeaponProfile_enableLegacyCameraShake, false);

                    wpm<float>(wd + Offsets::WeaponProfile_visualPitchScale, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_visualFollowRealRecoil, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_hipRecoilPitchNoise, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_crouchRecoilScale, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_hipRecoilPitchMax, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_hipRecoilYawScale, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_hipRecoilPitchMin, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilMaxYawOffset, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilMaxPitchOffset, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilYawNoise, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilPitchNoise, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_hipRecoilYawMin, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilYawMin, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilPitchMax, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilPitchScale, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_hipRecoilYawMax, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilYawScale, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilPitchMin, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsRecoilYawMax, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_hipRecoilPitchScale, 0.0f);
                }

                // Working No Spread
                if (cfg::aim::no_spread) {
                    wpm<bool>(wd + Offsets::WeaponProfile_enableSpread, false);
                    wpm<float>(wd + Offsets::WeaponProfile_adsSpreadClampMax, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_adsSpreadClampMin, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_hipSpreadClampMax, 0.0f);
                    wpm<float>(wd + Offsets::WeaponProfile_hipSpreadClampMin, 0.0f);
                }
            }
        }

        if (!cfg::aim::enable) return;

        // ════════════════════════════════════════════════════════════════════
        // 3. TRIGGERS VERIFICATION
        // ════════════════════════════════════════════════════════════════════
        if (cfg::aim::aim_fire) {
            bool isFiring = rpm<bool>(localAddr + Offsets::Player_IsFiring);
            if (!isFiring) return;
        }

        if (cfg::aim::aim_scope) {
            bool isScoping = rpm<bool>(localAddr + Offsets::Player_IsScoping);
            if (!isScoping) return;
        }

        // ════════════════════════════════════════════════════════════════════
        // 4. BONE TARGET SELECTION & AUTO DRAG
        // ════════════════════════════════════════════════════════════════════
        int effectiveBone = cfg::aim::target_bone;
        bool isDraggingUp = (s_drag_boost_timer.load(std::memory_order_relaxed) > 0.0f);

        if (cfg::aim::auto_drag) {
            if (isDraggingUp) {
                effectiveBone = 0; // Hard aim to Head on light drag
            } else {
                effectiveBone = 2; // Only aim Body when not dragging
            }
        }

        // ════════════════════════════════════════════════════════════════════
        // 5. FIND CLOSEST TARGET WITHIN FOV
        // ════════════════════════════════════════════════════════════════════
        float screenCx = g_sw * 0.5f;
        float screenCy = g_sh * 0.5f;

        TargetInfo best;

        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            for (const auto& p : data::players) {
                if (!p.addr || p.addr == localAddr) continue;
                if (p.curHP <= 0) continue;
                if (p.distance > cfg::aim::max_range) continue;
                if (cfg::aim::aim_visible && !p.isVisible) continue;

                Vector3 boneScreen = p.Head;
                if (effectiveBone == 1 && is_screen_valid(p.Breast)) boneScreen = p.Breast;
                else if (effectiveBone == 2 && is_screen_valid(p.Hip)) boneScreen = p.Hip;

                if (!is_screen_valid(boneScreen)) continue;

                float sDist = hypotf(boneScreen.x - screenCx, boneScreen.y - screenCy);

                if (cfg::aim::draw_fov || cfg::aim::fov > 0.f) {
                    if (sDist > cfg::aim::fov) continue;
                }

                if (sDist < best.screenDist) {
                    uintptr_t boneOff = Offsets::Bone_Head;
                    if (effectiveBone == 1) boneOff = Offsets::Bone_Breast;
                    else if (effectiveBone == 2) boneOff = Offsets::Bone_Hip;

                    uint64_t bNode = rpm<uint64_t>(p.addr + boneOff);
                    if (!bNode) bNode = rpm<uint64_t>(p.addr + Offsets::Bone_Head);

                    Vector3 wPos = bNode ? GetNodePosition(bNode) : Vector3::Zero();
                    if (wPos == Vector3::Zero()) continue;

                    best.addr = p.addr;
                    best.worldPos = wPos;
                    best.screenPos = boneScreen;
                    best.screenDist = sDist;
                    best.distance = p.distance;
                    best.isHeadTarget = (effectiveBone == 0);
                }
            }
        }

        if (!best.addr || best.worldPos == Vector3::Zero()) return;

        // ════════════════════════════════════════════════════════════════════
        // 6. AIM LOCK & PARALLAX COMPENSATION & SILENT AIM
        // ════════════════════════════════════════════════════════════════════
        if (cfg::aim::aim_lock) {
            Vector3 camOrigin = (cameraPos != Vector3::Zero()) ? cameraPos : localPos;
            Vector3 camDiff = best.worldPos - camOrigin;
            float camDist = Vector3::Magnitude(camDiff);

            Vector3 playerDiff = best.worldPos - localPos;
            float playerDist = Vector3::Magnitude(playerDiff);

            if (camDist > 0.05f && playerDist > 0.05f) {
                Vector3 camDir = camDiff / camDist;
                Vector3 playerDir = playerDiff / playerDist;

                // Parallax compensation
                Vector3 rightVec = Vector3::Normalize(Vector3::Cross(Vector3::Up(), camDir));
                Vector3 compensatedTarget = best.worldPos - (rightVec * (0.12f + cfg::aim::aim_x_offset));

                Vector3 finalCamDir = Vector3::Normalize(compensatedTarget - camOrigin);
                Quaternion camTargetQuat = Quaternion::LookRotation(finalCamDir, Vector3::Up());
                Quaternion playerTargetQuat = Quaternion::LookRotation(playerDir, Vector3::Up());

                float smooth = cfg::aim::aim_smooth;
                if (smooth <= 0.05f) smooth = 0.25f;
                if (smooth > 0.85f) smooth = 0.85f;

                // 1) Update Player model rotation & aim rotation
                Quaternion curPlayerRot = rpm<Quaternion>(localAddr + Offsets::Player_Rotation);
                Quaternion newPlayerRot = Quaternion::Slerp(curPlayerRot, playerTargetQuat, smooth);

                float dotPlayer = curPlayerRot.x * newPlayerRot.x + curPlayerRot.y * newPlayerRot.y +
                                  curPlayerRot.z * newPlayerRot.z + curPlayerRot.w * newPlayerRot.w;
                if (std::fabs(dotPlayer) < 0.99999f) {
                    wpm<Quaternion>(localAddr + Offsets::Player_Rotation, newPlayerRot);
                    wpm<Quaternion>(localAddr + Offsets::Player_CurrentAimRotation, newPlayerRot);

                    // Increment CallSetAimRotationCount so network serialization validates rotation
                    uint32_t callCount = rpm<uint32_t>(localAddr + Offsets::Player_CallSetAimRotationCount);
                    wpm<uint32_t>(localAddr + Offsets::Player_CallSetAimRotationCount, callCount + 1);
                }

                // 2) Update FollowCamera rotation with parallax compensation
                uint64_t followCam = rpm<uint64_t>(localAddr + Offsets::Player_Camera);
                if (followCam) {
                    Quaternion curCamRot = rpm<Quaternion>(followCam + Offsets::Camera_Rotation);
                    Quaternion newCamRot = Quaternion::Slerp(curCamRot, camTargetQuat, smooth);
                    wpm<Quaternion>(followCam + Offsets::Camera_Rotation, newCamRot);
                }

                // 3) Update bullet trajectory in Player AimInfo & AuxAimInfo
                uint64_t aimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AimInfo);
                if (aimInfo) {
                    wpm<Vector3>(aimInfo + Offsets::AimInfo_Direction, playerDir);
                    if (cfg::aim::silent_aim) {
                        // Silent Aim / Magic Bullet: force bullet origin aligned directly with target
                        wpm<Vector3>(aimInfo + Offsets::AimInfo_StartPosition, localPos + Vector3(0.0f, 0.45f, 0.0f));
                    }
                }
                uint64_t auxAimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AuxAimInfo);
                if (auxAimInfo) {
                    wpm<Vector3>(auxAimInfo + Offsets::AimInfo_Direction, playerDir);
                }
            }
        }
    }
}
