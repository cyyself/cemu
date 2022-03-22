#include <core.hpp>
#include <memory.hpp>
#include <bitset>
#include "rv_instr.hpp"
#include <assert.h>

enum mem_err_code {
    MEM_OK = 0,
    MEM_ERR_LOAD_ACCESS_FAULT   = 5,
    MEM_ERR_STORE_ACCESS_FAULT  = 7,
    MEM_ERR_LOAD_PGFAULT        = 13,
    MEM_ERR_STORE_PGFAULT       = 15,
};

class rv_core : public core {
public:
    rv_core(memory &mem, memory &mmio, std::bitset<4> &irq):mem(mem),mmio(mmio),irq(irq) {
        GPR[0] = 0;
    }
    void step() {
        exec();
    }
private:
    memory &mem;
    memory &mmio;
    std::bitset<4> &irq;
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
                bool funct7_m_2 = false;
                switch (inst->i_type.funct3) {
                    case FUNCT3_SRL_SRA:
                        if (fun6 == FUNCT6_SRA) funct7_m_2 = true;
                        else if (fun6 != FUNCT6_NORMAL) ri = true;
                        break;
                    case FUNCT3_SLL:
                        if (fun6 != FUNCT6_NORMAL) ri = true;
                        break;
                    default:
                        break;
                }
                if (!ri) set_GPR(inst->i_type.rd,alu_exec(GPR[inst->i_type.rs1],imm,static_cast<funct3_op>(inst->i_type.funct3),funct7_m_2));
                break;
            }
            case OPCODE_OPIMM32: {
                int64_t imm = inst->i_type.imm12;
                bool funct7_m_2 = false;
                funct6 fun6 = static_cast<funct6>((inst->r_type.funct7) >> 1);
                switch (inst->i_type.funct3) {
                    case FUNCT3_SRL_SRA:
                        if (fun6 == FUNCT6_SRA) funct7_m_2 = true;
                        else if (fun6 != FUNCT6_NORMAL) ri = true;
                        break;
                    case FUNCT3_SLL:
                        if (fun6 != FUNCT6_NORMAL) ri = true;
                        break;
                    case FUNCT3_ADD_SUB:
                        break;
                    default:
                        ri = true;
                }
                if (!ri) set_GPR(inst->i_type.rd,alu_exec(GPR[inst->i_type.rs1],imm,static_cast<funct3_op>(inst->i_type.funct3),funct7_m_2,true));
                break;
            }
            case OPCODE_OP: {
                switch (inst->r_type.funct3) {
                    case FUNCT3_ADD_SUB:
                        if (inst->r_type.funct7 != FUNCT7_NORMAL && inst->r_type.funct7 != FUNCT7_SUB_SRA)  ri = true;
                        break;
                    case FUNCT3_SRL_SRA:
                        if (inst->r_type.funct7 != FUNCT7_NORMAL && inst->r_type.funct7 != FUNCT7_SUB_SRA)  ri = true;
                        break;
                    default:
                        if (inst->r_type.funct7 != FUNCT7_NORMAL) ri = true;
                }
                if (!ri) set_GPR(inst->r_type.rd,alu_exec(GPR[inst->r_type.rs1],GPR[inst->r_type.rs2],static_cast<funct3_op>(inst->r_type.funct3),inst->r_type.funct7 != 0));
                break;
            }
            case OPCODE_OP32: {
                switch (inst->r_type.funct3) {
                    case FUNCT3_ADD_SUB:
                        if (inst->r_type.funct7 != FUNCT7_NORMAL && inst->r_type.funct7 != FUNCT7_SUB_SRA)  ri = true;
                        break;
                    case FUNCT3_SLL:
                        if (inst->r_type.funct7 != FUNCT7_NORMAL)  ri = true;
                        break;
                    case FUNCT3_SRL_SRA:
                        if (inst->r_type.funct7 != FUNCT7_NORMAL && inst->r_type.funct7 != FUNCT7_SUB_SRA)  ri = true;
                        break;
                    default:
                        ri = true;
                }
                if (!ri) set_GPR(inst->r_type.rd,alu_exec(GPR[inst->r_type.rs1],GPR[inst->r_type.rs2],static_cast<funct3_op>(inst->r_type.funct3),inst->r_type.funct7 != 0));
                break;
            }
            case OPCODE_FENCE:
                break;
            case OPCODE_EEI:
                break;
            default:
                ri = true;
                break;
        }
        if (!new_pc) pc = pc + 4;
        assert(!ri);
    }
    mem_err_code mem_read(unsigned long start_addr, unsigned long size, unsigned char* buffer) {
        if (start_addr & 0xffffffff80000000lu == 0x80000000) {
            bool stat = mem.do_read(start_addr,size,buffer);
            assert(stat);
            return stat ? MEM_OK : MEM_ERR_LOAD_ACCESS_FAULT;
        }
        else {
            bool stat = mmio.do_read(start_addr,size,buffer);
            assert(stat);
            return stat ? MEM_OK : MEM_ERR_LOAD_ACCESS_FAULT;
        }
    }
    mem_err_code mem_write(unsigned long start_addr, unsigned long size, const unsigned char* buffer) {
        if (start_addr & 0xffffffff80000000lu == 0x80000000) {
            bool stat = mem.do_write(start_addr,size,buffer);
            assert(stat);
            return stat ? MEM_OK : MEM_ERR_STORE_ACCESS_FAULT;
        }
        else {
            bool stat = mmio.do_write(start_addr,size,buffer);
            assert(stat);
            return stat ? MEM_OK : MEM_ERR_STORE_ACCESS_FAULT;
        }
    }
    int64_t alu_exec(int64_t a, int64_t b, funct3_op op, bool funct7_m_2, bool op_32 = false) {
        if (op_32) {
            a = (int32_t)a;
            b = (int32_t)b;
        }
        switch (op) {
            case FUNCT3_ADD_SUB:
                return funct7_m_2 ? (a - b) : (a + b);
            case FUNCT3_SLL:
                return a << b;
            case FUNCT3_SLT:
                return a < b;
            case FUNCT3_SLTU:
                return (uint64_t)a < (uint64_t)b;
            case FUNCT3_XOR:
                return a ^ b;
            case FUNCT3_SRL_SRA:
                return funct7_m_2 ? (a >> b) : ((uint64_t)a >> b);
            case FUNCT3_OR:
                return (a | b);
            case FUNCT3_AND:
                return (a & b);
            default:
                return 0;
        }
    }
    void set_GPR(unsigned int GPR_index, long value) {
        if (GPR_index) GPR[GPR_index] = value;
    }
};