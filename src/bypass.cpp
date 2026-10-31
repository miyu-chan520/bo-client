// bypass.cpp — Full Security Bypass Implementation via ShadowHook + Libc Cloaking
// Reconstructed & Integrated from P2077kng libmain.so & BO Client Security Subsystem
#include "shadowhook/shadowhook.h"
#include "memory.hpp"
#include "ui/cfg.hpp"
#include "game/libmain_offsets.hpp"
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <errno.h>
#include <dlfcn.h>
#include <thread>
#include <android/log.h>

#define LOG_TAG "SecuritySubsystem"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// 1. LIBC STEALTH HOOKS (Từ libmain.so 0xd4694: openat, close, read, process_vm_readv)
// ============================================================================
static void* orig_openat = nullptr;
static int hook_openat(int dirfd, const char* pathname, int flags, mode_t mode) {
    if (pathname && cfg::bypass::libc_stealth) {
        // Intercept maps inspection to hide injected modules
        if (strstr(pathname, "maps") || strstr(pathname, "pagemap") || strstr(pathname, "status")) {
            LOGI("[stealth] openat intercepted: %s", pathname);
        }
    }
    typedef int (*openat_fn)(int, const char*, int, mode_t);
    if (orig_openat) {
        return ((openat_fn)orig_openat)(dirfd, pathname, flags, mode);
    }
    return syscall(__NR_openat, dirfd, pathname, flags, mode);
}

static void* orig_read = nullptr;
static ssize_t hook_read(int fd, void* buf, size_t count) {
    typedef ssize_t (*read_fn)(int, void*, size_t);
    ssize_t ret = orig_read ? ((read_fn)orig_read)(fd, buf, count) : syscall(__NR_read, fd, buf, count);
    if (ret > 0 && cfg::bypass::libc_stealth && buf) {
        char* str = (char*)buf;
        // Scrub traces of our modules and shadowhook from maps buffer
        if (strstr(str, "bo_service") || strstr(str, "libshadowhook") || strstr(str, "libmain")) {
            // Nullify line contents
            memset(buf, 0, ret);
            ret = 0;
        }
    }
    return ret;
}

static void* orig_process_vm_readv = nullptr;
static ssize_t hook_process_vm_readv(pid_t pid, const struct iovec *local_iov,
                                     unsigned long liovcnt, const struct iovec *remote_iov,
                                     unsigned long riovcnt, unsigned long flags) {
    if (cfg::bypass::libc_stealth) {
        // Block external anti-cheat scanning (libanort / libanogs cross-memory reads)
        LOGI("[stealth] process_vm_readv blocked for pid %d", pid);
        errno = EPERM;
        return -1;
    }
    typedef ssize_t (*vm_readv_fn)(pid_t, const struct iovec*, unsigned long, const struct iovec*, unsigned long, unsigned long);
    if (orig_process_vm_readv) {
        return ((vm_readv_fn)orig_process_vm_readv)(pid, local_iov, liovcnt, remote_iov, riovcnt, flags);
    }
    return syscall(__NR_process_vm_readv, pid, local_iov, liovcnt, remote_iov, riovcnt, flags);
}

// ============================================================================
// 2. ANOGS / ANORT SECURITY HOOKS (Neutralize detection routines)
// ============================================================================
static constexpr uint64_t OFF_ANTI_FRIDA       = 0x3516DC;
static constexpr uint64_t OFF_ANTI_DEBUG       = 0x365954;
static constexpr uint64_t OFF_ANTI_MONO_MAIN   = 0x307EF0;
static constexpr uint64_t OFF_ANTI_MONO_DLOPEN = 0x308B2C;
static constexpr uint64_t OFF_SCAN_WRAPPER     = 0x419E3C;
static constexpr uint64_t OFF_SCAN_THREAD      = 0x419EE4;

static void* orig_anti_frida = nullptr;
static int hook_anti_frida(void* a1, void* a2) { return 0; }

static void* orig_anti_debug = nullptr;
static int hook_anti_debug(void* a1, void* a2, uint32_t a3, const char* a4,
                            uint64_t* a5, int a6, int a7, int a8, int a9,
                            int a10, int a11, int a12, int a13, int a14,
                            int a15, int a16, int a17, int a18, int a19,
                            int a20, int a21, int64_t a22) { return 0; }

static void* orig_anti_mono_main = nullptr;
static int hook_anti_mono_main(void* a1) { return 0; }

static void* orig_anti_mono_dlopen = nullptr;
static int hook_anti_mono_dlopen(void* a1) { return 1; }

static void* orig_scan_wrapper = nullptr;
static int hook_scan_wrapper(void* a1) { return 0; }

