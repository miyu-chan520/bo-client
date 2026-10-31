// ZqwClient — NEXORA PROJECT (External Overlay & Native Surface)
#include "Android_draw/draw.h"
#include "ui/styles/styles.h"
#include "ui/menu.hpp"
#include "other/memory.hpp"
#include "game/data.hpp"
#include "game/AimSilent.h"
#include "game/Offsets.h"
#include "game/features.hpp"
#include "func/esp.hpp"
#include "Login/Login.h"
#include "Login/Jni_Stuff.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <unistd.h>
#include <sys/wait.h>
#include <android/log.h>
#include <iostream>
#include <string>

#define LOG_TAG "NEXORA"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

// ═══════════════════════════════════════════════════════════
// ANSI COLOR
// ═══════════════════════════════════════════════════════════
#define RST        "\033[0m"
#define BOLD       "\033[1m"
#define FG_BRED    "\033[91m"
#define FG_BGREEN  "\033[92m"
#define FG_BYELLOW "\033[93m"
#define FG_BBLUE   "\033[94m"
#define FG_BCYAN   "\033[96m"
#define FG_BWHITE  "\033[97m"

// ═══════════════════════════════════════════════════════════
//  FLAG GLOBAL
// ═══════════════════════════════════════════════════════════
extern float global_anim_time;
bool g_hideRecord = true;

// ═══════════════════════════════════════════════════════════
// BYPASS INITIALIZATION
// ═══════════════════════════════════════════════════════════
extern "C" void init_bypass();

// ═══════════════════════════════════════════════════════════
// TARGET GAME
// ═══════════════════════════════════════════════════════════
enum TargetGame {
    GAME_FF_AUTO = 0,
    GAME_FF_TH   = 1,
    GAME_FF_VN   = 2,
    GAME_FF_MAX  = 3
};

static TargetGame g_targetGame = GAME_FF_AUTO;

static const char* kFFActivity    = "com.dts.freefireth/com.dts.freefireth.FFMainActivity";
static const char* kFFMaxActivity = "com.dts.freefiremax/com.dts.freefiremax.FFMainActivity";
static const char* kFFVnActivity  = "com.dts.freefirevn/com.dts.freefirevn.FFMainActivity";

// ═══════════════════════════════════════════════════════════
// BANNER
// ═══════════════════════════════════════════════════════════
static void print_banner() {
    printf("\n");
    printf(FG_BRED BOLD);
    printf("  ██╗   ██╗██╗███╗   ██╗██╗   ██╗██╗███████╗██╗   ██╗\n");
    printf("  ██║   ██║██║████╗  ██║██║   ██║██║██╔════╝██║   ██║\n");
    printf("  ██║   ██║██║██╔██╗ ██║██║   ██║██║███████╗██║   ██║\n");
    printf("  ██║   ██║██║██║╚██╗██║██║   ██║██║╚════██║██║   ██║\n");
    printf("  ╚██████╔╝██║██║ ╚████║╚██████╔╝██║███████║╚██████╔╝\n");
    printf("   ╚═════╝ ╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝╚══════╝ ╚═════╝ \n");
    printf(RST);
    printf(FG_BCYAN BOLD "  ╔═══════════════════════════════════════════════╗\n" RST);
    printf(FG_BCYAN "  ║" RST FG_BYELLOW BOLD "        NEXORA PROJECT — FREE FIRE VIP         " RST FG_BCYAN "║\n" RST);
    printf(FG_BCYAN "  ║" RST FG_BGREEN "         EXTERNAL OVERLAY & EXPLOIT           " RST FG_BCYAN "║\n" RST);
    printf(FG_BCYAN "  ╚═══════════════════════════════════════════════╝\n" RST);
    printf("\n");
}

