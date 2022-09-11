#ifndef LA32R_CSR
#define LA32R_CSR

#include "la32r_mmu.hpp"

template<int nr_tlb_entry = 32>
class la32r_csr {
public:
    la32r_csr(uint32_t core_id, uint32_t &pc, la32r_mmu<nr_tlb_entry> &mmu) : core_id(core_id), pc(pc), mmu(mmu) {
        reset();
    }

    void reset() {
        crmd = 0;
        prmd = 0;
        euen = 0;
        ecfg = 0;
        estat = 0;
        era = 0;
        badv = 0;
        eentry = 0;
        tlbidx = 0;
        tlbehi = 0;
        tlbelo0 = 0;
        tlbelo1 = 0;
        asid = 0;
        pgdl = 0;
        pgdh = 0;
        cpuid = 0;
        save0 = 0;
        save1 = 0;
        save2 = 0;
        save3 = 0;
        tid = 0;
        tcfg = 0;
        tval = 0;
        llbctl = 0;
        tlbrentry = 0;
        ctag = 0;
        dmw0 = 0;
        dmw1 = 0;
        ((csr_crmd *) &crmd)->da = 1;
        ((csr_asid *) &asid)->asidbits = 10;
        ((csr_cpuid *) &cpuid)->core_id = core_id;

        random = nr_tlb_entry - 1;
        cur_need_trap = false;
        trap_pc = 0;
        timer_en = false;
    }

    uint32_t read(uint32_t index) {
        switch (index) {
        case CRMD:
            return crmd;
        case PRMD:
            return prmd;
        case EUEN:
            return euen;
        case ECFG:
            return ecfg;
        case ESTAT:
            return estat;
        case ERA:
            return era;
        case BADV:
            return badv;
        case EENTRY:
            return eentry;
        case TLBIDX:
            return tlbidx;
        case TLBEHI:
            return tlbehi;
        case TLBELO0:
            return tlbelo0;
        case TLBELO1:
            return tlbelo1;
        case ASID:
            return asid;
        case PGDL:
            return pgdl;
        case PGDH:
            return pgdh;
        case PGD:
            return ((badv >> 31) == 0) ? pgdl : pgdh;
        case CPUID:
            return cpuid;
        case SAVE0:
            return save0;
        case SAVE1:
            return save1;
        case SAVE2:
            return save2;
        case SAVE3:
            return save3;
        case TID:
            return tid;
        case TCFG:
            return tcfg;
        case TVAL:
            return tval;
        case TICLR:
            return 0;
        case LLBCTL:
            return llbctl;
        case TLBRENTRY:
            return tlbrentry;
        case CTAG:
            return ctag;
        case DMW0:
            return dmw0;
        case DMW1:
            return dmw1;
        default:
            return 0;
        }
    }

