#include <iostream>
#include <bitset>

#include <termios.h>
#include <unistd.h>
#include <thread>
#include <signal.h>

#include "device/nscscc_confreg.hpp"
#include "device/uart8250.hpp"
#include "memory/memory_bus.hpp"
#include "memory/ram.hpp"
#include "core/la32r/la32r_core.hpp"

void uart_input(uart8250 &uart) {
    termios tmp;
    tcgetattr(STDIN_FILENO, &tmp);
    tmp.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tmp);
    while (true) {
        char c = getchar();
        if (c == 10) c = 13; // convert lf to cr
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

int main(int argc, const char *argv[]) {
    memory_bus mmio;

    ram func_mem(1024 * 1024, "./main.bin");
    ram data_mem0(0x1000000);
    ram data_mem1(0x1000000);
    ram data_mem2(0x1000000);
    ram data_mem3(0x1000000);
    ram data_mem4(0x1000000);
    ram data_mem5(0x1000000);
    func_mem.set_allow_warp(true);

    nscscc_confreg confreg(true);

    assert(mmio.add_dev(0x1c000000, 0x100000, &func_mem));
    assert(mmio.add_dev(0x00000000, 0x1000000, &data_mem0));
    assert(mmio.add_dev(0x80000000, 0x1000000, &data_mem1));
    assert(mmio.add_dev(0x90000000, 0x1000000, &data_mem2));
    assert(mmio.add_dev(0xa0000000, 0x1000000, &data_mem1));
    assert(mmio.add_dev(0xc0000000, 0x1000000, &data_mem3));
    assert(mmio.add_dev(0xd0000000, 0x1000000, &data_mem4));
    assert(mmio.add_dev(0xe0000000, 0x1000000, &data_mem5));
    assert(mmio.add_dev(0x1faf0000, 0x10000, &confreg));
    assert(mmio.add_dev(0xbfaf0000, 0x10000, &confreg));

    la32r_core<32> core(0, mmio, true);
    bool running = true;
    while (running) {
        core.step();
        confreg.tick();
        running = !core.is_end();
    }
    return 0;
}
