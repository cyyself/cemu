#ifndef RV_CORE_HPP
#define RV_CORE_HPP

#include <bitset>
#include "rv_common.hpp"
#include <assert.h>
#include "rv_systembus.hpp"
#include "rv_priv.hpp"
#include <deque>

extern bool riscv_test;

enum alu_op {
    ALU_ADD, ALU_SUB, ALU_SLL, ALU_SLT, ALU_SLTU, ALU_XOR, ALU_SRL, ALU_SRA, ALU_OR, ALU_AND,
    ALU_MUL, ALU_MULH, ALU_MULHU, ALU_MULHSU, ALU_DIV, ALU_DIVU, ALU_REM, ALU_REMU,
    ALU_NOP
};

#define binary_concat(value,r,l,shift) ((((value)>>(l))&(1<<((r)-(l)+1))-1)<<(shift))

#define PC_ALIGN 2

class rv_core {
public:
    rv_core(rv_systembus &systembus, uint8_t hart_id = 0):systembus(systembus),priv(hart_id,pc,systembus) {
        for (int i=0;i<32;i++) GPR[i] = 0;
    }
    void step(bool meip, bool msip, bool mtip, bool seip) {
        exec(meip,msip,mtip,seip);
    }
    void jump(uint64_t new_pc) {
        pc = new_pc;
    }
    void set_GPR(uint8_t GPR_index, int64_t value) {
        assert(GPR_index >= 0 && GPR_index < 32);
        if (GPR_index) GPR[GPR_index] = value;
    }
    uint64_t getPC() {
        return pc;
    }
private:
    uint32_t trace_size = riscv_test ? 128 : 0;
    std::queue <uint64_t> trace;
    rv_systembus &systembus;
    uint64_t pc = 0;
    rv_priv priv;
    int64_t GPR[32];
    void exec(bool meip, bool msip, bool mtip, bool seip) {
        if (riscv_test && priv.get_cycle() >= 1000000) {
            printf("Test timeout! at pc 0x%lx\n",pc);
            exit(1);
        }
        if (trace_size) {
            trace.push(pc);
            while (trace.size() > trace_size) trace.pop();
        }
        bool ri = false;
        bool new_pc = false;
        bool is_rvc = false;
        uint32_t cur_instr = 0;
        uint64_t pc_bad_va = 0;
        rv_instr *inst = (rv_instr*)&cur_instr;
        rv_exc_code if_exc;
    instr_fetch:
        priv.pre_exec(meip,msip,mtip,seip);
        if (priv.need_trap()) goto exception;
        if (pc % PC_ALIGN) {
            priv.raise_trap(csr_cause_def(exc_instr_misalign),pc);
            goto exception;
        }
        if_exc = priv.va_if(pc,4,(uint8_t*)&cur_instr,pc_bad_va);
        if (if_exc != exc_custom_ok) {
            priv.raise_trap(csr_cause_def(if_exc),pc_bad_va);
            goto exception;
        }
    decode_exec:
        is_rvc = (inst->r_type.opcode & 0b11) != 0b11;
        if (!is_rvc) {
            // non rvc
            switch (inst->r_type.opcode) {
            case OPCODE_LUI:
                set_GPR(inst->u_type.rd,((int64_t)inst->u_type.imm_31_12) << 12);
                break;
            case OPCODE_AUIPC:
                set_GPR(inst->u_type.rd,(((int64_t)inst->u_type.imm_31_12) << 12) + pc);
                break;
            case OPCODE_JAL: {
                uint64_t npc = pc + ((inst->j_type.imm_20 << 20) | (inst->j_type.imm_19_12 << 12) | (inst->j_type.imm_11 << 11) | (inst->j_type.imm_10_1 << 1));
                if (npc & 1) npc ^= 1; // we don't need to check.
                if (npc % PC_ALIGN) priv.raise_trap(csr_cause_def(exc_instr_misalign),npc);
                else {
                    set_GPR(inst->j_type.rd,pc + 4);
                    pc = npc;
                    new_pc = true;
                }
                break;
            }
            case OPCODE_JALR: {
                uint64_t npc = GPR[inst->i_type.rs1] + inst->i_type.imm12;
                if (npc & 1) npc ^= 1;
                if (npc % PC_ALIGN) priv.raise_trap(csr_cause_def(exc_instr_misalign),npc);
                else {
                    set_GPR(inst->j_type.rd,pc + 4);
                    pc = npc;
                    new_pc = true;
                }
                break;
            }
            case OPCODE_BRANCH: {
                int64_t offset = (inst->b_type.imm_12 << 12) | (inst->b_type.imm_11 << 11) | (inst->b_type.imm_10_5 << 5) | (inst->b_type.imm_4_1 << 1);
                uint64_t npc;
                switch (inst->b_type.funct3) {
                    case FUNCT3_BEQ:
                        if (GPR[inst->b_type.rs1] == GPR[inst->b_type.rs2]) {
                            npc = pc + offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BNE:
                        if (GPR[inst->b_type.rs1] != GPR[inst->b_type.rs2]) {
                            npc = pc + offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BLT:
                        if (GPR[inst->b_type.rs1] < GPR[inst->b_type.rs2]) {
                            npc = pc + offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BGE:
                        if (GPR[inst->b_type.rs1] >= GPR[inst->b_type.rs2]) {
                            npc = pc + offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BLTU:
                        if ((uint64_t)GPR[inst->b_type.rs1] < (uint64_t)GPR[inst->b_type.rs2]) {
                            npc = pc + offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BGEU:
                        if ((uint64_t)GPR[inst->b_type.rs1] >= (uint64_t)GPR[inst->b_type.rs2]) {
                            npc = pc + offset;
                            new_pc = true;
                        }
                        break;
                    default:
                        ri = true;
                }
                if (new_pc) {
                    if (npc % PC_ALIGN) priv.raise_trap(csr_cause_def(exc_instr_misalign),npc);
                    else pc = npc;
                }
                break;
            }
            case OPCODE_LOAD: {
                uint64_t mem_addr = GPR[inst->i_type.rs1] + (inst->i_type.imm12);
                switch (inst->i_type.funct3) {
                    case FUNCT3_LB: {
                        int8_t buf;
                        bool ok = mem_read(mem_addr,1,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LH: {
                        int16_t buf;
                        bool ok = mem_read(mem_addr,2,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LW: {
                        int32_t buf;
                        bool ok = mem_read(mem_addr,4,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LD: {
                        int64_t buf;
                        bool ok = mem_read(mem_addr,8,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LBU: {
                        uint8_t buf;
                        bool ok = mem_read(mem_addr,1,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LHU: {
                        uint16_t buf;
                        bool ok = mem_read(mem_addr,2,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LWU: {
                        uint32_t buf;
                        bool ok = mem_read(mem_addr,4,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_STORE: {
                uint64_t mem_addr = GPR[inst->s_type.rs1] + ( (inst->s_type.imm_11_5 << 5) | (inst->s_type.imm_4_0));
                switch (inst->i_type.funct3) {
                    case FUNCT3_SB: {
                        mem_write(mem_addr,1,(unsigned char*)&GPR[inst->s_type.rs2]);
                        break;
                    }
                    case FUNCT3_SH: {
                        mem_write(mem_addr,2,(unsigned char*)&GPR[inst->s_type.rs2]);
                        break;
                    }
                    case FUNCT3_SW: {
                        mem_write(mem_addr,4,(unsigned char*)&GPR[inst->s_type.rs2]);
                        break;
                    }
                    case FUNCT3_SD: {
                        mem_write(mem_addr,8,(unsigned char*)&GPR[inst->s_type.rs2]);
                        break;
                    }
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_OPIMM: {
                int64_t imm = inst->i_type.imm12;
                funct6 fun6 = static_cast<funct6>((inst->r_type.funct7) >> 1);
                alu_op op;
                switch (inst->i_type.funct3) {
                    case FUNCT3_ADD_SUB:
                        op = ALU_ADD;
                        break;
                    case FUNCT3_SLT:
                        op = ALU_SLT;
                        break;
                    case FUNCT3_SLTU:
                        op = ALU_SLTU;
                        break;
                    case FUNCT3_XOR:
                        op = ALU_XOR;
                        break;
                    case FUNCT3_OR:
                        op = ALU_OR;
                        break;
                    case FUNCT3_AND:
                        op = ALU_AND;
                        break;
                    case FUNCT3_SLL:
                        if (imm >= 64) ri = true;
                        op = ALU_SLL;
                        break;
                    case FUNCT3_SRL_SRA: 
                        imm = imm & ((1 << 6) - 1);
                        op = ( (fun6 == FUNCT6_SRA) ? ALU_SRA : ALU_SRL);
                        if (fun6 != FUNCT6_NORMAL && fun6 != FUNCT6_SRA) ri = true;
                        break;
                    default:
                        assert(false);
                }
                if (!ri) {
                    int64_t result = alu_exec(GPR[inst->r_type.rs1],imm,op);
                    set_GPR(inst->r_type.rd,result);
                }
                break;
            }
            case OPCODE_OPIMM32: {
                int64_t imm = inst->i_type.imm12;
                funct7 fun7 = static_cast<funct7>((inst->r_type.funct7));
                alu_op op;
                switch (inst->i_type.funct3) {
                    case FUNCT3_ADD_SUB:
                        op = ALU_ADD;
                        break;
                    case FUNCT3_SLL:
                        if (imm >= 64) ri = true;
                        op = ALU_SLL;
                        if (fun7 != FUNCT7_NORMAL) ri = true;
                        break;
                    case FUNCT3_SRL_SRA: 
                        imm = imm & ((1 << 6) - 1);
                        op = ( (fun7 == FUNCT7_SUB_SRA) ? ALU_SRA : ALU_SRL);
                        if (fun7 != FUNCT7_NORMAL && fun7 != FUNCT7_SUB_SRA) ri = true;
                        break;
                    default:
                        ri = true;
                }
                if (!ri) {
                    int64_t result = alu_exec(GPR[inst->r_type.rs1],imm,op,true);
                    set_GPR(inst->r_type.rd,result);
                }
                break;
            }
            case OPCODE_OP: {
                alu_op op = ALU_NOP;
                switch (inst->r_type.funct7) {
                    case FUNCT7_NORMAL: {
                        switch (inst->r_type.funct3) {
                            case FUNCT3_ADD_SUB:
                                op = ALU_ADD;
                                break;
                            case FUNCT3_SLL:
                                op = ALU_SLL;
                                break;
                            case FUNCT3_SLT:
                                op = ALU_SLT;
                                break;
                            case FUNCT3_SLTU:
                                op = ALU_SLTU;
                                break;
                            case FUNCT3_XOR:
                                op = ALU_XOR;
                                break;
                            case FUNCT3_SRL_SRA:
                                op = ALU_SRL;
                                break;
                            case FUNCT3_OR:
                                op = ALU_OR;
                                break;
                            case FUNCT3_AND:
                                op = ALU_AND;
                                break;
                            default:
                                assert(false);
                        }
                        break;
                    }
                    case FUNCT7_SUB_SRA: {
                        switch (inst->r_type.funct3) {
                            case FUNCT3_ADD_SUB:
                                op = ALU_SUB;
                                break;
                            case FUNCT3_SRL_SRA:
                                op = ALU_SRA;
                                break;
                            default:
                                ri = true;
                        }
                        break;
                    }
                    case FUNCT7_MUL: {
                        switch (inst->r_type.funct3) {
                            case FUNCT3_MUL:
                                op = ALU_MUL;
                                break;
                            case FUNCT3_MULH:
                                op = ALU_MULH;
                                break;
                            case FUNCT3_MULHSU:
                                op = ALU_MULHSU;
                                break;
                            case FUNCT3_MULHU:
                                op = ALU_MULHU;
                                break;
                            case FUNCT3_DIV:
                                op = ALU_DIV;
                                break;
                            case FUNCT3_DIVU:
                                op = ALU_DIVU;
                                break;
                            case FUNCT3_REM:
                                op = ALU_REM;
                                break;
                            case FUNCT3_REMU:
                                op = ALU_REMU;
                                break;
                            default:
                                assert(false);
                        }
                        break;
                    }
                    default:
                        ri = true;
                }
                if (!ri) {
                    int64_t result = alu_exec(GPR[inst->r_type.rs1],GPR[inst->r_type.rs2],op);
                    set_GPR(inst->r_type.rd,result);
                }
                break;
            }
            case OPCODE_OP32: {
                alu_op op = ALU_NOP;
                switch (inst->r_type.funct7) {
                    case FUNCT7_NORMAL: {
                        switch (inst->r_type.funct3) {
                            case FUNCT3_ADD_SUB:
                                op = ALU_ADD;
                                break;
                            case FUNCT3_SLL:
                                op = ALU_SLL;
                                break;
                            case FUNCT3_SRL_SRA:
                                op = ALU_SRL;
                                break;
                            default:
                                ri = true;
                        }
                        break;
                    }
                    case FUNCT7_SUB_SRA: {
                        switch (inst->r_type.funct3) {
                            case FUNCT3_ADD_SUB:
                                op = ALU_SUB;
                                break;
                            case FUNCT3_SRL_SRA:
                                op = ALU_SRA;
                                break;
                            default:
                                ri = true;
                        }
                        break;
                    };
                    case FUNCT7_MUL: {
                        switch (inst->r_type.funct3) {
                            case FUNCT3_MUL:
                                op = ALU_MUL;
                                break;
                            case FUNCT3_DIV:
                                op = ALU_DIV;
                                break;
                            case FUNCT3_DIVU:
                                op = ALU_DIVU;
                                break;
                            case FUNCT3_REM:
                                op = ALU_REM;
                                break;
                            case FUNCT3_REMU:
                                op = ALU_REMU;
                                break;
                            default:
                                ri = true;
                        }
                        break;
                    }
                    default:
                        ri = true;
                }
                if (!ri) {
                    int64_t result = alu_exec(GPR[inst->r_type.rs1],GPR[inst->r_type.rs2],op,true);
                    set_GPR(inst->r_type.rd,result);
                }
                break;
            }
            case OPCODE_AMO: {
                uint8_t funct5 = (inst->r_type.funct7) >> 2;
                if (inst->r_type.funct3 != 0b010 && inst->r_type.funct3 != 0b011) {
                    ri = true;
                    break;
                }
                switch (funct5) {
                    case AMOLR: {
                        if (inst->r_type.rs2 != 0) ri = true;
                        else {
                            if (inst->r_type.funct3 == 0b011) {
                                int64_t result;
                                rv_exc_code exc = priv.va_lr(GPR[inst->r_type.rs1],(1<<inst->r_type.funct3),(uint8_t*)&result);
                                if (exc == exc_custom_ok) set_GPR(inst->r_type.rd,result);
                                else priv.raise_trap(csr_cause_def(exc),GPR[inst->r_type.rs1]);
                            }
                            else {
                                int32_t result;
                                rv_exc_code exc = priv.va_lr(GPR[inst->r_type.rs1],(1<<inst->r_type.funct3),(uint8_t*)&result);
                                if (exc == exc_custom_ok) set_GPR(inst->r_type.rd,result);
                                else priv.raise_trap(csr_cause_def(exc),GPR[inst->r_type.rs1]);
                            }
                        }
                        break;
                    }
                    case AMOSC: {
                        bool result;
                        rv_exc_code exc = priv.va_sc(GPR[inst->r_type.rs1],(1<<inst->r_type.funct3),(uint8_t*)&GPR[inst->r_type.rs2],result);
                        if (exc == exc_custom_ok) set_GPR(inst->r_type.rd,result);
                        else priv.raise_trap(csr_cause_def(exc),GPR[inst->r_type.rs1]);
                        break;
                    }
                    case AMOSWAP: case AMOADD: case AMOXOR: case AMOAND: case AMOOR: case AMOMIN: case AMOMAX: case AMOMINU: case AMOMAXU: {
                        int64_t result;
                        rv_exc_code exc = priv.va_amo(GPR[inst->r_type.rs1],(1<<inst->r_type.funct3),static_cast<amo_funct>(funct5),GPR[inst->r_type.rs2],result);
                        if (exc == exc_custom_ok) set_GPR(inst->r_type.rd,result);
                        else priv.raise_trap(csr_cause_def(exc),GPR[inst->r_type.rs1]);
                        break;
                    }
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_FENCE:
                break;
            case OPCODE_SYSTEM: {
                switch (inst->i_type.funct3) {
                    case FUNCT3_PRIV: {
                        uint64_t funct7 = ((inst->i_type.imm12) & ((1<<12)-1)) >> 5;
                        uint64_t rs2 = (inst->i_type.imm12) & ((1<<5)-1);
                        switch (funct7) {
                            case FUNCT7_ECALL_EBREAK: {
                                if (inst->r_type.rs1 == 0 && inst->r_type.rd == 0) {
                                    switch (rs2) {
                                        case 0: {// ECALL
                                            if (riscv_test && GPR[17] == 93) {
                                                if (GPR[17] == 93) {
                                                    if (GPR[10] == 0) {
                                                        printf("Test Pass!\n");
                                                        exit(0);
                                                    }
                                                    else {
                                                        printf("Failed with value 0x%lx\n",GPR[10]);
                                                        exit(1);
                                                    }
                                                }
                                                else assert(false);
                                            }
                                            else priv.ecall();
                                            break;
                                        }
                                        case 1: // EBREAK
                                            priv.ebreak();
                                            break;
                                        default:
                                            ri = true;
                                            break;
                                    }
                                }
                                else ri = true;
                                break;
                            }
                            case FUNCT7_SRET_WFI: {
                                if (inst->r_type.rs1 == 0 && inst->r_type.rd == 0) {
                                    switch (rs2) {
                                        case 0b00010: // SRET
                                            ri = !priv.sret();
                                            break;
                                        case 0b00101: // WFI
                                            break;
                                        default:
                                            ri = true;
                                    }
                                }
                                break;
                            }
                            case FUNCT7_MRET: {
                                if (rs2 == 0b00010 && inst->r_type.rs1 == 0 && inst->r_type.rd == 0) {
                                    ri = !priv.mret();
                                }
                                else ri = true;
                                break;
                            }
                            case FUNCT7_SFENCE_VMA:
                                ri = !priv.sfence_vma(GPR[inst->r_type.rs1],GPR[inst->r_type.rs2] & 0xffff);
                                break;
                            default:
                                ri = true;
                        }
                        break;
                    }
                    case FUNCT3_CSRRW: {
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri) ri = !priv.csr_write(csr_index,GPR[inst->i_type.rs1]);
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result);
                        break;
                    }
                    case FUNCT3_CSRRS: {
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri && inst->i_type.rs1) ri = !priv.csr_write(csr_index,csr_result | GPR[inst->i_type.rs1]);
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result);
                        break;
                    }
                    case FUNCT3_CSRRC: {
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri && inst->i_type.rs1) ri = !priv.csr_write(csr_index,csr_result & (~GPR[inst->i_type.rs1]));
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result);
                        break;
                    }
                    case FUNCT3_CSRRWI: {
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri) ri = !priv.csr_write(csr_index,inst->i_type.rs1);
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result);
                        break;
                    }
                    case FUNCT3_CSRRSI: {
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri && inst->i_type.rs1) ri = !priv.csr_write(csr_index,csr_result | inst->i_type.rs1);
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result);
                        break;
                    }
                    case FUNCT3_CSRRCI: {
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri && inst->i_type.rs1) ri = !priv.csr_write(csr_index,csr_result & (~(inst->i_type.rs1)));
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result);
                        break;
                    }
                    default:
                        ri = true; // HLV
                }
                break;
            }
            default:
                ri = true;
                break;
            }
        }
        else {
            // rvc
            uint8_t rvc_opcode = ( (cur_instr & 0b11) << 3) | ((cur_instr >> 13) & 0b111);
            uint8_t rs2 = (cur_instr >> 2) & 0x1f;
            uint8_t rs2_c = (cur_instr >> 2) & 0x7;
            switch (rvc_opcode) {
                case OPCODE_C_ADDI4SPN: {
                    uint8_t rd = 8 + binary_concat(cur_instr,4,2,0);
                    int64_t imm = binary_concat(cur_instr,12,11,4) | binary_concat(cur_instr,10,7,6) | binary_concat(cur_instr,6,6,2) | binary_concat(cur_instr,5,5,3);
                    int64_t value = GPR[2] + imm;
                    if (imm) set_GPR(rd,value); // nzimm
                    else ri = true;
                    break;
                }
                case OPCODE_C_LW: {
                    uint64_t imm = (binary_concat(cur_instr,6,6,2) | binary_concat(cur_instr,5,5,6) | binary_concat(cur_instr,12,10,3));
                    uint8_t rs1 = 8 + binary_concat(cur_instr,9,7,0);
                    uint64_t mem_addr = GPR[rs1] + imm;
                    uint8_t rd = 8 + binary_concat(cur_instr,4,2,0);
                    int32_t buf;
                    bool ok = mem_read(mem_addr,4,(unsigned char*)&buf);
                    if (ok) set_GPR(rd,buf);
                    break;
                }
                case OPCODE_C_LD: {
                    uint64_t imm = (binary_concat(cur_instr,6,5,6) | binary_concat(cur_instr,12,10,3));
                    uint8_t rs1 = 8 + binary_concat(cur_instr,9,7,0);
                    uint64_t mem_addr = GPR[rs1] + imm;
                    uint8_t rd = 8 + binary_concat(cur_instr,4,2,0);
                    int64_t buf;
                    bool ok = mem_read(mem_addr,8,(unsigned char*)&buf);
                    if (ok) set_GPR(rd,buf);
                    break;
                }
                case OPCODE_C_SW: {
                    uint64_t imm = (binary_concat(cur_instr,6,6,2) | binary_concat(cur_instr,5,5,6) | binary_concat(cur_instr,12,10,3));
                    uint8_t rs1 = 8 + binary_concat(cur_instr,9,7,0);
                    uint64_t mem_addr = GPR[rs1] + imm;
                    uint8_t rs2 = 8 + binary_concat(cur_instr,4,2,0);
                    mem_write(mem_addr,8,(unsigned char*)&GPR[rs2]);
                    break;
                }
                case OPCODE_C_SD: {
                    uint64_t imm = binary_concat(cur_instr,6,5,6) | binary_concat(cur_instr,12,10,3);
                    uint8_t rs1 = 8 + binary_concat(cur_instr,9,7,0);
                    uint64_t mem_addr = GPR[rs1] + imm;
                    uint8_t rs2 = 8 + binary_concat(cur_instr,4,2,0);
                    mem_write(mem_addr,8,(unsigned char*)&GPR[rs2]);
                    break;
                }
                case OPCODE_C_ADDI: {
                    uint64_t imm = binary_concat(cur_instr,12,12,5) | binary_concat(cur_instr,6,2,0);
                    if (imm >> 5) imm |= 0xffffffffffffffc0u; // sign extend[5]
                    uint8_t rd = binary_concat(cur_instr,11,7,0);
                    if (imm) set_GPR(rd,alu_exec(GPR[rd],imm,ALU_ADD)); // nzimm
                    break;
                }
                case OPCODE_C_ADDIW: {
                    uint64_t imm = binary_concat(cur_instr,12,12,5) | binary_concat(cur_instr,6,2,0);
                    if (imm >> 5) imm |= 0xffffffffffffffc0u; // sign extend[5]
                    uint8_t rd = binary_concat(cur_instr,11,7,0);
                    set_GPR(rd,alu_exec(GPR[rd],imm,ALU_ADD,true));
                    break;
                }
                case OPCODE_C_LI: {
                    int64_t imm = binary_concat(cur_instr,12,12,5) | binary_concat(cur_instr,6,2,0);
                    if (imm >> 5) imm |= 0xffffffffffffffc0u; // sign extend[5]
                    uint8_t rd = binary_concat(cur_instr,11,7,0);
                    set_GPR(rd,imm);
                    break;
                }
                case OPCODE_C_ADDI16SPN_LUI: {
                    uint8_t rd = binary_concat(cur_instr,11,7,0);
                    if (rd == 2)  { // ADDI16SPN
                        int64_t imm = binary_concat(cur_instr,12,12,9) | binary_concat(cur_instr,6,6,4) | binary_concat(cur_instr,5,5,6) | binary_concat(cur_instr,4,3,7) | binary_concat(cur_instr,2,2,5);
                        if (imm >> 9) imm |= 0xfffffffffffffc00u; // sign extend[9]
                        uint64_t value = GPR[rd] + imm;
                        if (imm) set_GPR(rd,value); // nzimm
                    }
                    else { // LUI
                        int64_t imm = binary_concat(cur_instr,12,12,17) | binary_concat(cur_instr,6,2,12);
                        if (imm >> 17) imm |= 0xfffffffffffc0000u; // sign extend[17]
                        if (imm) set_GPR(rd,imm); // nzimm
                    }
                    break;
                }
                case OPCODE_C_ALU: {
                    bool is_srli_srai = !(binary_concat(cur_instr,11,11,0));
                    uint8_t rs1 = 8 + binary_concat(cur_instr,9,7,0);
                    if (is_srli_srai) { // SRLI, SRAI
                        bool is_srai = binary_concat(cur_instr,10,10,0);
                        uint64_t imm = binary_concat(cur_instr,12,12,5) | binary_concat(cur_instr,6,2,0);
                        if (imm) { // nzimm
                            if (is_srai) {
                                set_GPR(rs1,alu_exec(GPR[rs1],imm,ALU_SRA));
                            }
                            else {
                                set_GPR(rs1,alu_exec(GPR[rs1],imm,ALU_SRL));
                            }
                        }
                    }
                    else {
                        bool is_andi = !binary_concat(cur_instr,10,10,0);
                        if (is_andi) {
                            int64_t imm = binary_concat(cur_instr,12,12,5) | binary_concat(cur_instr,6,2,0);
                            if (imm >> 5) imm |= 0xffffffffffffffc0u; // sign extend[5]
                            set_GPR(rs1,alu_exec(GPR[rs1],imm,ALU_AND));
                        }
                        else {
                            uint8_t rs2 = 8 + binary_concat(cur_instr,4,2,0);
                            uint8_t funct2 = binary_concat(cur_instr,6,5,0);
                            bool instr_12 = binary_concat(cur_instr,12,12,0);
                            switch (funct2) {
                                case FUNCT2_SUB:
                                    set_GPR(rs1,alu_exec(GPR[rs1],GPR[rs2],ALU_SUB,instr_12));
                                    break;
                                case FUNCT2_XOR_ADDW:
                                    set_GPR(rs1,alu_exec(GPR[rs1],GPR[rs2],instr_12?ALU_ADD:ALU_XOR,instr_12));
                                    break;
                                case FUNCT2_OR: {
                                    if (instr_12) ri = true;
                                    else set_GPR(rs1,alu_exec(GPR[rs1],GPR[rs2],ALU_OR));
                                    break;
                                }
                                case FUNCT2_AND: {
                                    if (instr_12) ri = true;
                                    else set_GPR(rs1,alu_exec(GPR[rs1],GPR[rs2],ALU_AND));
                                    break;
                                }
                            }
                        }
                    }
                    break;
                }
                case OPCODE_C_J: {
                    int64_t imm =   binary_concat(cur_instr,12,12,11) | binary_concat(cur_instr,11,11,4) |
                                    binary_concat(cur_instr,10,9,8) | binary_concat(cur_instr,8,8,10) |
                                    binary_concat(cur_instr,7,7,6) | binary_concat(cur_instr,6,6,7) |
                                    binary_concat(cur_instr,5,3,1) | binary_concat(cur_instr,2,2,5);
                    if (imm >> 11) imm |= 0xfffffffffffff000u; // sign extend [11]
                    uint64_t npc = pc + imm;
                    set_GPR(1,pc + 2);
                    pc = npc;
                    new_pc = true;
                    break;
                }
                case OPCODE_C_BEQZ: {
                    int64_t imm =   binary_concat(cur_instr,12,12,8) | binary_concat(cur_instr,11,10,3) | binary_concat(cur_instr,6,5,6) | binary_concat(cur_instr,4,3,1) | binary_concat(cur_instr,2,2,5);
                    if (imm>>8) imm |= 0xfffffffffffffe00u; // sign extend [8]
                    uint8_t rs1 = 8 + binary_concat(cur_instr,9,7,0);
                    if (GPR[rs1] == 0) {
                        pc = pc + imm;
                        new_pc = true;
                    }
                    break;
                }
                case OPCODE_C_BNEZ: {
                    int64_t imm =   binary_concat(cur_instr,12,12,8) | binary_concat(cur_instr,11,10,3) | binary_concat(cur_instr,6,5,6) | binary_concat(cur_instr,4,3,1) | binary_concat(cur_instr,2,2,5);
                    if (imm>>8) imm |= 0xfffffffffffffe00u; // sign extend [8]
                    uint8_t rs1 = 8 + binary_concat(cur_instr,9,7,0);
                    if (GPR[rs1] != 0) {
                        pc = pc + imm;
                        new_pc = true;
                    }
                    break;
                }
                case OPCODE_C_SLLI: {
                    int64_t imm =   binary_concat(cur_instr,12,12,5) | binary_concat(cur_instr,6,2,0);
                    uint8_t rs1 = binary_concat(cur_instr,11,7,0);
                    if (imm) set_GPR(rs1,alu_exec(rs1,imm,ALU_SLL)); // nzimm
                    break;
                }
                case OPCODE_C_LWSP: {
                    uint64_t imm = (binary_concat(cur_instr,6,4,2) | binary_concat(cur_instr,3,2,6) | binary_concat(cur_instr,12,12,5));
                    uint64_t mem_addr = GPR[2] + imm;
                    uint8_t rd = binary_concat(cur_instr,11,7,0);
                    int32_t buf;
                    bool ok = mem_read(mem_addr,4,(unsigned char*)&buf);
                    if (ok) set_GPR(rd,buf);
                    // TODO: rd != 0
                    break;
                }
                case OPCODE_C_LDSP: {
                    uint64_t imm = (binary_concat(cur_instr,6,5,3) | binary_concat(cur_instr,4,2,6) | binary_concat(cur_instr,12,12,5));
                    uint64_t mem_addr = GPR[2] + imm;
                    uint8_t rd = binary_concat(cur_instr,11,7,0);
                    int64_t buf;
                    bool ok = mem_read(mem_addr,8,(unsigned char*)&buf);
                    if (ok) set_GPR(rd,buf);
                    // TODO: rd != 0
                    break;
                }
                case OPCODE_C_JR_MV_EB_JALR_ADD: {
                    bool is_ebreak_jalr_add = binary_concat(cur_instr,12,12,0);
                    uint8_t rs2 = binary_concat(cur_instr,6,2,0);
                    uint8_t rs1 = binary_concat(cur_instr,11,7,0);
                    if (is_ebreak_jalr_add) {
                        if (rs2 == 0) { // EBREAK, JALR
                            if (rs1 == 0) { // EBREAK
                                priv.ebreak();
                            }
                            else { // JALR
                                uint64_t npc = GPR[rs1];
                                if (npc & 1) npc ^= 1;
                                if (npc % PC_ALIGN) priv.raise_trap(csr_cause_def(exc_instr_misalign),npc);
                                else {
                                    set_GPR(inst->j_type.rd,pc + 4);
                                    pc = npc;
                                    new_pc = true;
                                }
                            }
                        }
                        else { // ADD
                            set_GPR(rs1,alu_exec(GPR[rs1],GPR[rs2],ALU_ADD));
                        }
                    }
                    else {
                        if (rs2 == 0) { // JR
                            if (rs1 == 0) ri = true;
                            else {
                                uint64_t npc = GPR[rs1];
                                if (npc & 1) npc ^= 1;
                                if (npc % PC_ALIGN) priv.raise_trap(csr_cause_def(exc_instr_misalign),npc);
                                else {
                                    set_GPR(inst->j_type.rd,pc + 2);
                                    pc = npc;
                                    new_pc = true;
                                }
                            }
                        }
                        else { // MV
                            set_GPR(rs1,GPR[rs2]);
                            // TODO: rs1(rd) != 0
                        }
                    }
                    break;
                }
                case OPCODE_C_SWSP: {
                    uint64_t imm = (binary_concat(cur_instr,12,9,2) | binary_concat(cur_instr,8,7,6));
                    uint64_t mem_addr = GPR[2] + imm;
                    uint8_t rs2 = binary_concat(cur_instr,6,2,0);
                    mem_write(mem_addr,4,(unsigned char*)&GPR[rs2]);
                    break;
                }
                case OPCODE_C_SDSP: {
                    uint64_t imm = (binary_concat(cur_instr,12,10,3) | binary_concat(cur_instr,9,7,6));
                    uint64_t mem_addr = GPR[2] + imm;
                    uint8_t rs2 = binary_concat(cur_instr,6,2,0);
                    mem_write(mem_addr,8,(unsigned char*)&GPR[rs2]);
                    break;
                }
                default:
                    ri = true;
            }
        }
    exception:
        if (ri) {
            priv.raise_trap(csr_cause_def(exc_illegal_instr),cur_instr);
        }
        if (priv.need_trap()) {
            pc = priv.get_trap_pc();
        }
        else if (!new_pc) pc = pc + (is_rvc ? 2 : 4);
        priv.post_exec();
    }
    bool mem_read(uint64_t start_addr, uint64_t size, uint8_t *buffer) {
        if (start_addr % size != 0) {
            priv.raise_trap(csr_cause_def(exc_load_misalign),start_addr);
            return false;
        }
        rv_exc_code va_err = priv.va_read(start_addr,size,buffer);
        if (va_err == exc_custom_ok) {
            return true;

        }
        else {
            priv.raise_trap(csr_cause_def(va_err),start_addr);
            return false;
        }
    }
    bool mem_write(uint64_t start_addr, uint64_t size, const uint8_t *buffer) {
        if (start_addr % size != 0) {
            priv.raise_trap(csr_cause_def(exc_store_misalign),start_addr);
            return false;
        }
        rv_exc_code va_err = priv.va_write(start_addr,size,buffer);
        if (va_err == exc_custom_ok) {
            return true;
        }
        else {
            priv.raise_trap(csr_cause_def(va_err),start_addr);
            return false;
        }
    }
    int64_t alu_exec(int64_t a, int64_t b, alu_op op, bool op_32 = false) {
        if (op_32) {
            a = (int32_t)a;
            b = (int32_t)b;
        }
        int64_t result = 0;
        switch (op) {
            case ALU_ADD:
                result = a + b;
                break;
            case ALU_SUB:
                result = a - b;
                break;
            case ALU_SLL:
                result = a << (b & (op_32 ? 0x1f : 0x3f));
                break;
            case ALU_SLT:
                result = a < b;
                break;
            case ALU_SLTU:
                result = (uint64_t)a < (uint64_t)b;
                break;
            case ALU_XOR:
                result = a ^ b;
                break;
            case ALU_SRL:
                result = (uint64_t)(op_32 ? (a&0xffffffff) : a) >> (b & (op_32 ? 0x1f : 0x3f));
                break;
            case ALU_SRA:
                result = a >> (b & (op_32 ? 0x1f : 0x3f));
                break;
            case ALU_OR:
                result = a | b;
                break;
            case ALU_AND:
                result = a & b;
                break;
            case ALU_MUL:
                result = a * b;
                break;
            case ALU_MULH:
                result = ((__int128_t)a*(__int128_t)b) >> 64;
                break;
            case ALU_MULHU: 
                result = (static_cast<__uint128_t>(static_cast<uint64_t>(a))*static_cast<__uint128_t>(static_cast<uint64_t>(b))) >> 64;
                break;
            case ALU_MULHSU:
                result = (static_cast<__int128_t>(a)*static_cast<__uint128_t>(static_cast<uint64_t>(b))) >> 64;
                break;
            case ALU_DIV:
                if (b == 0) result = -1;
                else if (a == LONG_MIN && b == -1) result = LONG_MIN;
                else result = a / b;
                break;
            case ALU_DIVU:
                if (b == 0) result = ULONG_MAX;
                else if (op_32) result = (uint32_t)a / (uint32_t)b;
                else result = ((uint64_t)a) / ((uint64_t)b);
                break;
            case ALU_REM:
                if (b == 0) result = a;
                else if (a == LONG_MIN && b == -1) result = 0;
                else result = a % b;
                break;
            case ALU_REMU:
                if (b == 0) result = a;
                else if (op_32) result = (uint32_t)a % (uint32_t)b;
                else result = (uint64_t)a % (uint64_t)b;
                break;
            default:
                assert(false);
        }
        if (op_32) result = (int32_t)result;
        return result;
    }
};

#endif
