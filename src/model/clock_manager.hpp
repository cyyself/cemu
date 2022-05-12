#ifndef CLOCK_MANAGER_HPP
#define CLOCK_MANAGER_HPP

#include <cstdint>
#include <algorithm>

template <int max_nr_core = 32>
class clock_manager {
    // clock manager stores the next available time for calculating IPC including cache coherence protocol
public:
    clock_manager(int nr_core):nr_core(nr_core) {
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
    uint64_t pipe_if[max_nr_core];
    uint64_t pipe_id[max_nr_core];
    uint64_t pipe_ex[max_nr_core];
    uint64_t pipe_mem[max_nr_core];
    uint64_t pipe_wb[max_nr_core];

    uint64_t l1d[max_nr_core];
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
    uint64_t get_min_wb() {
        uint64_t res = 0;
        for (int i=1;i<nr_core;i++) {
            if (pipe_wb[i] < pipe_wb[res]) {
                res = i;
            }
        }
        return res;
    }
private:
    int nr_core;
};

#endif