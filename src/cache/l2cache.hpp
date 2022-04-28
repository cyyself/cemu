#ifndef L2_CACHE_HPP
#define L2_CACHE_HPP

#include "memory.hpp"
#include <map>
#include <utility>
#include <cstdint>
#include <climits>
#include <bitset>
#include <assert.h>
#include <random>
#include <vector>
#include "co_slave.hpp"


enum l2_line_status {
    L2_INVALID, L2_OWNED, L2_SHARED, L2_SLAVE_EXCLUSIVE
};

template <int nr_ways = 4, int sz_cache_line = 64, int nr_sets = 2048, int nr_max_slave = 32>
struct l2cache_set {
    uint64_t tag[nr_ways];
    uint8_t data[nr_ways][sz_cache_line];
    l2_line_status status[nr_ways];
    std::bitset <nr_max_slave> shared_slave[nr_ways];
    std::bitset <nr_ways> dirty;
    l2cache_set() {
        for (int i=0;i<nr_ways;i++) status[i] = L2_INVALID;
    }
    bool match(uint64_t addr, int &hit_way_id) {
        uint64_t search_tag = addr / sz_cache_line / nr_sets;
        for (int i=0;i<nr_ways;i++) {
            if (tag[i] == search_tag && status[i] != L2_INVALID) {
                hit_way_id = i;
                return true;
            }
        }
        return false;
    }
};

template <int nr_ways = 4, int nr_sets = 2048, int sz_cache_line = 64, int nr_max_slave = 32>
class l2_cache {
public:
    // Uncached operations {
    /*
        Warn: uncached operation will not check existing cache status, 
        only use it for MMIO and l2 itself to read and write memory, 
        and should not be used by cached slave.
     */
    bool pa_read_uncached(uint64_t start_addr, uint64_t size, uint8_t *buffer) {
        auto it = devices.upper_bound(std::make_pair(start_addr,ULONG_MAX));
        if (it == devices.begin()) return false;
        it = std::prev(it);
        uint64_t end_addr = start_addr + size;
        if (it->first.first <= start_addr && end_addr <= it->first.second) {
            uint64_t dev_size = it->first.second - it->first.first;
            return it->second->do_read(start_addr % dev_size, size, buffer);
        }
        else return false;
    }
    bool pa_write_uncached(uint64_t start_addr, uint64_t size, const uint8_t *buffer) {
        auto it = devices.upper_bound(std::make_pair(start_addr,ULONG_MAX));
        if (it == devices.begin()) return false;
        it = std::prev(it);
        uint64_t end_addr = start_addr + size;
        if (it->first.first <= start_addr && end_addr <= it->first.second) {
            uint64_t dev_size = it->first.second - it->first.first;
            return it->second->do_write(start_addr % dev_size, size, buffer);
        }
        else return false;
    }
    // Uncached operations }
    // Cached and coherence operations {
    void acquire_exclusive(uint64_t start_addr, int slave_id) {
        // slave should already acquired shared
        l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> *select_set = &set_data[get_index(start_addr)];
        int way_id;
        assert(select_set->match(start_addr, way_id));
        assert(select_set->shared_slave[way_id][slave_id]);
        for (int i=0;i<slaves.size();i++) if (i != slave_id && select_set->shared_slave[way_id][i]) {
            slaves[i]->invalidate_shared(start_addr);
            select_set->shared_slave[way_id].reset(i);
        }
        assert(select_set->shared_slave[way_id].count() == 1);
        select_set->status = L2_SLAVE_EXCLUSIVE;
    }
    void release_shared(uint64_t start_addr, int slave_id) {
        // slave should already acquired shared
        // This happends when l1d replace.
        l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> *select_set = &set_data[get_index(start_addr)];
        int way_id;
        assert(select_set->match(start_addr, way_id));
        assert(select_set->shared_slave[way_id][slave_id]);
        select_set->shared_slave[way_id].reset(slave_id);
    }
    
