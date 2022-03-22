#ifndef XBAR_HPP
#define XBAR_HPP

#include "memory.hpp"
#include <map>
#include <utility>
#include <climits>

class memory_bus : public memory {
public:
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
    bool do_read(unsigned long start_addr, unsigned long size, unsigned char* buffer) {
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
    bool do_write(unsigned long start_addr, unsigned long size, const unsigned char* buffer) {
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
private:
    std::map < std::pair<unsigned long,unsigned long>, memory* > devices;
};

#endif