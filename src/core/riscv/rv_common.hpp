#ifndef RV_COMMON
#define RV_COMMON

union rv_instr {
    struct rv_r {
        unsigned int opcode : 7;
        unsigned int rd     : 5;
        unsigned int funct3 : 3;
        unsigned int rs1    : 5;
        unsigned int rs2    : 5;
        unsigned int funct7 : 7;
    } r_type;

    struct rv_i {
        unsigned int opcode : 7;
        unsigned int rd     : 5;
        unsigned int funct3 : 3;
        unsigned int rs1    : 5;
        int          imm12  : 12;
    } i_type;

    struct rv_s {
        unsigned int opcode : 7;
        unsigned int imm_4_0: 5;
        unsigned int funct3 : 3;
        unsigned int rs1    : 5;
        unsigned int rs2    : 5;
        int     imm_11_5    : 7;
    } s_type;

    struct rv_b {
        unsigned int opcode : 7;
        unsigned int imm_11 : 1;
        unsigned int imm_4_1: 4;
        unsigned int funct3 : 3;
        unsigned int rs1    : 5;
        unsigned int rs2    : 5;
        unsigned int imm_10_5: 6;
        int     imm_12      : 1;
    } b_type;

    struct rv_u {
        unsigned int opcode : 7;
        unsigned int rd     : 5;
        int     imm_31_12   : 20;
    } u_type;

