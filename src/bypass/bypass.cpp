#include "bypass.hpp"
#include "shadowhook/shadowhook.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <android/log.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string>

#define LOG_TAG "BoBypass"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace bypass {

static bool g_anogs_bypassed = false;
static bool g_stealth_active = false;

bool is_anogs_hooked() { return g_anogs_bypassed; }
bool is_stealth_active() { return g_stealth_active; }

// ============================================================================
// LAYER 1: libanogs.so Anti-Cheat Neutralization
// ============================================================================
static constexpr uint64_t OFF_ANTI_FRIDA       = 0x3516DC;
static constexpr uint64_t OFF_ANTI_DEBUG       = 0x365954;
static constexpr uint64_t OFF_ANTI_MONO_MAIN   = 0x307EF0;
static constexpr uint64_t OFF_ANTI_MONO_DLOPEN = 0x308B2C;
static constexpr uint64_t OFF_SCAN_WRAPPER     = 0x419E3C;
static constexpr uint64_t OFF_SCAN_THREAD      = 0x419EE4;
static constexpr uint64_t OFF_SCAN_WRAPPER_ALT = 0x39FF60;
static constexpr uint64_t OFF_SCAN_THREAD_ALT  = 0x39FF78;
static constexpr uint64_t OFF_DETECT_CHEAT_DLL = 0x30A108;
static constexpr uint64_t OFF_DETECT_CHEAT_DLL2= 0x319CE4;
static constexpr uint64_t OFF_DETECT_CHEAT_DLL3= 0x2D7A78;
static constexpr uint64_t OFF_ANTI_MONO_DUP    = 0x318EF0;

static int hook_stub_zero(...) {
    return 0;
}

static int hook_stub_one(...) {
    return 1;
}

static uint64_t find_libanogs_base() {
    FILE* f = fopen("/proc/self/maps", "r");
    if (!f) return 0;
    char line[512];
    uint64_t base = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "libanogs.so")) {
            unsigned long start = 0;
            if (sscanf(line, "%lx-", &start) == 1) {
                base = (uint64_t)start;
                break;
            }
        }
    }
    fclose(f);
    return base;
}

static void apply_anogs_bypass() {
    uint64_t base = 0;
    for (int i = 0; i < 60; ++i) {
        base = find_libanogs_base();
        if (base != 0) break;
        usleep(250000);
    }

    if (!base) {
        LOGE("[-] libanogs.so not loaded or timed out");
        return;
    }

    LOGI("[+] libanogs.so base: 0x%lx. Hooking detection routines...", (unsigned long)base);

    void* dummy = nullptr;
    shadowhook_hook_func_addr((void*)(base + OFF_ANTI_FRIDA),       (void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_ANTI_DEBUG),       (void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_ANTI_MONO_MAIN),   (void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_ANTI_MONO_DLOPEN), (void*)hook_stub_one,  &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_SCAN_WRAPPER),     (void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_SCAN_THREAD),      (void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_SCAN_WRAPPER_ALT), (void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_SCAN_THREAD_ALT),  (void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_DETECT_CHEAT_DLL), (void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_DETECT_CHEAT_DLL2),(void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_DETECT_CHEAT_DLL3),(void*)hook_stub_zero, &dummy);
    shadowhook_hook_func_addr((void*)(base + OFF_ANTI_MONO_DUP),    (void*)hook_stub_zero, &dummy);

    g_anogs_bypassed = true;
    LOGI("[✓] Layer 1: libanogs.so detection vectors neutralized!");
}

// ============================================================================
// LAYER 2: Stealth Memory Maps & Status Cloaking (/proc Filter)
// ============================================================================
static FILE* (*orig_fopen)(const char* filename, const char* mode) = nullptr;
static int   (*orig_openat)(int dirfd, const char* pathname, int flags, mode_t mode) = nullptr;

static bool is_dangerous_line(const char* line) {
    if (strstr(line, "zygisk") ||
        strstr(line, "bo_client") ||
        strstr(line, "libshadowhook") ||
        strstr(line, "magisk") ||
        strstr(line, "modules")) {
        return true;
    }
    return false;
}

static FILE* hook_fopen(const char* filename, const char* mode) {
    if (filename && (strstr(filename, "/proc/self/maps") || strstr(filename, "/maps"))) {
        FILE* real = orig_fopen(filename, mode);
        if (!real) return nullptr;

        // Create sanitized temporary in-memory map
        char tmp_path[64];
        snprintf(tmp_path, sizeof(tmp_path), "/data/data/com.dts.freefireth/cache/.map_%d", getpid());
        FILE* fake = orig_fopen(tmp_path, "w+");
        if (!fake) {
            // fallback
            return real;
        }
        unlink(tmp_path); // unlinked immediately, remains open in memory

        char buf[512];
        while (fgets(buf, sizeof(buf), real)) {
            if (!is_dangerous_line(buf)) {
                fputs(buf, fake);
            }
        }
        fclose(real);
        rewind(fake);
        return fake;
    }

    if (filename && strstr(filename, "/proc/self/status")) {
        FILE* real = orig_fopen(filename, mode);
        if (!real) return nullptr;

        char tmp_path[64];
        snprintf(tmp_path, sizeof(tmp_path), "/data/data/com.dts.freefireth/cache/.stat_%d", getpid());
        FILE* fake = orig_fopen(tmp_path, "w+");
        if (!fake) return real;
        unlink(tmp_path);

        char buf[512];
        while (fgets(buf, sizeof(buf), real)) {
            if (strncmp(buf, "TracerPid:", 10) == 0) {
                fputs("TracerPid:\t0\n", fake);
            } else {
                fputs(buf, fake);
            }
        }
        fclose(real);
        rewind(fake);
        return fake;
    }

    return orig_fopen(filename, mode);
}

static void apply_stealth_cloak() {
    shadowhook_hook_sym_name("libc.so", "fopen", (void*)hook_fopen, (void**)&orig_fopen);
    g_stealth_active = true;
    LOGI("[✓] Layer 2: Cloaking /proc/self/maps and status TracerPid active!");
}

// ============================================================================
// LAYER 3: Combined Orchestrator
// ============================================================================
void init_combined_bypass() {
    LOGI("[*] Initializing Triple-Layer Bypass & Stealth Suite...");
    
    // Initialize ShadowHook in unique mode
    int err = shadowhook_init(SHADOWHOOK_MODE_UNIQUE, true);
    if (err != 0) {
        LOGE("[-] shadowhook_init error: %d", err);
    } else {
        LOGI("[+] ShadowHook engine online");
    }

    // Apply libc cloaking first before anogs scans memory
    apply_stealth_cloak();

    // Spawn thread to catch anogs initialization
    std::thread([]() {
        apply_anogs_bypass();
    }).detach();
}

} // namespace bypass
