// ZqwClient — NEXORA PROJECT
#include "Android_draw/draw.h"
#include "ui/styles/styles.h"
#include "ui/menu.hpp"
#include "other/memory.hpp"
#include "game/data.hpp"
#include "game/AimSilent.h"
#include "game/Offsets.h"
#include "func/esp.hpp"
#include "Login/Login.h"
#include "Login/Jni_Stuff.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <chrono>
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
std::atomic<bool> g_menu_started{false};

// ═══════════════════════════════════════════════════════════
// BYPASS
// ═══════════════════════════════════════════════════════════
extern "C" void init_bypass();

// ═══════════════════════════════════════════════════════════
// GLOBAL
// ═══════════════════════════════════════════════════════════
bool g_hideRecord = true;

// ═══════════════════════════════════════════════════════════
// TARGET GAME
// ═══════════════════════════════════════════════════════════
enum TargetGame {
    GAME_FF     = 0,
    GAME_FF_MAX = 1
};

static TargetGame g_targetGame = GAME_FF;

static const char* kFFActivity    = "com.dts.freefireth/com.dts.freefireth.FFMainActivity";
static const char* kFFMaxActivity = "com.dts.freefiremax/com.dts.freefiremax.FFMainActivity";
static const char* kFFPackage     = "com.dts.freefireth";
static const char* kFFMaxPackage  = "com.dts.freefiremax";

static const char* GetTargetActivity() {
    return (g_targetGame == GAME_FF) ? kFFActivity : kFFMaxActivity;
}
static const char* GetTargetPackage() {
    return (g_targetGame == GAME_FF) ? kFFPackage : kFFMaxPackage;
}

// ═══════════════════════════════════════════════════════════
// LAUNCH TARGET
// ═══════════════════════════════════════════════════════════
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

// ═══════════════════════════════════════════════════════════
// ATTACH / LAUNCH
// ═══════════════════════════════════════════════════════════
static int attach_or_launch() {
    proc::pid = get_pid();
    if (proc::pid > 0) {
        LOGI("[+] Already running, pid=%d", proc::pid);
        return proc::pid;
    }
    LOGI("[+] Launching %s...", GetTargetPackage());
    launch_target_silent();
    for (int i = 0; i < 120; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        proc::pid = get_pid();
        if (proc::pid > 0) {
            LOGI("[+] Attached, pid=%d", proc::pid);
            return proc::pid;
        }
    }
    return -1;
}

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
    printf(FG_BCYAN "  ║" RST FG_BYELLOW BOLD "        NEXORA PROJECT — FREE FIRE          " RST FG_BCYAN "║\n" RST);
    printf(FG_BCYAN "  ║" RST FG_BGREEN "              v1.0 VIP EDITION                 " RST FG_BCYAN "║\n" RST);
    printf(FG_BCYAN "  ╚═══════════════════════════════════════════════╝\n" RST);
    printf("\n");
}

// ═══════════════════════════════════════════════════════════
// TERMINAL LOGIN — INPUT KEY
// ═══════════════════════════════════════════════════════════
static bool terminal_login() {
    char keyBuf[256] = {0};

    printf(FG_BYELLOW "\n  ┌─────────────────────────────────────────────┐\n" RST);
    printf(FG_BYELLOW "  │" RST FG_BWHITE BOLD "        🔑  ENTER LICENSE KEY               " RST FG_BYELLOW "│\n" RST);
    printf(FG_BYELLOW "  └─────────────────────────────────────────────┘\n" RST);
    printf("\n");

    printf(FG_BCYAN "  ► " RST FG_BWHITE "License Key: " RST);
    fflush(stdout);

    if (fgets(keyBuf, sizeof(keyBuf), stdin) == nullptr) {
        printf(FG_BRED "\n  [!] Gagal baca input\n" RST);
        return false;
    }

    std::string key = keyBuf;
    key.erase(0, key.find_first_not_of(" \t\n\r"));
    if (!key.empty())
        key.erase(key.find_last_not_of(" \t\n\r") + 1);

    if (key.empty()) {
        printf(FG_BRED "  [!] Key kosong!\n" RST);
        return false;
    }

    printf("\n");
    printf(FG_BBLUE "  [~] Verifying key: " RST FG_BYELLOW "%s" RST "\n", key.c_str());
    LOGI("[+] Login with key: %s", key.c_str());

    std::string result = Login(key.c_str());

    if (result == "OK") {
        g_license_activated.store(0x4E580021, std::memory_order_release);
        printf(FG_BGREEN "  [✓] Key VALID!" RST);
        if (!g_license_user_name.empty())
            printf(FG_BYELLOW " Welcome %s" RST, g_license_user_name.c_str());
        printf("\n");
        return true;
    } else {
        printf(FG_BRED "  [!] Key INVALID: " RST FG_BYELLOW "%s" RST "\n", result.c_str());
        return false;
    }
}

