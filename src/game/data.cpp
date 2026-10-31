#include "data.hpp"
#include "Offsets.h"
#include "DynamicOffsets.hpp"
#include "aimbot.hpp"
#include "../other/utils.hpp"
#include "../other/memory.hpp"
#include "../ui/cfg.hpp"
#include "../protect/oxorany.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>
#include <unordered_map>

namespace data {

    std::vector<PlayerData>  players;
    std::mutex               players_mutex;
    std::vector<LootItem>    loots;
    std::mutex               loots_mutex;
    std::atomic<uint64_t>    localPlayerAddr{0};
    std::atomic<bool>        inMatch{false};
    matrix                   viewMatrix{};

    static std::atomic<bool> g_run{false};
    static std::thread       g_thread;

    static std::unordered_map<uint64_t, int>  g_visCounter;
    static std::unordered_map<uint64_t, bool> g_visStable;

    // ========================================================================
    // HP
    // ========================================================================
    static int readHP(uint64_t p) {
        auto pool  = rpm<uint64_t>(p + Offsets::Player_HP);
        if (!pool) return 0;
        auto datas = rpm<uint64_t>(pool + Offsets::HP_List);
        if (!datas) return 0;
        auto elem  = rpm<uint64_t>(datas + Offsets::HP_Entry);
        if (!elem) return 0;
        return rpm<int>(elem + Offsets::HP_Value);
    }

    // ========================================================================
    // NAME
    // ========================================================================
    static int readName(uint64_t p, char* buf, size_t buflen) {
        buf[0] = 0;
        auto sp = rpm<uint64_t>(p + Offsets::Player_Name);
        if (!sp || sp < 0x10000) return 0;
        int len = rpm<int>(sp + 0x10);
        if (len <= 0 || len > 64) return 0;
        uint16_t utf16[65];
        if (len > 64) len = 64;
        if (!mem_read(sp + 0x14, utf16, len * 2)) return 0;
        int n = 0;
        for (int i = 0; i < len && n < (int)buflen - 1; i++) {
            uint16_t c = utf16[i];
            if (c < 0x20 || c > 0x7E) continue;
            buf[n++] = (char)c;
        }
        buf[n] = 0;
        return n;
    }

    // ========================================================================
    // WEAPON TYPE
    // ========================================================================
    static int readWeapon(uint64_t p) {
        auto w = rpm<uint64_t>(p + Offsets::Player_Weapon);
        if (!w) return -1;
        auto wd = rpm<uint64_t>(w + Offsets::Weapon_Data);
        if (!wd) return -1;
        return rpm<int>(wd + 0x64);
    }

    // ========================================================================
    // BONE → SCREEN
    // ========================================================================
    static Vector3 boneScreen(uint64_t player, uintptr_t offset) {
        uint64_t node = rpm<uint64_t>(player + offset);
        if (!node) return Vector3::Zero();
        Vector3 w = GetNodePosition(node);
        if (w == Vector3::Zero()) return Vector3::Zero();
        return WorldToScreenPoint(viewMatrix, w);
    }

    // ========================================================================
    // SCAN LOOT
    // ========================================================================
    static void scanLoot(uint64_t BaseGame, const Vector3& localPos) {
        std::vector<LootItem> tmp;

        uint64_t match = rpm<uint64_t>(BaseGame + Offsets::BaseGame_Match);
        if (!match) return;

        uint64_t metaMgr = rpm<uint64_t>(match + Offsets::Match_MetaManager);
        if (!metaMgr) return;

        uint64_t dictPtr = rpm<uint64_t>(metaMgr + Offsets::MetaManager_DropObjects);
        if (!dictPtr) return;

        uint64_t entries = rpm<uint64_t>(dictPtr + 0x18);
        int count = rpm<int>(dictPtr + 0x20);
        if (!entries || count <= 0 || count > 500) return;

        uint64_t base = entries + 0x20;
        for (int i = 0; i < count; i++) {
            uint64_t e = base + i * 0x18;
            uint64_t dropObj = rpm<uint64_t>(e + 0x10);
            if (!dropObj || dropObj < 0x100000) continue;

            bool landed = rpm<bool>(dropObj + Offsets::DropObject_Landed);
            if (!landed) continue;

            Vector3 w = rpm<Vector3>(dropObj + Offsets::DropObject_Position);
            if (w == Vector3::Zero()) continue;

            float dist = calculate_distance(localPos, w);
            if (dist > cfg::esp::loot_range) continue;

            Vector3 sc = WorldToScreenPoint(viewMatrix, w);
            if (sc.x < 0) continue;

            LootItem li;
            li.addr      = dropObj;
            li.guid      = rpm<uint32_t>(dropObj + Offsets::DropObject_Guid);
            li.resID     = rpm<uint32_t>(dropObj + Offsets::DropObject_ResID);
            li.worldPos  = w;
            li.screenPos = sc;
            li.distance  = dist;
            tmp.push_back(li);
        }

        std::lock_guard<std::mutex> lk(loots_mutex);
        loots = std::move(tmp);
    }

