#include <iostream>
#include <bitset>
#include <cassert>

#include "mips_core.hpp"
#include "nscscc_confreg.hpp"
#include "memory_bus.hpp"
#include "ram.hpp"

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

    mips_core mips(mmio, confreg);
    mips.set_confreg_trace(true);

    int test_point = 0;
    while (true) {
        mips.step();
        confreg.tick();
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

    mips_core mips(mmio, confreg);

    for (int test_num=1;test_num<=10;test_num ++) {
        confreg.set_switch(test_num);
        mips.reset();
        while (true) {
            mips.step();
            confreg.tick();
            if (mips.get_pc() == 0xbfc00100u) break;
        }
        printf("%x\n",confreg.get_num());
    }
}

int main(int argc, const char* argv[]) {
    nscscc_func();
    nscscc_perf();
    return 0;
}