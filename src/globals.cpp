// src/globals.cpp
#include <atomic>

float g_sw = 0.0f;
float g_sh = 0.0f;
float global_anim_time = 0.0f;
float g_ui_scale = 1.0f;
std::atomic<bool> menu_visible{false};