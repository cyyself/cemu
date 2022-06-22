#ifndef MIPS_COMMON
#define MIPS_COMMON

union mips_instr {
    struct mips_i {
        int imm : 16;
        unsigned int rt : 5;
        unsigned int rs : 5;
        unsigned int opcode : 6;
    };
    struct mips_j {
        int imm : 26;
        unsigned int opcode : 6;
    };
    struct mips_r {
        unsigned int funct : 6;
        unsigned int sa : 5;
        unsigned int rd : 5;
        unsigned int rt : 5;
        unsigned int rs : 5;
        unsigned int opcode : 6;
    };
};

enum mips32_opcode {
    OPCODE_SPECIAL  = 0b000000,
    OPCODE_REGIMM   = 0b000001,
    OPCODE_J        = 0b000010,
    OPCODE_JAL      = 0b000011,
    OPCODE_BEQ      = 0b000100,
    OPCODE_BNE      = 0b000101,
    OPCODE_BLEZ     = 0b000110,
    OPCODE_BGTZ     = 0b000111,
    OPCODE_ADDI     = 0b001000,
    OPCODE_ADDIU    = 0b001001,
    OPCODE_SLTI     = 0b001010,
    OPCODE_SLTIU    = 0b001011,
    OPCODE_ANDI     = 0b001100,
    OPCODE_ORI      = 0b001101,
    OPCODE_XORI     = 0b001110,
    OPCODE_LUI      = 0b001111,
    OPCODE_COP0     = 0b010000,
    OPCODE_SPECIAL2 = 0b011100,
    OPCODE_LB       = 0b100000,
    OPCODE_LH       = 0b100001,
    OPCODE_LWL      = 0b100010,
    OPCODE_LW       = 0b100011,
    OPCODE_LBU      = 0b100100,
    OPCODE_LHU      = 0b100101,
    OPCODE_LWR      = 0b100110,
    OPCODE_SB       = 0b101000,
    OPCODE_SH       = 0b101001,
    OPCODE_SWL      = 0b101010,
    OPCODE_SW       = 0b101011,
    OPCODE_SWR      = 0b101110,
    OPCODE_CACHE    = 0b101111,
    OPCODE_LL       = 0b110000,
    OPCODE_SC       = 0b111000
};

enum mips32_special_funct {
    FUNCT_SLL       = 0b000000,
    FUNCT_SRL       = 0b000010,
    FUNCT_SRA       = 0b000011,
    FUNCT_SLLV      = 0b000100,
    FUNCT_SRLV      = 0b000110,
    FUNCT_SRAV      = 0b000111,
    FUNCT_JR        = 0b001000,
    FUNCT_JALR      = 0b001001,
    FUNCT_MOVZ      = 0b001010,
    FUNCT_MOVN      = 0b001011,
    FUNCT_SYSCALL   = 0b001100,
    FUNCT_BREAK     = 0b001101,
    FUNCT_SYNC      = 0b001111,
    FUNCT_MFHI      = 0b010000,
    FUNCT_MTHI      = 0b010001,
    FUNCT_MFLO      = 0b010010,
    FUNCT_MTLO      = 0b010011,
    FUNCT_ADD       = 0b100000,
    FUNCT_ADDU      = 0b100001,
    FUNCT_SUB       = 0b100010,
    FUNCT_SUBU      = 0b100011,
    FUNCT_AND       = 0b100100,
    FUNCT_OR        = 0b100101,
    FUNCT_XOR       = 0b100110,
    FUNCT_NOR       = 0b100111,
    FUNCT_SLT       = 0b101010,
    FUNCT_SLTU      = 0b101011,
    FUNCT_TGE       = 0b110000,
    FUNCT_TGEU      = 0b110001,
    FUNCT_TLT       = 0b110010,
    FUNCT_TLTU      = 0b110011,
    FUNCT_TEQ       = 0b110100,
    FUNCT_TNE       = 0b110110
};

enum mips32_regimm_rt {
    RT_BLTZ         = 0b00000,
    RT_BGEZ         = 0b00001,
    RT_BLTZAL       = 0b10000,
    RT_BGEZAL       = 0b10001,
    // no branch likely
    RT_TGEI         = 0b01000,
    RT_TGEIU        = 0b01001,
    RT_TLTI         = 0b01010,
    RT_TLTIU        = 0b01011,
    RT_TEQI         = 0b01100,
    RT_TNEI         = 0b01110,
    RT_SYNCI        = 0b11111
};

enum mips32_special2_funct {
    FUNCT_MADD  = 0b000000,
    FUNCT_MADDU = 0b000001,
    FUNCT_MUL   = 0b000010,
    FUNCT_MSUB  = 0b000100,
    FUNCT_MSUBU = 0b000101,
    FUNCT_CLZ   = 0b100000,
    FUNCT_CLO   = 0b100001
};

enum mips32_cop0_rs {
    RS_MFC0     = 0b00000,
    RS_MTC0     = 0b00100,
    RS_CO       = 0b10000
};

enum mips32_co_funct {
    FUNCT_TLBR  = 0b000001,
    FUNCT_TLBWI = 0b000010,
    FUNCT_TLBWR = 0b000110,
    FUNCT_TLBP  = 0b001000,
    FUNCT_ERET  = 0b011000,
    FUNCT_WAIT  = 0b100000
};

enum mips32_ExcCode {
    EXC_INT     = 0,    // Interrupt
    EXC_MOD     = 1,    // TLB modification
    EXC_TLBL    = 2,    // TLB exception (load or instruction fetch)
    EXC_TLBS    = 3,    // TLB exception (store)
    EXC_ADEL    = 4,    // Address error exception (load or instruction fetch)
    EXC_ADES    = 5,    // Address error exception (store)
    EXC_IBE     = 6,    // Bus error exception (instruction fetch)
    EXC_DBE     = 7,    // Bus error exception (data reference: load or store)
    EXC_SYS     = 8,    // Syscall exception
    EXC_BP      = 9,    // Breakpoint exception
    EXC_RI      = 10,   // Reserved instruction exception
    EXC_CPU     = 11,   // Coprocessor Unusable exception
    EXC_OV      = 12,   // Arithmetic Overflow exception
    EXC_TR      = 13,   // Trap exception
    EXC_TLBRI   = 19,   // TLB Read-Inhibit exception
    EXC_TLBXI   = 20    // TLB Execution-Inhibit exception
};

#endif