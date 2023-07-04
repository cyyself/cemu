#ifndef CEMU_LIB
#define CEMU_LIB

#include <stdint.h>
#include <stdbool.h>

extern "C" {
    void cemu_main(const char* load_path);
    void cemu_mmio(uint64_t addr, void *buf, uint64_t len, bool is_write);
    void cemu_dma(uint64_t addr, void *buf, uint64_t len, bool is_write);
}

#endif