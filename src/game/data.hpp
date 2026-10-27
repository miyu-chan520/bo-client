#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include "player.hpp"
#include "math.hpp"

namespace data {
    void update();
    void thread_start();
    void thread_stop();

    extern std::vector<PlayerData> players;
    extern std::mutex              players_mutex;

    extern std::vector<LootItem>   loots;
    extern std::mutex              loots_mutex;

    extern std::atomic<uint64_t>   localPlayerAddr;
    extern std::atomic<bool>       inMatch;
    extern matrix                  viewMatrix;
}