static void* orig_scan_thread = nullptr;
static int hook_scan_thread(void* a1) { return 0; }

// ============================================================================
// 3. IN-GAME IL2CPP TELEMETRY BYPASS (COW EventLogger & GarenaMSDK)
// ============================================================================
static void* orig_send_event_log = nullptr;
static void hook_send_event_log(void* self, void* jsonStr) {
    // Drop JSON telemetry logs sent to Garena servers
    LOGI("[bypass] COW::EventLogger::SendEventLogJson suppressed");
}

static void* orig_msdk_update = nullptr;
static void hook_msdk_update(void* self) {
    // Drop Garena MSDK security heartbeat updates
}

static uint64_t find_libanogs_base() {
    if (proc::pid <= 0) return 0;
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/maps", proc::pid);
    FILE* f = fopen(path, "r");
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

// ============================================================================
// 4. MAIN BYPASS INITIALIZATION
// ============================================================================
static void run_bypass_internal() {
    LOGI("=== Security bypass initializing ===");

    if (!cfg::bypass::enable) {
        LOGI("[~] Bypass disabled in configuration");
        return;
    }

    // Initialize ShadowHook engine
    int err = shadowhook_init(SHADOWHOOK_MODE_UNIQUE, true);
    if (err != 0) {
        LOGE("[!] shadowhook_init status: %d", err);
    } else {
        LOGI("[+] ShadowHook engine active");
    }

    // Hook Libc stealth functions
    if (cfg::bypass::libc_stealth) {
        void* libc = dlopen("libc.so", RTLD_NOW);
        if (libc) {
            void* p_openat = dlsym(libc, "openat");
            if (p_openat) shadowhook_hook_func_addr(p_openat, (void*)hook_openat, (void**)&orig_openat);

            void* p_read = dlsym(libc, "read");
            if (p_read) shadowhook_hook_func_addr(p_read, (void*)hook_read, (void**)&orig_read);

            void* p_vm_readv = dlsym(libc, "process_vm_readv");
            if (p_vm_readv) shadowhook_hook_func_addr(p_vm_readv, (void*)hook_process_vm_readv, (void**)&orig_process_vm_readv);
            LOGI("[+] Libc stealth hooks installed successfully");
        }
    }

    // Hook IL2CPP telemetry if available in-process
    if (cfg::bypass::telemetry_nullify) {
        uintptr_t pSendLog = LibmainOffsets::ResolveMethod("Assembly-CSharp.dll", "COW", "EventLogger", "SendEventLogJson", 2);
        if (pSendLog) {
            shadowhook_hook_func_addr((void*)pSendLog, (void*)hook_send_event_log, (void**)&orig_send_event_log);
            LOGI("[+] COW::EventLogger::SendEventLogJson hooked");
        }

        uintptr_t pMsdkUpdate = LibmainOffsets::ResolveMethod("Assembly-CSharp.dll", "GarenaMSDK", "GarenaMSDKMgr", "Update", 0);
        if (pMsdkUpdate) {
            shadowhook_hook_func_addr((void*)pMsdkUpdate, (void*)hook_msdk_update, (void**)&orig_msdk_update);
            LOGI("[+] GarenaMSDKMgr::Update hooked");
        }
    }

    // Hook libanogs anti-debug/anti-frida if loaded
    uint64_t anogsBase = find_libanogs_base();
    if (anogsBase) {
        shadowhook_hook_func_addr((void*)(anogsBase + OFF_ANTI_FRIDA), (void*)hook_anti_frida, (void**)&orig_anti_frida);
        shadowhook_hook_func_addr((void*)(anogsBase + OFF_ANTI_DEBUG), (void*)hook_anti_debug, (void**)&orig_anti_debug);
        shadowhook_hook_func_addr((void*)(anogsBase + OFF_ANTI_MONO_MAIN), (void*)hook_anti_mono_main, (void**)&orig_anti_mono_main);
        shadowhook_hook_func_addr((void*)(anogsBase + OFF_ANTI_MONO_DLOPEN), (void*)hook_anti_mono_dlopen, (void**)&orig_anti_mono_dlopen);
        shadowhook_hook_func_addr((void*)(anogsBase + OFF_SCAN_WRAPPER), (void*)hook_scan_wrapper, (void**)&orig_scan_wrapper);
        shadowhook_hook_func_addr((void*)(anogsBase + OFF_SCAN_THREAD), (void*)hook_scan_thread, (void**)&orig_scan_thread);
        LOGI("[+] libanogs.so security routines neutralized");
    }

    LOGI("=== Security bypass fully engaged ===");
}

extern "C" void init_bypass() {
    std::thread(run_bypass_internal).detach();
}
