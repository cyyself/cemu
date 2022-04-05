#include <iostream>
#include <bitset>

#include "memory_bus.hpp"
#include "uartlite.hpp"
#include "ram.hpp"
#include "rv_core.hpp"
#include "rv_systembus.hpp"
#include "rv_clint.hpp"

int main() {
    rv_systembus system_bus;
    
    ram bootram(32*1024,"../cyyrv64/test/workbench/start.bin");
    uartlite uart;
    rv_clint<1> clint;
    ram dram(4096l*1024l*1024l);

    assert(system_bus.add_dev(0x0,32*1024,&bootram));
    assert(system_bus.add_dev(0x60000000,1024*1024,&uart));
    assert(system_bus.add_dev(0x2000000,0x10000,&clint));
    assert(system_bus.add_dev(0x80000000,2048l*1024l*1024l,&dram));
    
    rv_core rv(system_bus,0);
    while (1) {
        clint.tick();
        rv.step(false,false,false,false);
        while (uart.exist_tx()) {
            char c = uart.getc();
            if (c != '\r') std::cout << c;
        }
        std::cout.flush();
    }
    return 0;
}