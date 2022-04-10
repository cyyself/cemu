# Steps to booting Linux with RISCV-CEMU

## Step 0. Toolchain

Make sure your toolchain is built with parameter `--with-arch=rv64imac --with-abi=lp64`.

To build Toolchain, Linux, and Busybox, you can refer to [https://twd2.me/archives/13406](https://twd2.me/archives/13406).

## Step 1. Build Linux and rootfs

You can build with defconfig but you need to change the following settings.

For rootfs, you can use busybox.

```
CONFIG_CMDLINE="earlycon=uartlite,mmio,0x60100000 console=ttyUL0 rdinit=/sbin/init"
CONFIG_INITRAMFS_SOURCE="../rootfs"
CONFIG_SERIAL_UARTLITE=y
CONFIG_SERIAL_UARTLITE_CONSOLE=y
CONFIG_SERIAL_UARTLITE_NR_UARTS=1
CONFIG_EFI=n # Note: CONFIG_EFI should set to n before set CONFIG_RISCV_ISA_C
CONFIG_RISCV_ISA_C=n
```

## Step 2. Write Device Tree source

```dts
/dts-v1/;

/ {
	#address-cells = <1>;
	#size-cells = <1>;
	compatible = "cyy,cemu";
	model = "cyy,cemu";

    chosen {
        bootargs = "earlycon=uartlite,mmio,0x60100000 console=ttyUL0 rdinit=/sbin/init";
        stdout-path = "serial0";
    };

	aliases {
		serial0 = &uart0;
	};

    uart0: uartlite_0@60100000 {
        compatible = "xlnx,axi-uartlite-1.02.a", "xlnx,xps-uartlite-1.00.a";
        reg = <0x60100000 0x1000>;
        interrupt-parent = <&L8>;
        interrupts = <1>;
        clock = <&L0>;
        current-speed = <115200>;
        xlnx,data-bits = <8>;
        xlnx,use-parity = <0>;
    };

	L18: cpus {
		#address-cells = <1>;
		#size-cells = <0>;
		timebase-frequency = <100000000>;
		L4: cpu@0 {
			clock-frequency = <0>;
			compatible = "cyy,cemu-core", "riscv";
			d-cache-block-size = <64>;
			d-cache-sets = <64>;
			d-cache-size = <16384>;
			d-tlb-sets = <1>;
			d-tlb-size = <32>;
			device_type = "cpu";
			hardware-exec-breakpoint-count = <1>;
			i-cache-block-size = <64>;
			i-cache-sets = <64>;
			i-cache-size = <16384>;
			i-tlb-sets = <1>;
			i-tlb-size = <32>;
			mmu-type = "riscv,sv39";
			next-level-cache = <&L13>;
			reg = <0x0>;
			riscv,isa = "rv64ima";
			riscv,pmpgranularity = <4>;
			riscv,pmpregions = <8>;
			status = "okay";
			timebase-frequency = <100000000>;
			tlb-split;
			L2: interrupt-controller {
				#interrupt-cells = <1>;
				compatible = "riscv,cpu-intc";
				interrupt-controller;
			};
		};
		L7: cpu@1 {
			clock-frequency = <0>;
			compatible = "cyy,cemu-core", "riscv";
			d-cache-block-size = <64>;
			d-cache-sets = <64>;
			d-cache-size = <16384>;
			d-tlb-sets = <1>;
			d-tlb-size = <32>;
			device_type = "cpu";
			hardware-exec-breakpoint-count = <1>;
			i-cache-block-size = <64>;
			i-cache-sets = <64>;
			i-cache-size = <16384>;
			i-tlb-sets = <1>;
			i-tlb-size = <32>;
			mmu-type = "riscv,sv39";
			next-level-cache = <&L13>;
			reg = <0x1>;
			riscv,isa = "rv64ima";
			riscv,pmpgranularity = <4>;
			riscv,pmpregions = <8>;
			status = "okay";
			timebase-frequency = <100000000>;
			tlb-split;
			L5: interrupt-controller {
				#interrupt-cells = <1>;
				compatible = "riscv,cpu-intc";
				interrupt-controller;
			};
		};
	};
	L13: memory@80000000 {
		device_type = "memory";
		reg = <0x80000000 0x80000000>;
	};
	L17: soc {
		#address-cells = <1>;
		#size-cells = <1>;
		compatible = "freechips,rocketchip-unknown-soc", "simple-bus";
		ranges;
		L9: clint@2000000 {
			compatible = "riscv,clint0";
			interrupts-extended = <&L2 3 &L2 7 &L5 3 &L5 7>;
			reg = <0x2000000 0x10000>;
			reg-names = "control";
		};
		L12: external-interrupts {
			interrupt-parent = <&L8>;
			interrupts = <1 2 3 4>;
		};
		L8: interrupt-controller@c000000 {
			#interrupt-cells = <1>;
			compatible = "riscv,plic0";
			interrupt-controller;
			interrupts-extended = <&L2 11 &L2 9 &L5 11 &L5 9>;
			reg = <0xc000000 0x4000000>;
			reg-names = "control";
			riscv,max-priority = <7>;
			riscv,ndev = <4>;
		};
		L14: mmio-port-axi4@60000000 {
			#address-cells = <1>;
			#size-cells = <1>;
			compatible = "simple-bus";
			ranges = <0x60000000 0x60000000 0x20000000>;
		};
		L0: subsystem_pbus_clock {
			#clock-cells = <0>;
			clock-frequency = <100000000>;
			clock-output-names = "subsystem_pbus_clock";
			compatible = "fixed-clock";
		};
	};
};
```

## Step 3. Build OpenSBI

```shell
git clone git@github.com:riscv-software-src/opensbi.git
cd opensbi
# Save dts to cemu.dts
dtc cemu.dts -o cemu.dtb
make CROSS_COMPILE=riscv64-unknown-linux-gnu- PLATFORM=generic FW_FDT_PATH=cemu.dtb FW_PAYLOAD_PATH=../linux/arch/riscv/boot/Image
```

## Step 4. Write CEMU main and build

```cpp
// at cemu/src/main.cpp
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
        uart.putc(c);
    }
}

int main(int argc, const char* argv[]) {

    const char *load_path = "../opensbi/build/platform/generic/firmware/fw_payload.bin";
    if (argc >= 2) load_path = argv[1];
    for (int i=1;i<argc;i++) if (strcmp(argv[i],"-rvtest") == 0) riscv_test = true;

    rv_systembus system_bus;

    uartlite uart;
    rv_clint<2> clint;
    rv_plic <4,4> plic;
    ram dram(4096l*1024l*1024l,load_path);
    assert(system_bus.add_dev(0x2000000,0x10000,&clint));
    assert(system_bus.add_dev(0xc000000,0x4000000,&plic));
    assert(system_bus.add_dev(0x60100000,1024*1024,&uart));
    assert(system_bus.add_dev(0x80000000,2048l*1024l*1024l,&dram));

    rv_core rv_0(system_bus,0);
    rv_0_ptr = &rv_0;
    rv_core rv_1(system_bus,1);
    rv_1_ptr = &rv_1;

    std::thread        uart_input_thread(uart_input,std::ref(uart));

    rv_0.jump(0x80000000);
    rv_1.jump(0x80000000);
    rv_1.set_GPR(10,1);
    while (1) {
        clint.tick();
        plic.update_ext(1,uart.irq());
        rv_0.step(plic.get_int(0),clint.m_s_irq(0),clint.m_t_irq(0),plic.get_int(1));
        rv_1.step(plic.get_int(2),clint.m_s_irq(1),clint.m_t_irq(1),plic.get_int(3));
        while (uart.exist_tx()) {
            char c = uart.getc();
            if (c != '\r') std::cout << c;
        }
        std::cout.flush();
    }
    return 0;
}
```

```shell
cd cemu
make
./cemu
```