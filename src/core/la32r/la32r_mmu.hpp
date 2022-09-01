#ifndef LA32R_MMU
#define LA32R_MMU

#include "../../memory/memory_bus.hpp"
#include "la32r_common.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>

template<int nr_tlb_entry = 32>
class la32r_mmu {
public:
    la32r_mmu(memory_bus &bus) : bus(bus) {
        reset();
    }

    void reset() {
        memset(tlb, 0, sizeof(tlb));
        memset(dmw, 0, sizeof(dmw));
    }

    la32r_exccode va_if(uint32_t addr, uint8_t *buffer, la32r_plv cur_plv, bool map, uint8_t asid) {
        if (addr % 4 != 0) {
            return std::make_pair(ADE, ADEF_SUBCODE);
        }
        if (!map) {
            assert(bus.do_read(addr, 4, buffer));
            return std::make_pair(OK, 0);
        }
        auto dmwe = dmw_match(addr, cur_plv);
        if (dmwe != nullptr) {
            assert(bus.do_read((dmwe->pseg << 29) | (addr & 0x1fffffffu), 4, buffer));
            return std::make_pair(OK, 0);
        }
        if (cur_plv == plv3 && addr >= 0x80000000u) {
            return std::make_pair(ADE, ADEF_SUBCODE);
        }
        bool dirty;
        bool to_refill;
        uint32_t pa;
        la32r_mat mat;
        la32r_plv plv;
        if (!page_trans(addr, asid, dirty, to_refill, pa, mat, plv)) {
            return std::make_pair(to_refill ? TLBR : PIF, 0);
        }
        if (cur_plv > plv) {
            return std::make_pair(PPI, 0);
        }
        assert(bus.do_read(pa, 4, buffer));
        return std::make_pair(OK, 0);
    }

    la32r_exccode va_read(uint32_t addr, uint32_t size, uint8_t *buffer, la32r_plv cur_plv, bool map, uint8_t asid) {
        if (addr % size != 0) {
            return std::make_pair(ALE, 0);
        }
        if (!map) {
            assert(bus.do_read(addr, size, buffer));
            return std::make_pair(OK, 0);
        }
        auto dmwe = dmw_match(addr, cur_plv);
        if (dmwe != nullptr) {
            assert(bus.do_read((dmwe->pseg << 29) | (addr & 0x1fffffffu), size, buffer));
            return std::make_pair(OK, 0);
        }
        if (cur_plv == plv3 && addr >= 0x80000000u) {
            return std::make_pair(ADE, ADEM_SUBCODE);
        }
        bool dirty;
        bool to_refill;
        uint32_t pa;
        la32r_mat mat;
        la32r_plv plv;
        if (!page_trans(addr, asid, dirty, to_refill, pa, mat, plv)) {
            return std::make_pair(to_refill ? TLBR : PIL, 0);
        }
        if (cur_plv > plv) {
            return std::make_pair(PPI, 0);
        }
        assert(bus.do_read(pa, size, buffer));
        return std::make_pair(OK, 0);
    }

    la32r_exccode va_write(uint32_t addr, uint32_t size, uint8_t *buffer, la32r_plv cur_plv, bool map, uint8_t asid) {
        if (addr % size == 0) {
            return std::make_pair(ALE, 0);
        }
        if (!map) {
            assert(bus.do_write(addr, size, buffer));
            return std::make_pair(OK, 0);
        }
        auto dwme = dmw_match(addr, cur_plv);
        if (dwme != nullptr) {
            assert(bus.do_write((dmwe->pseg << 29) | (addr & 0x1fffffffu), size, buffer));
            return std::make_pair(OK, 0);
        }
        if (cur_plv == plv3 && addr >= 0x80000000u) {
            return std::make_pair(ADE, ADEM_SUBCODE);
        }
        bool dirty;
        bool to_refill;
        uint32_t pa;
        la32r_mat mat;
        la32r_plv plv;
        if (!page_trans(addr, asid, dirty, to_refill, pa, mat, plv)) {
            return std::make_pair(to_refill ? TLBR : PIS, 0);
        }
        if (cur_plv > plv) {
            return std::make_pair(PPI, 0);
        }
        if (!dirty) {
            return std::make_pair(PME, 0);
        }
        assert(bus.do_write(pa, size, buffer));
        return std::make_pair(OK, 0);
    }

    bool tlb_search(uint32_t va, uint8_t asid, uint8_t &index) {
        la32r_tlb *tlbe = tlb_match(va, asid);
        if (tlbe == nullptr) {
            return false;
        }
        index = tlbe - tlb;
        return true;
    }

    la32r_tlb tlb_rd(uint8_t index) {
        assert(index < nr_tlb_entry);
        return tlb[index];
    }

    void tlb_wr(la32r_tlb tlbe, uint8_t index) {
        assert(index < nr_tlb_entry);
        tlb[index] = tlbe;
    }

    void tlb_inv(bool (*pred)(const la32r_tlb &)) {
        for (int i = 0; i < nr_tlb_entry; i++) {
            if (pred(tlb[i])) {
                tlb[i] = 0;
            }
        }
    }

    void dmw_fill(la32r_dmw dmwe, uint8_t index) {
        dmw[index] = dmwe;
    }

private:
    bool page_trans(uint32_t va, uint8_t asid,
                    bool &dirty, bool &to_refill, uint32_t &pa, la32r_mat &mat, la32r_plv &plv) {
        to_refill = false;
        auto tlbe = tlb_match(va, asid);
        if (tlbe == nullptr) {
            to_refill = true;
            return false;
        }
        uint32_t offset = va & ((1 << tlbe->ps) - 1);
        if (((va & (1 << tlbe->ps)) == 0) && tlbe->v0) {
            dirty = tlbe->d0;
            mat = tlbe->mat0;
            plv = tlbe->plv0;
            pa = (tlbe->ppn0 << tlbe->ps) | offset;
            return true;
        } else if (((va & (1 << tlbe->ps)) == 1) && tlbe->v1) {
            dirty = tlbe->d1;
            mat = tlbe->mat1;
            plv = tlbe->plv1;
            pa = (tlbe->ppn1 << tlbe->ps) | offset;
            return true;
        }
        return false;
    }

    la32r_tlb *tlb_match(uint32_t va, uint8_t asid) {
        for (int i = 0; i < nr_tlb_entry; i++) {
            if (tlb[i].e
                && (asid == tlb[i].asid || tlb[i].g)
                && (tlb[i].vppn >> (tlb[i].ps - 12)) == (va >> (tlb[i].ps + 1))
                    ) {
                return &tlb[i];
            }
        }
        return nullptr;
    }

    la32r_dmw *dmw_match(uint32_t va, la32r_plv cur_plv) {
        for (int i = 0; i < sizeof(dmw) / sizeof(la32r_dmw); i++) {
            if ((dmw[i] & (1 << cur_plv)) && (va >> 30) == dmw[i].vseg) {
                return &dmw[i];
            }
        }
        return nullptr;
    }

    memory_bus &bus;
    la32r_tlb tlb[nr_tlb_entry];
    la32r_dmw dmw[2];
};

#endif // LA32R_MMU
