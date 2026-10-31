#pragma once
#include "../other/vector3.h"
#include <cstdint>

struct PlayerData {
    uint64_t addr;

    // ── Box ──
    Vector3  Head;
    Vector3  Toe;
    Vector3  headPos; // 3D world position

    // ── Bones ──
    Vector3  Hip;
    Vector3  Breast;
    Vector3  Root;
    Vector3  LShoulder;
    Vector3  RShoulder;
    Vector3  LAnkle;
    Vector3  RAnkle;
    Vector3  LToe;
    Vector3  RToe;
    Vector3  LForeArm;
    Vector3  RForeArm;
    Vector3  LHand;
    Vector3  RHand;

    // ── Meta ──
    bool     isVisible;
    bool     isBot;
    bool     isDead;
    float    distance;
    int      curHP;
    int      maxHP;
    int      curArmor;
    bool     isFiring;
    bool     isGrounded;
    float    speed;
    char     name[36];
    int      weaponType;

    PlayerData()
        : addr(0),
          Head(Vector3::Zero()), Toe(Vector3::Zero()), headPos(Vector3::Zero()),
          Hip(Vector3::Zero()), Breast(Vector3::Zero()), Root(Vector3::Zero()),
          LShoulder(Vector3::Zero()), RShoulder(Vector3::Zero()),
          LAnkle(Vector3::Zero()), RAnkle(Vector3::Zero()),
          LToe(Vector3::Zero()), RToe(Vector3::Zero()),
          LForeArm(Vector3::Zero()), RForeArm(Vector3::Zero()),
          LHand(Vector3::Zero()), RHand(Vector3::Zero()),
          isVisible(false), isBot(false), isDead(false), distance(0.f),
          curHP(0), maxHP(200), curArmor(0),
          isFiring(false), isGrounded(true), speed(0.f),
          weaponType(-1)
    { name[0] = 0; }
};

struct LootItem {
    uint64_t addr;
    uint32_t guid;
    uint32_t resID;
    Vector3  worldPos;
    Vector3  screenPos;
    float    distance;

    LootItem()
        : addr(0), guid(0), resID(0),
          worldPos(Vector3::Zero()), screenPos(Vector3::Zero()),
          distance(0.f) {}
};