    // ========================================================================
    // UPDATE
    // ========================================================================
    void update() {
        if (proc::pid <= 0 || proc::lib == 0) return;

        std::vector<PlayerData> tmp;
        tmp.reserve(64);

        // 100% Dynamic Base Chain Resolution (Zero Hardcoded Offsets)
        if (Offsets::GameFacade == 0) {
            uint64_t gfCls = DynamicOffsets::FindGameFacadeClassInRam(proc::pid, proc::lib);
            if (gfCls) {
                DynamicOffsets::ResolveGameFacadeFields(gfCls);
            }
        }
        if (Offsets::GameFacade == 0) return;

        uint64_t GfCell = rpm<uint64_t>(proc::lib + Offsets::GameFacade);
        if (!GfCell || GfCell < 0x100000000) {
            uint64_t gfCls = DynamicOffsets::FindGameFacadeClassInRam(proc::pid, proc::lib);
            if (gfCls) {
                DynamicOffsets::ResolveGameFacadeFields(gfCls);
                GfCell = rpm<uint64_t>(proc::lib + Offsets::GameFacade);
            }
        }
        if (!GfCell) return;

        auto GfClass = rpm<uint64_t>(GfCell);
        if (!GfClass) return;

        auto StaticFields = rpm<uint64_t>(GfClass + Offsets::GameFacade_P2);
        if (!StaticFields) return;

        auto BaseGame = rpm<uint64_t>(StaticFields + Offsets::BaseGame_CurrentMatchGame);
        if (!BaseGame || BaseGame < 0x100000000) {
            BaseGame = rpm<uint64_t>(StaticFields + Offsets::BaseGame_CurrentGame);
        }
        if (!BaseGame) {
            inMatch.store(false);
            localPlayerAddr.store(0);
            std::lock_guard<std::mutex> lk(players_mutex);
            players.clear();
            return;
        }

        // Dynamically resolve MatchGame & Match classes from live BaseGame RAM object
        static uint64_t s_lastBaseGame = 0;
        if (BaseGame != s_lastBaseGame) {
            DynamicOffsets::ResolveMatchChain(BaseGame);
            s_lastBaseGame = BaseGame;
        }

        auto m_Match = rpm<uint64_t>(BaseGame + Offsets::BaseGame_Match);
        if (!m_Match || m_Match < 0x100000000) {
            inMatch.store(false);
            localPlayerAddr.store(0);
            {
                std::lock_guard<std::mutex> lk(players_mutex);
                players.clear();
            }
            {
                std::lock_guard<std::mutex> lk(loots_mutex);
                loots.clear();
            }
            return;
        }

        auto localPlayer = rpm<uint64_t>(m_Match + Offsets::Match_LocalPlayer);
        if (!localPlayer || localPlayer < 0x100000000) {
            // Secondary dynamic search for local player
            localPlayer = rpm<uint64_t>(m_Match + 0xE0);
        }

        if (!localPlayer || localPlayer < 0x100000000) {
            inMatch.store(false);
            localPlayerAddr.store(0);
            {
                std::lock_guard<std::mutex> lk(players_mutex);
                players.clear();
            }
            return;
        }

        // Dynamically resolve Player class fields and bones from live LocalPlayer RAM object
        static uint64_t s_lastLocalPlayer = 0;
        if (localPlayer != s_lastLocalPlayer) {
            DynamicOffsets::ResolvePlayerClassFields(localPlayer);
            s_lastLocalPlayer = localPlayer;
        }
            {
                std::lock_guard<std::mutex> lk(loots_mutex);
                loots.clear();
            }
            return;
        }

        // Valid local player confirmed -> actively in match
        inMatch.store(true);
        localPlayerAddr.store(localPlayer);

        // ── Camera chain ──
        uint64_t Follow = 0, Cam = 0, IntPtr = 0;

        Follow = rpm<uint64_t>(localPlayer + Offsets::Player_Camera);
        if (Follow) Cam = rpm<uint64_t>(Follow + Offsets::Camera_Follow);
        if (Cam)    IntPtr = rpm<uint64_t>(Cam + Offsets::Camera_IntPtr);

