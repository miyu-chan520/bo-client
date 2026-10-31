#pragma once

#include <cstdint>
#include <cstring>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/syscall.h>
#include "../protect/oxorany.hpp"

namespace proc {
    inline int      pid = -1;
    inline uint64_t lib = 0;
    inline int      pm  = -1;
    inline bool     is_max = false;
    inline char     pkg_name[64] = "com.dts.freefireth";
}

namespace sys {
    #if defined(__aarch64__)
    static constexpr long SYS_openat     = 56;
    static constexpr long SYS_close      = 57;
    static constexpr long SYS_read       = 63;
    static constexpr long SYS_getdents64 = 61;
    static constexpr long SYS_vm_read    = 270;
    static constexpr long SYS_vm_write   = 271;
    #elif defined(__x86_64__)
    static constexpr long SYS_openat     = 257;
    static constexpr long SYS_close      = 3;
    static constexpr long SYS_read       = 0;
    static constexpr long SYS_getdents64 = 217;
    static constexpr long SYS_vm_read    = 310;
    static constexpr long SYS_vm_write   = 311;
    #endif

    #ifndef AT_FDCWD
    #define AT_FDCWD -100
    #endif

    static inline long sc6(long n, long a1, long a2, long a3, long a4, long a5, long a6) {
        long r;
        #if defined(__aarch64__)
        register long x8 __asm__("x8") = n;
        register long x0 __asm__("x0") = a1;
        register long x1 __asm__("x1") = a2;
        register long x2 __asm__("x2") = a3;
        register long x3 __asm__("x3") = a4;
        register long x4 __asm__("x4") = a5;
        register long x5 __asm__("x5") = a6;
        __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory", "cc");
        r = x0;
        #elif defined(__x86_64__)
        register long rax asm("rax") = n;
        register long rdi asm("rdi") = a1;
        register long rsi asm("rsi") = a2;
        register long rdx asm("rdx") = a3;
        register long r10 asm("r10") = a4;
        register long r8  asm("r8")  = a5;
        register long r9  asm("r9")  = a6;
        asm volatile("syscall" : "=a"(r) : "a"(rax), "D"(rdi), "S"(rsi), "d"(rdx), "r"(r10), "r"(r8), "r"(r9) : "rcx", "r11", "memory");
        #else
        r = syscall(n, a1, a2, a3, a4, a5, a6);
        #endif
        return r;
    }

    static inline int  openat(int fd, const char* p, int f)  { return static_cast<int>(sc6(SYS_openat, fd, reinterpret_cast<long>(p), f, 0, 0, 0)); }
    static inline int  close (int fd)                        { return static_cast<int>(sc6(SYS_close, fd, 0, 0, 0, 0, 0)); }
    static inline long read  (int fd, void* b, size_t c)     { return sc6(SYS_read, fd, reinterpret_cast<long>(b), c, 0, 0, 0); }
    static inline long getdents64(int fd, void* d, size_t c) { return sc6(SYS_getdents64, fd, reinterpret_cast<long>(d), c, 0, 0, 0); }
    static inline long vm_read (int pid, struct iovec* l, unsigned long lc, struct iovec* r, unsigned long rc) {
        return sc6(SYS_vm_read, pid, reinterpret_cast<long>(l), lc, reinterpret_cast<long>(r), rc, 0);
    }
    static inline long vm_write(int pid, struct iovec* l, unsigned long lc, struct iovec* r, unsigned long rc) {
        return sc6(SYS_vm_write, pid, reinterpret_cast<long>(l), lc, reinterpret_cast<long>(r), rc, 0);
    }
}

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t  d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char d_name[];
};

static inline void wipe(void* p, size_t n) {
    volatile uint8_t* v = reinterpret_cast<volatile uint8_t*>(p);
    for (size_t i = 0; i < n; i++) v[i] = 0;
}

static inline bool addr_valid(uint64_t a) {
    return (a >= 0x10000 && a < 0x00007fffffffffffULL);
}

static inline bool addr_valid(const void* ptr) {
    return addr_valid(reinterpret_cast<uint64_t>(ptr));
}

#ifdef ZYGISK_MODULE

