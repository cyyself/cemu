#include <iostream>
#include <bitset>

#include "memory_bus.hpp"
#include "uartlite.hpp"
#include "ram.hpp"
#include "rv_core.hpp"

int main() {
    memory_bus mmio_bus;
    ram bootram(32*1024,"bootram.bin");
    uartlite uart;
    assert(mmio_bus.add_dev(0x0,32*1024,&bootram));
    assert(mmio_bus.add_dev(0x60000000,1024*1024,&uart));

    ram dram(4096l*1024l*1024l);
    std::bitset<4> irq;
    rv_core rv(dram,mmio_bus,irq);
    while (1) {
        rv.step();
        while (uart.exist_tx()) {
            char c = uart.getc();
            if (c != '\r') std::cout << c;
        }
        std::cout.flush();
    }
    return 0;
}