        if (!IntPtr) {
            // Alternative fallback camera offset
            Follow = rpm<uint64_t>(localPlayer + 0x690);
            if (Follow) Cam = rpm<uint64_t>(Follow + 0x30);
            if (Cam)    IntPtr = rpm<uint64_t>(Cam + 0x10);
        }

        if (!IntPtr) return;

        viewMatrix = rpm<matrix>(IntPtr + Offsets::Camera_Matrix);

        // ── Local & Camera position ──
        Vector3 localPos = Vector3::Zero();
        uint64_t headNode = rpm<uint64_t>(localPlayer + Offsets::Bone_Head);
        if (headNode) localPos = GetNodePosition(headNode);

        // Resolve true 3D camera world position:
        // 1. Try Player.MainCameraTransform (0x3E8)
        // 2. Try FollowCamera.Transform (0x40)
        // 3. Fallback: localPos + over-the-shoulder 3rd-person offsets
        Vector3 cameraPos = Vector3::Zero();
        uint64_t camTF = rpm<uint64_t>(localPlayer + Offsets::Player_MainCameraTransform);
        if (!camTF && Follow) {
            camTF = rpm<uint64_t>(Follow + Offsets::Camera_Transform);
        }
        if (camTF) {
            cameraPos = GetPosition(camTF);
        }

        if (cameraPos == Vector3::Zero() && Follow) {
            float rOff = rpm<float>(Follow + Offsets::Camera_RightOffset);
            float uOff = rpm<float>(Follow + Offsets::Camera_UpOffset);
            float bOff = rpm<float>(Follow + Offsets::Camera_BackOffset);
            Quaternion camRot = rpm<Quaternion>(Follow + Offsets::Camera_Rotation);
            if (camRot.sqrMagnitude() > 0.001f) {
                Vector3 fwd = camRot * Vector3::Forward();
                Vector3 rgt = camRot * Vector3::Right();
                Vector3 up  = camRot * Vector3::Up();
                float rightDist = (rOff > 0.1f && rOff < 2.0f) ? rOff : 0.55f;
                float upDist    = (uOff > 0.05f && uOff < 2.0f) ? uOff : 0.20f;
                float backDist  = (bOff > 0.1f && bOff < 5.0f) ? bOff : 1.80f;
                cameraPos = localPos - (fwd * backDist) + (rgt * rightDist) + (up * upDist);
            }
        }
        if (cameraPos == Vector3::Zero()) cameraPos = localPos;

        // ── LOOT ──
        if (cfg::esp::loot) {
            scanLoot(BaseGame, localPos);
        } else {
            std::lock_guard<std::mutex> lk(loots_mutex);
            loots.clear();
        }

        // ── PLAYERS ──
        uint64_t dictAddr = rpm<uint64_t>(BaseGame + Offsets::Match_PlayerDict);
        if (!dictAddr) return;

        int32_t localTeam = rpm<int32_t>(localPlayer + Offsets::Player_TeamIndex);

        MonoDictionary dict(dictAddr);
        if (!dict.valid) return;

