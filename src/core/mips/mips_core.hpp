#ifndef MIPS_CORE_HPP
#define MIPS_CORE_HPP

#include "mips_common.hpp"
#include "memory_bus.hpp"
#include "mips_cp0.hpp"
#include "mips_mmu.hpp"
#include <cstring>
#include <cassert>
#include <queue>

class mips_core {
public:
    mips_core(memory_bus &bus):mmu(bus),cp0(pc,in_delay_slot,mmu) {
        reset();
    }
    void step(uint8_t ext_int = 0) {
        exec(ext_int);
    }
    void set_GPR(uint8_t GPR_index, int32_t value) {
        GPR[GPR_index] = value;
        debug_wb_wen = 0xfu;
        debug_wb_wnum = GPR_index;
        debug_wb_wdata = value;
    }
    void jump(uint32_t new_pc) {
        pc = new_pc;
    }
    void reset() {
        pc = 0xbfc00000;
        memset(GPR,0,sizeof(GPR));
        hi = 0;
        lo = 0;
        next_delay_slot = false;
        in_delay_slot = false;
        next_control_trans = false;
        cur_control_trans = false;
        cp0.reset();
        mmu.reset();
        debug_wb_pc = 0;
        debug_wb_wen = 0;
        debug_wb_wnum = 0;
        debug_wb_wdata = 0;
        debug_wb_is_timer = 0;
        forward_branch = 0;
        forward_branch_taken = 0;
        backward_branch = 0;
        backward_branch_taken = 0;
        insret = 0;
    }
    uint32_t get_pc() {
        return pc;
    }
    uint32_t forward_branch;
    uint32_t forward_branch_taken;
    uint32_t backward_branch;
    uint32_t backward_branch_taken;
    uint32_t insret;
    uint32_t debug_wb_pc;
    uint8_t  debug_wb_wen;
    uint8_t  debug_wb_wnum;
    uint32_t debug_wb_wdata;
    bool     debug_wb_is_timer;
    // TODO: trace with exceptions (add exception signal at commit stage is need)
private:
    void exec(uint8_t ext_int) {
        in_delay_slot = next_delay_slot;
        next_delay_slot = false;
        cur_control_trans = next_control_trans;
        next_control_trans = false;
        bool ri = false;
        debug_wb_pc = pc;
        debug_wb_wen = 0;
        debug_wb_wnum = 0;
        debug_wb_wdata = 0;
        debug_wb_is_timer = false;
        mips_instr instr;
        mips32_exccode if_exc = EXC_OK;
        cp0.pre_exec(ext_int);
        if (cp0.need_trap()) goto ctrl_trans_and_exception;
        pc_trace.push(pc);
        if (pc_trace.size() > 16) pc_trace.pop();
        if_exc = mmu.va_if(pc, (uint8_t*)&instr, cp0.get_ksu(), cp0.get_asid());
        if (if_exc != EXC_OK) {
            cp0.raise_trap(if_exc, pc);
            goto ctrl_trans_and_exception;
        }
        switch (instr.i_type.opcode) {
            case OPCODE_SPECIAL: {
                switch (instr.r_type.funct) {
                    case FUNCT_ADD: {
                        // ADD
                        if (instr.r_type.sa) ri = true;
                        else {
                            int64_t result = static_cast<int64_t>(GPR[instr.r_type.rs]) + static_cast<int64_t>(GPR[instr.r_type.rt]);
                            if ( ((result >> 32) & 1) != ((result >> 31) & 1)) {
                                cp0.raise_trap(EXC_OV);
                            }
                            else {
                                set_GPR(instr.r_type.rd, result);
                            }
                        }
                        break;
                    }
                    case FUNCT_ADDU: {
                        // ADDU
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rs] + GPR[instr.r_type.rt]);
                        }
                        break;
                    }
                    case FUNCT_SUB: {
                        // SUB
                        if (instr.r_type.sa) ri = true;
                        else {
                            int64_t result = static_cast<int64_t>(GPR[instr.r_type.rs]) - static_cast<int64_t>(GPR[instr.r_type.rt]);
                            if ( ((result >> 32) & 1) != ((result >> 31) & 1)) {
                                cp0.raise_trap(EXC_OV);
                            }
                            else {
                                set_GPR(instr.r_type.rd, result);
                            }
                        }
                        break;
                    }
                    case FUNCT_SUBU: {
                        // SUBU
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, static_cast<int64_t>(GPR[instr.r_type.rs]) - static_cast<int64_t>(GPR[instr.r_type.rt]));
                        }
                        break;
                    }
                    case FUNCT_SLT: {
                        // SLT
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rs] < GPR[instr.r_type.rt]);
                        }
                        break;
                    }
                    case FUNCT_SLTU: {
                        // SLTU
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, static_cast<uint32_t>(GPR[instr.r_type.rs]) < static_cast<uint32_t>(GPR[instr.r_type.rt]));
                        }
                        break;
                    }
                    case FUNCT_MULT: {
                        // MULT
                        if (instr.r_type.sa || instr.r_type.rd) ri = true;
                        else {
                            uint64_t result = static_cast<int64_t>(GPR[instr.r_type.rs]) * static_cast<int64_t>(GPR[instr.r_type.rt]);
                            lo = result;
                            hi = result >> 32;
                        }
                        break;
                    }
                    case FUNCT_MULTU: {
                        // MULTU
                        if (instr.r_type.sa) ri = true;
                        else {
                            uint64_t result = static_cast<uint32_t>(GPR[instr.r_type.rs])*1llu * static_cast<uint32_t>(GPR[instr.r_type.rt]);
                            lo = result;
                            hi = result >> 32;
                        }
                        break;
                    }
                    case FUNCT_DIV: {
                        // DIV
                        if (instr.r_type.sa) ri = true;
                        else {
                            lo = static_cast<int64_t>(GPR[instr.r_type.rs]) / static_cast<int64_t>(GPR[instr.r_type.rt]);
                            hi = static_cast<int64_t>(GPR[instr.r_type.rs]) % static_cast<int64_t>(GPR[instr.r_type.rt]);
                        }
                        break;
                    }
                    case FUNCT_DIVU: {
                        // DIVU
                        if (instr.r_type.sa) ri = true;
                        else {
                            lo = static_cast<uint32_t>(GPR[instr.r_type.rs]) / static_cast<uint32_t>(GPR[instr.r_type.rt]);
                            hi = static_cast<uint32_t>(GPR[instr.r_type.rs]) % static_cast<uint32_t>(GPR[instr.r_type.rt]);
                        }
                        break;
                    }
                    case FUNCT_AND: {
                        // AND
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rs] & GPR[instr.r_type.rt]);
                        }
                        break;
                    }
                    case FUNCT_NOR: {
                        // NOR
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, ~(GPR[instr.r_type.rs] | GPR[instr.r_type.rt]));
                        }
                        break;
                    }
                    case FUNCT_OR: {
                        // OR
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rs] | GPR[instr.r_type.rt]);
                        }
                        break;
                    }
                    case FUNCT_XOR: {
                        // XOR
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rs] ^ GPR[instr.r_type.rt]);
                        }
                        break;
                    }
                    case FUNCT_SLLV: {
                        // SLLV
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rt] << (GPR[instr.r_type.rs] & 0x1f));
                        }
                        break;
                    }
                    case FUNCT_SLL: {
                        // SLL
                        if (instr.r_type.rs) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rt] << (instr.r_type.sa));
                        }
                        break;
                    }
                    case FUNCT_SRAV: {
                        // SRAV
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rt] >> (GPR[instr.r_type.rs] & 0x1f));
                        }
                        break;
                    }
                    case FUNCT_SRA: {
                        // SRA
                        if (instr.r_type.rs) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rt] >> instr.r_type.sa);
                        }
                        break;
                    }
                    case FUNCT_SRLV: {
                        // SRLV
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, static_cast<uint32_t>(GPR[instr.r_type.rt]) >> (GPR[instr.r_type.rs] & 0x1f));
                        }
                        break;
                    }
                    case FUNCT_SRL: {
                        // SRL
                        if (instr.r_type.rs) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, static_cast<uint32_t>(GPR[instr.r_type.rt]) >> instr.r_type.sa);
                        }
                        break;
                    }
                    case FUNCT_JR: {
                        // JR
                        next_delay_slot = true;
                        if (instr.r_type.rt || instr.r_type.rd || instr.r_type.sa) {
                            ri = true;
                        }
                        else {
                            delay_npc = GPR[instr.r_type.rs];
                            next_control_trans = true;
                        }
                        break;
                    }
                    case FUNCT_JALR: {
                        // JALR
                        next_delay_slot = true;
                        if (instr.r_type.rt || instr.r_type.sa) {
                            ri = true;
                        }
                        else {
                            delay_npc = GPR[instr.r_type.rs];
                            next_control_trans = true;
                            set_GPR(instr.r_type.rd, pc + 8);
                        }
                        break;
                    }
                    case FUNCT_MFHI: {
                        // MFHI
                        if (instr.r_type.rs || instr.r_type.rt || instr.r_type.sa) ri = true;
                        else set_GPR(instr.r_type.rd, hi);
                        break;
                    }
                    case FUNCT_MFLO: {
                        // MFLO
                        if (instr.r_type.rs || instr.r_type.rt || instr.r_type.sa) ri = true;
                        else set_GPR(instr.r_type.rd, lo);
                        break;
                    }
                    case FUNCT_MTHI: {
                        // MTHI
                        if (instr.r_type.rt || instr.r_type.rd || instr.r_type.sa) ri = true;
                        else hi = GPR[instr.r_type.rs];
                        break;
                    }
                    case FUNCT_MTLO: {
                        // MTLO
                        if (instr.r_type.rt || instr.r_type.rd || instr.r_type.sa) ri = true;
                        else lo = GPR[instr.r_type.rs];
                        break;
                    }
                    case FUNCT_BREAK: {
                        // BREAK
                        cp0.raise_trap(EXC_BP);
                        break;
                    }
                    case FUNCT_SYSCALL: {
                        // SYSCALL
                        cp0.raise_trap(EXC_SYS);
                        break;
                    }
                    case FUNCT_SYNC: {
                        // SYNC as NOP
                        break;
                    }
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_SPECIAL2: {
                switch (instr.r_type.funct) {
                    case FUNCT_MUL: {
                        if (instr.r_type.sa) ri = true;
                        else {
                            set_GPR(instr.r_type.rd, GPR[instr.r_type.rs] * GPR[instr.r_type.rt]);
                        }
                        break;
                    }
                    case FUNCT_MADD: {
                        if (instr.r_type.sa || instr.r_type.rd) ri = true;
                        else {
                            uint64_t result = (((hi*1llu) << 32) + lo*11lu) + static_cast<int64_t>(GPR[instr.r_type.rs]) * static_cast<int64_t>(GPR[instr.r_type.rt]);
                            lo = result;
                            hi = result >> 32;
                        }
                        break;
                    }
                    case FUNCT_MADDU: {
                        if (instr.r_type.sa || instr.r_type.rd) ri = true;
                        else {
                            uint64_t result = (((hi*1llu) << 32) + lo*11lu) + static_cast<uint32_t>(GPR[instr.r_type.rs])*1llu * static_cast<uint32_t>(GPR[instr.r_type.rt]);
                            lo = result;
                            hi = result >> 32;
                        }
                        break;
                    }
                    case FUNCT_MSUB: {
                        if (instr.r_type.sa || instr.r_type.rd) ri = true;
                        else {
                            uint64_t result = (((hi*1llu) << 32) + lo*11lu) - static_cast<int64_t>(GPR[instr.r_type.rs]) * static_cast<int64_t>(GPR[instr.r_type.rt]);
                            lo = result;
                            hi = result >> 32;
                        }
                        break;
                    }
                    case FUNCT_MSUBU: {
                        if (instr.r_type.sa || instr.r_type.rd) ri = true;
                        else {
                            uint64_t result = (((hi*1llu) << 32) + lo*11lu) - static_cast<uint32_t>(GPR[instr.r_type.rs])*1llu * static_cast<uint32_t>(GPR[instr.r_type.rt]);
                            lo = result;
                            hi = result >> 32;
                        }
                        break;
                    }
                    case FUNCT_CLO: {
                        if (instr.r_type.sa) ri = true;
                        else {
                            int res = 32;
                            for (int i=31;i>=0;i--) if (((GPR[instr.r_type.rs] >> i) & 1) == 0) {
                                res = 31 - i;
                                break;
                            }
                            set_GPR(instr.r_type.rd, res);
                        }
                        break;
                    }
                    case FUNCT_CLZ: {
                        if (instr.r_type.sa) ri = true;
                        else {
                            int res = 32;
                            for (int i=31;i>=0;i--) if ((GPR[instr.r_type.rs] >> i) & 1) {
                                res = 31 - i;
                                break;
                            }
                            set_GPR(instr.r_type.rd, res);
                        }
                        break;
                    }
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_ADDI: {
                int64_t result = static_cast<int64_t>(GPR[instr.i_type.rs]) + instr.i_type.imm;
                if ( ((result >> 32) & 1) != ((result >> 31) & 1)) {
                    cp0.raise_trap(EXC_OV);
                }
                else {
                    set_GPR(instr.r_type.rt, result);
                }
                break;
            }
            case OPCODE_ADDIU: {
                int32_t result = static_cast<int32_t>(GPR[instr.i_type.rs]) + instr.i_type.imm;
                set_GPR(instr.i_type.rt, result);
                break;
            }
            case OPCODE_SLTI: {
                int32_t result = GPR[instr.i_type.rs] < instr.i_type.imm;
                set_GPR(instr.i_type.rt, result);
                break;
            }
            case OPCODE_SLTIU: {
                int32_t result = static_cast<uint32_t>(GPR[instr.i_type.rs]) < static_cast<uint32_t>(instr.i_type.imm);
                set_GPR(instr.i_type.rt, result);
                break;
            }
            case OPCODE_ANDI: {
                int32_t result = static_cast<uint32_t>(GPR[instr.i_type.rs]) & (static_cast<uint32_t>(instr.i_type.imm) & 0xffffu);
                set_GPR(instr.i_type.rt, result);
                break;
            }
            case OPCODE_LUI: {
                if (instr.i_type.rs != 0) ri = true;
                else {
                    int32_t result = instr.i_type.imm << 16;
                    set_GPR(instr.i_type.rt, result);
                }
                break;
            }
            case OPCODE_ORI: {
                int32_t result = GPR[instr.i_type.rs] | (uint16_t)instr.i_type.imm;
                set_GPR(instr.i_type.rt, result);
                break;
            }
            case OPCODE_XORI: {
                int32_t result = GPR[instr.i_type.rs] ^ (uint16_t)instr.i_type.imm;
                set_GPR(instr.i_type.rt, result);
                break;
            }
            case OPCODE_BEQ: {
                // BEQ
                next_delay_slot = true;
                if (instr.i_type.imm < 0) backward_branch ++;
                else forward_branch ++;
                if (GPR[instr.i_type.rs] == GPR[instr.i_type.rt]) {
                    next_control_trans = true;
                    delay_npc = pc + (instr.i_type.imm << 2) + 4;
                    if (instr.i_type.imm < 0) backward_branch_taken ++;
                    else forward_branch_taken ++;
                }
                break;
            }
            case OPCODE_BNE: {
                // BNE
                if (instr.i_type.imm < 0) backward_branch ++;
                else forward_branch ++;
                next_delay_slot = true;
                if (GPR[instr.i_type.rs] != GPR[instr.i_type.rt]) {
                    next_control_trans = true;
                    delay_npc = pc + (instr.i_type.imm << 2) + 4;
                    if (instr.i_type.imm < 0) backward_branch_taken ++;
                    else forward_branch_taken ++;
                }
                break;
            }
            case OPCODE_BGTZ: {
                // BGTZ
                next_delay_slot = true;
                if (instr.i_type.imm < 0) backward_branch ++;
                else forward_branch ++;
                if (instr.i_type.rt != 0) ri = true;
                else {
                    if (GPR[instr.i_type.rs] > 0) {
                        next_control_trans = true;
                        delay_npc = pc + (instr.i_type.imm << 2) + 4;
                        if (instr.i_type.imm < 0) backward_branch_taken ++;
                        else forward_branch_taken ++;
                    }
                }
                break;
            }
            case OPCODE_BLEZ: {
                // BLEZ
                next_delay_slot = true;
                if (instr.i_type.imm < 0) backward_branch ++;
                else forward_branch ++;
                if (instr.i_type.rt != 0) ri = true;
                else {
                    if (GPR[instr.i_type.rs] <= 0) {
                        next_control_trans = true;
                        delay_npc = pc + (instr.i_type.imm << 2) + 4;
                        if (instr.i_type.imm < 0) backward_branch_taken ++;
                        else forward_branch_taken ++;
                    }
                }
                break;
            }
            case OPCODE_REGIMM: {
                switch (instr.i_type.rt) {
                    case RT_BGEZ: {
                        // BGEZ
                        next_delay_slot = true;
                        if (instr.i_type.imm < 0) backward_branch ++;
                        else forward_branch ++;
                        if (GPR[instr.i_type.rs] >= 0) {
                            next_control_trans = true;
                            delay_npc = pc + (instr.i_type.imm << 2) + 4;
                            if (instr.i_type.imm < 0) backward_branch_taken ++;
                            else forward_branch_taken ++;
                        }
                        break;
                    }
                    case RT_BLTZ: {
                        // BLTZ
                        next_delay_slot = true;
                        if (instr.i_type.imm < 0) backward_branch ++;
                        else forward_branch ++;
                        if (GPR[instr.i_type.rs] < 0) {
                            next_control_trans = true;
                            delay_npc = pc + (instr.i_type.imm << 2) + 4;
                            if (instr.i_type.imm < 0) backward_branch_taken ++;
                            else forward_branch_taken ++;
                        }
                        break;
                    }
                    case RT_BGEZAL: {
                        // BGEZAL
                        next_delay_slot = true;
                        if (instr.i_type.imm < 0) backward_branch ++;
                        else forward_branch ++;
                        if (GPR[instr.i_type.rs] >= 0) {
                            next_control_trans = true;
                            delay_npc = pc + (instr.i_type.imm << 2) + 4;
                            if (instr.i_type.imm < 0) backward_branch_taken ++;
                            else forward_branch_taken ++;
                        }
                        set_GPR(31, pc + 8);
                        break;
                    }
                    case RT_BLTZAL: {
                        // BLTZAL
                        if (instr.i_type.imm < 0) backward_branch ++;
                        else forward_branch ++;
                        next_delay_slot = true;
                        if (GPR[instr.i_type.rs] < 0) {
                            next_control_trans = true;
                            delay_npc = pc + (instr.i_type.imm << 2) + 4;
                            if (instr.i_type.imm < 0) backward_branch_taken ++;
                            else forward_branch_taken ++;
                        }
                        set_GPR(31, pc + 8);
                        break;
                    }
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_J: {
                // J
                assert(!next_delay_slot);
                delay_npc = (pc & 0xf0000000) | (instr.j_type.imm << 2);
                next_delay_slot = true;
                next_control_trans = true;
                break;
            }
            case OPCODE_JAL: {
                // JAL
                assert(!next_delay_slot);
                set_GPR(31, pc + 8);
                delay_npc = (pc & 0xf0000000) | (instr.j_type.imm << 2);
                next_delay_slot = true;
                next_control_trans = true;
                break;
            }
            case OPCODE_LB: {
                // LB
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                int8_t buf;
                mips32_exccode stat = mmu.va_read(vaddr, 1, (unsigned char*)&buf, cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                else set_GPR(instr.i_type.rt, buf);
                break;
            }
            case OPCODE_LBU: {
                // LBU
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                uint8_t buf;
                mips32_exccode stat = mmu.va_read(vaddr, 1, (unsigned char*)&buf, cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                else set_GPR(instr.i_type.rt, buf);
                break;
            }
            case OPCODE_LH: {
                // LH
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                int16_t buf;
                mips32_exccode stat = mmu.va_read(vaddr, 2, (unsigned char*)&buf, cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                else set_GPR(instr.i_type.rt, buf);
                break;
            }
            case OPCODE_LHU: {
                // LHU
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                uint16_t buf;
                mips32_exccode stat = mmu.va_read(vaddr, 2, (unsigned char*)&buf, cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                else set_GPR(instr.i_type.rt, buf);
                break;
            }
            case OPCODE_LW: {
                // LW
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                uint32_t buf;
                if (vaddr == 0xbfafe000u) debug_wb_is_timer = true; // for difftest
                mips32_exccode stat = mmu.va_read(vaddr, 4, (unsigned char*)&buf, cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                else set_GPR(instr.i_type.rt, buf);
                break;
            }
            case OPCODE_LWL: {
                // LWL
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                uint32_t buf;
                mips32_exccode stat = mmu.va_read(vaddr, 4, (unsigned char*)&buf, cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                else {
                    buf = (GPR[instr.i_type.rt] & 0xffff) | (buf & 0xffff0000);
                    set_GPR(instr.i_type.rt, buf);
                }
                break;
            }
            case OPCODE_LWR: {
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                uint32_t buf;
                mips32_exccode stat = mmu.va_read(vaddr, 4, (unsigned char*)&buf, cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                else {
                    buf = (GPR[instr.i_type.rt] & 0x0000ffff) | (buf & 0xffff);
                    set_GPR(instr.i_type.rt, buf);
                }
            }
            case OPCODE_SB: {
                // SB
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                mips32_exccode stat = mmu.va_write(vaddr, 1, (unsigned char*)&GPR[instr.i_type.rt], cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                break;
            }
            case OPCODE_SH: {
                // SH
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                mips32_exccode stat = mmu.va_write(vaddr, 2, (unsigned char*)&GPR[instr.i_type.rt], cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                break;
            }
            case OPCODE_SW: {
                // SW
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                mips32_exccode stat = mmu.va_write(vaddr, 4, (unsigned char*)&GPR[instr.i_type.rt], cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                break;
            }
            case OPCODE_SWL: {
                // SWL
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm + 2;
                uint16_t to_store = GPR[instr.i_type.rt] >> 16;
                mips32_exccode stat = mmu.va_write(vaddr, 2, (unsigned char*)&to_store, cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                break;
            }
            case OPCODE_SWR: {
                // SWR
                uint32_t vaddr = GPR[instr.i_type.rs] + instr.i_type.imm;
                uint16_t to_store = GPR[instr.i_type.rt];
                mips32_exccode stat = mmu.va_write(vaddr, 2, (unsigned char*)&to_store, cp0.get_ksu(), cp0.get_asid());
                if (stat != EXC_OK) cp0.raise_trap(stat, vaddr);
                break;
            }
            case OPCODE_COP0: {
                switch (instr.r_type.rs) {
                    case RS_MFC0: {
                        // MFC0
                        if (instr.r_type.rd == RD_COUNT && (instr.r_type.funct & 0b111) == 0) debug_wb_is_timer = true; // for difftest
                        set_GPR(instr.r_type.rt, cp0.mfc0(instr.r_type.rd, instr.r_type.funct&0b111));
                        break;
                    }
                    case RS_MTC0: {
                        // MTC0
                        cp0.mtc0(instr.r_type.rd, instr.r_type.funct & 0b111, GPR[instr.r_type.rt]);
                        break;
                    }
                    case RS_CO: {
                        switch (instr.r_type.funct) {
                            case FUNCT_ERET:
                                cp0.eret();
                                break;
                            case FUNCT_TLBP:
                                cp0.tlbp();
                                break;
                            case FUNCT_TLBR:
                                cp0.tlbr();
                                break;
                            case FUNCT_TLBWI:
                                cp0.tlbwi();
                                break;
                            case FUNCT_TLBWR:
                                cp0.tlbwr();
                                break;
                            default:
                                assert(false);
                                break;
                        }
                        break;
                    }
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_CACHE: {
                printf("Cache instruction executed. addr = %x, op = %d\n",instr.i_type.rs + instr.i_type.imm, instr.i_type.rt);
                break;
            }
            default:
                ri = true; // unknown opcode
        }
        if (ri) cp0.raise_trap(EXC_RI);
    ctrl_trans_and_exception:
        if (!cp0.need_trap()) {
            if (cur_control_trans) pc = delay_npc;
            else pc = pc + 4;
            insret ++;
        }
        else {
            pc = cp0.get_trap_pc();
            next_delay_slot = false;
            next_control_trans = false;
        }
        // assert(!(in_delay_slot && next_delay_slot));
        if ((in_delay_slot && next_delay_slot)) {
            while (!pc_trace.empty()) {
                printf("%x\n", pc_trace.front());
                pc_trace.pop();
            }
            exit(1);
        }
    }
    uint32_t pc;    
    bool next_delay_slot = false;
    bool in_delay_slot = false;
    bool next_control_trans = false;
    bool cur_control_trans = false;
    uint32_t delay_npc;
    int32_t GPR[32];
    uint32_t hi,lo;
    mips_mmu<8> mmu;
    mips_cp0<8> cp0;
    std::queue <uint32_t> pc_trace;
};


#endif