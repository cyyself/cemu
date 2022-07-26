#ifndef MIPS_CP0
#define MIPS_CP0

#include "mips_common.hpp"
#include <cstdint>
#include <cassert>
#include <cstdio>
class mips_cp0 {
public:
    mips_cp0(uint32_t &pc, bool &bd):pc(pc),bd(bd) {
        reset();
    }
    void reset() {
        badva = 0;
        count = 0;
        status = 0;
        cause = 0;
        epc = 0;
        ebase = 0x8000000u;
        cp0_status *status_reg = (cp0_status*)&status;
        status_reg->BEV = 1;
        cur_need_trap = false;
    }
    uint32_t mfc0(uint32_t reg, uint32_t sel) {
        switch (reg) {
            case RD_BADVA:
                return badva;
            case RD_COUNT:
                return count;
            case RD_COMPARE:
                return compare;
            case RD_STATUS:
                return status;
            case RD_CAUSE:
                return cause;
            case RD_EPC:
                return epc;
            default:
                return 0;
        }
    }
    void mtc0(uint32_t reg, uint32_t sel, uint32_t value) {
        switch (reg) {
            case RD_COUNT:
                count = value;
                assert(sel == 0);
                break;
            case RD_COMPARE: {
                compare = value;
                cp0_cause *cause_reg = (cp0_cause*)&cause;
                cause_reg->TI = 0;
                assert(sel == 0);
                break;
            }
            case RD_STATUS: {
                cp0_status *status_reg = (cp0_status*)&status;
                cp0_status *status_new = (cp0_status*)&value;
                status_reg->IE = status_new->IE;
                status_reg->EXL = status_new->EXL;
                status_reg->ERL = status_new->ERL;
                status_reg->KSU = status_new->KSU == USER_MODE ? USER_MODE : KERNEL_MODE;
                status_reg->IM = status_new->IM;
                if (status_reg->BEV != status_new->BEV) {
                    printf("bev changed at pc %x to %x\n",pc,status_new->BEV);
                }
                status_reg->BEV = status_new->BEV;
                assert(sel == 0);
                break;
            }
            case RD_CAUSE: {
                cp0_cause *cause_reg = (cp0_cause*)&cause;
                cp0_cause *cause_new = (cp0_cause*)&value;
                cause_reg->IP = (cause_reg->IP & 0b11111100u) | (cause_new->IP & 0b11);
                cause_reg->IV = cause_new->IV;
                assert(sel == 0);
                break;
            }
            case RD_EPC: {
                epc = value;
                break;
            }
            default: {
                printf("%x\n",pc);
                assert(false);
                break;
            }
        }
    }
    void pre_exec(unsigned int ext_int) {
        cur_need_trap = false;
        count = (count + 1llu) &0xfffffffflu;
        cp0_cause *cause_reg = (cp0_cause*)&cause;
        cause_reg->IP = (cause_reg->IP & 0b11) | ( (ext_int & 0b11111) << 2);
        check_and_raise_int();
    }
    bool need_trap() {
        return cur_need_trap;
    }
    uint32_t get_trap_pc() {
        return trap_pc;
    }
    void raise_trap(mips32_exccode exc, uint32_t badva_val = 0) {
        cur_need_trap = true;
        cp0_status *status_reg = (cp0_status*)&status;
        cp0_cause *cause_reg = (cp0_cause*)&cause;
        uint32_t trap_base = (status_reg->BEV ? 0xbfc00200u : ebase);
        if (!status_reg->EXL) {
            epc = bd ? pc - 4 : pc;
            cause_reg->BD = bd;
            trap_pc = trap_base + ((exc == EXC_TLBL || exc == EXC_TLBS) ? 0x000 : (exc == EXC_INT && cause_reg->IV && !status_reg->BEV) ? 0x200 : 0x180);
        }
        else trap_pc = trap_base + 0x180u;
        cause_reg->exccode = exc;
        status_reg->EXL = 1;
        if (exc == EXC_ADEL || exc == EXC_ADES) {
            badva = badva_val;
        }
    }
    void eret() {
        cur_need_trap = true;
        cp0_status *status_reg = (cp0_status*)&status;
        trap_pc = status_reg->ERL ? errorepc : epc;
        if (status_reg->ERL) status_reg->ERL = 0;
        else status_reg->EXL = 0;
    }
    mips32_ksu get_ksu() {
        cp0_status *status_def = (cp0_status*)&status;
        return (status_def->EXL || status_def->ERL || status_def->KSU == KERNEL_MODE) ? KERNEL_MODE : USER_MODE;
    }
private:
    void check_and_raise_int() {
        cp0_status *status_def = (cp0_status*)&status;
        cp0_cause *cause_def = (cp0_cause*)&cause;
        if (status_def->IE && status_def->EXL == 0 && status_def->ERL == 0 && (status_def->IM & cause_def->IP) != 0) {
            raise_trap(EXC_INT);
        }
    }
    uint32_t &pc;
    bool &bd;

    bool cur_need_trap;
    uint32_t trap_pc;
    uint32_t compare;
    uint32_t badva;
    uint64_t count_shadow;
    uint32_t count;
    uint32_t status;
    uint32_t cause;
    uint32_t epc;
    uint32_t errorepc;
    uint32_t ebase;
};

#endif
