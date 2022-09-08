#ifndef LA32R_COMMON
#define LA32R_COMMON

#include <cstdio>
#include <utility>

#define TLB_INDEX_WIDTH 5

union la32r_instr {
    struct la32r_2r {
        unsigned int rd: 5;
        unsigned int rj: 5;
        unsigned int opcode: 22;
    } _2r;
    struct la32r_3r {
        unsigned int rd: 5;
        unsigned int rj: 5;
        unsigned int rk: 5;
        unsigned int opcode: 17;
    } _3r;
    struct la32r_4r {
        unsigned int rd: 5;
        unsigned int rj: 5;
        unsigned int rk: 5;
        unsigned int ra: 5;
        unsigned int opcode: 12;
    } _4r;
    struct la32r_2ri8 {
        unsigned int rd: 5;
        unsigned int rj: 5;
        int i8: 8;
        unsigned int opcode: 14;
    } _2ri8;
    struct la32r_2ri12 {
        unsigned int rd: 5;
        unsigned int rj: 5;
        int i12: 12;
        unsigned int opcode: 10;
    } _2ri12;
    struct la32r_2ri14 {
        unsigned int rd: 5;
        unsigned int rj: 5;
        int i14: 14;
        unsigned int opcode: 8;
    } _2ri14;
    struct la32r_1rsi20 { // LU12I.W & PCADDU12I
        unsigned int rd: 5;
        int si20: 20;
        unsigned int opcode: 7;
    } _1rsi20;
    struct la32r_2ri16 {
        unsigned int rd: 5;
        unsigned int rj: 5;
        int i16: 16;
        unsigned int opcode: 6;
    } _2ri16;
    struct la32r_1ri21 {
        int i21_hi: 5;
        unsigned int rj: 5;
        int i21_lo: 16;
        unsigned int opcode: 6;
    } _1ri21;
    struct la32r_i26 {
        unsigned int i26_hi: 10;
        unsigned int i26_lo: 16;
        unsigned int opcode: 6;
    } _i26;
};

// TODO: FP Instr...
enum la32r_opcode {
    // OPCODE.length == 22
    RDTIMEL_W = 0b11000,
    RDTIMEH_W = 0b11001,
    TLBSRCH = 0b11001001000001010,
    TLBRD = 0b11001001000001011,
    TLBWR = 0b11001001000001100,
    TLBFILL = 0b11001001000001101,
    ERTN = 0b11001001000001110,
    // OPCODE.length == 17
    ADD_W = 0b100000,
    SUB_W = 0b100010,
    SLT = 0b100100,
    SLTU = 0b100101,
    NOR = 0b101000,
    AND = 0b101001,
    OR = 0b101010,
    XOR = 0b101011,
    SLL_W = 0b101110,
    SRL_W = 0b101111,
    SRA_W = 0b110000,
    MUL_W = 0b111000,
    MULH_W = 0b111001,
    MULH_WU = 0b111010,
    DIV_W = 0b1000000,
    MOD_W = 0b1000001,
    DIV_WU = 0b1000010,
    MOD_WU = 0b1000011,
    BREAK = 0b1010100,
    SYSCALL = 0b1010110,
    SLLI_W = 0b10000001,
    SRLI_W = 0b10001001,
    SRAI_W = 0b10010001,
    IDLE = 0b110010010001,
    INVTLB = 0b110010010011,
    DBAR = 0b111000011100100,
    IBAR = 0b111000011100101,
    // OPCODE.length == 10
    SLTI = 0b1000,
    SLTUI = 0b1001,
    ADDI_W = 0b1010,
    ANDI = 0b1101,
    ORI = 0b1110,
    XORI = 0b1111,
    CACOP = 0b11000,
    LD_B = 0b10100000,
    LD_H = 0b10100001,
    LD_W = 0b10100010,
    ST_B = 0b10100100,
    ST_H = 0b10100101,
    ST_W = 0b10100110,
    LD_BU = 0b10101000,
    LD_HU = 0b10101001,
    PRELD = 0b10101011,
    // OPCODE.length == 8
    CSR = 0b100,
    LL_W = 0b100000,
    SC_W = 0b100001,
    // OPCODE.length == 7
    LU12I_W = 0b1010,
    PCADDU12I = 0b1110,
    // OPCODE.length == 6
    JIRL = 0b10011,
    B = 0b10100,
    BL = 0b10101,
    BEQ = 0b10110,
    BNE = 0b10111,
    BLT = 0b11000,
    BGE = 0b11001,
    BLTU = 0b11010,
    BGEU = 0b11011,
};

enum la32r_csr_idx {
    CRMD = 0x0,
    PRMD = 0x1,
    EUEN = 0x2,
    ECFG = 0x4,
    ESTAT = 0x5,
    ERA = 0x6,
    BADV = 0x7,
    EENTRY = 0xc,
    TLBIDX = 0x10,
    TLBEHI = 0x11,
    TLBELO0 = 0x12,
    TLBELO1 = 0x13,
    ASID = 0x18,
    PGDL = 0x19,
    PGDH = 0x1A,
    PGD = 0x1B,
    CPUID = 0x20,
    SAVE0 = 0x30,
    SAVE1 = 0x31,
    SAVE2 = 0x32,
    SAVE3 = 0x33,
    TID = 0x40,
    TCFG = 0x41,
    TVAL = 0x42,
    TICLR = 0x44,
    LLBCTL = 0x60,
    TLBRENTRY = 0x88,
    CTAG = 0x98,
    DMW0 = 0x180,
    DMW1 = 0x181,
};

