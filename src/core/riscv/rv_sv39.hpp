#ifndef RV_SV39_HPP
#define RV_SV39_HPP

#include <cstdint>
#include <utility>
#include "rv_common.hpp"
#include "rv_systembus.hpp"
#include "l2_cache.hpp"

struct sv39_tlb_entry {
    uint64_t ppa;
    uint64_t vpa;
    uint16_t asid;
    uint8_t  pagesize; // 0: invalid, 1: 4KB , 2: 2M, 3: 1G
    bool  R; // read
    bool  W; // write
    bool  X; // execute
    bool  U; // user
    bool  G; // global
    bool  A; // access
    bool  D; // dirty
};

uint64_t pa_pc;
extern uint64_t cache_watchpoint;

/*
void myassert(bool exp, uint64_t satp, uint64_t va, const char* msg = "") {
    if (!exp) {
        printf("TLB Assert False. satp=%lx, va=%lx\n",satp,va);
        printf("%s\n",msg);
    }
}
*/
template <unsigned int nr_tlb_entry = 32>
class rv_sv39 {
public:
    rv_sv39(l2_cache<4,2048,64,32> &bus):bus(bus){
        random = 0;
        for (int i=0;i<nr_tlb_entry;i++) tlb[i].pagesize = 0;
    }
    void sfence_vma(uint64_t vaddr, uint64_t asid) {
        for (int i=0;i<nr_tlb_entry;i++) {
            if (tlb[i].asid == asid || asid == 0) {
                if (vaddr == 0) tlb[i].pagesize = 0;
                else {
                    switch (tlb[i].pagesize) {
                        case 1: // 4KB
                            if ((tlb[i].vpa & (-(1ll<<12))) == (vaddr & (-(1ll<<12))) ) tlb[i].pagesize = 0;
                            break;
                        case 2: // 2MB
                            if ((tlb[i].vpa & (-(1ll<<21))) == (vaddr & (-(1ll<<21))) ) tlb[i].pagesize = 0;
                            break;
                        case 3: // 1G
                            if ((tlb[i].vpa & (-(1ll<<30))) == (vaddr & (-(1ll<<30))) ) tlb[i].pagesize = 0;
                        default:
                            break;
                    }
                }
            }
        }
    }
    sv39_tlb_entry* local_tlbe_get(satp_def satp, uint64_t va) {
        sv39_va *va_struct = (sv39_va*)&va;
        assert((va_struct->blank == 0b1111111111111111111111111 && (va_struct->vpn_2 >> 8)) || (va_struct->blank == 0 && ((va_struct->vpn_2 >> 8) == 0)));
        // we should raise access fault before call sv39
        sv39_tlb_entry *res = local_tlb_get(satp,va);
        if (res) {
            if (va == 0x1918c0) {
                if (cache_watchpoint != res->ppa) {
                    cache_watchpoint = res->ppa;
                    printf("\n\nwatch point set to %lx\n\n",cache_watchpoint);
                }
            }
            /*
            sv39_pte pte2;
            uint64_t page_size2;
            bool ptw_result2 = ptw(satp,va,pte2,page_size2);
            myassert(ptw_result2,*((uint64_t*)&satp),va,"ptw ok");
            uint64_t ppa = (((((uint64_t)pte2.PPN2 << 9) | (uint64_t)pte2.PPN1) << 9) | (uint64_t)pte2.PPN0) << 12;
            myassert(ppa == res->ppa,*((uint64_t*)&satp),va,"ppa");
            myassert(satp.asid == res->asid || pte2.G,*((uint64_t*)&satp),va,"asid");
            myassert( (8ll<<(res->pagesize * 9)) == page_size2,*((uint64_t*)&satp),va,"page_size");
            myassert(res->R <= pte2.R,*((uint64_t*)&satp),va,"R");
            if (res->W != pte2.W) printf("error at asid %lx\n",(uint64_t)satp.asid);
            // res->W = pte2.W;
            myassert(res->W <= pte2.W,*((uint64_t*)&satp),va,"W");
            myassert(res->X <= pte2.X,*((uint64_t*)&satp),va,"X");
            myassert(res->U <= pte2.U,*((uint64_t*)&satp),va,"U");
            myassert(res->G == pte2.G,*((uint64_t*)&satp),va,"G");
            myassert(res->A <= pte2.A,*((uint64_t*)&satp),va,"A");
            myassert(res->D <= pte2.D,*((uint64_t*)&satp),va,"D");
             */
            return res;
        }
        // slow path, ptw
        sv39_pte pte;
        uint64_t page_size;
        bool ptw_result = ptw(satp,va,pte,page_size);
        if (!ptw_result) return NULL; // return null when page fault.
        // write back to tlb
        res = &tlb[random];
        random = (random + 1) % nr_tlb_entry;
        res->ppa = (((((uint64_t)pte.PPN2 << 9) | (uint64_t)pte.PPN1) << 9) | (uint64_t)pte.PPN0) << 12;
        res->vpa = (page_size == (1<<12)) ? (va - (va % (1<<12))) : (page_size == (1<<21)) ? (va - (va % (1<<21))) : (va - (va % (1<<30)));
        res->asid = satp.asid;
        res->pagesize = (page_size == (1<<12)) ? 1 : (page_size == (1<<21)) ? 2 : 3;
        res->R = pte.R;
        res->W = pte.W;
        res->X = pte.X;
        res->U = pte.U;
        res->G = pte.G;
        res->A = pte.A;
        res->D = pte.D;
        if (local_tlb_get(satp,va) != res) assert(false);
        return res;
    }
private:
    l2_cache <4,2048,64,32> &bus;
    unsigned int random;
    sv39_tlb_entry tlb[nr_tlb_entry];
    bool ptw(satp_def satp, uint64_t va_in, sv39_pte &pte_out, uint64_t &pagesize) {
        sv39_va *va = (sv39_va*)&va_in;
        if (satp.mode != 8) return false; // mode is not sv39
        uint64_t pt_addr = ((satp.ppn) << 12);
        sv39_pte pte;
        for (int i=2;i>=0;i--) {
            bool res = bus.pa_read_cached(pt_addr+((i==2?va->vpn_2:(i==1?va->vpn_1:va->vpn_0))*sizeof(sv39_pte)),sizeof(sv39_pte),(uint8_t*)&pte);
            if (!res) {
                //printf("pt_addr=%lx, vpn=%lx\n",pt_addr,((i==2?va->vpn_2:(i==1?va->vpn_1:va->vpn_0))));
                //printf("\nerror ptw pa=%lx, level=%d, satp=%lx\n",pt_addr+((i==2?va->vpn_2:(i==1?va->vpn_1:va->vpn_0))*sizeof(sv39_pte)),i,satp);
                return false;
            }
            if (!pte.V || (!pte.R && pte.W) || pte.reserved || pte.PBMT) {
                /*
                printf("\nerror ptw. level = %d, vpn = %d, satp=%lx\n",i,((i==2?va->vpn_2:(i==1?va->vpn_1:va->vpn_0))),satp);
                for (int j=0;j<(1<<9);j++) {
                    uint64_t tmp_pte = 0;
                    bus.pa_read(pt_addr+(j*8),sizeof(sv39_pte),(uint8_t*)&tmp_pte);
                    if (tmp_pte != 0) {
                        printf("but we found pte at vpn %d\n",j);
                    }
                }
                */
                return false;
            }
            if (pte.R || pte.X) { // leaf
                if (i == 2 && (pte.PPN1 || pte.PPN0)) return false; // Make sure that superpage entries trap when PPN LSBs are set.
                if (i == 1 && pte.PPN0) return false; // Make sure that superpage entries trap when PPN LSBs are set.
                pte_out = pte;
                pagesize = (1<<12) << (9*i);
                uint64_t pa = (((((uint64_t)pte.PPN2 << 9) | (uint64_t)pte.PPN1) << 9) | (uint64_t)pte.PPN0) << 12;
                //printf("ptw ok!\n");
                return true;
            }
            else { // valid non-leaf
                pt_addr = (((((uint64_t)pte.PPN2 << 9) | (uint64_t)pte.PPN1) << 9) | (uint64_t)pte.PPN0) << 12;
            }
        }
        return false;
    }
    sv39_tlb_entry* local_tlb_get(satp_def satp, uint64_t va) {
        sv39_tlb_entry* res = NULL;
        for (int i=0;i<nr_tlb_entry;i++) {
            if (tlb[i].asid == satp.asid || tlb[i].G) {
                switch (tlb[i].pagesize) {
                    case 1: // 4KB
                        if ((va & (-(1ll<<12))) == tlb[i].vpa) {
                            assert(res == NULL);
                            res = &tlb[i];
                        }
                        break;
                    case 2: // 2MB
                        if ((va & (-(1ll<<21))) == tlb[i].vpa) {
                            assert(res == NULL);
                            res = &tlb[i];
                        }
                        break;
                    case 3: // 1G
                        if ((va & (-(1ll<<30))) == tlb[i].vpa) {
                            assert(res == NULL);
                            res = &tlb[i];
                        }
                    default:
                        break;
                }
            }
        }
        return res;
    }
};

#endif
