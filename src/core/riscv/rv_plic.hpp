#ifndef RV_PLIC_HPP
#define RV_PLIC_HPP

#include "memory.hpp"
#include <cstring>

class rv_plic : public memory {
public:
    bool do_read(unsigned long start_addr, unsigned long size, unsigned char* buffer) {
        memset(buffer,0,size);
        return true;
    }
    bool do_write(unsigned long start_addr, unsigned long size, const unsigned char* buffer) {
        return true;
    }
};

#endif