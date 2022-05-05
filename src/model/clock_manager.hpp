#ifndef CLOCK_MANAGER_HPP
#define CLOCK_MANAGER_HPP

#include <cstdint>
#include <algorithm>

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
    void sync_with_l2(uint64_t &clk) {
        l2 = std::max(l2,clk);
        clk = l2;
    }
    void sync_with_l1d(uint64_t hart_id, uint64_t &clk) {
        l1d[hart_id] = std::max(l1d[hart_id],clk);
        clk = l1d[hart_id];
    }
    uint64_t pipe_if[nr_core];
    uint64_t pipe_id[nr_core];
    uint64_t pipe_ex[nr_core];
    uint64_t pipe_mem[nr_core];
    uint64_t pipe_wb[nr_core];

    uint64_t l1d[nr_core];
    uint64_t l2;

    uint64_t cur_time() {
        uint64_t res = 0;
        for (int i=0;i<nr_core;i++) {
            res = std::max(res,pipe_if[i]);
            res = std::max(res,pipe_id[i]);
            res = std::max(res,pipe_ex[i]);
            res = std::max(res,pipe_mem[i]);
            res = std::max(res,pipe_wb[i]);
            res = std::max(res,l1d[i]);
        }
        res = std::max(res,l2);
        return res;
    }
};

#endif