#ifndef CLOCK_MANAGER_HPP
#define CLOCK_MANAGER_HPP

#include <cstdint>

template <int nr_core = 2>
class clock_manager {
    // clock manager stores the next available time for calculating IPC including cache coherence protocol
public:
    clock_manager() {
        for (int i=0;i<nr_core;i++) {
            pipe_if[i] = 0;
            pipe_id[i] = 0;
            pipe_ex[i] = 0;
            pipe_mem[i] = 0;
            pipe_wb[i] = 0;
            l1d[i] = 0;
        }
        l2 = 0;
    }
    uint64_t pipe_if[nr_core];
    uint64_t pipe_id[nr_core];
    uint64_t pipe_ex[nr_core];
    uint64_t pipe_mem[nr_core];
    uint64_t pipe_wb[nr_core];

    uint64_t l1d[nr_core];
    uint64_t l2;
};

#endif