    struct rv_j {
        unsigned int opcode : 7;
        unsigned int rd     : 5;
        unsigned int imm_19_12: 8;
        unsigned int imm_11 : 1;
        unsigned int imm_10_1: 10;
        int     imm_20      : 1;
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
    OPCODE_SYSTEM   = 0b1110011,
    OPCODE_AMO      = 0b0101111
};

// Concat(instr(1,0),instr(15,13))
enum rv64c_opcode { // without f/d
    OPCODE_C_ADDI4SPN=0b00000,
    OPCODE_C_LW     = 0b00010,
    OPCODE_C_LD     = 0b00011,
    OPCODE_C_SW     = 0b00110,
    OPCODE_C_SD     = 0b00111,
    OPCODE_C_ADDI   = 0b01000,
    OPCODE_C_ADDIW  = 0b01001,
    OPCODE_C_LI     = 0b01010,
    OPCODE_C_ADDI16SPN_LUI = 0b01011,
    OPCODE_C_ALU    = 0b01100,
    OPCODE_C_J      = 0b01101,
    OPCODE_C_BEQZ   = 0b01110,
    OPCODE_C_BNEZ   = 0b01111,
    OPCODE_C_SLLI   = 0b10000,
    OPCODE_C_LWSP   = 0b10010,
    OPCODE_C_LDSP   = 0b10011,
    OPCODE_C_JR_MV_EB_JALR_ADD = 0b10100,
    OPCODE_C_SWSP   = 0b10110,
    OPCODE_C_SDSP   = 0b10111
};

enum rv64c_funct2 {
    FUNCT2_SUB      = 0b00,
    FUNCT2_XOR_ADDW = 0b01,
    FUNCT2_OR       = 0b10,
    FUNCT2_AND      = 0b11
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

enum funct3_m {
    FUNCT3_MUL      = 0b000,
    FUNCT3_MULH     = 0b001,
    FUNCT3_MULHSU   = 0b010,
    FUNCT3_MULHU    = 0b011,
    FUNCT3_DIV      = 0b100,
    FUNCT3_DIVU     = 0b101,
    FUNCT3_REM      = 0b110,
    FUNCT3_REMU     = 0b111
};

enum funct3_system {
    FUNCT3_PRIV     = 0b000,
    FUNCT3_CSRRW    = 0b001,
    FUNCT3_CSRRS    = 0b010,
    FUNCT3_CSRRC    = 0b011,
    FUNCT3_HLSV     = 0b100,
    FUNCT3_CSRRWI   = 0b101,
    FUNCT3_CSRRSI   = 0b110,
    FUNCT3_CSRRCI   = 0b111
};

enum funct7_priv {
    FUNCT7_ECALL_EBREAK = 0b0000000,
    FUNCT7_SRET_WFI     = 0b0001000,
    FUNCT7_MRET         = 0b0011000,
    FUNCT7_SFENCE_VMA   = 0b0001001
};

enum amo_funct {
    AMOLR   = 0b00010,
    AMOSC   = 0b00011,
    AMOSWAP = 0b00001,
    AMOADD  = 0b00000,
    AMOXOR  = 0b00100,
    AMOAND  = 0b01100,
    AMOOR   = 0b01000,
    AMOMIN  = 0b10000,
    AMOMAX  = 0b10100,
    AMOMINU = 0b11000,
    AMOMAXU = 0b11100
};

enum funct7 {
    FUNCT7_NORMAL   = 0b0000000,
    FUNCT7_SUB_SRA  = 0b0100000,
    FUNCT7_MUL      = 0b0000001
};

enum funct6 {
    FUNCT6_NORMAL   = 0b000000,
    FUNCT6_SRA      = 0b010000
};

enum priv_mode {
    U_MODE = 0,
    S_MODE = 1,
    H_MODE = 2,
    M_MODE = 3
};

enum rv_csr_addr {
// Unprivileged Counter/Timers
    csr_cycle   = 0xc00,
    csr_time    = 0xc01,
    csr_instret = 0xc02,
// Supervisor Trap Setup
    csr_sstatus = 0x100,
    csr_sie     = 0x104,
    csr_stvec   = 0x105,
    csr_scounteren  = 0x106,
// Supervisor Trap Handling
    csr_sscratch= 0x140,
    csr_sepc    = 0x141,
    csr_scause  = 0x142,
    csr_stval   = 0x143,
    csr_sip     = 0x144,
// Supervisor Protection and Translation
    csr_satp    = 0x180,
// Machine Information Registers
    csr_mvendorid   = 0xf11,
    csr_marchid = 0xf12,
    csr_mimpid  = 0xf13,
    csr_mhartid = 0xf14,
    csr_mconfigptr  = 0xf15,
// Machine Trap Setup
    csr_mstatus = 0x300,
    csr_misa    = 0x301,
    csr_medeleg = 0x302,
    csr_mideleg = 0x303,
    csr_mie     = 0x304,
    csr_mtvec   = 0x305,
    csr_mcounteren  = 0x306,
// Machine Trap Handling
    csr_mscratch= 0x340,
    csr_mepc    = 0x341,
    csr_mcause  = 0x342,
    csr_mtval   = 0x343,
    csr_mip     = 0x344,
// Machine Memory Protection
    csr_pmpcfg0 = 0x3a0,
    csr_pmpcfg1 = 0x3a1,
    csr_pmpcfg2 = 0x3a2,
    csr_pmpcfg3 = 0x3a3,
    csr_pmpaddr0= 0x3b0,
    csr_pmpaddr1= 0x3b1,
    csr_pmpaddr2= 0x3b2,
    csr_pmpaddr3= 0x3b3,
    csr_pmpaddr4= 0x3b4,
    csr_pmpaddr5= 0x3b5,
    csr_pmpaddr6= 0x3b6,
    csr_pmpaddr7= 0x3b7,
    csr_pmpaddr8= 0x3b8,
    csr_pmpaddr9= 0x3b9,
    csr_pmpaddr10   = 0x3ba,
    csr_pmpaddr11   = 0x3bb,
    csr_pmpaddr12   = 0x3bc,
    csr_pmpaddr13   = 0x3bd,
    csr_pmpaddr14   = 0x3be,
    csr_pmpaddr15   = 0x3bf,
// Machine Counter/Timers
    csr_mcycle  = 0xb00,
    csr_minstret= 0xb02,
    csr_tselect = 0x7a0,
    csr_tdata1  = 0x7a1
};

#define rv_ext(x) (1<<(x-'a'))

struct csr_misa_def {
    uint64_t ext    : 26;
    uint64_t blank  : 36;
    uint64_t mxl    : 2;
};

struct csr_mstatus_def {
    uint64_t blank0 : 1;
    uint64_t sie    : 1;// supervisor interrupt enable
    uint64_t blank1 : 1;
    uint64_t mie    : 1;// machine interrupt enable
    uint64_t blank2 : 1;
    uint64_t spie   : 1;// sie prior to trapping
    uint64_t ube    : 1;// u big-endian, zero
    uint64_t mpie   : 1;// mie prior to trapping
    uint64_t spp    : 1;// supervisor previous privilege mode.
    uint64_t vs     : 2;// without vector, zero
    uint64_t mpp    : 2;// machine previous privilege mode.
    uint64_t fs     : 2;// without float, zero
    uint64_t xs     : 2;// without user ext, zero
    uint64_t mprv   : 1;// Modify PRiVilege (Turn on virtual memory and protection for load/store in M-Mode) when mpp is not M-Mode
    // mprv will be used by OpenSBI.
    uint64_t sum    : 1;// permit Supervisor User Memory access
    uint64_t mxr    : 1;// Make eXecutable Readable
    uint64_t tvm    : 1;// Trap Virtual Memory (raise trap when sfence.vma and sinval.vma executing in S-Mode)
    uint64_t tw     : 1;// Timeout Wait for WFI
    uint64_t tsr    : 1;// Trap SRET
    uint64_t blank3 : 9;
    uint64_t uxl    : 2;// user xlen
    uint64_t sxl    : 2;// supervisor xlen
    uint64_t sbe    : 1;// s big-endian
    uint64_t mbe    : 1;// m big-endian
    uint64_t blank4 : 25;
    uint64_t sd     : 1;// no vs,fs,xs, zero
};

struct csr_sstatus_def {
    uint64_t blank0 : 1;
    uint64_t sie    : 1;// supervisor interrupt enable
    uint64_t blank1 : 3;
    uint64_t spie   : 1;// sie prior to trapping
    uint64_t ube    : 1;// u big-endian, zero
    uint64_t blank2 : 1;// mie prior to trapping
    uint64_t spp    : 1;// supervisor previous privilege mode.
    uint64_t vs     : 2;// without vector, zero
    uint64_t blank3 : 2;// machine previous privilege mode.
    uint64_t fs     : 2;// without float, zero
    uint64_t xs     : 2;// without user ext, zero
    uint64_t blank4 : 1;
    uint64_t sum    : 1;// permit Supervisor User Memory access
    uint64_t mxr    : 1;// Make eXecutable Readable
    uint64_t blank5 : 12;
    uint64_t uxl    : 2;// user xlen
    uint64_t blank6 : 29;
    uint64_t sd     : 1;// no vs,fs,xs, zero
};

struct csr_cause_def {
    uint64_t cause  : 63;
    uint64_t interrupt : 1;
    csr_cause_def(uint64_t cause, uint64_t interrupt=0):cause(cause),interrupt(interrupt){}
    csr_cause_def():cause(0),interrupt(0){}
};

struct csr_tvec_def {
    uint64_t mode   : 2; // 0: Direct, 1: Vectored
    uint64_t base   : 62;
};

struct int_def { // interrupt pending
    uint64_t blank0 : 1;
    uint64_t s_s_ip : 1; // 1
    uint64_t blank1 : 1;
    uint64_t m_s_ip : 1; // 3
    uint64_t blank2 : 1;
    uint64_t s_t_ip : 1; // 5
    uint64_t blank3 : 1;
    uint64_t m_t_ip : 1; // 7
    uint64_t blank4 : 1;
    uint64_t s_e_ip : 1; // 9
    uint64_t blank5 : 1;
    uint64_t m_e_ip : 1; // 11
};


enum rv_int_code {
    int_s_sw    = 1,
    int_m_sw    = 3,
    int_s_timer = 5,
    int_m_timer = 7,
    int_s_ext   = 9,
    int_m_ext   = 11
};

// supervisor interrupt mask
const uint64_t s_int_mask = (1ull<<int_s_ext) | (1ull<<int_s_sw) | (1ull<<int_s_timer);
// machine interrupt mask
const uint64_t m_int_mask = s_int_mask | (1ull<<int_m_ext) | (1ull<<int_m_sw) | (1ull<<int_m_timer);

enum rv_exc_code {
    exc_instr_misalign  = 0,
    exc_instr_acc_fault = 1,
    exc_illegal_instr   = 2,
    exc_breakpoint      = 3,
    exc_load_misalign   = 4,
    exc_load_acc_fault  = 5,
    exc_store_misalign  = 6,// including amo
    exc_store_acc_fault = 7,// including amo
    exc_ecall_from_user = 8,
    exc_ecall_from_supervisor   = 9,
    exc_ecall_from_machine      = 11,
    exc_instr_pgfault   = 12,
    exc_load_pgfault    = 13,
    exc_store_pgfault   = 15,// including amo
    exc_custom_ok       = 24
};

const uint64_t s_exc_mask = (1<<16) - 1 - (1<<exc_ecall_from_machine);

struct csr_counteren_def {
    uint64_t cycle  : 1;
    uint64_t time   : 1;
    uint64_t instr_retire   : 1;
    uint64_t blank  : 61;
};

const uint64_t counter_mask = (1<<0) | (1<<2);

struct sv39_pte {
    uint64_t V : 1; // valid
    uint64_t R : 1; // read
    uint64_t W : 1; // write
    uint64_t X : 1; // execute
    uint64_t U : 1; // user
    uint64_t G : 1; // global
    uint64_t A : 1; // access
    uint64_t D : 1; // dirty
    uint64_t RSW : 2; // reserved for use by supervisor softwar
    uint64_t PPN0 : 9;
    uint64_t PPN1 : 9;
    uint64_t PPN2 : 26;
    uint64_t reserved : 7;
    uint64_t PBMT : 2; // Svpbmt is not implemented, return 0
    uint64_t N : 1;
};
static_assert(sizeof(sv39_pte) == 8, "sv39_pte shoule be 8 bytes.");
// Note: A and D not implement. So use them as permission bit and raise page fault.

struct satp_def {
    uint64_t ppn    : 44;
    uint64_t asid   : 16;
    uint64_t mode   : 4;
};

struct sv39_va {
    uint64_t page_off   : 12;
    uint64_t vpn_0      : 9;
    uint64_t vpn_1      : 9;
    uint64_t vpn_2      : 9;
    uint64_t blank      : 25;
};

#endif