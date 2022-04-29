#ifndef L1_I_CACHE
#define L1_I_CACHE

#include <cstdint>
#include <bitset>
#include "tree_plru.hpp"
#include "l2_cache.hpp"

template <int nr_ways = 4, int sz_cache_line = 64, int nr_sets = 64>
struct l1_i_cache_set {
    uint64_t tag[nr_ways];
    uint8_t data[nr_ways][sz_cache_line];
    tree_plru <4> replace;
    std::bitset <nr_ways> valid;
    bool match(uint64_t addr, int &hit_way_id) {
        uint64_t search_tag = addr / sz_cache_line / nr_sets;
        for (int i=0;i<nr_ways;i++) {
            if (tag[i] == search_tag && valid[i]) {
                hit_way_id = i;
                return true;
            }
        }
        return false;
    }
};

template <int nr_ways = 4, int sz_cache_line = 64, int nr_sets = 64>
class l1_i_cache {
    static_assert(__builtin_popcount(nr_ways) == 1);
    static_assert(__builtin_popcount(nr_sets) == 1);
    static_assert(__builtin_popcount(sz_cache_line) == 1);
public:
    l1_i_cache(l2_cache <4, 2048, 64, 32> *l2_cache) {
        l2 = l2_cache;
    }
    void fence_i() {
        for (int i=0;i<nr_sets;i++) set_data[i].valid.reset();
    }
    bool pa_if(uint64_t start_addr, uint64_t size, uint8_t *buffer) {
        // For C Extension, 2 bytes buffer should be added to core to prevent request cross cacheline.
        if (!l1_include(start_addr)) return false;
        l1_i_cache_set <nr_ways, sz_cache_line, nr_sets> *select_set = &set_data[get_index(start_addr)];
        int way_id;
        assert(select_set->match(start_addr, way_id));
        select_set->replace.mark_used(way_id);
        memcpy(buffer,&(select_set->data[way_id][start_addr%sz_cache_line]),size);
        return true;
    }
private:
    bool l1_include(uint64_t addr) {
        l1_i_cache_set <nr_ways, sz_cache_line, nr_sets> *select_set = &set_data[get_index(addr)];
        int way_id;
        if (!select_set->match(addr, way_id)) {
            way_id = select_set->replace.get_replace();
            select_set->valid.reset(way_id);
            uint64_t req_addr = addr % sz_cache_line;
            if (addr < 0x80000000) { // for MMIO region, we also cached instruction to speedup.
                for (int i=0;i<(sz_cache_line/8);i++) {
                    bool res = l2->pa_read_uncached(req_addr + (i * 8), 8, &(select_set->data[way_id][i * 8]));
                    if (!res) return false;
                }
            }
            else {
                bool res = l2->pa_read_cached(req_addr, sz_cache_line, select_set->data[way_id]);
                if (!res) return false;
            }
            select_set->valid.set(way_id);
            return true;
        }
        else return true;
    }
    uint64_t get_index(uint64_t addr) {
        return (addr / sz_cache_line) % nr_sets;
    }
    uint64_t get_tag(uint64_t addr) {
        return addr / sz_cache_line / nr_sets;
    }
    l1_i_cache_set <nr_ways, sz_cache_line, nr_sets> set_data[nr_sets];
    l2_cache <4, 2048, 64, 32> *l2;
};

#endif