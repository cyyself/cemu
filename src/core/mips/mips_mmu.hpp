#ifndef MIPS_MMU
#define MIPS_MMU

#include "memory_bus.hpp"
#include "mips_common.hpp"
#include <cstdint>

template <int nr_tlb_entry = 8>
class mips_mmu {
public:
    mips_mmu(memory_bus &bus):bus(bus) {
        reset();
    }
    void reset() {
        memset(tlb,0,sizeof(tlb));
    }
    // Only mask high 3 bit to translate from va to pa.
    // TODO: impl TLB and virtual address space segments
    mips32_exccode va_if(uint32_t addr, uint8_t *buffer, mips32_ksu mode, uint8_t asid, bool &tlb_invalid) {
        tlb_invalid = false;
        if ((mode == USER_MODE && addr >= 0x80000000u) || addr % 4 != 0) return EXC_ADEL;
        else {
            bool dirty;
            bool to_refill;
            uint32_t pa = 0;
            bool trans_res = translation(addr, asid, dirty, to_refill, pa);
            if (!trans_res) {
                if (!to_refill) tlb_invalid = true;
                return EXC_TLBL;
            }
            else {
                if (!bus.do_read(pa, 4, buffer)) return EXC_IBE;
                else return EXC_OK;
            }
        }
    }
    // size should be 1, 2, 4
    mips32_exccode va_read(uint32_t addr, uint32_t size, uint8_t *buffer, mips32_ksu mode, uint8_t asid, bool &tlb_invalid) {
        tlb_invalid = false;
        if ((mode == USER_MODE && addr >= 0x80000000u) || addr % size != 0) return EXC_ADEL;
        else {
            bool dirty;
            bool to_refill;
            uint32_t pa = 0;
            bool trans_res = translation(addr, asid, dirty, to_refill, pa);
            if (!trans_res) {
                if (!to_refill) tlb_invalid = true;
                return EXC_TLBL;
            }
            else {
                if (!bus.do_read(pa, size, buffer)) return EXC_DBE;
                else return EXC_OK;
            }
        }
    }
    mips32_exccode va_write(uint32_t addr, uint32_t size, const uint8_t *buffer, mips32_ksu mode, uint8_t asid, bool &tlb_invalid) {
        tlb_invalid = false;
        if ((mode == USER_MODE && addr >= 0x80000000u) || addr % size != 0) return EXC_ADES;
        else {
            bool dirty;
            bool to_refill;
            uint32_t pa = 0;
            bool trans_res = translation(addr, asid, dirty, to_refill, pa);
            if (!trans_res) {
                if (!to_refill) tlb_invalid = true;
                return EXC_TLBS;
            }
            else {
                if (!dirty) return EXC_MOD;
                if (!bus.do_write(pa, size, buffer)) return EXC_DBE;
                else return EXC_OK;
            }
        }
    }
    bool probe_index(uint8_t &index, uint32_t va, uint8_t asid) {
        mips_tlb *tlbe = tlb_match(va, asid);
        if (tlbe == NULL) return false;
        index = tlbe - tlb;
        return true;
    }
    mips_tlb get_tlb(uint8_t idx) {
        assert(idx < nr_tlb_entry);
        return tlb[idx];
    }
    void tlbw(mips_tlb tlb_entry, uint8_t idx) {
        assert(idx < nr_tlb_entry);
        tlb[idx] = tlb_entry;
    }
private:
    // don't care CCA
    bool translation(uint32_t va, uint8_t asid, bool &dirty, bool &to_refill, uint32_t &pa) {
        to_refill = false;
        if (va >= 0x80000000u && va <= 0xbfffffffu) {
            dirty = true;
            pa = va & 0x1fffffffu;
            return true;
        }
        else {
            mips_tlb *tlbe = tlb_match(va, asid);
            if (!tlbe) {
                to_refill = true;
                return false;
            }
            if (((va >> 12) & 1) && tlbe->V1) {
                dirty = tlbe->D1;
                pa = (va & 0xfff) | (tlbe->PFN1 << 12);
                // printf("pa ok! pa=%x\n", pa);
                return true;
            }
            else if (((va >> 12) & 1) == 0 && tlbe->V0) {
                dirty = tlbe->D0;
                pa = (va & 0xfff) | (tlbe->PFN0 << 12);
                // printf("pa ok! pa=%x\n", pa);
                return true;
            }
            else {
                // printf("tlblo err\n");
                return false;
            }
        }
    }
    mips_tlb* tlb_match(uint32_t va, uint8_t asid) {
        // printf("tlb match %x\n",va);
        for (int i=0;i<nr_tlb_entry;i++) {
            if ((asid == tlb[i].ASID || tlb[i].G) && tlb[i].VPN2 == (va >> 13)) {
                // printf("tlb found!\n");
                return &tlb[i];
            }
        }
        return NULL;
    }
    memory_bus &bus;
    mips_tlb tlb[nr_tlb_entry];
};

#endif