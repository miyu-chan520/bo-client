#pragma once

#include "Offsets.h"
#include "../other/vector3.h"
#include "../other/memory.hpp"
#include "../other/utils.hpp"
#include "math.hpp"
#include "data.hpp"
#include "../ui/cfg.hpp"
#include "../ui/bar.hpp"
#include "imgui.h"
#include <cstdint>
#include <mutex>
#include <cmath>
#include <chrono>
#include <atomic>
#include <android/log.h>

#define AIM_LOG_TAG "BO_AimEngine"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO,  AIM_LOG_TAG, __VA_ARGS__)

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
        uint64_t targetCol = 0;
        int32_t  hitGroup = 0;
    };

    // Touch tracker for responsive aim assistance
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

    static inline void Init() {}

    // Track last AimKill execution to enforce strict 10ms minimum interval
    inline std::chrono::steady_clock::time_point s_lastAimKillTime = std::chrono::steady_clock::now();

    // ========================================================================
    // MAIN AIMBOT & EXPLOIT TICK
    // ========================================================================
    inline void Update(uint64_t localAddr, const Vector3& localPos, const Vector3& cameraPos) {
        if (!localAddr) return;

        // ════════════════════════════════════════════════════════════════════
        // 1. NON-RECOIL (Triệt tiêu độ giật & độ tản đạn)
        // ════════════════════════════════════════════════════════════════════
        if (cfg::nonrecoil::enable) {
            uint64_t weapon = rpm<uint64_t>(localAddr + Offsets::Player_Weapon);
            if (weapon) {
                uint64_t wData = rpm<uint64_t>(weapon + Offsets::Weapon_Data);
                if (wData) {
                    // Zero out recoil pitch, yaw, and spread growth
                    wpm<float>(wData + 0x88, 0.0f); // Recoil Pitch Min
                    wpm<float>(wData + 0x8C, 0.0f); // Recoil Pitch Max
                    wpm<float>(wData + 0x90, 0.0f); // Recoil Yaw Min
                    wpm<float>(wData + 0x94, 0.0f); // Recoil Yaw Max
                    wpm<float>(wData + 0xB0, 0.0f); // Bullet Spread
                }
            }
        }

        // ════════════════════════════════════════════════════════════════════
        // 2. SPEED TIME (Timescale Engine Acceleration)
        // ════════════════════════════════════════════════════════════════════
        if (cfg::speedtime::enable && cfg::speedtime::scale > 1.0f) {
            // Apply speed multiplier to player movement velocity
            Vector3 vel = rpm<Vector3>(localAddr + Offsets::Player_Velocity);
            if (vel.sqrMagnitude() > 0.01f) {
                wpm<Vector3>(localAddr + Offsets::Player_Velocity, vel * cfg::speedtime::scale);
            }
        }

        if (ui::bar::g_open || !cfg::aim::enable) return;

        // ════════════════════════════════════════════════════════════════════
        // 3. TARGET SEARCH (Check Visible & Bỏ Qua Kẻ Địch Gục)
        // ════════════════════════════════════════════════════════════════════
        float screenCx = g_sw * 0.5f;
        float screenCy = g_sh * 0.5f;

        TargetInfo best;
        int targetBone = cfg::aim::target_bone;

        {
            std::lock_guard<std::mutex> lk(data::players_mutex);
            for (const auto& p : data::players) {
                if (!p.addr || p.addr == localAddr) continue;
                if (p.isTeammate) continue;

                // [BỎ QUA KẺ ĐỊCH GỤC / CHẾT]
                if (cfg::aim::ignore_knocked && (p.isKnocked || p.curHP <= 0)) continue;
                if (p.distance > cfg::aim::kMaxRange) continue;

                // [CHECK VISIBLE]
                if (cfg::aim::aim_visible && !p.isVisible) continue;

                Vector3 boneScreen = p.Head;
                if (targetBone == 1 && is_screen_valid(p.Breast)) boneScreen = p.Breast;
                else if (targetBone == 2 && is_screen_valid(p.Breast)) boneScreen = p.Breast;

                if (!is_screen_valid(boneScreen)) continue;

                float sDist = hypotf(boneScreen.x - screenCx, boneScreen.y - screenCy);
                if (cfg::aim::fov > 0.f && sDist > cfg::aim::fov) continue;

                if (sDist < best.screenDist) {
                    Vector3 wPos = Vector3::Zero();
                    uint64_t col = 0;
                    int32_t grp = 0;

                    if (targetBone == 0) {
                        uint64_t bNode = rpm<uint64_t>(p.addr + Offsets::Bone_Head);
                        if (bNode) wPos = GetNodePosition(bNode);
                    } else {
                        uint64_t bNode = rpm<uint64_t>(p.addr + Offsets::Bone_Breast);
                        if (bNode) wPos = GetNodePosition(bNode);
                    }

                    if (wPos == Vector3::Zero()) continue;

                    uint64_t headCol = rpm<uint64_t>(p.addr + Offsets::Player_HedColider);
                    uint64_t bodyCol = rpm<uint64_t>(p.addr + Offsets::Player_ColWrite);

                    if (targetBone == 0) {
                        col = headCol ? headCol : bodyCol;
                        grp = 1; // Head
                    } else {
                        col = bodyCol ? bodyCol : headCol;
                        grp = 0; // Torso
                    }

                    best.addr = p.addr;
                    best.worldPos = wPos;
                    best.screenPos = boneScreen;
                    best.screenDist = sDist;
                    best.distance = p.distance;
                    best.targetCol = col;
                    best.hitGroup = grp;
                }
            }
        }

        if (!best.addr || best.worldPos == Vector3::Zero()) return;

        // ════════════════════════════════════════════════════════════════════
        // 4. UNDER KILL (Bắn ngầm xuyên sàn / địa hình)
        // ════════════════════════════════════════════════════════════════════
        if (cfg::underkill::enable) {
            // Hạ thấp tọa độ vector xuống dưới đất để xuyên qua cover và chân tường
            best.worldPos.y -= cfg::underkill::depth_offset;
        }

        // ════════════════════════════════════════════════════════════════════
        // 5. AUTO TELEMARK (Dịch chuyển tức thời tới vị trí mục tiêu)
        // ════════════════════════════════════════════════════════════════════
        if (cfg::telemark::enable) {
            // Đặt tọa độ người chơi ngay phía sau mục tiêu 1.5m
            Vector3 telePos = best.worldPos + Vector3(0.0f, 0.5f, -1.5f);
            uint64_t rootNode = rpm<uint64_t>(localAddr + Offsets::Bone_Root);
            if (rootNode) {
                // Ghi tọa độ Transform
                uint64_t tf = rpm<uint64_t>(rootNode + 0x10);
                if (tf) {
                    wpm<Vector3>(tf + 0x90, telePos);
                }
            }
        }

        // ════════════════════════════════════════════════════════════════════
        // 6. AIMKILL (Giới hạn delay tối thiểu 10ms)
        // ════════════════════════════════════════════════════════════════════
        if (cfg::aimkill::enable) {
            auto now = std::chrono::steady_clock::now();
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastAimKillTime).count();

            // Clamp cứng: chỉ bắn khi đã đủ 10ms kể từ lần bắn trước
            if (elapsedMs >= cfg::aimkill::kDelayLimitMs) {
                s_lastAimKillTime = now;

                // Nạp sát thương tức thì trực tiếp vào Head Collider
                uint64_t aimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AimInfo);
                if (aimInfo) {
                    wpm<Vector3>(aimInfo + Offsets::HitInfo_HitLocation, best.worldPos);
                    wpm<uint64_t>(aimInfo + Offsets::HitInfo_HitCollider, best.targetCol);
                    wpm<int32_t>(aimInfo + Offsets::HitInfo_HitGroup, 1); // Head
                    wpm<float>(aimInfo + Offsets::HitInfo_DamageRatio, 1.0f);
                    wpm<bool>(aimInfo + Offsets::HitInfo_ViewBlocked, false);
                }
            }
        }

        // ════════════════════════════════════════════════════════════════════
        // 7. AIMBOT RAY & CAMERA LOCK
        // ════════════════════════════════════════════════════════════════════
        Vector3 camOrigin = (cameraPos != Vector3::Zero()) ? cameraPos : localPos;
        Vector3 targetDiff = best.worldPos - camOrigin;
        float targetDist = Vector3::Magnitude(targetDiff);

        if (targetDist > 0.05f) {
            Vector3 aimDir = targetDiff / targetDist;

            // Override AimInfo
            uint64_t aimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AimInfo);
            if (aimInfo) {
                wpm<Vector3>(aimInfo + Offsets::HitInfo_HitLocation, best.worldPos);
                wpm<Vector3>(aimInfo + Offsets::HitInfo_RayDir, aimDir);
                if (best.targetCol) {
                    wpm<uint64_t>(aimInfo + Offsets::HitInfo_HitCollider, best.targetCol);
                }
                wpm<int32_t>(aimInfo + Offsets::HitInfo_HitGroup, best.hitGroup);
                wpm<float>(aimInfo + Offsets::HitInfo_DamageRatio, 1.0f);
                wpm<bool>(aimInfo + Offsets::HitInfo_ViewBlocked, false);
            }

            // Ghi nhận mục tiêu
            wpm<uint64_t>(localAddr + Offsets::Player_LastAimTargetRef, best.addr);

            // Bẻ góc nhìn Camera khi AimLock bật
            if (cfg::aim::aim_lock) {
                uint64_t followCam = rpm<uint64_t>(localAddr + Offsets::Player_Camera);
                if (followCam) {
                    Quaternion curCamRot = rpm<Quaternion>(followCam + Offsets::Camera_Rotation);
                    Quaternion targetCamRot = Quaternion::LookRotation(aimDir, Vector3::Up());

                    float smoothness = cfg::aim::kFastSmoothness;
                    Quaternion newCamRot = Quaternion::Slerp(curCamRot, targetCamRot, smoothness);
                    wpm<Quaternion>(followCam + Offsets::Camera_Rotation, newCamRot);

                    Quaternion curPlayerRot = rpm<Quaternion>(localAddr + Offsets::Player_Rotation);
                    Quaternion newPlayerRot = Quaternion::Slerp(curPlayerRot, targetCamRot, smoothness);
                    wpm<Quaternion>(localAddr + Offsets::Player_Rotation, newPlayerRot);
                    wpm<Quaternion>(localAddr + Offsets::Player_CurrentAimRotation, newPlayerRot);

                    uint32_t rotCount = rpm<uint32_t>(localAddr + Offsets::Player_CallSetAimRotationCount);
                    wpm<uint32_t>(localAddr + Offsets::Player_CallSetAimRotationCount, rotCount + 1);
                }
            }
        }
    }
}
