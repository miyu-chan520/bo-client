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
    };

    // Touch tracker for responsive drag aim
    inline std::atomic<bool> s_touching_aim_area{false};

    inline void NotifyTouch(bool isDown, float deltaY) {
        if (ui::bar::g_open) {
            s_touching_aim_area.store(false, std::memory_order_relaxed);
            return;
        }
        s_touching_aim_area.store(isDown, std::memory_order_relaxed);
    }

    inline void TickTimer(float dt) {
        (void)dt;
    }

    static inline void Init() {
        // Aimbot initialized
    }

    // Main Aimbot Tick — executed on each background memory cycle
    inline void Update(uint64_t localAddr, const Vector3& localPos, const Vector3& cameraPos) {
        // If menu is open or aimbot is disabled, never hijack aim or camera
        if (ui::bar::g_open || !localAddr || !cfg::aim::enable) return;

        // ════════════════════════════════════════════════════════════════════
        // 1. TRIGGER VERIFICATION (Aim Fire, Aim Scope, or Touch Drag Aim)
        // ════════════════════════════════════════════════════════════════════
        bool trigger = false;

        if (!cfg::aim::aim_fire && !cfg::aim::aim_scope) {
            // Neither fire nor scope constraint active -> continuous drag aim lock within FOV
            trigger = true;
        } else {
            if (cfg::aim::aim_fire) {
                bool isFiring = rpm<bool>(localAddr + Offsets::Player_IsFiring);
                if (isFiring) trigger = true;
            }
            if (cfg::aim::aim_scope) {
                bool isScoping = rpm<bool>(localAddr + Offsets::Player_IsScoping);
                if (isScoping) trigger = true;
            }
        }

        if (!trigger) return;

        // ════════════════════════════════════════════════════════════════════
        // 2. TARGET BONE SELECTION (Head = 0, Neck = 1, Body = 2)
        // ════════════════════════════════════════════════════════════════════
        int targetBone = cfg::aim::target_bone;

        // ════════════════════════════════════════════════════════════════════
        // 3. TARGET SEARCH (Closest to crosshair within configurable FOV)
        // ════════════════════════════════════════════════════════════════════
        float screenCx = g_sw * 0.5f;
        float screenCy = g_sh * 0.5f;

        TargetInfo best;

        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            for (const auto& p : data::players) {
                if (!p.addr || p.addr == localAddr) continue;
                if (p.curHP <= 0) continue;
                if (p.distance > cfg::aim::kMaxRange) continue;
                if (cfg::aim::aim_visible && !p.isVisible) continue;

                Vector3 boneScreen = p.Head;
                if (targetBone == 1 && is_screen_valid(p.Breast)) boneScreen = p.Breast;
                else if (targetBone == 2 && is_screen_valid(p.Hip)) boneScreen = p.Hip;

                if (!is_screen_valid(boneScreen)) continue;

                float sDist = hypotf(boneScreen.x - screenCx, boneScreen.y - screenCy);

                if (cfg::aim::fov > 0.f && sDist > cfg::aim::fov) continue;

                if (sDist < best.screenDist) {
                    uintptr_t boneOff = Offsets::Bone_Head;
                    if (targetBone == 1) boneOff = Offsets::Bone_Breast;
                    else if (targetBone == 2) boneOff = Offsets::Bone_Hip;

                    uint64_t bNode = rpm<uint64_t>(p.addr + boneOff);
                    if (!bNode) bNode = rpm<uint64_t>(p.addr + Offsets::Bone_Head);

                    Vector3 wPos = bNode ? GetNodePosition(bNode) : Vector3::Zero();
                    if (wPos == Vector3::Zero()) continue;

                    best.addr = p.addr;
                    best.worldPos = wPos;
                    best.screenPos = boneScreen;
                    best.screenDist = sDist;
                    best.distance = p.distance;
                }
            }
        }

        if (!best.addr || best.worldPos == Vector3::Zero()) return;

        // ════════════════════════════════════════════════════════════════════
        // 4. AIM LOCK & PARALLAX-ALIGNED ROTATION
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

                // Parallax alignment for third-person over-the-shoulder view
                Vector3 rightVec = Vector3::Normalize(Vector3::Cross(Vector3::Up(), camDir));
                Vector3 compensatedTarget = best.worldPos - (rightVec * cfg::aim::kParallaxX);

                Vector3 finalCamDir = Vector3::Normalize(compensatedTarget - camOrigin);
                Quaternion camTargetQuat = Quaternion::LookRotation(finalCamDir, Vector3::Up());
                Quaternion playerTargetQuat = Quaternion::LookRotation(playerDir, Vector3::Up());

                // 1) Update Player model rotation & current aim rotation
                Quaternion curPlayerRot = rpm<Quaternion>(localAddr + Offsets::Player_Rotation);
                Quaternion newPlayerRot = Quaternion::Slerp(curPlayerRot, playerTargetQuat, cfg::aim::kSmoothness);

                float dotPlayer = curPlayerRot.x * newPlayerRot.x + curPlayerRot.y * newPlayerRot.y +
                                  curPlayerRot.z * newPlayerRot.z + curPlayerRot.w * newPlayerRot.w;
                if (std::fabs(dotPlayer) < 0.99999f) {
                    wpm<Quaternion>(localAddr + Offsets::Player_Rotation, newPlayerRot);
                    wpm<Quaternion>(localAddr + Offsets::Player_CurrentAimRotation, newPlayerRot);

                    // Increment CallSetAimRotationCount so game network serialization validates rotation
                    uint32_t callCount = rpm<uint32_t>(localAddr + Offsets::Player_CallSetAimRotationCount);
                    wpm<uint32_t>(localAddr + Offsets::Player_CallSetAimRotationCount, callCount + 1);
                }

                // 2) Update FollowCamera rotation
                uint64_t followCam = rpm<uint64_t>(localAddr + Offsets::Player_Camera);
                if (followCam) {
                    Quaternion curCamRot = rpm<Quaternion>(followCam + Offsets::Camera_Rotation);
                    Quaternion newCamRot = Quaternion::Slerp(curCamRot, camTargetQuat, cfg::aim::kSmoothness);
                    wpm<Quaternion>(followCam + Offsets::Camera_Rotation, newCamRot);
                }

                // 3) Update bullet trajectory in Player AimInfo & AuxAimInfo
                uint64_t aimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AimInfo);
                if (aimInfo) {
                    wpm<Vector3>(aimInfo + Offsets::AimInfo_Direction, playerDir);
                }
                uint64_t auxAimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AuxAimInfo);
                if (auxAimInfo) {
                    wpm<Vector3>(auxAimInfo + Offsets::AimInfo_Direction, playerDir);
                }
            }
        }
    }
}
