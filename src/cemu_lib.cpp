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

bool riscv_test = false;

rv_core *rv_0_ptr;
rv_core *rv_1_ptr;


extern "C" {
    void cemu_main(const char* load_path) {
        rv_systembus system_bus;

        uartlite uart;
        rv_clint<2> clint;
        rv_plic <4,4> plic;
        ram dram(4096l*1024l*1024l,load_path);
        assert(system_bus.add_dev(0x2000000,0x10000,&clint));
        assert(system_bus.add_dev(0xc000000,0x4000000,&plic));
        assert(system_bus.add_dev(0x60100000,1024*1024,&uart));
        assert(system_bus.add_dev(0x80000000,2048l*1024l*1024l,&dram));

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