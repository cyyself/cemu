#ifndef MIPS_MMU
#define MIPS_MMU

#include "memory_bus.hpp"
#include "mips_common.hpp"
#include <cstdint>

template <uint32_t nr_tlb_entry = 8>
class mips_mmu {
public:
    mips_mmu(memory_bus &bus):bus(bus) {

    }
    // Only mask high 3 bit to translate from va to pa.
    // TODO: impl TLB and virtual address space segments
    mips32_exccode va_if(uint32_t addr, uint8_t *buffer, mips32_ksu mode) {
        uint32_t pa = addr & 0x1fffffff;
        if (pa % 4 != 0) return EXC_ADEL;
        if (!bus.do_read(pa, 4, buffer)) return EXC_IBE;
        return EXC_OK;
    }
    // size should be 1, 2, 4
    mips32_exccode va_read(uint32_t addr, uint32_t size, uint8_t *buffer, mips32_ksu mode) {
        uint32_t pa = addr & 0x1fffffff;
        if (pa % size != 0) return EXC_ADEL;
        if (!bus.do_read(pa, size, buffer)) return EXC_DBE;
        return EXC_OK;
    }
    mips32_exccode va_write(uint32_t addr, uint32_t size, const uint8_t *buffer, mips32_ksu mode) {
        uint32_t pa = addr & 0x1fffffff;
        if (pa % size != 0) return EXC_ADES;
        if (!bus.do_write(pa, size, buffer)) return EXC_DBE;
        return EXC_OK;
    }
private:
    memory_bus &bus;
};

#endif