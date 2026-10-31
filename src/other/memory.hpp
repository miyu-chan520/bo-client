#pragma once

#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/syscall.h>
#include "../protect/oxorany.hpp"

namespace proc {
    inline int      pid = -1;
    inline uint64_t lib = 0;
    inline int      pm  = -1;
}

namespace sys {
    static inline long sc6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
        #if defined(__aarch64__)
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a1;
        register long x1 __asm__("x1") = a2;
        register long x2 __asm__("x2") = a3;
        register long x3 __asm__("x3") = a4;
        register long x4 __asm__("x4") = a5;
        register long x5 __asm__("x5") = a6;
        __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory", "cc");
        return x0;
        #else
        return syscall(n, a1, a2, a3, a4, a5, a6);
        #endif
    }
}

// In-Process Safe Memory Access
static inline bool addr_valid(uint64_t addr) {
    return (addr >= 0x10000 && addr < 0x00007fffffffffffULL);
}

static inline bool addr_valid(const void* ptr) {
    return addr_valid(reinterpret_cast<uint64_t>(ptr));
}

static inline bool mem_read(uintptr_t addr, void* buf, size_t sz) {
    if (!addr || addr < 0x10000 || !buf) return false;
    // In-process direct memory copy
    memcpy(buf, (const void*)addr, sz);
    return true;
}

static inline bool mem_write(uintptr_t addr, const void* buf, size_t sz) {
    if (!addr || addr < 0x10000 || !buf) return false;
    memcpy((void*)addr, buf, sz);
    return true;
}

template <typename T>
static inline T rpm(uintptr_t addr) {
    T val{};
    if (addr && addr >= 0x10000) {
        memcpy(&val, (const void*)addr, sizeof(T));
    }
    return val;
}

template <typename T>
static inline bool wpm(uintptr_t addr, T val) {
    if (addr && addr >= 0x10000) {
        memcpy((void*)addr, &val, sizeof(T));
        return true;
    }
    return false;
}

inline int get_pid() {
    return getpid();
}

inline uint64_t get_lib() {
    return proc::lib;
}

inline void init_mem() {}

namespace game {
    inline bool valid() {
        if (proc::pid <= 0) proc::pid = getpid();
        return (proc::lib != 0);
    }
    inline void init() {
        proc::pid = getpid();
    }
}
