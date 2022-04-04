#ifndef RV_CORE_HPP
#define RV_CORE_HPP

#include <core.hpp>
#include <bitset>
#include "rv_common.hpp"
#include <assert.h>
#include "rv_systembus.hpp"

enum mem_err_code {
    MEM_OK = 0,
    MEM_ERR_LOAD_ACCESS_FAULT   = 5,
    MEM_ERR_STORE_ACCESS_FAULT  = 7,
    MEM_ERR_LOAD_PGFAULT        = 13,
    MEM_ERR_STORE_PGFAULT       = 15,
};

enum alu_op {
    ALU_ADD, ALU_SUB, ALU_SLL, ALU_SLT, ALU_SLTU, ALU_XOR, ALU_SRL, ALU_SRA, ALU_OR, ALU_AND,
    ALU_MUL, ALU_MULH, ALU_MULHU, ALU_MULHSU, ALU_DIV, ALU_DIVU, ALU_REM, ALU_REMU,
    ALU_NOP
};

class rv_core : public core {
public:
    rv_core(rv_systembus &systembus):systembus(systembus) {
        GPR[0] = 0;
    }
    void step() {
        exec();
    }
private:
    rv_systembus &systembus;
    uint64_t pc = 0;
    int64_t GPR[32];
    void exec() {
    instr_fetch:
        bool instr_align = ( (pc % 4) == 0);
        uint32_t cur_instr;
        union rv_instr *inst = (rv_instr*)&cur_instr;
        mem_err_code instr_err = mem_read(pc,4,(unsigned char*)inst);
    decode_exec:
        bool ri = false;
        bool new_pc = false;
        switch (inst->r_type.opcode) {
            case OPCODE_LUI:
                set_GPR(inst->u_type.rd,((int64_t)inst->u_type.imm_31_12) << 12);
                break;
            case OPCODE_AUIPC:
                set_GPR(inst->u_type.rd,(((int64_t)inst->u_type.imm_31_12) << 12) + pc);
                break;
            case OPCODE_JAL:
                set_GPR(inst->j_type.rd,pc + 4);
                pc += (inst->j_type.imm_20 << 20) | (inst->j_type.imm_19_12 << 12) | (inst->j_type.imm_11 << 11) | (inst->j_type.imm_10_1 << 1);
                new_pc = true;
                break;
            case OPCODE_JALR:
                set_GPR(inst->j_type.rd,pc + 4);
                pc =  GPR[inst->i_type.rs1] + GPR[inst->i_type.imm12];
                new_pc = true;
                break;
            case OPCODE_BRANCH: {
                int64_t offset = (inst->b_type.imm_12 << 12) | (inst->b_type.imm_11 << 11) | (inst->b_type.imm_10_5 << 5) | (inst->b_type.imm_4_1 << 1);
                switch (inst->b_type.funct3) {
                    case FUNCT3_BEQ:
                        if (GPR[inst->b_type.rs1] == GPR[inst->b_type.rs2]) {
                            pc += offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BNE:
                        if (GPR[inst->b_type.rs1] != GPR[inst->b_type.rs2]) {
                            pc += offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BLT:
                        if (GPR[inst->b_type.rs1] < GPR[inst->b_type.rs2]) {
                            pc += offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BGE:
                        if (GPR[inst->b_type.rs1] >= GPR[inst->b_type.rs2]) {
                            pc += offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BLTU:
                        if ((uint64_t)GPR[inst->b_type.rs1] < (uint64_t)GPR[inst->b_type.rs2]) {
                            pc += offset;
                            new_pc = true;
                        }
                        break;
                    case FUNCT3_BGEU:
                        if ((uint64_t)GPR[inst->b_type.rs1] >= (uint64_t)GPR[inst->b_type.rs2]) {
                            pc += offset;
                            new_pc = true;
                        }
                        break;
                    default:
                        ri = true;
                }
                break;
            }
            case OPCODE_LOAD: {
                uint64_t mem_addr = GPR[inst->i_type.rs1] + (inst->i_type.imm12);
                switch (inst->i_type.funct3) {
                    case FUNCT3_LB: {
                        int8_t buf;
                        mem_read(mem_addr,1,(unsigned char*)&buf);
                        set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LH: {
                        int16_t buf;
                        mem_read(mem_addr,2,(unsigned char*)&buf);
                        set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LW: {
                        int32_t buf;
                        mem_read(mem_addr,4,(unsigned char*)&buf);
                        set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LD: {
                        int64_t buf;
                        mem_read(mem_addr,8,(unsigned char*)&buf);
                        set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LBU: {
                        uint8_t buf;
                        mem_read(mem_addr,1,(unsigned char*)&buf);
                        set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LHU: {
                        uint16_t buf;
                        mem_read(mem_addr,2,(unsigned char*)&buf);
                        set_GPR(inst->i_type.rd,buf);
                        break;
                    }
                    case FUNCT3_LWU: {
                        uint32_t buf;
                        mem_read(mem_addr,4,(unsigned char*)&buf);
                        set_GPR(inst->i_type.rd,buf);
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
                        op = ALU_SRL;
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
                funct6 fun6 = static_cast<funct6>((inst->r_type.funct7) >> 1);
                alu_op op;
                switch (inst->i_type.funct3) {
                    case FUNCT3_ADD_SUB:
                        op = ALU_ADD;
                        break;
                    case FUNCT3_SLL:
                        if (imm >= 64) ri = true;
                        op = ALU_SLL;
                        break;
                    case FUNCT3_SRL_SRA: 
                        imm = imm & ((1 << 6) - 1);
                        op = ALU_SRL;
                        if (fun6 != FUNCT6_NORMAL && fun6 != FUNCT6_SRA) ri = true;
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
            case OPCODE_FENCE:
                break;
            case OPCODE_SYSTEM: {
                ri = true;
                break;
            }
            default:
                ri = true;
                break;
        }
        exception:
        if (!new_pc) pc = pc + 4;
        assert(!ri);
    }
    mem_err_code mem_read(unsigned long start_addr, unsigned long size, unsigned char* buffer) {
        bool stat = systembus.pa_read(start_addr,size,buffer);
        assert(stat);
        return stat ? MEM_OK : MEM_ERR_LOAD_ACCESS_FAULT;
    }
    mem_err_code mem_write(unsigned long start_addr, unsigned long size, const unsigned char* buffer) {
        bool stat = systembus.pa_write(start_addr,size,buffer);
        assert(stat);
        return stat ? MEM_OK : MEM_ERR_STORE_ACCESS_FAULT;
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
                result = a << b;
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
                result = (uint64_t)a >> b;
                break;
            case ALU_SRA:
                result = a >> b;
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
                result = ((__uint128_t)a*(__uint128_t)b) >> 64;
                break;
            case ALU_MULHSU:
                result = ((__int128_t)a*(__uint128_t)b) >> 64;
                break;
            case ALU_DIV:
                if (b == 0) result = -1;
                else result = a / b;
                break;
            case ALU_DIVU:
                if (b == 0) result = ULONG_MAX;
                else result = ((uint64_t)a) / ((uint64_t)b);
                break;
            case ALU_REM:
                if (b == 0) result = a;
                else result = a % b;
                break;
            case ALU_REMU:
                if (b == 0) result = a;
                else result = (uint64_t)a % (uint64_t)b;
                break;
            default:
                assert(false);
        }
        if (op_32) result = (int32_t)result;
        return result;
    }
    void set_GPR(uint8_t GPR_index, int64_t value) {
        assert(GPR_index >= 0 && GPR_index < 32);
        if (GPR_index) GPR[GPR_index] = value;
    }
};

#endif
