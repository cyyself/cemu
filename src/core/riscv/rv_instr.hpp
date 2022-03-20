union rv_instr {
    struct rv_r {
        unsigned int funct7 : 7;
        unsigned int rs2    : 5;
        unsigned int rs1    : 5;
        unsigned int funct3 : 3;
        unsigned int rd     : 5;
        unsigned int opcode : 7;
    } r_type;

    struct rv_i {
        int          imm12  : 12;
        unsigned int rs1    : 5;
        unsigned int funct3 : 3;
        unsigned int rd     : 5;
        unsigned int opcode : 7;
    } i_type;

    struct rv_s {
        int     imm_11_5    : 7;
        unsigned int rs2    : 5;
        unsigned int rs1    : 5;
        unsigned int funct3 : 3;
        unsigned int imm_4_0: 5;
        unsigned int opcode : 7;
    } s_type;

    struct rv_b {
        int     imm_12      : 1;
        unsigned int imm_10_5: 6;
        unsigned int rs2    : 5;
        unsigned int rs1    : 5;
        unsigned int funct3 : 3;
        unsigned int imm_4_1: 4;
        unsigned int imm_11 : 1;
        unsigned int opcode : 7;
    } b_type;

    struct rv_u {
        int     imm_31_12   : 20;
        unsigned int rd     : 5;
        unsigned int opcode : 7;
    } u_type;

    struct rv_j {
        int     imm_20      : 1;
        unsigned int imm_10_1: 10;
        unsigned int imm_11 : 1;
        unsigned int imm_19_12: 8;
        unsigned int rd     : 5;
        unsigned int opcode : 7;
    } j_type;
};

static_assert(sizeof(rv_instr) == 4, "Instruction Struct Error");

enum rv64i_opcode {
    OPCODE_LUI      = 0b0110111,
    OPCODE_AUIPC    = 0b0010111,
    OPCODE_JAL      = 0b1101111,
    OPCODE_JALR     = 0b1100111,
    OPCODE_BRANCH   = 0b1100011,
    OPCODE_LOAD     = 0b0000011,
    OPCODE_STORE    = 0b0100011,
    OPCODE_OPIMM    = 0b0010011,
    OPCODE_OPIMM32  = 0b0011011,
    OPCODE_OP       = 0b0110011,
    OPCODE_OP32     = 0b0111011,
    OPCODE_FENCE    = 0b0001111,
    OPCODE_EEI      = 0b1110011
};
enum funct3_branch {
    FUNCT3_BEQ  = 0b000,
    FUNCT3_BNE  = 0b001,
    FUNCT3_BLT  = 0b100,
    FUNCT3_BGE  = 0b101,
    FUNCT3_BLTU = 0b110,
    FUNCT3_BGEU = 0b111
};
enum funct3_load {
    FUNCT3_LB   = 0b000,
    FUNCT3_LH   = 0b001,
    FUNCT3_LW   = 0b010,
    FUNCT3_LD   = 0b011,
    FUNCT3_LBU  = 0b100,
    FUNCT3_LHU  = 0b101,
    FUNCT3_LWU  = 0b110
};
enum funct3_store {
    FUNCT3_SB   = 0b000,
    FUNCT3_SH   = 0b001,
    FUNCT3_SW   = 0b010,
    FUNCT3_SD   = 0b011
};
enum funct3_op {
    FUNCT3_ADD_SUB  = 0b000,
    FUNCT3_SLL      = 0b001,
    FUNCT3_SLT      = 0b010,
    FUNCT3_SLTU     = 0b011,
    FUNCT3_XOR      = 0b100,
    FUNCT3_SRL_SRA  = 0b101,
    FUNCT3_OR       = 0b110,
    FUNCT3_AND      = 0b111
};

enum funct7 {
    FUNCT7_NORMAL   = 0b0000000,
    FUNCT7_SUB_SRA  = 0b0100000
};

enum funct6 {
    FUNCT6_NORMAL   = 0b000000,
    FUNCT6_SRA      = 0b010000
};