    bool cache_line_fetch(uint64_t start_addr, uint8_t *buffer, int slave_id) {
        // after this, the slave got shared status
        assert(start_addr % sz_cache_line == 0);
        if (!l2_include(start_addr)) return false;
        l2_return_to_shared(start_addr);
        l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> *select_set = &set_data[get_index(start_addr)];
        int way_id;
        assert(select_set->match(start_addr, way_id));
        select_set->shared_slave.set(slave_id);
        memcpy(buffer,select_set->data[way_id],sz_cache_line);
        return true;
    }
    void cache_line_writeback(uint64_t start_addr, const uint8_t *buffer, int slave_id) { 
        /*
            The slave should got exclusive beforce call this function, 
            after this, slave status go to shared.
            This happens when another slave request from address which 
            this slave got exclusive or l1 d cache replace.
            If a slave wants to write back and invalidate, it should 
            call release_shared function.
         */
        assert(start_addr % sz_cache_line == 0);
        l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> *select_set = &set_data[get_index(start_addr)];
        int way_id;
        assert(select_set->match(start_addr, way_id));
        assert(select_set->status[way_id] == L2_SLAVE_EXCLUSIVE);
        assert(select_set->shared_slave[way_id][slave_id]);
        memcpy(select_set->data[way_id],buffer,sz_cache_line);
        select_set->dirty.set(way_id);
        select_set->status[way_id] = L2_SHARED;
    }
    // Cached and coherence operations }
    // Cached and non-coherence operations {
    /*
        Warn: Cached operations is not permit to cross cache line boundary, 
        is used for dma operations to operate l2 directly and icache fetch 
        with explicit fence.i.
     */
    bool pa_read_cached(uint64_t start_addr, uint64_t size, uint8_t *buffer) {
        assert(sz_cache_line - (start_addr) % sz_cache_line >= size);
        if (!l2_include(start_addr)) return false;
        l2_return_to_shared(start_addr);
        l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> *select_set = &set_data[get_index(start_addr)];
        int way_id;
        assert(select_set->match(start_addr, way_id));
        memcpy(buffer,&(select_set->data[way_id][start_addr%sz_cache_line]),size);
        return true;
    }
    bool pa_write_cached(uint64_t start_addr, uint64_t size, const uint8_t *buffer) {
        assert(sz_cache_line - (start_addr) % sz_cache_line >= size);
        if (!l2_include(start_addr)) return false;
        l2_return_to_shared(start_addr);
        l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> *select_set = &set_data[get_index(start_addr)];
        int way_id;
        assert(select_set->match(start_addr, way_id));
        select_set->dirty.set(way_id);
        memcpy(&(select_set->data[way_id][start_addr%sz_cache_line],buffer,size));
        return true;
    }
    // Cached and non-coherence operations }
    bool add_dev(uint64_t start_addr, uint64_t length, memory *dev) {
        std::pair<uint64_t, uint64_t> addr_range = std::make_pair(start_addr,start_addr+length);
        if (start_addr % length) return false;
        // check range
        auto it = devices.upper_bound(addr_range);
        if (it != devices.end()) {
            uint64_t l_max = std::max(it->first.first,addr_range.first);
            uint64_t r_min = std::min(it->first.second,addr_range.second);
            if (l_max < r_min) return false; // overleap
        }
        if (it != devices.begin()) {
            it = std::prev(it);
            uint64_t l_max = std::max(it->first.first,addr_range.first);
            uint64_t r_min = std::min(it->first.second,addr_range.second);
            if (l_max < r_min) return false; // overleap
        }
        // overleap check pass
        devices[addr_range] = dev;
        return true;
    }
    int register_slave(co_slave *slave) {
        int res = static_cast<int>(slaves.size());
        slaves.push_back(slave);
        return res;
    }
private:
    std::map < std::pair<uint64_t ,uint64_t>, memory* > devices;
    l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> set_data[nr_sets];
    std::vector <co_slave*> slaves;
    std::mt19937 rng();

    void l2_return_to_shared(uint64_t addr) {
        l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> *select_set = &set_data[get_index(addr)];
        int way_id;
        if (!(select_set->match(addr, way_id))) {
            return;
        }
        if (select_set->status == L2_SLAVE_EXCLUSIVE) {
            for (int i=0;i<slaves.size();i++) if (select_set->shared_slave[way_id][i]) {
                slaves[i]->invalidate_exclusive(addr);
            }
            select_set->status[way_id] = L2_SHARED;
        }
    }
    void l2_invalidate(uint64_t addr) { // invalite some line from l2
        l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> *select_set = &set_data[get_index(addr)];
        int way_id;
        if (!(select_set->match(addr, way_id))) {
            return;
        }
        switch (select_set->status) {
            case L2_SLAVE_EXCLUSIVE: case L2_SHARED: {
                for (int i=0;i<slaves.size();i++) if (select_set->shared_slave[way_id][i]) {
                    slaves[i]->invalidate_shared(addr);
                    select_set->shared_slave[way_id].reset();
                }
                select_set->status[way_id] = L2_OWNED;
            } // We don't need break as we need the same operations
            case L2_OWNED: {
                // check if we need write it back to dram
                if (select_set->dirty[way_id]) {
                    uint64_t addr_align = (addr / sz_cache_line) * sz_cache_line;
                    assert(pa_write_uncached(addr_align,sz_cache_line,select_set->data[way_id]));
                    select_set->dirty.reset();
                }
                select_set->status[way_id] = L2_INVALID;
            }
        }
    }
    bool l2_include(uint64_t addr) { // fetch line from dram
        l2cache_set <nr_ways, sz_cache_line, nr_sets, nr_max_slave> *select_set = &set_data[get_index(addr)];
        int way_id;
        if (!(select_set->match(addr, way_id))) {
            way_id = rng() % nr_ways;
            if (select_set->status[way_id] != L2_INVALID) {
                l2_invalidate( (select_set->tag[way_id] * nr_sets * sz_cache_line) | (get_index(addr) * sz_cache_line) );
                assert(select_set->status[way_id] == L2_INVALID);
            }
            bool res = pa_read_uncached(addr / sz_cache_line * sz_cache_line, sz_cache_line, select_set->data[way_id]);
            if (!res) return false;
            select_set->tag[way_id] = get_tag(addr);
            select_set->status[way_id] = L2_OWNED;
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
};


#endif