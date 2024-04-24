#ifndef RV_ACLINT_HPP
#define RV_ACLINT_HPP

#include "mmio_dev.hpp"
#include <cstdint>
#include "rv_clint.hpp"

template <unsigned int nr_hart=1>
class rv_mtime : public mmio_dev {
public:
    rv_mtime(rv_clint<nr_hart> &clint):clint(clint) {

    }
    bool do_read(uint64_t start_addr, uint64_t size, char* buffer) {
        return clint.do_read(start_addr+0xbff8,size,buffer);
    }
    bool do_write(uint64_t start_addr, uint64_t size, const char* buffer) {
        return clint.do_write(start_addr+0xbff8,size,buffer);
    }
private:
    rv_clint<nr_hart> &clint;
};

template <unsigned int nr_hart=1>
class rv_mtimecmp : public mmio_dev {
public:
    rv_mtimecmp(rv_clint<nr_hart> &clint):clint(clint) {

    }
    bool do_read(uint64_t start_addr, uint64_t size, char* buffer) {
        return clint.do_read(start_addr+0x4000,size,buffer);
    }
    bool do_write(uint64_t start_addr, uint64_t size, const char* buffer) {
        return clint.do_write(start_addr+0x4000,size,buffer);
    }
private:
    rv_clint<nr_hart> &clint;
};

template <unsigned int nr_hart=1>
class rv_mswi : public mmio_dev {
public:
    rv_mswi(rv_clint<nr_hart> &clint):clint(clint) {

    }
    bool do_read(uint64_t start_addr, uint64_t size, char* buffer) {
        return clint.do_read(start_addr,size,buffer);
    }
    bool do_write(uint64_t start_addr, uint64_t size, const char* buffer) {
        return clint.do_write(start_addr,size,buffer);
    }
private:
    rv_clint<nr_hart> &clint;
};

#endif