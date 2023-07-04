#include <iostream>
#include <bitset>

#include "memory_bus.hpp"
#include "uartlite.hpp"
#include "ram.hpp"
#include "rv_core.hpp"
#include "rv_systembus.hpp"
#include "rv_clint.hpp"
#include "rv_plic.hpp"
#include <termios.h>
#include <unistd.h>
#include <thread>
#include <signal.h>
#include "qemu_bridge.hpp"

bool riscv_test = false;

rv_core *rv_0_ptr;
rv_core *rv_1_ptr;
rv_systembus system_bus;
rv_plic <4,4> plic;

extern "C" {
    void cemu_dma(uint64_t addr, void *buf, uint64_t len, bool is_write) {
        if (is_write) system_bus.pa_write(addr, len, (uint8_t*)buf);
        else system_bus.pa_read(addr, len, (uint8_t*)buf);
    }
    void cemu_irq(int plic_source) {
        plic.update_ext(plic_source, true);
    }
    void cemu_main(const char* load_path) {

        uartlite uart;
        rv_clint<2> clint;
        ram dram(4096l*1024l*1024l,load_path);
        qemu_bridge bridge;
        assert(system_bus.add_dev(0x2000000,0x10000,&clint));
        assert(system_bus.add_dev(0xc000000,0x4000000,&plic));
        assert(system_bus.add_dev(0x60100000,1024*1024,&uart));
        assert(system_bus.add_dev(0x80000000,2048l*1024l*1024l,&dram));
        assert(system_bus.add_dev(0x10010000,0x1000,&bridge,true));
        

        rv_core rv_0(system_bus,0);
        rv_0_ptr = &rv_0;
        rv_core rv_1(system_bus,1);
        rv_1_ptr = &rv_1;

        rv_0.jump(0x80000000);
        rv_1.jump(0x80000000);
        rv_1.set_GPR(10,1);
        bool delay_cr = false;
        while (1) {
            clint.tick();
            plic.update_ext(1,uart.irq());
            rv_0.step(plic.get_int(0),clint.m_s_irq(0),clint.m_t_irq(0),plic.get_int(1));
            rv_1.step(plic.get_int(2),clint.m_s_irq(1),clint.m_t_irq(1),plic.get_int(3));
            while (uart.exist_tx()) {
                char c = uart.getc();
                if (c == '\r') delay_cr = true;
                else {
                    if (delay_cr && c != '\n') std::cout << "\r" << c;
                    else std::cout << c;
                    std::cout.flush();
                    delay_cr = false;
                }
            }
        }
    }
}