    void write(uint32_t index, uint32_t value) {
        switch (index) {
        case CRMD: {
            auto old_val = (csr_crmd *) &crmd;
            auto new_val = (csr_crmd *) &value;
            old_val->plv = new_val->plv;
            old_val->ie = new_val->ie;
            old_val->da = new_val->da;
            old_val->pg = new_val->pg;
            old_val->datf = new_val->datf;
            old_val->datm = new_val->datm;
            break;
        }
        case PRMD: {
            auto old_val = (csr_prmd *) &prmd;
            auto new_val = (csr_prmd *) &value;
            old_val->pplv = new_val->pplv;
            old_val->pie = new_val->pie;
            break;
        }
        case EUEN: {
            auto old_val = (csr_euen *) &euen;
            auto new_val = (csr_euen *) &value;
            old_val->fpe = new_val->fpe;
            break;
        }
        case ECFG: {
            auto old_val = (csr_ecfg *) &ecfg;
            auto new_val = (csr_ecfg *) &value;
            old_val->lie = new_val->lie;
            break;
        }
        case ESTAT: {
            auto old_val = (csr_estat *) &estat;
            auto new_val = (csr_estat *) &value;
            old_val->is = (old_val->is & ~0b11) | (new_val->is & 0b11); // only is[1:0] are writable
            break;
        }
        case ERA: {
            era = value;
            break;
        }
        case BADV: {
            badv = value;
            break;
        }
        case EENTRY: {
            auto old_val = (csr_eentry *) &eentry;
            auto new_val = (csr_eentry *) &value;
            old_val->va = new_val->va;
            break;
        }
        case TLBIDX: {
            auto old_val = (csr_tlbidx *) &tlbidx;
            auto new_val = (csr_tlbidx *) &value;
            old_val->index = new_val->index;
            old_val->ps = new_val->ps;
            old_val->ne = new_val->ne;
            break;
        }
        case TLBEHI: {
            auto old_val = (csr_tlbehi *) &tlbehi;
            auto new_val = (csr_tlbehi *) &value;
            old_val->vppn = new_val->vppn;
            break;
        }
        case TLBELO0:
        case TLBELO1: {
            auto old_val = (csr_tlbelo *) ((index == TLBELO0) ? &tlbelo0 : &tlbelo1);
            auto new_val = (csr_tlbelo *) &value;
            old_val->v = new_val->v;
            old_val->d = new_val->d;
            old_val->plv = new_val->plv;
            old_val->mat = new_val->mat;
            old_val->g = new_val->g;
            old_val->ppn = new_val->ppn;
            break;
        }
        case ASID: {
            auto old_val = (csr_asid *) &asid;
            auto new_val = (csr_asid *) &value;
            old_val->asid = new_val->asid;
            break;
        }
        case PGDL:
        case PGDH: {
            auto old_val = (csr_pgd *) ((index == PGDL) ? &pgdl : &pgdh);
            auto new_val = (csr_pgd *) &value;
            old_val->base = new_val->base;
            break;
        }
        case SAVE0:
        case SAVE1:
        case SAVE2:
        case SAVE3: {
            auto old_val = save_of(index);
            assert(old_val != nullptr);
            *old_val = value;
            break;
        }
        case TID: {
            tid = value;
            break;
        }
        case TCFG: {
            auto old_val = (csr_tcfg *) &tcfg;
            auto new_val = (csr_tcfg *) &value;
            old_val->en = new_val->en;
            old_val->periodic = new_val->periodic;
            old_val->init_val = new_val->init_val;
            tval = new_val->init_val << 2;
            timer_en = new_val->en == 1;
            break;
        }
        case TICLR: {
            if (value & 0b1) {
                ((csr_estat *) &estat)->is &= ~(1 << 11);
            }
            break;
        }
        case LLBCTL: {
            auto old_val = (csr_llbctl *) &llbctl;
            auto new_val = (csr_llbctl *) &value;
            old_val->klo = new_val->klo;
            if (new_val->wcllb) {
                old_val->rollb = 0;
            }
        }
        case TLBRENTRY: {
            auto old_val = (csr_tlbrentry *) &tlbrentry;
            auto new_val = (csr_tlbrentry *) &value;
            old_val->pa = new_val->pa;
            break;
        }
        case DMW0:
        case DMW1: {
            auto old_val = (csr_dmw *) ((index == DMW0) ? &dmw0 : &dmw1);
            auto new_val = (csr_dmw *) &value;
            old_val->plv0 = new_val->plv0;
            old_val->plv3 = new_val->plv3;
            old_val->mat = new_val->mat;
            old_val->pseg = new_val->pseg;
            old_val->vseg = new_val->vseg;
            mmu.dmw_fill(la32r_dmw{
                    new_val->plv0,
                    0,
                    0,
                    new_val->plv3,
                    new_val->mat,
                    new_val->pseg,
                    new_val->vseg
            }, (index == DMW0) ? 0 : 1);
            break;
        }
        default:
            break;
        }
    }

    void pre_exec(unsigned int ext_int) {
        cur_need_trap = false;
        auto tcfg_reg = (csr_tcfg *) &tcfg;
        auto estat_reg = (csr_estat *) &estat;
        auto ecfg_reg = (csr_ecfg *) &ecfg;
        if (timer_en) {
            if (tval == 0) {
                ((csr_estat *) &estat)->is |= (1 << 11);
                timer_en = tcfg_reg->periodic;
                if (tcfg_reg->periodic) {
                    tval = tcfg_reg->init_val << 2;
                } else {
                    tval = 0xffffffffu;
                }
            } else {
                tval -= 1;
            }
        }
        random = (random == 0) ? (nr_tlb_entry - 1) : (random - 1);
        estat_reg->is = (estat_reg->is & 0b1100000000011u) | ((ext_int & 0b111111111u) << 2);
        if ((estat_reg->is & ecfg_reg->lie) != 0 && ((csr_crmd *) &crmd)->ie != 0) {
            raise_trap(std::make_pair(INT, 0));
        }
    }

    bool need_trap() {
        return cur_need_trap;
    }

    uint32_t get_trap_pc() {
        return trap_pc;
    }

