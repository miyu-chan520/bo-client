// bo-client — Game Instrumentation & Overlay Service
// Author: @bodz520
#include "Android_draw/draw.h"
#include "ui/styles/styles.h"
#include "ui/menu.hpp"
#include "other/memory.hpp"
#include "game/data.hpp"
#include "game/aimbot.hpp"
#include "game/Offsets.h"
#include "game/DynamicOffsets.hpp"
#include "func/esp.hpp"
#include "globals.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <android/log.h>
#include <iostream>
#include <string>
#include <atomic>

#define LOG_TAG "GameOverlayService"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)

// ============================================================================
// ANSI COLORS
// ============================================================================
#define RST        "\033[0m"
#define BOLD       "\033[1m"
#define FG_BRED    "\033[91m"
#define FG_BGREEN  "\033[92m"
#define FG_BYELLOW "\033[93m"
#define FG_BBLUE   "\033[94m"
#define FG_BCYAN   "\033[96m"
#define FG_BWHITE  "\033[97m"

// ============================================================================
// GLOBAL FLAGS
// ============================================================================
std::atomic<bool> g_menu_started{true};
bool g_hideRecord = true;

// ============================================================================
// BYPASS INITIALIZATION
// ============================================================================
extern "C" void init_bypass();

// ============================================================================
// TARGET GAME DEFINITION (Free Fire TH)
// ============================================================================
static const char* kFFActivity    = "com.dts.freefireth/com.dts.freefireth.FFMainActivity";
static const char* kFFPackage     = "com.dts.freefireth";

static const char* GetTargetActivity() {
    return kFFActivity;
}
static const char* GetTargetPackage() {
    return kFFPackage;
}

