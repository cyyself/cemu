# CEMU

A simple full system emulator.

Easy to be used for difftest with [soc-simulator](https://github.com/cyyself/soc-simulator).

## ISAs Support

- RISC-V
  - RV64IMACSU
  - Sv39 MMU and TLB
  - RISC-V CLINT
  - RISC-V PLIC
  - Capable of booting S-Mode SMP Linux with OpenSBI

- MIPS32
  - MIPS Release 1 support without Branch-Likely instruction
  - TLB based MMU Support
  - Capable of booting Linux and [ucore-thumips](https://github.com/cyyself/ucore-thumips)

- LoongArch32(Reduced)
  - Support LoongArch32(Reduced) instruction set, except FP instructions
  - TLB based MMU Support
  - Capable of booting [Linux](https://gitee.com/loongson-edu/la32r-Linux) and [ucore-loongarch32](https://github.com/cyyself/ucore-loongarch32)

## TODO List

- Cache simulation support (Currently at out-of-tree `cache` branch)

## Devices Support

- Xilinx UARTLite
- Serial 8250 (16550 Compatible)

All devices class is shared with [soc-simulator](https://github.com/cyyself/soc-simulator).

## How to run?

See `src/main.cpp` and `example_main` folder to replace the main.

## Help

[Steps to booting Linux with RISCV-CEMU](docs/riscv64-linux.md)
