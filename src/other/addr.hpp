#pragma once
#include <cstdint>
#include "memory.hpp"

namespace proc {
    extern int pid;
    extern uint64_t lib;
}

namespace func {
    inline uint64_t find_addr(uint64_t base, const uint64_t* offsets, int count) {
        if (proc::lib == 0) return 0;
        
        uint64_t addr = proc::lib + base;

        for (int i = 0; i < count; i++) {
            addr = rpm<uint64_t>(addr); 
            if (addr == 0) return 0;
            addr += offsets[i];
        }
        return addr;
    }
}
