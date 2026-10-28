#pragma once

#include "Offsets.h"
#include "../other/vector3.h"
#include "../other/memory.hpp"
#include "../other/utils.hpp"
#include "math.hpp"
#include "data.hpp"
#include "../ui/cfg.hpp"
#include "styles/styles.h"
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
        uint64_t headCollider = 0;
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

    // Euler angles to Unity Quaternion (x, y, z, w)
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

    static inline void Init() {
        // Aimbot initialized
    }

    // Main Aimbot Tick — executed on each background memory cycle
    inline void Update(uint64_t localAddr, const Vector3& localPos) {
        if (!localAddr || !cfg::aim::enable) return;

        // 1. Triggers verification
        if (cfg::aim::aim_fire) {
            bool isFiring = rpm<bool>(localAddr + Offsets::Player_IsFiring);
            if (!isFiring) return;
        }

        if (cfg::aim::aim_scope) {
            bool isScoping = (rpm<uint64_t>(localAddr + 0x600) != 0) || rpm<bool>(localAddr + 0x624);
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
                    best.headCollider = rpm<uint64_t>(p.addr + Offsets::Player_HedColider);
                    best.screenDist = sDist;
                    best.distance = p.distance;
                    best.isHeadTarget = (effectiveBone == 0);
                }
            }
        }

        if (!best.addr || best.worldPos == Vector3::Zero()) return;

        // 4. Aim Lock (Camera View Angle Lock to Target)
        if (cfg::aim::aim_lock) {
            Vector3 myHead = localPos;
            if (myHead != Vector3::Zero()) {
                Vector3 dir = Vector3::Normalize(best.worldPos - myHead);
                Vector4 quat = DirectionToQuaternion(dir);

                wpm<Vector4>(localAddr + Offsets::Player_Rotation, quat);
                wpm<Vector4>(localAddr + Offsets::Player_CurrentAimRotation, quat);
            }
        }

        // 5. Head Collider Injection (when locked to Head)
        if (best.isHeadTarget && best.headCollider) {
            wpm<uint64_t>(best.addr + Offsets::Player_ColWrite, best.headCollider);
        }

        // 6. Safe No Recoil
        if (cfg::aim::no_recoil) {
            uint64_t w = rpm<uint64_t>(localAddr + Offsets::Player_Weapon);
            if (w) {
                uint64_t wd = rpm<uint64_t>(w + Offsets::Weapon_Data);
                if (wd) {
                    wpm<float>(wd + Offsets::Weapon_Recoil, 0.0f);
                }
            }
        }
    }
}
