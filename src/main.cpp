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

bool riscv_test_u_ecall = false;

void uart_input(uartlite &uart) {
    termios tmp;
    tcgetattr(STDIN_FILENO,&tmp);
    tmp.c_lflag &=(~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO,TCSANOW,&tmp);
    while (1) {
        char c = getchar();
        if (c == 10) c = 13; // convert lf to cr
        uart.putc(c);
    }
}

int main(int argc, const char* argv[]) {

    const char *load_path = "../opensbi/build/platform/generic/firmware/fw_payload.bin";
    if (argc >= 2) load_path = argv[1];
    for (int i=1;i<argc;i++) if (strcmp(argv[i],"-rvtestu") == 0) riscv_test_u_ecall = true;

    rv_systembus system_bus;

    uartlite uart;
    rv_clint<1> clint;
    rv_plic plic;
    ram dram(4096l*1024l*1024l,load_path);
    assert(system_bus.add_dev(0x2000000,0x10000,&clint));
    assert(system_bus.add_dev(0xc000000,0x4000000,&plic));
    assert(system_bus.add_dev(0x60100000,1024*1024,&uart));
    assert(system_bus.add_dev(0x80000000,2048l*1024l*1024l,&dram));

    rv_core rv(system_bus,0);

    std::thread        uart_input_thread(uart_input,std::ref(uart));

    rv.jump(0x80000000);
    // char uart_history[8] = {0};
    // int uart_history_idx = 0;
    while (1) {
        clint.tick();
        rv.step(false,clint.m_s_irq(0),clint.m_t_irq(0),uart.irq());
        while (uart.exist_tx()) {
            char c = uart.getc();
            if (c != '\r') std::cout << c;
            // uart_history[uart_history_idx] = c;
            // uart_history_idx = (uart_history_idx + 1) % 8;
        }
        std::cout.flush();
    }
    return 0;
}