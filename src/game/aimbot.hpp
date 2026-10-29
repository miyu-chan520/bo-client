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
        uint64_t targetCol = 0;
        int32_t  hitGroup = 0;
    };

    // Touch tracker for drag aim state
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
        // 2. TARGET BONE SELECTION (0: Head, 1: Neck, 2: Chest)
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
                else if (targetBone == 2 && is_screen_valid(p.Breast)) boneScreen = p.Breast;

                if (!is_screen_valid(boneScreen)) continue;

                float sDist = hypotf(boneScreen.x - screenCx, boneScreen.y - screenCy);

                if (cfg::aim::fov > 0.f && sDist > cfg::aim::fov) continue;

                if (sDist < best.screenDist) {
                    Vector3 wPos = Vector3::Zero();
                    uint64_t col = 0;
                    int32_t grp = 0;

                    // 1. Resolve Aim Location position
                    if (targetBone == 0) {
                        // Head
                        uint64_t bNode = rpm<uint64_t>(p.addr + Offsets::Bone_Head);
                        if (bNode) wPos = GetNodePosition(bNode);
                    } else if (targetBone == 1) {
                        // Neck (Upper breast base)
                        uint64_t bNode = rpm<uint64_t>(p.addr + Offsets::Bone_Breast);
                        if (bNode) wPos = GetNodePosition(bNode) + Vector3(0.0f, 0.12f, 0.0f);
                    } else {
                        // Chest (Breast bone level, above default abdomen)
                        uint64_t bNode = rpm<uint64_t>(p.addr + Offsets::Bone_Breast);
                        if (bNode) wPos = GetNodePosition(bNode);
                    }

                    if (wPos == Vector3::Zero()) continue;

                    // 2. Resolve Hit Collider & HitGroup:
                    // If collision_zone_headshot is active, when firing to the chosen collision zone,
                    // convert it to Head Collider and Headshot damage group (grp = 1).
                    uint64_t headCol = rpm<uint64_t>(p.addr + Offsets::Player_HedColider);
                    uint64_t bodyCol = rpm<uint64_t>(p.addr + Offsets::Player_ColWrite);

                    if (cfg::aim::collision_zone_headshot) {
                        // Collision zone feature enabled: transform chosen zone hits into Headshot damage
                        col = headCol ? headCol : bodyCol;
                        grp = 1; // Headshot damage group
                    } else {
                        // Normal zone collision mapping
                        if (targetBone == 0) {
                            col = headCol;
                            grp = 1; // Head
                        } else if (targetBone == 1) {
                            col = bodyCol;
                            grp = 2; // Neck / upper torso
                        } else {
                            col = bodyCol;
                            grp = 0; // Torso / Chest
                        }
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
        // 4. LITE TARGETING OVERRIDE (Replaces default abdomen target)
        // ════════════════════════════════════════════════════════════════════
        Vector3 camOrigin = (cameraPos != Vector3::Zero()) ? cameraPos : localPos;
        Vector3 targetDiff = best.worldPos - camOrigin;
        float targetDist = Vector3::Magnitude(targetDiff);

        if (targetDist > 0.05f) {
            // Direct vector centered precisely on chosen bone (zero lateral shift)
            Vector3 aimDir = targetDiff / targetDist;

            // 1) Override Main Weapon AimInfo targeting point
            uint64_t aimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AimInfo);
            if (aimInfo) {
                wpm<Vector3>(aimInfo + Offsets::HitInfo_HitLocation, best.worldPos);
                wpm<Vector3>(aimInfo + Offsets::HitInfo_RayDir, aimDir);
                if (best.targetCol) {
                    wpm<uint64_t>(aimInfo + Offsets::HitInfo_HitCollider, best.targetCol);
                }
                wpm<int32_t>(aimInfo + Offsets::HitInfo_HitGroup, best.hitGroup);
                wpm<int16_t>(aimInfo + Offsets::HitInfo_HitGroupShort, (int16_t)best.hitGroup);
                if (cfg::aim::collision_zone_headshot) {
                    wpm<float>(aimInfo + Offsets::HitInfo_DamageRatio, 2.0f);
                }
                wpm<bool>(aimInfo + Offsets::HitInfo_ViewBlocked, false);
            }

            // 2) Override Aux/ADS AimInfo targeting point
            uint64_t auxAimInfo = rpm<uint64_t>(localAddr + Offsets::Player_AuxAimInfo);
            if (auxAimInfo) {
                wpm<Vector3>(auxAimInfo + Offsets::HitInfo_HitLocation, best.worldPos);
                wpm<Vector3>(auxAimInfo + Offsets::HitInfo_RayDir, aimDir);
                if (best.targetCol) {
                    wpm<uint64_t>(auxAimInfo + Offsets::HitInfo_HitCollider, best.targetCol);
                }
                wpm<int32_t>(auxAimInfo + Offsets::HitInfo_HitGroup, best.hitGroup);
                wpm<int16_t>(auxAimInfo + Offsets::HitInfo_HitGroupShort, (int16_t)best.hitGroup);
                if (cfg::aim::collision_zone_headshot) {
                    wpm<float>(auxAimInfo + Offsets::HitInfo_DamageRatio, 2.0f);
                }
                wpm<bool>(auxAimInfo + Offsets::HitInfo_ViewBlocked, false);
            }

            // 3) Update target reference
            wpm<uint64_t>(localAddr + Offsets::Player_LastAimTargetRef, best.addr);

            // 4) FollowCamera targeting point override & natural smooth assist
            uint64_t followCam = rpm<uint64_t>(localAddr + Offsets::Player_Camera);
            if (followCam) {
                uint64_t camAimInfo = rpm<uint64_t>(followCam + 0xC8);
                if (camAimInfo) {
                    wpm<Vector3>(camAimInfo + Offsets::HitInfo_HitLocation, best.worldPos);
                    wpm<Vector3>(camAimInfo + Offsets::HitInfo_RayDir, aimDir);
                    if (best.targetCol) {
                        wpm<uint64_t>(camAimInfo + Offsets::HitInfo_HitCollider, best.targetCol);
                    }
                    wpm<int32_t>(camAimInfo + Offsets::HitInfo_HitGroup, best.hitGroup);
                }

                // Natural, micro-damped camera assist (eliminates jerky upward snap)
                if (cfg::aim::aim_lock) {
                    Quaternion curCamRot = rpm<Quaternion>(followCam + Offsets::Camera_Rotation);
                    Quaternion targetCamRot = Quaternion::LookRotation(aimDir, Vector3::Up());

                    Quaternion newCamRot = Quaternion::Slerp(curCamRot, targetCamRot, cfg::aim::kLiteSmoothness);
                    wpm<Quaternion>(followCam + Offsets::Camera_Rotation, newCamRot);

                    Quaternion curPlayerRot = rpm<Quaternion>(localAddr + Offsets::Player_Rotation);
                    Quaternion newPlayerRot = Quaternion::Slerp(curPlayerRot, targetCamRot, cfg::aim::kLiteSmoothness);
                    wpm<Quaternion>(localAddr + Offsets::Player_Rotation, newPlayerRot);
                    wpm<Quaternion>(localAddr + Offsets::Player_CurrentAimRotation, newPlayerRot);
                }
            }
        }
    }
}