    void raise_trap(la32r_exccode exc, uint32_t bad_va = 0) {
        auto estat_reg = (csr_estat *) &estat;
        auto crmd_reg = (csr_crmd *) &crmd;
        auto prmd_reg = (csr_prmd *) &prmd;
        auto tlbehi_reg = (csr_tlbehi *) &tlbehi;
        cur_need_trap = true;
        trap_pc = (exc.first == TLBR) ? tlbrentry : eentry;
        era = pc;
        prmd_reg->pplv = crmd_reg->plv;
        prmd_reg->pie = crmd_reg->ie;
        estat_reg->ecode = exc.first;
        estat_reg->esubcode = exc.second;
        crmd_reg->ie = 0;
        crmd_reg->plv = plv0;
        if (exc.first == TLBR) {
            crmd_reg->da = 1;
            crmd_reg->pg = 0;
        }
        if (exc.first == TLBR || exc.first == ADE || exc.first == ALE || exc.first == PIL || exc.first == PIS
            || exc.first == PIF || exc.first == PME || exc.first == PPI) {
            badv = bad_va;
        }
        if (exc.first == TLBR || exc.first == PIL || exc.first == PIS || exc.first == PIF || exc.first == PME
            || exc.first == PPI) {
            tlbehi_reg->vppn = bad_va >> 13;
        }
    }

    void ertn() {
        auto estat_reg = (csr_estat *) &estat;
        auto crmd_reg = (csr_crmd *) &crmd;
        auto prmd_reg = (csr_prmd *) &prmd;
        auto llbctl_reg = (csr_llbctl *) &llbctl;
        cur_need_trap = true;
        trap_pc = era;
        crmd_reg->plv = prmd_reg->pplv;
        crmd_reg->ie = prmd_reg->pie;
        if (estat_reg->ecode == TLBR) {
            crmd_reg->da = 0;
            crmd_reg->pg = 1;
        }
        if (llbctl_reg->klo) {
            llbctl_reg->klo = 0;
        } else {
            llbctl_reg->rollb = 0;
        }
    }

    void tlb_search() {
        uint8_t index;
        auto asid_reg = (csr_asid *) &asid;
        auto tlbehi_reg = (csr_tlbehi *) &tlbehi;
        auto tlbidx_reg = (csr_tlbidx *) &tlbidx;
        if (mmu.tlb_search(tlbehi_reg->vppn << 13, asid_reg->asid, index)) {
            tlbidx_reg->ne = 0;
            tlbidx_reg->index = index;
        } else {
            tlbidx_reg->ne = 1;
        }
    }

    void tlb_read() {
        auto tlbidx_reg = (csr_tlbidx *) &tlbidx;
        auto tlbehi_reg = (csr_tlbehi *) &tlbehi;
        auto tlbelo0_reg = (csr_tlbelo *) &tlbelo0;
        auto tlbelo1_reg = (csr_tlbelo *) &tlbelo1;
        auto asid_reg = (csr_asid *) &asid;
        la32r_tlb tlbe = mmu.tlb_rd(tlbidx_reg->index);
        if (tlbe.e) {
            tlbehi_reg->vppn = tlbe.vppn;
            tlbelo0_reg->plv = tlbe.plv0;
            tlbelo0_reg->mat = tlbe.mat0;
            tlbelo0_reg->ppn = tlbe.ppn0;
            tlbelo0_reg->g = tlbe.g;
            tlbelo0_reg->d = tlbe.d0;
            tlbelo0_reg->v = tlbe.v0;
            tlbelo1_reg->plv = tlbe.plv1;
            tlbelo1_reg->mat = tlbe.mat1;
            tlbelo1_reg->ppn = tlbe.ppn1;
            tlbelo1_reg->g = tlbe.g;
            tlbelo1_reg->d = tlbe.d1;
            tlbelo1_reg->v = tlbe.v1;
            tlbidx_reg->ps = tlbe.ps;
            tlbidx_reg->ne = 0;
            asid_reg->asid = tlbe.asid;
        } else {
            tlbidx_reg->ne = 1;
            asid_reg->asid = 0;
            tlbehi = 0;
            tlbelo0 = 0;
            tlbelo1 = 0;
            tlbidx_reg->ps = 0;
        }
    }

