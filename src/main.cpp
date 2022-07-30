#include <iostream>
#include <bitset>
#include <cassert>
#include <thread>
#include <termios.h>
#include <cassert>
#include <unistd.h>
#include <csignal>

#include "mips_core.hpp"
#include "nscscc_confreg.hpp"
#include "memory_bus.hpp"
#include "ram.hpp"
#include "uart8250.hpp"

void uart_input(uart8250 &uart) {
    termios tmp;
    tcgetattr(STDIN_FILENO,&tmp);
    tmp.c_lflag &=(~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO,TCSANOW,&tmp);
    while (true) {
        char c = getchar();
        if (c == 10) c = 13; // convert lf to cr
        uart.putc(c);
    }
}

void nscscc_func() {
    memory_bus mmio;
    
    ram func_mem(262144*4, "../nscscc-group/func_test_v0.01/soft/func/obj/main.bin");
    func_mem.set_allow_warp(true);
    assert(mmio.add_dev(0x1fc00000,0x100000  ,&func_mem));
    assert(mmio.add_dev(0x00000000,0x10000000,&func_mem));
    assert(mmio.add_dev(0x20000000,0x20000000,&func_mem));
    assert(mmio.add_dev(0x40000000,0x40000000,&func_mem));
    assert(mmio.add_dev(0x80000000,0x80000000,&func_mem));

    nscscc_confreg confreg(true);
    confreg.set_trace_file("../nscscc-group/func_test_v0.01/cpu132_gettrace/golden_trace.txt");
    assert(mmio.add_dev(0x1faf0000,0x10000,&confreg));

    mips_core mips(mmio);

    uint32_t test_point = 0;
    bool running = true;
    while (running) {
        mips.step();
        confreg.tick();
        running = confreg.do_trace(mips.debug_wb_pc, mips.debug_wb_wen, mips.debug_wb_wnum, mips.debug_wb_wdata);
        while (confreg.has_uart()) printf("%c", confreg.get_uart());
        if (confreg.get_num() != test_point) {
            test_point = confreg.get_num();
            printf("Number %d Functional Test Point PASS!\n", test_point>>24);
            if ( (test_point >> 24) == 89) return;
        }
    }
}

void nscscc_perf() {
    memory_bus mmio;
    
    ram perf_mem(262144*4, "../nscscc-group/perf_test_v0.01/soft/perf_func/obj/allbench/inst_data.bin");
    perf_mem.set_allow_warp(true);
    assert(mmio.add_dev(0x1fc00000,0x100000  ,&perf_mem));
    assert(mmio.add_dev(0x00000000,0x10000000,&perf_mem));
    assert(mmio.add_dev(0x20000000,0x20000000,&perf_mem));
    assert(mmio.add_dev(0x40000000,0x40000000,&perf_mem));
    assert(mmio.add_dev(0x80000000,0x80000000,&perf_mem));

    nscscc_confreg confreg(false);
    assert(mmio.add_dev(0x1faf0000,0x10000,&confreg));

    mips_core mips(mmio);

    for (int test_num=1;test_num<=10;test_num ++) {
        confreg.set_switch(test_num);
        mips.reset();
        while (true) {
            mips.step();
            confreg.tick();
            if (mips.get_pc() == 0xbfc00100u) break;
        }
        printf("%x\n",confreg.get_num());
        // printf("%u,%u,%u,%u,%u\n",confreg.get_num(),mips.forward_branch,mips.forward_branch_taken,mips.backward_branch,mips.backward_branch_taken);
        // printf("%u,%u\n", mips.insret, mips.forward_branch_taken + mips.backward_branch_taken);
        // printf("forward : %u,  forward_taken: %u,  forward_taken_rate: %0.5lf.\n", mips.forward_branch, mips.forward_branch_taken, (double)mips.forward_branch_taken/mips.forward_branch);
        // printf("backward: %u, backward_taken: %u, backward_taken_rate: %0.5lf.\n", mips.backward_branch, mips.backward_branch_taken, (double)mips.backward_branch_taken/mips.backward_branch);
    }
}

bool send_ctrl_c;

void sigint_handler(int x) {
    static time_t last_time;
    if (time(NULL) - last_time < 1) exit(0);
    last_time = time(NULL);
    send_ctrl_c = true;
}

void ucore_run(int argc, const char* argv[]) {
    signal(SIGINT, sigint_handler);

    memory_bus cemu_mmio;

    ram cemu_memory(128*1024*1024, "../ucore-thumips/obj/ucore-kernel-initrd.bin");
    assert(cemu_mmio.add_dev(0,128*1024*1024,&cemu_memory));

    // uart8250 at 0x1fe40000 (APB)
    uart8250 uart;
    std::thread *uart_input_thread = new std::thread(uart_input,std::ref(uart));
    assert(cemu_mmio.add_dev(0x1fe40000,0x10000,&uart));

    mips_core mips(cemu_mmio);
    mips.jump(0x80000000u);
    uint32_t lastpc = 0;
    bool delay_cr = false;
    while (true) {
        mips.step(uart.irq() << 1);
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
        if (mips.get_pc() == lastpc) {
            printf("error!\n");
            exit(1);
        }
        else lastpc = mips.get_pc();
    }
}

int main(int argc, const char* argv[]) {
    // nscscc_func();
    // nscscc_perf();
    ucore_run(argc, argv);
    return 0;
}