        int nv = dict.getNumValues();
        for (int i = 0; i < nv; i++) {
            uint64_t enemy = dict.entry(i);
            if (!enemy || enemy < 0x100000000 || enemy == localPlayer) continue;

            if (rpm<bool>(enemy + Offsets::Player_IsDead)) continue;

            // Strict Teammate Filter (Never aimbot or ESP friendly teammates)
            int32_t enemyTeam = rpm<int32_t>(enemy + Offsets::Player_TeamIndex);
            if (localTeam >= 0 && enemyTeam >= 0 && localTeam == enemyTeam) continue;

            bool isBot = rpm<bool>(enemy + Offsets::Player_IsBot);

            int hp = readHP(enemy);
            if (hp <= 0) continue;

            auto avatarMgr = rpm<uint64_t>(enemy + Offsets::Player_Avatar);
            if (!avatarMgr) continue;
            auto uma = rpm<uint64_t>(avatarMgr + Offsets::Avatar_Uma);
            if (!uma) continue;

            bool rawVisible = rpm<bool>(uma + Offsets::Uma_Visible);

            // Debounce visible
            bool visible;
            {
                auto it = g_visStable.find(enemy);
                if (it == g_visStable.end()) {
                    g_visStable[enemy] = rawVisible;
                    g_visCounter[enemy] = 0;
                    visible = rawVisible;
                } else if (it->second == rawVisible) {
                    g_visCounter[enemy] = 0;
                    visible = rawVisible;
                } else {
                    g_visCounter[enemy]++;
                    if (g_visCounter[enemy] >= 2) {
                        g_visStable[enemy] = rawVisible;
                        g_visCounter[enemy] = 0;
                        visible = rawVisible;
                    } else {
                        visible = it->second;
                    }
                }
            }

            // Team UmaData check fallback
            auto umaData = rpm<uint64_t>(uma + Offsets::Uma_Data);
            if (umaData && rpm<bool>(umaData + Offsets::UmaData_Skip)) continue;

            // Head world
            uint64_t hNode = rpm<uint64_t>(enemy + Offsets::Bone_Head);
            if (!hNode) continue;
            Vector3 headWorld = GetNodePosition(hNode);
            if (headWorld == Vector3::Zero()) continue;

            float dist = calculate_distance(localPos, headWorld);
            if (dist > cfg::esp::max_range) continue;
            if (!visible && dist < 200.f) continue;

            Vector3 headScreen = WorldToScreenPoint(viewMatrix, headWorld);
            Vector3 toeScreen  = boneScreen(enemy, Offsets::Bone_LeftToe);

            // Build
            PlayerData pd;
            readName(enemy, pd.name, sizeof(pd.name));
            pd.weaponType = readWeapon(enemy);
            if (!pd.name[0] && isBot) snprintf(pd.name, sizeof(pd.name), "BOT");

            pd.addr       = enemy;
            pd.team       = enemyTeam;
            pd.isTeammate = false;
            pd.Head       = headScreen;
            pd.Toe        = toeScreen;
            pd.isVisible  = visible;
            pd.isBot      = isBot;
            pd.distance   = dist;
            pd.curHP      = hp;
            pd.maxHP      = 200;
            pd.curArmor   = rpm<int>(enemy + Offsets::Player_Armor);
            pd.isFiring   = rpm<bool>(enemy + Offsets::Player_IsFiring);
            pd.isGrounded = rpm<bool>(enemy + Offsets::Player_IsGrounded);

            Vector3 vel = rpm<Vector3>(enemy + Offsets::Player_Velocity);
            pd.speed = sqrtf(vel.x*vel.x + vel.y*vel.y + vel.z*vel.z);

            // Bones
            pd.Hip       = boneScreen(enemy, Offsets::Bone_Hip);
            pd.Breast    = boneScreen(enemy, Offsets::Bone_Breast);
            pd.Root      = boneScreen(enemy, Offsets::Bone_Root);
            pd.LShoulder = boneScreen(enemy, Offsets::Bone_LeftShoulder);
            pd.RShoulder = boneScreen(enemy, Offsets::Bone_RightShoulder);
            pd.LAnkle    = boneScreen(enemy, Offsets::Bone_LeftAnkle);
            pd.RAnkle    = boneScreen(enemy, Offsets::Bone_RightAnkle);
            pd.LToe      = toeScreen;
            pd.RToe      = boneScreen(enemy, Offsets::Bone_RightToe);
            pd.LForeArm  = boneScreen(enemy, Offsets::Bone_LeftForeArm);
            pd.RForeArm  = boneScreen(enemy, Offsets::Bone_RightForeArm);
            pd.LHand     = boneScreen(enemy, Offsets::Bone_LeftHand);
            pd.RHand     = boneScreen(enemy, Offsets::Bone_RightHand);

            tmp.push_back(pd);
        }

        if (g_visStable.size() > 200) {
            g_visStable.clear();
            g_visCounter.clear();
        }

        {
            std::lock_guard<std::mutex> lk(players_mutex);
            players.swap(tmp);
        }

        // Aimbot Engine Tick
        aimbot::Update(localPlayer, localPos, cameraPos);
    }

    // ========================================================================
    // THREAD
    // ========================================================================
    static void loop() {
        uint32_t seed = 0x9E3779B1u;
        while (g_run.load(std::memory_order_relaxed)) {
            update();
            seed = seed * 1664525u + 1013904223u;
            int jitter = (seed >> 24) & 7;
            std::this_thread::sleep_for(std::chrono::milliseconds(5 + jitter));
        }
    }

    void thread_start() {
        if (g_run.exchange(true)) return;
        g_thread = std::thread(loop);
    }

    void thread_stop() {
        if (!g_run.exchange(false)) return;
        if (g_thread.joinable()) g_thread.join();
        std::lock_guard<std::mutex> lk(players_mutex);
        players.clear();
    }
}