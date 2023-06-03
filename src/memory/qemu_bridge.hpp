#ifndef QEMU_BRIDGE
#define QEMU_BRIDGE

#include "mmio_dev.hpp"
#include "cemu_lib.h"

class qemu_bridge : public mmio_dev {
public:
    bool do_read(uint64_t start_addr, uint64_t size, unsigned char* buffer) {
        cemu_mmio(start_addr, buffer, size, 0);
    }
    bool do_write(uint64_t start_addr, uint64_t size, const unsigned char* buffer) {
        cemu_mmio(start_addr, (unsigned char*)buffer, size, 1);
    }
};

#endif