    void tlb_write(bool rand) {
        auto asid_reg = (csr_asid *) &asid;
        auto tlbehi_reg = (csr_tlbehi *) &tlbehi;
        auto tlbelo0_reg = (csr_tlbelo *) &tlbelo0;
        auto tlbelo1_reg = (csr_tlbelo *) &tlbelo1;
        auto tlbidx_reg = (csr_tlbidx *) &tlbidx;
        auto estat_reg = (csr_estat *) &estat;
        mmu.tlb_wr(la32r_tlb{
                !tlbidx_reg->ne || estat_reg->ecode == 0x3f,
                asid_reg->asid,
                tlbelo0_reg->g && tlbelo1_reg->g,
                tlbidx_reg->ps,
                tlbehi_reg->vppn,
                tlbelo0_reg->v,
                tlbelo0_reg->d,
                tlbelo0_reg->mat,
                tlbelo0_reg->plv,
                tlbelo0_reg->ppn,
                tlbelo1_reg->v,
                tlbelo1_reg->d,
                tlbelo1_reg->mat,
                tlbelo1_reg->plv,
                tlbelo1_reg->ppn
        }, rand ? random : tlbidx_reg->index);
    }

    void tlb_invalidate(uint8_t op, uint32_t inv_asid, uint32_t inv_va) {
        switch (op) {
        case 0x0:
        case 0x1:
            mmu.tlb_inv([](const la32r_tlb &tlbe) {
                return true;
            });
            break;
        case 0x2:
            mmu.tlb_inv([](const la32r_tlb &tlbe) {
                return tlbe.g == 1;
            });
            break;
        case 0x3:
            mmu.tlb_inv([](const la32r_tlb &tlbe) {
                return tlbe.g == 0;
            });
            break;
        case 0x4:
            mmu.tlb_inv([=](const la32r_tlb &tlbe) {
                return tlbe.g == 0 && tlbe.asid == inv_asid;
            });
            break;
        case 0x5:
            mmu.tlb_inv([=](const la32r_tlb &tlbe) {
                return tlbe.g == 0 && tlbe.asid == inv_asid
                       && (tlbe.vppn >> (tlbe.ps - 12) == inv_va >> (tlbe.ps + 1));
            });
            break;
        case 0x6:
            mmu.tlb_inv([=](const la32r_tlb &tlbe) {
                return (tlbe.g == 1 || tlbe.asid == inv_asid)
                       && (tlbe.vppn >> (tlbe.ps - 12) == inv_va >> (tlbe.ps + 1));
            });
            break;
        default:
            raise_trap(std::make_pair(IPE, 0));
            break;
        }
    }

    la32r_plv get_cur_plv() {
        return ((csr_crmd *) &crmd)->plv;
    }

    bool get_crmd_pg() {
        return ((csr_crmd *) &crmd)->pg == 1;
    }

    uint32_t get_asid() {
        return ((csr_asid *) &asid)->asid;
    }

    uint32_t get_timer_id() {
        return ((csr_tid *) &tid)->tid;
    }

    uint8_t get_llbit() {
        return ((csr_llbctl *) &llbctl)->rollb;
    }

    void set_llbit() {
        ((csr_llbctl *) &llbctl)->rollb = 1;
    }

    void clr_llbit() {
        ((csr_llbctl *) &llbctl)->rollb = 0;
    }

    void check_and_raise_int() {
        auto crmd_reg = (csr_crmd *) &crmd;
        auto ecfg_reg = (csr_ecfg *) &ecfg;
        auto estat_reg = (csr_estat *) &estat;
        if (crmd_reg->ie && (ecfg_reg->lie & estat_reg->is) != 0) {
            raise_trap(std::make_pair(INT, 0));
        }
    }

private:
    uint32_t *save_of(uint32_t index) {
        if (index == SAVE0) return &save0;
        if (index == SAVE1) return &save1;
        if (index == SAVE2) return &save2;
        if (index == SAVE3) return &save3;
        return nullptr;
    }

    uint32_t core_id;

    la32r_mmu<nr_tlb_entry> &mmu;
    uint32_t &pc;

    uint32_t random;
    bool cur_need_trap;
    uint32_t trap_pc;
    bool timer_en;

    uint32_t crmd;
    uint32_t prmd;
    uint32_t euen;
    uint32_t ecfg;
    uint32_t estat;
    uint32_t era;
    uint32_t badv;
    uint32_t eentry;
    uint32_t tlbidx;
    uint32_t tlbehi;
    uint32_t tlbelo0;
    uint32_t tlbelo1;
    uint32_t asid;
    uint32_t pgdl;
    uint32_t pgdh;
    uint32_t cpuid;
    uint32_t save0;
    uint32_t save1;
    uint32_t save2;
    uint32_t save3;
    uint32_t tid;
    uint32_t tcfg;
    uint32_t tval;
    uint32_t llbctl; // llbit is llbctl.rollb
    uint32_t tlbrentry;
    uint32_t ctag;
    uint32_t dmw0;
    uint32_t dmw1;
};

#endif // LA32R_CSR
