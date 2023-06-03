#include "cemu_lib.h"
#include <stdio.h>

void cemu_mmio(uint64_t addr, void *buf, uint64_t len, bool is_write) {
    printf("get cemu mmio addr = %lx, len = %ld\n", addr, len);
}

int main() {
    cemu_main("../opensbi/build/platform/generic/firmware/fw_payload.bin");
    return 0;
}