// In-process memory access (Direct memcpy inside target process)
static inline bool mem_read(uintptr_t addr, void* buf, size_t sz) {
    if (!addr || addr < 0x10000 || !buf) return false;
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
static inline bool wpm(uintptr_t addr, const T& val) {
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

#else

// External memory access via process_vm_readv / process_vm_writev syscalls
static inline bool mem_read(uint64_t a, void* b, size_t l) {
    if (proc::pid < 0 || a < 0x10000 || l == 0 || b == nullptr) return false;
    struct iovec loc[1], rem[1];
    loc[0].iov_base = b;
    loc[0].iov_len  = l;
    rem[0].iov_base = reinterpret_cast<void*>(a);
    rem[0].iov_len  = l;
    ssize_t r = sys::vm_read(proc::pid, loc, 1, rem, 1);
    if (r != static_cast<long>(l)) {
        memset(b, 0, l);
        return false;
    }
    return true;
}

static inline bool mem_write(uint64_t a, const void* b, size_t l) {
    if (proc::pid < 0 || a < 0x10000 || l == 0 || b == nullptr) return false;
    struct iovec loc[1], rem[1];
    loc[0].iov_base = const_cast<void*>(b);
    loc[0].iov_len  = l;
    rem[0].iov_base = reinterpret_cast<void*>(a);
    rem[0].iov_len  = l;
    return sys::vm_write(proc::pid, loc, 1, rem, 1) == static_cast<long>(l);
}

template<typename T> static inline T rpm(uint64_t a) {
    T v{};
    mem_read(a, &v, sizeof(T));
    return v;
}

template<typename T> static inline bool wpm(uint64_t a, const T& d) {
    return mem_write(a, &d, sizeof(T));
}

template<typename T> static inline T rpm(const void* a) {
    return rpm<T>(reinterpret_cast<uint64_t>(a));
}

template<typename T> static inline bool wpm(const void* a, const T& d) {
    return wpm<T>(reinterpret_cast<uint64_t>(a), d);
}

inline int get_pid() {
    int pfd = sys::openat(AT_FDCWD, oxorany("/proc"), O_RDONLY | 00200000 /* O_DIRECTORY */);
    if (pfd < 0) return -1;

    char buf[4096];
    int found = -1;

    while (found < 0) {
        long nread = sys::getdents64(pfd, buf, sizeof(buf));
        if (nread <= 0) break;

        long pos = 0;
        while (pos < nread) {
            auto* d = reinterpret_cast<struct linux_dirent64*>(buf + pos);
            if (d->d_type == 4 /* DT_DIR */) {
                bool is_num = true;
                for (int i = 0; d->d_name[i]; i++) {
                    if (d->d_name[i] < '0' || d->d_name[i] > '9') { is_num = false; break; }
                }
                if (is_num && d->d_name[0] != '\0') {
                    int p = atoi(d->d_name);
                    char cmdpath[64];
                    snprintf(cmdpath, sizeof(cmdpath), "/proc/%d/cmdline", p);
                    int cfd = sys::openat(AT_FDCWD, cmdpath, O_RDONLY);
                    if (cfd >= 0) {
                        char cmd[128] = {0};
                        long b = sys::read(cfd, cmd, sizeof(cmd) - 1);
                        sys::close(cfd);
                        if (b > 0) {
                            cmd[b] = '\0';
                            if (strstr(cmd, "com.dts.freefire") != nullptr) {
                                found = p;
                                strncpy(proc::pkg_name, cmd, sizeof(proc::pkg_name) - 1);
                                if (strstr(cmd, "max") != nullptr) proc::is_max = true;
                                wipe(cmd, sizeof(cmd));
                                break;
                            }
                            wipe(cmd, sizeof(cmd));
                        }
                    }
                }
            }
            pos += d->d_reclen;
        }
    }
    sys::close(pfd);
    wipe(buf, sizeof(buf));
    return found;
}

inline uint64_t get_lib() {
    if (proc::pid <= 0) return 0;

    char mp[64];
    snprintf(mp, sizeof(mp), "/proc/%d/maps", proc::pid);

    int mfd = sys::openat(AT_FDCWD, mp, O_RDONLY);
    if (mfd < 0) return 0;

    char buf[8192];
    long tr = 0;
    uint64_t bs = 0;

    while (true) {
        long nr = sys::read(mfd, buf + tr, sizeof(buf) - tr - 1);
        if (nr <= 0) break;
        tr += nr;
        buf[tr] = '\0';

        char* ls = buf;
        while (true) {
            char* le = ls;
            while (*le && *le != '\n') le++;
            if (*le == '\0') break;

            *le = '\0';

            uint64_t st = 0;
            char* pt = ls;

            while (*pt && ((*pt >= '0' && *pt <= '9') || (*pt >= 'a' && *pt <= 'f') || (*pt >= 'A' && *pt <= 'F'))) {
                uint64_t v = (*pt <= '9') ? (*pt - '0') : ((*pt >= 'a') ? (*pt - 'a' + 10) : (*pt - 'A' + 10));
                st = st * 16 + v;
                pt++;
            }
            if (*pt == '-' && strstr(ls, "libil2cpp.so") != nullptr && strstr(ls, "r-xp") != nullptr) {
                bs = st;
                sys::close(mfd);
                wipe(buf, sizeof(buf));
                return bs;
            }

            ls = le + 1;
        }

        if (ls < buf + tr) {
            int rm = (buf + tr) - ls;
            for (int i = 0; i < rm; i++) buf[i] = ls[i];
            tr = rm;
        } else {
            tr = 0;
        }
    }

    sys::close(mfd);
    wipe(buf, sizeof(buf));
    return bs;
}

inline void init_mem() {}

#endif

namespace game {
    inline bool valid() {
        if (proc::pid <= 0) proc::pid = get_pid();
        return (proc::lib != 0);
    }
    inline void init() {
        proc::pid = get_pid();
        if (proc::pid > 0) proc::lib = get_lib();
    }
}