// ═══════════════════════════════════════════════════════════
// START OVERLAY
// ═══════════════════════════════════════════════════════════
static int start_overlay() {
    LOGI("=== NEXORA starting ===");

    // ─── INIT GUI ───
    printf(FG_BBLUE "  [1/4] " RST FG_BWHITE "Init GUI...\n" RST);

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
        printf(FG_BRED "  [!] initGUI FAILED!\n" RST);
        return -1;
    }
    printf(FG_BGREEN "  [✓] GUI ready\n" RST);

    touch::init(max_size, min_size, (uint8_t)displayInfo.orientation);

    // ═══════════════════════════════════════════════════════
    //  [2/4] TERMINAL LOGIN
    // ═══════════════════════════════════════════════════════
    while (!IsLicenseActivated()) {
        if (terminal_login()) {
            break;
        }
        printf(FG_BYELLOW "\n  [~] Coba lagi...\n" RST);
    }

    printf(FG_BGREEN "\n  [✓] Login berhasil! Menunggu klik START...\n" RST);

    // ═══════════════════════════════════════════════════════
    //  [3/4] TAMPIL IMGUI
    // ═══════════════════════════════════════════════════════
    while (!g_menu_started.load()) {
        global_anim_time += 0.016f;

        drawBegin();
        DrawLicenseMenu();
        drawEnd();

        usleep(16000);
    }

    printf(FG_BGREEN "  [✓] Menu started!\n" RST);

    // ─── BYPASS ───
    printf(FG_BBLUE "  [4/4] " RST FG_BWHITE "Init bypass...\n" RST);
    init_bypass();

    // ─── ATTACH KE GAME ───
    printf(FG_BBLUE "  [~] " RST FG_BWHITE "Attaching to " FG_BYELLOW "%s" FG_BWHITE "...\n" RST,
           GetTargetPackage());

    if (attach_or_launch() <= 0) {
        printf(FG_BRED "  [!] Gagal attach\n" RST);
        return -1;
    }

    proc::lib = get_lib();
    printf(FG_BGREEN "  [✓] Attached! PID: " FG_BYELLOW "%d\n" RST, proc::pid);

    // ═══════════════════════════════════════════════════════
    //  INIT AIM SILENT HOOK (DEBUG)
    // ═══════════════════════════════════════════════════════
    printf(FG_BBLUE "  [~] " RST FG_BWHITE "Init Aim Silent Hook...\n" RST);

    printf(FG_BYELLOW "  [DEBUG] lib base       = 0x%lx\n" RST, (unsigned long)proc::lib);
    printf(FG_BYELLOW "  [DEBUG] RVA TakeDamage_9 = 0x%lx\n" RST, (unsigned long)Offsets::Player_TakeDamage_9);
    printf(FG_BYELLOW "  [DEBUG] RVA TakeDamage_4 = 0x%lx\n" RST, (unsigned long)Offsets::Player_TakeDamage_4);
    printf(FG_BYELLOW "  [DEBUG] RVA HeadCollider = 0x%lx\n" RST, (unsigned long)Offsets::Player_get_HeadCollider);
    printf(FG_BYELLOW "  [DEBUG] Hook addr 9    = 0x%lx\n" RST, (unsigned long)(proc::lib + Offsets::Player_TakeDamage_9));
    printf(FG_BYELLOW "  [DEBUG] Hook addr 4    = 0x%lx\n" RST, (unsigned long)(proc::lib + Offsets::Player_TakeDamage_4));
    printf(FG_BYELLOW "  [DEBUG] Hook addr HC   = 0x%lx\n" RST, (unsigned long)(proc::lib + Offsets::Player_get_HeadCollider));
    fflush(stdout);

    InitAimSilentHook();

    printf(FG_BGREEN "  [✓] Aim Silent Hook installed\n" RST);
    printf(FG_BYELLOW "  [DEBUG] AimSilentEnabled = %d\n" RST, (int)AimSilentEnabled);
    printf(FG_BYELLOW "  [DEBUG] AimSilentNobita  = %d\n" RST, (int)AimSilentNobita);
    fflush(stdout);

    // ─── START DATA THREAD ───
    data::thread_start();
    printf(FG_BGREEN "  [✓] Data thread started\n" RST);
    fflush(stdout);

    printf(FG_BGREEN BOLD "  ⭐ Overlay Started! ⭐\n" RST);
    printf(FG_BCYAN "  [INFO] Toggle Aim Silent di menu > tab ESP\n" RST);
    printf(FG_BCYAN "  [INFO] Masuk match, tembak musuh, cek log\n" RST);
    fflush(stdout);

    // ═══════════════════════════════════════════════════════
    //  MAIN LOOP — MENU CHEAT + DEBUG
    // ═══════════════════════════════════════════════════════
    int dbgCounter = 0;
    while (true) {
        global_anim_time += 0.016f;
        drawBegin();
        ui::menu::render();
        drawEnd();
        usleep(2000);

        // ─── DEBUG LOG TIAP 5 DETIK ───
        if (++dbgCounter >= 2500) {  // ~5 detik
            dbgCounter = 0;

            size_t nPlayers = 0;
            {
                std::lock_guard<std::mutex> lk(data::players_mutex);
                nPlayers = data::players.size();
            }

            printf(FG_BCYAN "[DBG] " RST
                   "inMatch=%d | players=%zu | aim=%d | nobita=%d\n",
                   (int)data::inMatch.load(std::memory_order_relaxed),
                   nPlayers,
                   (int)AimSilentEnabled,
                   (int)AimSilentNobita);
            fflush(stdout);
        }
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
            printf(FG_BGREEN "  [+] Target: " FG_BYELLOW "Free Fire MAX\n" RST);
        }
    }

    return start_overlay();
}
#endif