#pragma once

#include "game.hpp"
#include "Offsets.h"
#include "../other/vector3.h"
#include "../other/memory.hpp"
#include "styles/styles.h"     // g_sw / g_sh
#include <cmath>

inline Vector3 GetPosition(uint64_t Transform) {
    auto transformObjValue = rpm<uint64_t>(Transform + 0x10);
    if (transformObjValue == 0) return Vector3::Zero();

    struct { uint64_t matrices; uint64_t indices; } ptrs;
    auto matrixValue = rpm<uint64_t>(transformObjValue + 0x38);
    if (!matrixValue) return Vector3::Zero();

    ptrs.matrices = rpm<uint64_t>(matrixValue + 0x18);
    ptrs.indices  = rpm<uint64_t>(matrixValue + 0x20);
    if (!ptrs.matrices || !ptrs.indices) return Vector3::Zero();

    auto indexValue = rpm<uint64_t>(transformObjValue + 0x40);

    auto resultValue = rpm<Vector3>(ptrs.matrices + (indexValue * 0x30));
    auto transformIndexValue = rpm<int>(ptrs.indices + (indexValue * 4));

    for (int tries = 0; transformIndexValue >= 0 && tries < 50; tries++) {
        uint64_t matAddr = ptrs.matrices + (transformIndexValue * 0x30);
        auto t = rpm<TransformMatrix>(matAddr);

        auto rx = t.rotation.x, ry = t.rotation.y, rz = t.rotation.z, rw = t.rotation.w;
        auto sx = resultValue.x * t.scale.x;
        auto sy = resultValue.y * t.scale.y;
        auto sz = resultValue.z * t.scale.z;

        resultValue.x = t.position.x + sx +
                        (sx * ((ry * ry * -2.0) - (rz * rz * 2.0))) +
                        (sy * ((rw * rz * -2.0) - (ry * rx * -2.0))) +
                        (sz * ((rz * rx *  2.0) - (rw * ry * -2.0)));
        resultValue.y = t.position.y + sy +
                        (sx * ((rx * ry *  2.0) - (rw * rz * -2.0))) +
                        (sy * ((rz * rz * -2.0) - (rx * rx *  2.0))) +
                        (sz * ((rw * rx * -2.0) - (rz * ry * -2.0)));
        resultValue.z = t.position.z + sz +
                        (sx * ((rw * ry * -2.0) - (rx * rz * -2.0))) +
                        (sy * ((ry * rz *  2.0) - (rw * rx * -2.0))) +
                        (sz * ((rx * rx * -2.0) - (ry * ry *  2.0)));

        transformIndexValue = rpm<int>(ptrs.indices + (transformIndexValue * 4));
    }

    return resultValue;
}

inline Vector3 GetNodePosition(uint64_t nodeTransform) {
    auto transformValue = rpm<uint64_t>(nodeTransform + 0x10);
    if (transformValue == 0) return Vector3::Zero();
    return GetPosition(transformValue);
}

// ── Bone helpers ──
inline uint64_t GetPlayerHeadTF(uint64_t player) {
    return rpm<uint64_t>(player + Offsets::Bone_Head);
}

inline uint64_t GetPlayerPeTF(uint64_t player) {
    return rpm<uint64_t>(player + Offsets::Bone_LeftToe);
}

inline Vector3 WorldToScreenPoint(const matrix& vm, const Vector3& pos) {
    float v12 = (pos.x * vm.m14) + (pos.y * vm.m24) + (pos.z * vm.m34) + vm.m44;
    if (v12 >= 0.001f) {
        float inv = 1.0f / v12;
        float cx = g_sw * 0.5f;
        float cy = g_sh * 0.5f;
        float v9  = (pos.x * vm.m11) + (pos.y * vm.m21) + (pos.z * vm.m31) + vm.m41;
        float v10 = (pos.x * vm.m12) + (pos.y * vm.m22) + (pos.z * vm.m32) + vm.m42;
        return Vector3(cx + cx * v9 * inv, cy - cy * v10 * inv, 0);
    }
    return Vector3(-1, -1, -1);
}

inline float calculate_distance(const Vector3& a, const Vector3& b) noexcept {
    float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}