enum la32r_csr_op {
    RD = 0b00000,
    WR = 0b00001,
};

enum la32r_ecode : unsigned int {
    INT = 0x0,
    PIL = 0x1,
    PIS = 0x2,
    PIF = 0x3,
    PME = 0x4,
    PPI = 0x7,
    ADE = 0x8,
    ALE = 0x9,
    SYS = 0xB,
    BRK = 0xC,
    INE = 0xD,
    IPE = 0xE,
    FPD = 0xF,
    FPE = 0x12,
    TLBR = 0x3F,
    OK = 0x33,
};

#define ADEF_SUBCODE 0
#define ADEM_SUBCODE 1

enum la32r_plv : unsigned int {
    plv0 = 0b00,
    plv3 = 0b11,
};

enum la32r_mat : unsigned int {
    suc = 0b00,
    cc = 0b01,
};

struct csr_crmd {
    la32r_plv plv: 2;
    unsigned int ie: 1;
    unsigned int da: 1;
    unsigned int pg: 1;
    la32r_mat datf: 2;
    la32r_mat datm: 2;
    unsigned int blank0: 23;
};

struct csr_prmd {
    la32r_plv pplv: 2;
    unsigned int pie: 1;
    unsigned int blank0: 29;
};

struct csr_euen {
    unsigned int fpe: 1;
    unsigned int blank0: 31;
};

struct csr_ecfg {
    unsigned int lie: 13;
    unsigned int blank0: 19;
};

struct csr_estat {
    unsigned int is: 13;
    unsigned int blank0: 3;
    la32r_ecode ecode: 6;
    unsigned int esubcode: 9;
    unsigned int blank1: 1;
};

struct csr_era {
    unsigned int pc: 32;
};

struct csr_badv {
    unsigned int vaddr: 32;
};

struct csr_eentry {
    unsigned int blank0: 6;
    unsigned int va: 26;
};

struct csr_cpuid {
    unsigned int core_id: 9;
    unsigned int blank0: 23;
};

struct csr_save {
    unsigned int data: 32;
};

struct csr_llbctl {
    unsigned int rollb: 1;
    unsigned int wcllb: 1;
    unsigned int klo: 1;
    unsigned int blank0: 29;
};

struct csr_tlbidx {
    unsigned int index: 5; // 32 entries
    unsigned int blank0: 11;
    unsigned int blank1: 8;
    unsigned int ps: 6;
    unsigned int blank2: 1;
    unsigned int ne: 1;
};

struct csr_tlbehi {
    unsigned int blank0: 13;
    unsigned int vppn: 19;
};

struct csr_tlbelo {
    unsigned int v: 1;
    unsigned int d: 1;
    la32r_plv plv: 2;
    la32r_mat mat: 2;
    unsigned int g: 1;
    unsigned int blank0: 1;
    unsigned int ppn: 20; // PALEN == 32
    unsigned int blank1: 4;
};

struct csr_asid {
    unsigned int asid: 10;
    unsigned int blank0: 6;
    unsigned int asidbits: 8;
    unsigned int blank1: 8;
};

struct csr_pgd {
    unsigned int blank0: 12;
    unsigned int base: 20;
};

struct csr_tlbrentry {
    unsigned int blank0: 6;
    unsigned int pa: 26;
};

struct csr_dmw {
    unsigned int plv0: 1;
    unsigned int blank0: 2;
    unsigned int plv3: 1;
    la32r_mat mat: 2;
    unsigned int blank1: 19;
    unsigned int pseg: 3;
    unsigned int blank2: 1;
    unsigned int vseg: 3;
};

struct csr_tid {
    unsigned int tid: 32;
};

struct csr_tcfg {
    unsigned int en: 1;
    unsigned int periodic: 1;
    unsigned int init_val: 30;
};

struct csr_tval {
    unsigned int time_val: 32; // width same as NEMU
};

struct csr_ticlr {
    unsigned int clr: 1;
    unsigned int blank0: 31;
};

struct la32r_tlb {
    unsigned int e: 1;
    unsigned int asid: 10;
    unsigned int g: 1;
    unsigned int ps: 6;
    unsigned int vppn: 19;
    unsigned int v0: 1;
    unsigned int d0: 1;
    la32r_mat mat0: 2;
    la32r_plv plv0: 2;
    unsigned int ppn0: 20;
    unsigned int v1: 1;
    unsigned int d1: 1;
    la32r_mat mat1: 2;
    la32r_plv plv1: 2;
    unsigned int ppn1: 20;

    void print() {
        printf("-----TLB Entry BEGIN-----\n");
        printf("e: %d\n", e);
        printf("asid: %d\n", asid);
        printf("g: %d\n", g);
        printf("ps: %d\n", ps);
        printf("vppn: %x\n", vppn << 13);
        printf("v0:   %d       v1:   %d\n", v0, v1);
        printf("d0:   %d       d1:   %d\n", d0, d1);
        printf("mat0: %d       mat1: %d\n", mat0, mat1);
        printf("plv0: %d       plv1: %d\n", plv0, plv1);
        printf("ppn0: %08x ppn1: %08x\n", ppn0, ppn1);
        printf("-----TLB Entry  END -----\n");
    }
};

struct la32r_dmw {
    unsigned int plv0: 1;
    unsigned int plv1: 1;
    unsigned int plv2: 1;
    unsigned int plv3: 1;
    la32r_mat mat: 2;
    unsigned int pseg: 3;
    unsigned int vseg: 3;
};

typedef std::pair<la32r_ecode, unsigned int> la32r_exccode;

#endif // LA32R_COMMON