// ============================================================================
// LAUNCH TARGET PROCESS (SILENT)
// ============================================================================
static bool launch_target_silent() {
    pid_t pid = fork();
    if (pid < 0) return false;
    if (pid == 0) {
        execl("/system/bin/am", "am", "start", "-n",
              GetTargetActivity(), (char*)nullptr);
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

// ============================================================================
// ATTACH TO TARGET PROCESS (PASSIVE WAIT — DO NOT AUTO-LAUNCH)
// ============================================================================
static int attach_or_launch() {
    while (true) {
        proc::pid = get_pid();
        if (proc::pid <= 0) {
            LOGI("[+] Waiting for %s to start...", GetTargetPackage());
            while (proc::pid <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                proc::pid = get_pid();
            }
        }

        LOGI("[+] Detected process pid=%d. Waiting for libil2cpp.so to load...", proc::pid);
        proc::lib = 0;
        for (int retry = 0; retry < 120; retry++) {
            if (kill(proc::pid, 0) != 0 || get_pid() <= 0) {
                LOGI("[-] Process terminated before libraries loaded. Re-waiting...");
                proc::pid = 0;
                break;
            }
            proc::lib = get_lib();
            if (proc::lib != 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        if (proc::pid > 0 && proc::lib != 0) {
            LOGI("[+] Attached to target process (%s), pid=%d | libil2cpp=0x%lx", proc::pkg_name, proc::pid, (unsigned long)proc::lib);
            return proc::pid;
        }
    }
    return -1;
}

// ============================================================================
// TERMINAL BANNER
// ============================================================================
static void print_banner() {
    printf("\n");
    printf(FG_BCYAN BOLD);
    printf("  ██████╗  ██████╗      ██████╗██╗     ██╗███████╗███╗   ██╗████████╗\n");
    printf("  ██╔══██╗██╔═══██╗    ██╔════╝██║     ██║██╔════╝████╗  ██║╚══██╔══╝\n");
    printf("  ██████╔╝██║   ██║    ██║     ██║     ██║█████╗  ██╔██╗ ██║   ██║   \n");
    printf("  ██╔══██╗██║   ██║    ██║     ██║     ██║██╔══╝  ██║╚██╗██║   ██║   \n");
    printf("  ██████╔╝╚██████╔╝    ╚██████╗███████╗██║███████╗██║ ╚████║   ██║   \n");
    printf("  ╚═════╝  ╚═════╝      ╚═════╝╚══════╝╚═╝╚══════╝╚═╝  ╚═══╝   ╚═╝   \n");
    printf(RST);
    printf(FG_BCYAN BOLD "  ╔═══════════════════════════════════════════════════════╗\n" RST);
    printf(FG_BCYAN "  ║" RST FG_BYELLOW BOLD "          BO CLIENT — GAME INSTRUMENTATION CORE        " RST FG_BCYAN "║\n" RST);
    printf(FG_BCYAN "  ║" RST FG_BGREEN "               Developer: @bodz520                     " RST FG_BCYAN "║\n" RST);
    printf(FG_BCYAN "  ╚═══════════════════════════════════════════════════════╝\n" RST);
    printf("\n");
}

// ============================================================================
// STEALTH PROCESS MASKING
// ============================================================================
static void apply_process_camouflage(int argc, char** argv) {
    // Masquerade process name in /proc/self/comm as kernel worker thread
    prctl(PR_SET_NAME, "kworker/u16:2", 0, 0, 0);

    // Overwrite cmdline in /proc/self/cmdline
    if (argc > 0 && argv && argv[0]) {
        size_t len = strlen(argv[0]);
        const char* fake_name = "[kworker/u16:2]";
        size_t fake_len = strlen(fake_name);
        if (len >= fake_len) {
            memset(argv[0], 0, len);
            memcpy(argv[0], fake_name, fake_len);
        }
    }
}

// ============================================================================
// OVERLAY RUNTIME
// ============================================================================
static int start_overlay() {
    LOGI("=== BO Client service starting ===");

    // Step 1: Attach to Target Process first (prevents premature overlay creation)
    printf(FG_BBLUE "  [1/3] " RST FG_BWHITE "Waiting for game process (" FG_BYELLOW "com.dts.freefireth" FG_BWHITE ")...\n" RST);

    if (attach_or_launch() <= 0 || proc::lib == 0) {
        printf(FG_BRED "  [!] Failed to attach to target process!\n" RST);
        return -1;
    }

    printf(FG_BGREEN "  [✓] Successfully attached to " FG_BYELLOW "%s" FG_BGREEN "! Target PID: " FG_BYELLOW "%d" FG_BGREEN " | Base: " FG_BYELLOW "0x%lx\n" RST,
           proc::pkg_name, proc::pid, (unsigned long)proc::lib);

    // Step 2: Initialize Security Bypass & Memory Hooks
    printf(FG_BBLUE "  [2/3] " RST FG_BWHITE "Initializing security bypass...\n" RST);
    init_bypass();

    // Step 2.5: Initialize Dynamic IL2CPP Offsets (Filesystem Dump + Live RAM Scan)
    DynamicOffsets::Init(proc::pid, proc::lib, proc::pkg_name);

    printf(FG_BBLUE "  [~] " RST FG_BWHITE "Initializing Aimbot Engine...\n" RST);
    aimbot::Init();
    printf(FG_BGREEN "  [✓] Aimbot engine ready\n" RST);

    // Step 3: Initialize Graphics & Screen Surface
    printf(FG_BBLUE "  [3/3] " RST FG_BWHITE "Initializing Graphical Surface...\n" RST);
    screen_config();

    int max_size = (displayInfo.height > displayInfo.width)
                   ? displayInfo.height : displayInfo.width;
    int min_size = (displayInfo.height < displayInfo.width)
                   ? displayInfo.height : displayInfo.width;

    if (max_size <= 0 || min_size <= 0) {
        max_size = 2400;
        min_size = 1080;
    }

    g_sw = static_cast<float>(max_size);
    g_sh = static_cast<float>(min_size);
    native_window_screen_x = max_size;
    native_window_screen_y = min_size;

    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        printf(FG_BRED "  [!] Failed to initialize GUI subsystem!\n" RST);
        return -1;
    }
    printf(FG_BGREEN "  [✓] Graphical surface ready (%d x %d)\n" RST, max_size, min_size);

    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);

    // Start background memory reader thread
    data::thread_start();
    printf(FG_BGREEN "  [✓] Memory data thread active\n" RST);
    fflush(stdout);

    printf(FG_BGREEN BOLD "  ⭐ BO CLIENT Overlay Active! ⭐\n" RST);
    printf(FG_BCYAN "  [INFO] Developer: @bodz520\n" RST);
    printf(FG_BCYAN "  [INFO] Controls: Top bar floating icon opens the ESP/Aimbot menu\n" RST);
    fflush(stdout);

    // Main Render Loop
    int dbgCounter = 0;
    while (true) {
        global_anim_time += 0.016f;
        drawBegin();
        ui::menu::render();
        drawEnd();
        usleep(2000);

        // Periodic Status Check (~5s)
        if (++dbgCounter >= 2500) {
            dbgCounter = 0;

            // Verify target process is still alive
            if (kill(proc::pid, 0) != 0 || get_pid() <= 0) {
                LOGI("[-] Target process terminated. Exiting overlay cleanly.");
                printf(FG_BYELLOW "\n  [-] Target process exited. Cleaning up...\n" RST);
                break;
            }

            size_t nPlayers = 0;
            {
                std::lock_guard<std::mutex> lk(data::players_mutex);
                nPlayers = data::players.size();
            }

            LOGI("[STATUS] inMatch=%d | activePlayers=%zu | aimEnabled=%d",
                 (int)data::inMatch.load(std::memory_order_relaxed),
                 nPlayers,
                 (int)cfg::aim::enable);
        }
    }
    return 0;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main(int argc, char** argv) {
    apply_process_camouflage(argc, argv);
    print_banner();
    printf(FG_BGREEN "  [+] Selected Mode: " FG_BYELLOW "Free Fire TH (com.dts.freefireth)\n" RST);
    return start_overlay();
}
