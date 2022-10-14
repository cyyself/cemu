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

void uart_input(uartlite &uart) {
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
    }
}

bool send_ctrl_c;

void sigint_handler(int x) {
    static time_t last_time;
    if (time(NULL) - last_time < 1) exit(0);
    last_time = time(NULL);
    send_ctrl_c = true;
}

int main(int argc, const char* argv[]) {

    signal(SIGINT, sigint_handler);

    const char *load_path = "../linux/vmlinux.bin";
    if (argc >= 2) load_path = argv[1];
    for (int i=1;i<argc;i++) if (strcmp(argv[i],"-rvtest") == 0) riscv_test = true;

    rv_systembus system_bus;

    uartlite uart;
    rv_clint<2> clint;
    rv_plic <4,4> plic;
    ram dram(4096l*1024l*1024l,load_path);
    ram dtb_stor(4096,"../opensbi/cemu.dtb");
    assert(system_bus.add_dev(0x2000000,0x10000,&clint));
    assert(system_bus.add_dev(0xc000000,0x4000000,&plic));
    assert(system_bus.add_dev(0x60100000,1024*1024,&uart));
    assert(system_bus.add_dev(0x80000000,2048l*1024l*1024l,&dram));
    assert(system_bus.add_dev(0x60400000,4096,&dtb_stor));

    rv_core rv_0(system_bus,0);
    rv_0_ptr = &rv_0;
    rv_core rv_1(system_bus,1);
    rv_1_ptr = &rv_1;

    std::thread        uart_input_thread(uart_input,std::ref(uart));

    rv_0.jump(0x80000000);
    rv_1.jump(0x80000000);
    rv_1.set_GPR(10,1);
    rv_0.set_GPR(11,0x60400000);
    rv_1.set_GPR(11,0x60400000);
    // char uart_history[8] = {0};
    // int uart_history_idx = 0;
    bool delay_cr = false;
    while (1) {
        clint.tick();
        plic.update_ext(1,uart.irq());
        rv_0.step(plic.get_int(0),clint.m_s_irq(0),clint.m_t_irq(0),plic.get_int(1));
        rv_1.step(0,clint.m_s_irq(1),clint.m_t_irq(1),plic.get_int(1));
        while (uart.exist_tx()) {
            char c = uart.getc();
            if (c == '\r') delay_cr = true;
            else {
                if (delay_cr && c != '\n') std::cout << "\r" << c;
                else std::cout << c;
                std::cout.flush();
                delay_cr = false;
            }
            // uart_history[uart_history_idx] = c;
            // uart_history_idx = (uart_history_idx + 1) % 8;
        }
        if (send_ctrl_c) {
            uart.putc(3);
            send_ctrl_c = false;
        }
        //printf("%lx %lx\n",rv_0.getPC(),rv_1.getPC());
    }
    return 0;
}