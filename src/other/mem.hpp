#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdint>

namespace proc {
    extern int pid;
    extern uint64_t lib;
}

inline int get_mem_fd() {
    static int mem_fd = -1;
    if (mem_fd < 0) {
        char path[64];
        snprintf(path, sizeof(path), "/proc/%d/mem", proc::pid);
        mem_fd = open(path, O_RDWR);
    }
    return mem_fd;
}
