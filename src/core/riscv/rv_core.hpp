#ifndef RV_CORE_HPP
#define RV_CORE_HPP

#include <bitset>
#include "rv_common.hpp"
#include <assert.h>
#include "rv_priv.hpp"
#include <deque>
#include <algorithm>
#include "clock_manager.hpp"
#include "cache_uni_def.hpp"

extern bool riscv_test;

enum alu_op {
    ALU_ADD, ALU_SUB, ALU_SLL, ALU_SLT, ALU_SLTU, ALU_XOR, ALU_SRL, ALU_SRA, ALU_OR, ALU_AND,
    ALU_MUL, ALU_MULH, ALU_MULHU, ALU_MULHSU, ALU_DIV, ALU_DIVU, ALU_REM, ALU_REMU,
    ALU_NOP
};

extern clock_manager <2> cm;

class rv_core {
public:
    rv_core(l2_cache <L2_WAYS,L2_NR_SETS,L2_SZLINE,32> &l2, uint8_t hart_id = 0):priv(hart_id,pc,l2),hart_id(hart_id) {
        for (int i=0;i<32;i++) GPR[i] = 0;
    }
    void step(bool meip, bool msip, bool mtip, bool seip) {
        exec(meip,msip,mtip,seip);
    }
    void jump(uint64_t new_pc) {
        pc = new_pc;
    }
    void set_GPR(uint8_t GPR_index, int64_t value, uint64_t time) {
        assert(GPR_index >= 0 && GPR_index < 32);
        if (GPR_index) {
            GPR[GPR_index] = value;
            if (GPR_time[GPR_index] < time) GPR_time[GPR_index] = time;
        }
    }
    uint64_t getPC() {
        return pc;
    }
private:
    uint32_t trace_size = riscv_test ? 128 : 0;
    std::queue <uint64_t> trace;
    uint64_t pc = 0;
    rv_priv priv;
    int64_t GPR[32];
    uint64_t GPR_time[32];
    uint64_t hart_id;
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
        uint32_t cur_instr = 0;
        rv_instr *inst = (rv_instr*)&cur_instr;
        rv_exc_code if_exc;
    instr_fetch:
        priv.pre_exec(meip,msip,mtip,seip);
        if (priv.need_trap()) {
            goto exception;
        }
        enter_if();
        if (pc % 4) {
            priv.raise_trap(csr_cause_def(exc_instr_misalign),pc);
            goto exception;
        }
        if_exc = priv.va_if(pc,4,(uint8_t*)&cur_instr);
        if (if_exc != exc_custom_ok) {
            priv.raise_trap(csr_cause_def(if_exc),pc);
            goto exception;
        }
    decode_exec:
        enter_id();
        switch (inst->r_type.opcode) {
            case OPCODE_LUI: {
                enter_ex();
                set_GPR(inst->u_type.rd,((int64_t)inst->u_type.imm_31_12) << 12, cm.pipe_ex[hart_id]);
                enter_mem();
                break;
            }
            case OPCODE_AUIPC: {
                enter_ex();
                set_GPR(inst->u_type.rd,(((int64_t)inst->u_type.imm_31_12) << 12) + pc, cm.pipe_ex[hart_id]);
                enter_mem();
                break;
            }
            case OPCODE_JAL: {
                enter_ex();
                uint64_t npc = pc + ((inst->j_type.imm_20 << 20) | (inst->j_type.imm_19_12 << 12) | (inst->j_type.imm_11 << 11) | (inst->j_type.imm_10_1 << 1));
                if (npc & 1) npc ^= 1; // we don't need to check.
                if (npc % 4) priv.raise_trap(csr_cause_def(exc_instr_misalign),npc);
                else {
                    set_GPR(inst->j_type.rd,pc + 4, cm.pipe_ex[hart_id]);
                    pc = npc;
                    new_pc = true;
                }
                enter_mem();
                break;
            }
            case OPCODE_JALR: {
                id_wait_rs(inst->i_type.rs1);
                enter_ex();
                uint64_t npc = GPR[inst->i_type.rs1] + inst->i_type.imm12;
                if (npc & 1) npc ^= 1;
                if (npc % 4) priv.raise_trap(csr_cause_def(exc_instr_misalign),npc);
                else {
                    set_GPR(inst->j_type.rd,pc + 4, cm.pipe_ex[hart_id]);
                    pc = npc;
                    new_pc = true;
                }
                enter_mem();
                break;
            }
            case OPCODE_BRANCH: {
                id_wait_rs(inst->b_type.rs1);
                id_wait_rs(inst->b_type.rs2);
                enter_ex();
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
                    if (npc % 4) priv.raise_trap(csr_cause_def(exc_instr_misalign),npc);
                    else pc = npc;
                }
                enter_mem();
                break;
            }
            case OPCODE_LOAD: {
                id_wait_rs(inst->i_type.rs1);
                enter_ex();
                uint64_t mem_addr = GPR[inst->i_type.rs1] + (inst->i_type.imm12);
                enter_mem();
                switch (inst->i_type.funct3) {
                    case FUNCT3_LB: {
                        int8_t buf;
                        bool ok = mem_read(mem_addr,1,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf,cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_LH: {
                        int16_t buf;
                        bool ok = mem_read(mem_addr,2,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf,cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_LW: {
                        int32_t buf;
                        bool ok = mem_read(mem_addr,4,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf,cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_LD: {
                        int64_t buf;
                        bool ok = mem_read(mem_addr,8,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf,cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_LBU: {
                        uint8_t buf;
                        bool ok = mem_read(mem_addr,1,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf,cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_LHU: {
                        uint16_t buf;
                        bool ok = mem_read(mem_addr,2,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf,cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_LWU: {
                        uint32_t buf;
                        bool ok = mem_read(mem_addr,4,(unsigned char*)&buf);
                        if (ok) set_GPR(inst->i_type.rd,buf,cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_STORE: {
                id_wait_rs(inst->s_type.rs1);
                id_wait_rs(inst->s_type.rs2);
                enter_ex();
                uint64_t mem_addr = GPR[inst->s_type.rs1] + ( (inst->s_type.imm_11_5 << 5) | (inst->s_type.imm_4_0));
                enter_mem();
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
                id_wait_rs(inst->r_type.rs1);
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
                enter_ex();
                if (!ri) {
                    int64_t result = alu_exec(GPR[inst->r_type.rs1],imm,op);
                    set_GPR(inst->r_type.rd,result, cm.pipe_ex[hart_id]);
                }
                enter_mem();
                break;
            }
            case OPCODE_OPIMM32: {
                id_wait_rs(inst->r_type.rs1);
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
                enter_ex();
                if (!ri) {
                    int64_t result = alu_exec(GPR[inst->r_type.rs1],imm,op,true);
                    set_GPR(inst->r_type.rd,result, cm.pipe_ex[hart_id]);
                }
                enter_mem();
                break;
            }
            case OPCODE_OP: {
                id_wait_rs(inst->r_type.rs1);
                id_wait_rs(inst->r_type.rs2);
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
                enter_ex();
                if (!ri) {
                    int64_t result = alu_exec(GPR[inst->r_type.rs1],GPR[inst->r_type.rs2],op);
                    set_GPR(inst->r_type.rd,result, cm.pipe_ex[hart_id]);
                }
                enter_mem();
                break;
            }
            case OPCODE_OP32: {
                id_wait_rs(inst->r_type.rs1);
                id_wait_rs(inst->r_type.rs2);
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
                enter_ex();
                if (!ri) {
                    int64_t result = alu_exec(GPR[inst->r_type.rs1],GPR[inst->r_type.rs2],op,true);
                    set_GPR(inst->r_type.rd,result, cm.pipe_ex[hart_id]);
                }
                enter_mem();
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
                        id_wait_rs(inst->r_type.rs1);
                        enter_ex();
                        enter_mem();
                        if (inst->r_type.rs2 != 0) ri = true;
                        else {
                            if (inst->r_type.funct3 == 0b011) {
                                int64_t result;
                                rv_exc_code exc = priv.va_lr(GPR[inst->r_type.rs1],(1<<inst->r_type.funct3),(uint8_t*)&result);
                                if (exc == exc_custom_ok) set_GPR(inst->r_type.rd,result, cm.pipe_mem[hart_id]);
                                else priv.raise_trap(csr_cause_def(exc),GPR[inst->r_type.rs1]);
                            }
                            else {
                                int32_t result;
                                rv_exc_code exc = priv.va_lr(GPR[inst->r_type.rs1],(1<<inst->r_type.funct3),(uint8_t*)&result);
                                if (exc == exc_custom_ok) set_GPR(inst->r_type.rd,result, cm.pipe_mem[hart_id]);
                                else priv.raise_trap(csr_cause_def(exc),GPR[inst->r_type.rs1]);
                            }
                        }
                        break;
                    }
                    case AMOSC: {
                        id_wait_rs(inst->r_type.rs1);
                        id_wait_rs(inst->r_type.rs2);
                        enter_ex();
                        enter_mem();
                        bool result;
                        rv_exc_code exc = priv.va_sc(GPR[inst->r_type.rs1],(1<<inst->r_type.funct3),(uint8_t*)&GPR[inst->r_type.rs2],result);
                        if (exc == exc_custom_ok) set_GPR(inst->r_type.rd,result, cm.pipe_mem[hart_id]);
                        else priv.raise_trap(csr_cause_def(exc),GPR[inst->r_type.rs1]);
                        break;
                    }
                    case AMOSWAP: case AMOADD: case AMOXOR: case AMOAND: case AMOOR: case AMOMIN: case AMOMAX: case AMOMINU: case AMOMAXU: {
                        id_wait_rs(inst->r_type.rs1);
                        id_wait_rs(inst->r_type.rs2);
                        enter_ex();
                        enter_mem();
                        int64_t result;
                        rv_exc_code exc = priv.va_amo(GPR[inst->r_type.rs1],(1<<inst->r_type.funct3),static_cast<amo_funct>(funct5),GPR[inst->r_type.rs2],result);
                        if (exc == exc_custom_ok) set_GPR(inst->r_type.rd,result, cm.pipe_mem[hart_id]);
                        else priv.raise_trap(csr_cause_def(exc),GPR[inst->r_type.rs1]);
                        break;
                    }
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_FENCE: {
                enter_ex();
                enter_mem();
                switch (inst->i_type.funct3) {
                    case FUNCT3_FENCE:
                        break;
                    case FUNCT3_FENCE_I:
                        priv.fence_i();
                        break;
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_SYSTEM: {
                switch (inst->i_type.funct3) {
                    case FUNCT3_PRIV: {
                        uint64_t funct7 = ((inst->i_type.imm12) & ((1<<12)-1)) >> 5;
                        uint64_t rs2 = (inst->i_type.imm12) & ((1<<5)-1);
                        switch (funct7) {
                            case FUNCT7_ECALL_EBREAK: {
                                id_wait_rs(inst->r_type.rs1);
                                enter_ex();
                                enter_mem();
                                cm.pipe_mem[hart_id] += 1; // takes 1 clock to write CSR
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
                                enter_ex();
                                enter_mem();
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
                                enter_ex();
                                enter_mem();
                                cm.pipe_mem[hart_id] += 1; // takes 1 clock to write CSR
                                if (rs2 == 0b00010 && inst->r_type.rs1 == 0 && inst->r_type.rd == 0) {
                                    ri = !priv.mret();
                                }
                                else ri = true;
                                break;
                            }
                            case FUNCT7_SFENCE_VMA:
                                enter_ex();
                                enter_mem();
                                cm.pipe_mem[hart_id] += 1; // takes 1 clock to write TLB
                                ri = !priv.sfence_vma(GPR[inst->r_type.rs1],GPR[inst->r_type.rs2] & 0xffff);
                                if (!ri) flush_pipe();
                                break;
                            default:
                                ri = true;
                        }
                        break;
                    }
                    case FUNCT3_CSRRW: {
                        id_wait_rs(inst->i_type.rs1);
                        enter_ex();
                        enter_mem();
                        cm.pipe_mem[hart_id] += 1; // takes 1 clock to write CSR
                        flush_pipe();
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri) ri = !priv.csr_write(csr_index,GPR[inst->i_type.rs1]);
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result, cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_CSRRS: {
                        id_wait_rs(inst->i_type.rs1);
                        enter_ex();
                        enter_mem();
                        cm.pipe_mem[hart_id] += 1; // takes 1 clock to write CSR
                        flush_pipe();
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri && inst->i_type.rs1) ri = !priv.csr_write(csr_index,csr_result | GPR[inst->i_type.rs1]);
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result, cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_CSRRC: {
                        id_wait_rs(inst->i_type.rs1);
                        enter_ex();
                        enter_mem();
                        cm.pipe_mem[hart_id] += 1; // takes 1 clock to write CSR
                        flush_pipe();
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri && inst->i_type.rs1) ri = !priv.csr_write(csr_index,csr_result & (~GPR[inst->i_type.rs1]));
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result, cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_CSRRWI: {
                        enter_ex();
                        enter_mem();
                        cm.pipe_mem[hart_id] += 1; // takes 1 clock to write CSR
                        flush_pipe();
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri) ri = !priv.csr_write(csr_index,inst->i_type.rs1);
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result, cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_CSRRSI: {
                        enter_ex();
                        enter_mem();
                        cm.pipe_mem[hart_id] += 1; // takes 1 clock to write CSR
                        flush_pipe();
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri && inst->i_type.rs1) ri = !priv.csr_write(csr_index,csr_result | inst->i_type.rs1);
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result, cm.pipe_mem[hart_id] + 1);
                        break;
                    }
                    case FUNCT3_CSRRCI: {
                        enter_ex();
                        enter_mem();
                        cm.pipe_mem[hart_id] += 1; // takes 1 clock to write CSR
                        flush_pipe();
                        rv_csr_addr csr_index = static_cast<rv_csr_addr>(inst->i_type.imm12&((1<<12)-1));
                        ri = !priv.csr_op_permission_check(csr_index,inst->i_type.rs1 != 0);
                        uint64_t csr_result;
                        if (!ri) ri = !priv.csr_read(csr_index,csr_result);
                        if (!ri && inst->i_type.rs1) ri = !priv.csr_write(csr_index,csr_result & (~(inst->i_type.rs1)));
                        if (!ri && inst->i_type.rd) set_GPR(inst->i_type.rd,csr_result, cm.pipe_mem[hart_id] + 1);
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
    exception:
        if (ri) {
            priv.raise_trap(csr_cause_def(exc_illegal_instr),cur_instr);
        }
        if (priv.need_trap()) {
            pc = priv.get_trap_pc();
            cm.pipe_mem[hart_id] += 1;
            flush_pipe();
        }
        else if (!new_pc) pc = pc + 4;
        else flush_pipe();
        enter_wb();
        priv.post_exec();
    }
    // clock modeling control
    void enter_if() {
        cm.pipe_if[hart_id] = cm.pipe_if[hart_id] + 1;
    }
    void enter_id() {
        cm.pipe_id[hart_id] = std::max(cm.pipe_if[hart_id] + 1, cm.pipe_id[hart_id] + 1);
    }
    void enter_ex() {
        cm.pipe_ex[hart_id] = std::max(cm.pipe_id[hart_id] + 1, cm.pipe_ex[hart_id] + 1);
    }
    void enter_mem() {
        cm.pipe_mem[hart_id] = std::max(cm.pipe_ex[hart_id] + 1, cm.pipe_mem[hart_id] + 1);
    }
    void enter_wb() {
        cm.pipe_wb[hart_id] = std::max(cm.pipe_mem[hart_id] + 1, cm.pipe_wb[hart_id] + 1);
    }
    void flush_pipe() { // assume pipe flush is at mem
        cm.pipe_if[hart_id] = std::max(cm.pipe_mem[hart_id] + 1, cm.pipe_if[hart_id] + 1);
        // after this, instruction fetch clock is set to pc gen, and did increase when enter_if() being called
    }
    void id_wait_rs(uint8_t GPR_index) {
        if (GPR_time[GPR_index] < cm.pipe_id[hart_id]) cm.pipe_id[hart_id] = GPR_time[GPR_index];
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
            case ALU_MUL: {
                cm.pipe_ex[hart_id] += 2;
                result = a * b;
                break;
            }
            case ALU_MULH: {
                cm.pipe_ex[hart_id] += 2;
                result = ((__int128_t)a*(__int128_t)b) >> 64;
                break;
            }
            case ALU_MULHU: {
                cm.pipe_ex[hart_id] += 2;
                result = (static_cast<__uint128_t>(static_cast<uint64_t>(a))*static_cast<__uint128_t>(static_cast<uint64_t>(b))) >> 64;
                break;
            }
            case ALU_MULHSU: {
                cm.pipe_ex[hart_id] += 2;
                result = (static_cast<__int128_t>(a)*static_cast<__uint128_t>(static_cast<uint64_t>(b))) >> 64;
                break;
            }
            case ALU_DIV: {
                if (b == 0) result = -1;
                else if (a == LONG_MIN && b == -1) result = LONG_MIN;
                else {
                    cm.pipe_ex[hart_id] += 65;
                    result = a / b;
                }
                break;
            }
            case ALU_DIVU: {
                if (b == 0) result = ULONG_MAX;
                else if (op_32) {
                    cm.pipe_ex[hart_id] += 65;
                    result = (uint32_t)a / (uint32_t)b;
                }
                else {
                    cm.pipe_ex[hart_id] += 65;
                    result = ((uint64_t)a) / ((uint64_t)b);
                }
                break;
            }
            case ALU_REM: {
                if (b == 0) result = a;
                else if (a == LONG_MIN && b == -1) result = 0;
                else {
                    cm.pipe_ex[hart_id] += 65;
                    result = a % b;
                }
                break;
            }
            case ALU_REMU: {
                if (b == 0) result = a;
                else if (op_32) {
                    cm.pipe_ex[hart_id] += 65;
                    result = (uint32_t)a % (uint32_t)b;
                }
                else {
                    cm.pipe_ex[hart_id] += 65;
                    result = (uint64_t)a % (uint64_t)b;
                }
                break;
            }
            default:
                assert(false);
        }
        if (op_32) result = (int32_t)result;
        return result;
    }
};

#endif
