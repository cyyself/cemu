#ifndef MIPS_COMMON
#define MIPS_COMMON

#include <cstdio>

union mips_instr {
    struct mips_i {
        int imm : 16;
        unsigned int rt : 5;
        unsigned int rs : 5;
        unsigned int opcode : 6;
    } i_type;
    struct mips_j {
        unsigned int imm : 26;
        unsigned int opcode : 6;
    } j_type;
    struct mips_r {
        unsigned int funct : 6;
        unsigned int sa : 5;
        unsigned int rd : 5;
        unsigned int rt : 5;
        unsigned int rs : 5;
        unsigned int opcode : 6;
    } r_type;
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
    OPCODE_PREFETCH = 0b110011,
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
    FUNCT_MULT      = 0b011000,
    FUNCT_MULTU     = 0b011001,
    FUNCT_DIV       = 0b011010,
    FUNCT_DIVU      = 0b011011,
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

enum mips32_exccode : unsigned int {
    EXC_OK      = 31,   // OK for CEMU
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

enum mips32_ksu : unsigned int {
    KERNEL_MODE = 0b00,
    USER_MODE   = 0b10
};

struct cp0_status {
    unsigned int IE  : 1; // Interrupt Enable
    unsigned int EXL : 1; // Exception Level
    unsigned int ERL : 1; // Error Level
    mips32_ksu   KSU : 2;
    unsigned int blank0 : 3;
    unsigned int IM  : 8; // Interrupt Mask
    unsigned int blank1 : 6;
    unsigned int BEV : 1; // bootstrap exception vector
    unsigned int blank2 : 5;
    unsigned int CU : 4;
};

struct cp0_cause {
    unsigned int blank0 : 2;
    unsigned int exccode : 5;
    unsigned int blank1 : 1;
    unsigned int IP : 8; // IP1 and IP0 is writeable
    unsigned int blank2 : 7;
    unsigned int IV : 1; // interrupt vector 0: 0x180, 1: 0x200
    unsigned int blank3 : 7;
    // MIPS Release 1: NO TI
    unsigned int BD : 1; // last exception taken occurred in a branch delay slot
};

enum cp0_rd {
    RD_INDEX    = 0,
    RD_RANDOM   = 1,
    RD_ENTRYLO0 = 2,
    RD_ENTRYLO1 = 3,
    RD_CONTEXT  = 4,
    RD_PAGEMASK = 5,
    RD_WIRED    = 6,
    RD_BADVA    = 8,
    RD_COUNT    = 9,
    RD_ENTRYHI  = 10,
    RD_COMPARE  = 11,
    RD_STATUS   = 12,
    RD_CAUSE    = 13,
    RD_EPC      = 14,
    RD_PRID_EBASE = 15, // PRID, EBase
    RD_CONFIG   = 16, // 0:Config0, 1:Config1
    RD_TAGLO    = 28, // select 0, select 2
    RD_TAGHI    = 29, // select 0, select 2
    RD_ERREPC   = 30
};

enum CCA : unsigned int {
    CCA_Uncached = 2,
    CCA_Cacheable = 3
};

struct cp0_config0 {
    unsigned int k0 : 3; // Kseg0 cacheability and coherency
    unsigned int VI : 1; // Virtual instruction cache
    unsigned int blank0 : 3;
    unsigned int MT : 3; // MMU Type: 1: Standard TLB
    unsigned int AR : 3; // MIPS32 Architecture revision level. 1: Release 2
    unsigned int AT : 2; // Architecture Type 1: MIPS32
    unsigned int BE : 1; // 0: Little endian
    unsigned int Impl : 9; // 0
    unsigned int KU : 3; // 0 for TLB
    unsigned int K23 : 3; // 0 for TLB
    unsigned int M : 1; // 1
};

struct cp0_config1 {
    unsigned int FP : 1; // no FPU: 0
    unsigned int EP : 1; // no EJTAG : 0
    unsigned int CA : 1; // no Code compression: 0
    unsigned int WR : 1; // no Watch registers: 0
    unsigned int PC : 1; // no performance counter: 0
    unsigned int MD : 1; // no MDMX: 0
    unsigned int C2 : 1; // no CP2: 0
    unsigned int DA : 3; // Dcache associativity: 1: 2-way 3: 4-way
    unsigned int DL : 3; // Dcache line size: 3: 16bytes, 4: 32bytes, 5: 64bytes
    unsigned int DS : 3; // Dcache sets per way: 0:64, 1:128, 2:256, 3: 512
    unsigned int IA : 3; // Icache associativity
    unsigned int IL : 3; // Icache line size
    unsigned int IS : 3; // Icache sets per way
    unsigned int MS : 6; // MMU Size - 1
    unsigned int M  : 1; // Config2 is present: 0
};

struct cp0_entrylo {
    unsigned int G : 1; // Global
    unsigned int V : 1; // Valid
    unsigned int D : 1; // Dirty
    unsigned int C : 3; // Cacheability and Coherency
    unsigned int PFN : 20; // Page Frame Number
    unsigned int F : 6; // Fill
};

struct cp0_context {
    unsigned int blank0 : 4;
    unsigned int badvpn2 : 19;
    unsigned int PTEBase : 9;
};

struct cp0_pagemask {
    unsigned int blank0 : 13;
    // no 1KB page support, no MaskX
    unsigned int Mask : 16; 
    /*
        Software may determine which page sizes are 
        supported by writing all ones to the PageMask 
        register, then reading the value back. If a 
        pair of bits reads back as ones, the processor 
        implements that page size.
     */
    unsigned int blank1 : 3;
};

enum HWR_Numbers: unsigned int {
    HWR_CPUNum = 0,
    HWR_SYNCI_Step = 1,
    HWR_CC = 2,
    HWR_CCRes = 3
};

struct cp0_entryhi {
    unsigned int ASID : 8;
    unsigned int blank0 : 5;
    unsigned int VPN2 : 19;
};

struct mips_tlb {
    unsigned int G : 1;
    unsigned int V0 : 1;
    unsigned int V1 : 1;
    unsigned int D0 : 1;
    unsigned int D1 : 1;
    unsigned int C0 : 3;
    unsigned int C1 : 3;
    unsigned int PFN0 : 20;
    unsigned int PFN1 : 20;
    unsigned int VPN2 : 19;
    unsigned int ASID : 8;
    void print() {
        printf("-----TLB Entry BEGIN-----\n");
        printf("VPN2: %x\n", VPN2 << 13);
        printf("ASID: %d\n",ASID);
        printf("G: %d\n",G);
        printf("V0: %d\n",V0);
        printf("D0: %d\n",D0);
        printf("C0: %d\n",C0);
        printf("PFN0: %x\n", PFN0 << 12);
        printf("V1: %d\n",V1);
        printf("D1: %d\n",D1);
        printf("C1: %d\n",C1);
        printf("PFN1: %x\n", PFN1 << 12);
        printf("-----TLB Entry  END -----\n");
    }
};

#endif