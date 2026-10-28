#pragma once

#include "Offsets.h"
#include "../other/vector3.h"
#include "../other/memory.hpp"
#include "../other/utils.hpp"
#include "math.hpp"
#include "data.hpp"
#include "../ui/cfg.hpp"
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
        if (!localAddr || !cfg::aim::enable) return;

        // 1. Triggers verification
        if (cfg::aim::aim_fire) {
            bool isFiring = rpm<bool>(localAddr + Offsets::Player_IsFiring);
            if (!isFiring) return;
        }

        if (cfg::aim::aim_scope) {
            bool isScoping = rpm<bool>(localAddr + 0x624);
            if (!isScoping) return;
        }

        // 2. Bone Target Selection:
        // Choices: 0: Head, 1: Neck, 2: Body
        // If Auto Drag is active:
        // - When not dragging -> only aim Body (index 2)
        // - When dragging lightly upwards -> hard aim to Head (index 0)
        int effectiveBone = cfg::aim::target_bone;
        bool isDraggingUp = (s_drag_boost_timer.load(std::memory_order_relaxed) > 0.0f);

        if (cfg::aim::auto_drag) {
            if (isDraggingUp) {
                effectiveBone = 0; // Hard aim to Head on light drag
            } else {
                effectiveBone = 2; // Only aim Body when not dragging
            }
        }

        // 3. Find closest target to screen crosshair within FOV
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

                // Aim lock / assistance activates when crosshair moves over designated target point within FOV
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

        // 4. Aim Lock & Assistance
        // Computes the true line of sight from camera position to the designated target point.
        // Synchronizes both Player rotation and FollowCamera rotation, preventing angular desync
        // and eliminating the crosshair right-shift as well as lobby desync kicks.
        if (cfg::aim::aim_lock) {
            Vector3 origin = (cameraPos != Vector3::Zero()) ? cameraPos : localPos;
            Vector3 diff = best.worldPos - origin;
            float dist = Vector3::Magnitude(diff);

            if (dist > 0.05f) {
                Vector3 dir = diff / dist;
                Quaternion targetQuat = Quaternion::LookRotation(dir, Vector3::Up());

                // Read current player rotation and smoothly interpolate towards target
                // Clamped smoothing factor avoids unnatural angular velocity spikes
                Quaternion curRot = rpm<Quaternion>(localAddr + Offsets::Player_Rotation);
                float smooth = cfg::aim::aim_smooth;
                if (smooth <= 0.05f) smooth = 0.25f;
                if (smooth > 0.85f) smooth = 0.85f;

                Quaternion newRot = Quaternion::Slerp(curRot, targetQuat, smooth);

                // Check delta to avoid redundant writes when target is already centered
                float dot = curRot.x * newRot.x + curRot.y * newRot.y + curRot.z * newRot.z + curRot.w * newRot.w;
                if (std::fabs(dot) < 0.99999f) {
                    // Update Player model rotation & aim rotation
                    wpm<Quaternion>(localAddr + Offsets::Player_Rotation, newRot);
                    wpm<Quaternion>(localAddr + Offsets::Player_CurrentAimRotation, newRot);

                    // Increment CallSetAimRotationCount so network serialization validates rotation
                    uint32_t callCount = rpm<uint32_t>(localAddr + Offsets::Player_CallSetAimRotationCount);
                    wpm<uint32_t>(localAddr + Offsets::Player_CallSetAimRotationCount, callCount + 1);

                    // Synchronize FollowCamera rotation with player rotation
                    // This eliminates the camera-to-player angle discrepancy that previously
                    // triggered the lobby return kick after prolonged aiming.
                    uint64_t followCam = rpm<uint64_t>(localAddr + Offsets::Player_Camera);
                    if (followCam) {
                        wpm<Quaternion>(followCam + Offsets::Camera_Rotation, newRot);
                    }

                    // Update bullet ray direction in Player AimInfo
                    uint64_t aimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AimInfo);
                    if (aimInfo) {
                        wpm<Vector3>(aimInfo + Offsets::AimInfo_Direction, dir);
                    }
                    uint64_t auxAimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AuxAimInfo);
                    if (auxAimInfo) {
                        wpm<Vector3>(auxAimInfo + Offsets::AimInfo_Direction, dir);
                    }
                }
            }
        }

        // 5. Working & Safe No Recoil (Zeroes WeaponGunFeelProfileData parameters)
        if (cfg::aim::no_recoil) {
            uint64_t w = rpm<uint64_t>(localAddr + Offsets::Player_Weapon);
            if (w) {
                uint64_t wd = rpm<uint64_t>(w + Offsets::Weapon_ProfileData);
                if (wd) {
                    wpm<float>(wd + 0x88, 0.0f);  // visualPitchScale
                    wpm<float>(wd + 0x8C, 0.0f);  // visualFollowRealRecoil
                    wpm<float>(wd + 0x90, 0.0f);  // hipRecoilPitchNoise
                    wpm<float>(wd + 0xB8, 0.0f);  // crouchRecoilScale
                    wpm<float>(wd + 0xBC, 0.0f);  // hipRecoilPitchMax
                    wpm<float>(wd + 0xE4, 0.0f);  // hipRecoilYawScale
                    wpm<float>(wd + 0x124, 0.0f); // hipRecoilPitchMin
                    wpm<float>(wd + 0x17C, 0.0f); // adsRecoilMaxYawOffset
                    wpm<float>(wd + 0x180, 0.0f); // adsRecoilMaxPitchOffset
                    wpm<float>(wd + 0x18C, 0.0f); // adsRecoilYawNoise
                    wpm<float>(wd + 0x190, 0.0f); // adsRecoilPitchNoise
                    wpm<float>(wd + 0x194, 0.0f); // hipRecoilYawMin
                    wpm<float>(wd + 0x198, 0.0f); // adsRecoilYawMin
                    wpm<float>(wd + 0x19C, 0.0f); // adsRecoilPitchMax
                    wpm<float>(wd + 0x1A8, 0.0f); // adsRecoilPitchScale
                    wpm<float>(wd + 0x1B4, 0.0f); // hipRecoilYawMax
                    wpm<float>(wd + 0x1D8, 0.0f); // adsRecoilYawScale
                    wpm<float>(wd + 0x1DC, 0.0f); // adsRecoilPitchMin
                    wpm<float>(wd + 0x1E0, 0.0f); // adsRecoilYawMax
                    wpm<float>(wd + 0x1E8, 0.0f); // hipRecoilPitchScale
                }
            }
        }
    }
}
