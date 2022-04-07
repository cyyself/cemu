#ifndef RV_SYSTEMBUS
#define RV_SYSTEMBUS

#include "rv_common.hpp"
#include <cstdint>
#include <assert.h>
#include <algorithm>
#include "memory.hpp"
#include <map>
#include <utility>
#include <climits>

// TODO: add pma and check pma
class rv_systembus {
public:
    bool pa_read(uint64_t start_addr, uint64_t size, uint8_t *buffer) {
        auto it = devices.upper_bound(std::make_pair(start_addr,ULONG_MAX));
        if (it == devices.begin()) return false;
        it = std::prev(it);
        unsigned long end_addr = start_addr + size;
        if (it->first.first <= start_addr && end_addr <= it->first.second) {
            unsigned long dev_size = it->first.second - it->first.first;
            return it->second->do_read(start_addr % dev_size, size, buffer);
        }
        else return false;
    }
    bool pa_write(uint64_t start_addr, uint64_t size, const uint8_t *buffer) {
        if (start_addr <= lr_pa && lr_pa + size <= start_addr + size) {
            lr_valid = false;
        }
        auto it = devices.upper_bound(std::make_pair(start_addr,ULONG_MAX));
        if (it == devices.begin()) return false;
        it = std::prev(it);
        unsigned long end_addr = start_addr + size;
        if (it->first.first <= start_addr && end_addr <= it->first.second) {
            unsigned long dev_size = it->first.second - it->first.first;
            return it->second->do_write(start_addr % dev_size, size, buffer);
        }
        else return false;
    }
    // note: check address alignment in the core and raise address misalign exception
    bool pa_lr(uint64_t pa, uint64_t size, uint8_t *dst, uint64_t hart_id) {
        lr_pa = pa;
        lr_size = size;
        lr_valid = true;
        lr_hart = hart_id;
        return pa_read(pa,size,dst);
    }
    // Note: if pa_write return false, sc_fail shouldn't commit.
    bool pa_sc(uint64_t pa, uint64_t size, const uint8_t *src, uint64_t hart_id, bool &sc_fail) {
        if (!lr_valid || lr_pa != pa || lr_size != size || lr_hart != hart_id) {
            sc_fail = true;
            if (hart_id == lr_hart) lr_valid = false;
            return true;
        }
        sc_fail = false;
        lr_valid = false;
        return pa_write(pa,size,src);
    }
    // note: core should check whether amoop is valid 
    bool pa_amo_op(uint64_t pa, uint64_t size, amo_funct op, int64_t src, int64_t &dst) {
        int64_t res;
        bool read_ok = true;
        if (size == 4) {
            int32_t res32;
            read_ok &= pa_read(pa,size,(uint8_t*)&res32);
            res = res32;
        }
        else {
            read_ok &= pa_read(pa,size,(uint8_t*)&res);
        }
        if (!read_ok) return false;
        int64_t to_write;
        switch (op) {
            case AMOSWAP:
                to_write = src;
                break;
            case AMOADD:
                to_write = src + res;
                break;
            case AMOAND:
                to_write = src & res;
                break;
            case AMOOR:
                to_write = src | res;
                break;
            case AMOXOR:
                to_write = src ^ res;
                break;
            case AMOMAX:
                to_write = std::max(src,res);
                break;
            case AMOMIN:
                to_write = std::min(src,res);
                break;
            case AMOMAXU:
                to_write = std::max((uint64_t)src,(uint64_t)res);
                break;
            case AMOMINU:
                to_write = std::min((uint64_t)src,(uint64_t)res);
                break;
            default:
                assert(false);
        }
        dst = res;
        return pa_write(pa,size,(uint8_t*)&to_write);
    }
    bool add_dev(unsigned long start_addr, unsigned long length, memory *dev) {
        std::pair<unsigned long, unsigned long> addr_range = std::make_pair(start_addr,start_addr+length);
        if (start_addr % length) return false;
        // check range
        auto it = devices.upper_bound(addr_range);
        if (it != devices.end()) {
            unsigned long l_max = std::max(it->first.first,addr_range.first);
            unsigned long r_min = std::min(it->first.second,addr_range.second);
            if (l_max < r_min) return false; // overleap
        }
        if (it != devices.begin()) {
            it = std::prev(it);
            unsigned long l_max = std::max(it->first.first,addr_range.first);
            unsigned long r_min = std::min(it->first.second,addr_range.second);
            if (l_max < r_min) return false; // overleap
        }
        // overleap check pass
        devices[addr_range] = dev;
        return true;
    }
private:
    uint64_t lr_pa;
    uint64_t lr_size;
    uint64_t lr_hart;
    bool lr_valid = false;
    std::map < std::pair<unsigned long,unsigned long>, memory* > devices;
};

#endif