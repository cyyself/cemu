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
#include "l2_cache.hpp"

bool riscv_test = false;

rv_core *rv_0_ptr;
rv_core *rv_1_ptr;

void uart_input(uartlite &uart, uartlite &uart2) {
    termios tmp;
    tcgetattr(STDIN_FILENO,&tmp);
    tmp.c_lflag &=(~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO,TCSANOW,&tmp);
    while (1) {
        char c = getchar();
        if (c == 10) c = 13; // convert lf to cr
        /*
        if (c == 'p') {
            printf("hart0 pc=%lx\n",rv_0_ptr->getPC());
            printf("hart1 pc=%lx\n",rv_1_ptr->getPC());
        }
        */
        uart.putc(c);
        uart2.putc(c);
    }
}

int main(int argc, const char* argv[]) {

    const char *load_path = "../opensbi/build/platform/generic/firmware/fw_payload.bin";
    if (argc >= 2) load_path = argv[1];
    for (int i=1;i<argc;i++) if (strcmp(argv[i],"-rvtest") == 0) riscv_test = true;

    l2_cache <4, 2048, 64, 32> l2;

    uartlite uart;
    rv_clint<2> clint;
    rv_plic <4,4> plic;
    ram dram(4096l*1024l*1024l,load_path);
    assert(l2.add_dev(0x2000000,0x10000,&clint));
    assert(l2.add_dev(0xc000000,0x4000000,&plic));
    assert(l2.add_dev(0x60100000,1024*1024,&uart));
    assert(l2.add_dev(0x80000000,2048l*1024l*1024l,&dram));

    uartlite uart2;
    rv_clint<2> clint2;
    rv_plic <4,4> plic2;
    ram dram2(4096l*1024l*1024l,load_path);
    rv_systembus bus;
    assert(bus.add_dev(0x2000000,0x10000,&clint2));
    assert(bus.add_dev(0xc000000,0x4000000,&plic2));
    assert(bus.add_dev(0x60100000,1024*1024,&uart2));
    assert(bus.add_dev(0x80000000,2048l*1024l*1024l,&dram2));

    rv_core rv_0(l2,bus,0);
    rv_0_ptr = &rv_0;
    rv_core rv_1(l2,bus,1);
    rv_1_ptr = &rv_1;

    std::thread        uart_input_thread(uart_input,std::ref(uart),std::ref(uart2));

    rv_0.jump(0x80000000);
    rv_1.jump(0x80000000);
    rv_1.set_GPR(10,1);
    // char uart_history[8] = {0};
    // int uart_history_idx = 0;
    while (1) {
        clint.tick();
        clint2.tick();
        plic.update_ext(1,uart.irq());
        plic2.update_ext(1,uart.irq());
        rv_0.step(plic.get_int(0),clint.m_s_irq(0),clint.m_t_irq(0),plic.get_int(1));
        // rv_1.step(plic.get_int(2),clint.m_s_irq(1),clint.m_t_irq(1),plic.get_int(3));
        while (uart.exist_tx()) {
            char c = uart.getc();
            if (c != '\r') std::cout << c;
            // uart_history[uart_history_idx] = c;
            // uart_history_idx = (uart_history_idx + 1) % 8;
        }
        while (uart2.exist_tx()) {
            char c = uart2.getc();
        }
        //printf("%lx %lx\n",rv_0.getPC(),rv_1.getPC());
        std::cout.flush();
    }
    return 0;
}