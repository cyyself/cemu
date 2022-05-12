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
#include "clock_manager.hpp"
#include "cache_uni_def.hpp"
#include <signal.h>

#define NR_CORES 4

bool riscv_test = false;

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

clock_manager <32> cm(NR_CORES);
bool send_ctrl_c;

void sigint_handler(int x) {
    static time_t last_time;
    if (time(NULL) - last_time < 1) exit(0);
    last_time = time(NULL);
    send_ctrl_c = true;
}

rv_core *cores[NR_CORES] = {NULL};

int main(int argc, const char* argv[]) {

    signal(SIGINT, sigint_handler);

    const char *load_path = "../opensbi/build/platform/generic/firmware/fw_payload.bin";
    if (argc >= 2) load_path = argv[1];
    for (int i=1;i<argc;i++) if (strcmp(argv[i],"-rvtest") == 0) riscv_test = true;

    l2_cache <L2_WAYS, L2_NR_SETS, L2_SZLINE, 32> l2;

    uartlite uart;
    rv_clint<NR_CORES> clint;
    rv_plic <4,NR_CORES*2> plic;
    ram dram(4096l*1024l*1024l,load_path);
    assert(l2.add_dev(0x2000000,0x10000,&clint));
    assert(l2.add_dev(0xc000000,0x4000000,&plic));
    assert(l2.add_dev(0x60100000,1024*1024,&uart));
    assert(l2.add_dev(0x80000000,2048l*1024l*1024l,&dram));

    for (int i=0;i<NR_CORES;i++) {
        cores[i] = new rv_core(l2,i);
        cores[i]->jump(0x80000000);
        cores[i]->set_GPR(10,i,0);
    }

    std::thread        uart_input_thread(uart_input,std::ref(uart));

    // char uart_history[8] = {0};
    // int uart_history_idx = 0;
    bool delay_cr = false;
    while (1) {
        clint.set_time(cm.cur_time());
        plic.update_ext(1,uart.irq());
        uint64_t exec_core = cm.get_min_wb();
        cores[exec_core]->step(plic.get_int(exec_core*2),clint.m_s_irq(exec_core),clint.m_t_irq(exec_core),plic.get_int(exec_core*2+1));
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