// ═══════════════════════════════════════════════════════════
// START OVERLAY SERVICE
// ═══════════════════════════════════════════════════════════
static int start_overlay() {
    LOGI("=== NEXORA External Service starting ===");

    // ─── 1. INIT NATIVE SURFACE GUI ───
    printf(FG_BBLUE "  [1/4] " RST FG_BWHITE "Initializing Hardware Screen Overlay...\n" RST);

    screen_config();

    int max_size = (displayInfo.height > displayInfo.width)
                   ? displayInfo.height : displayInfo.width;
    int min_size = (displayInfo.height < displayInfo.width)
                   ? displayInfo.height : displayInfo.width;

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);
    native_window_screen_x = max_size;
    native_window_screen_y = min_size;

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        printf(FG_BRED "  [!] initGUI FAILED! Ensure running as root.\n" RST);
        return -1;
    }
    printf(FG_BGREEN "  [✓] Hardware Overlay ready (%dx%d)!\n" RST, max_size, min_size);

    // ─── 2. INIT TOUCH DRIVER ───
    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);
    printf(FG_BGREEN "  [✓] Touch input driver hooked via /dev/input\n" RST);

    // ─── 3. BYPASS & VIP LICENSE ───
    g_license_activated.store(0x4E580021, std::memory_order_release);
    g_menu_started.store(true, std::memory_order_release);

    printf(FG_BBLUE "  [2/4] " RST FG_BWHITE "Initializing Anti-Cheat Bypass...\n" RST);
    init_bypass();
    printf(FG_BGREEN "  [✓] Memory bypass initialized\n" RST);

    // ─── 4. ASYNC GAME ATTACH & DATA ENGINE ───
    printf(FG_BBLUE "  [3/4] " RST FG_BWHITE "Waiting for Free Fire process...\n" RST);
    std::thread([]() {
        while (proc::pid <= 0) {
            proc::pid = get_pid();
            if (proc::pid > 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        LOGI("[+] Free Fire process detected: PID %d (%s)", proc::pid, proc::pkg_name);
        printf(FG_BGREEN "  [✓] Free Fire detected! PID: " FG_BYELLOW "%d" FG_BGREEN " (%s)\n" RST,
               proc::pid, proc::pkg_name);

        for (int i = 0; i < 120; i++) {
            proc::lib = get_lib();
            if (proc::lib > 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        LOGI("[+] libil2cpp.so base: 0x%lx", (unsigned long)proc::lib);
        printf(FG_BGREEN "  [✓] libil2cpp.so mapped @ " FG_BYELLOW "0x%lx\n" RST,
               (unsigned long)proc::lib);

        InitAimSilentHook();
        data::thread_start();
        features::install_hooks();

        printf(FG_BGREEN BOLD "  ⭐ NEXORA VIP Overlay Fully Active & Synced! ⭐\n" RST);
        fflush(stdout);
    }).detach();

    printf(FG_BBLUE "  [4/4] " RST FG_BWHITE "Starting Render Engine...\n" RST);
    printf(FG_BGREEN BOLD "  ⭐ Overlay Started! ⭐\n" RST);
    printf(FG_BCYAN "  [INFO] Tap top-right floating Nexora badge to toggle Menu\n" RST);
    fflush(stdout);

    // ─── 5. MAIN RENDER LOOP ───
    while (true) {
        global_anim_time += 0.016f;
        drawBegin();
        ui::menu::render();
        drawEnd();
        usleep(2000);
    }

    return 0;
}

// ═══════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════
#ifndef ZYGISK_MODULE
int main(int argc, char** argv) {
    print_banner();

    if (argc >= 2) {
        if (strcmp(argv[1], "ffmax") == 0) {
            g_targetGame = GAME_FF_MAX;
            printf(FG_BGREEN "  [+] Target forced: " FG_BYELLOW "Free Fire MAX\n" RST);
        } else if (strcmp(argv[1], "ffvn") == 0) {
            g_targetGame = GAME_FF_VN;
            printf(FG_BGREEN "  [+] Target forced: " FG_BYELLOW "Free Fire VN\n" RST);
        }
    }

    return start_overlay();
}
#endif
