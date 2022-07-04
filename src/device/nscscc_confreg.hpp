#ifndef NSCSCC_CONFREG
#define NSCSCC_CONFREG

#include "memory.hpp"
#include <cstring>
#include <cassert>

// offset += 0x1faf8000

#define CR0_ADDR            0x8000  //32'hbfaf_8000 
#define CR1_ADDR            0x8004  //32'hbfaf_8004 
#define CR2_ADDR            0x8008  //32'hbfaf_8008 
#define CR3_ADDR            0x800c  //32'hbfaf_800c 
#define CR4_ADDR            0x8010  //32'hbfaf_8010 
#define CR5_ADDR            0x8014  //32'hbfaf_8014 
#define CR6_ADDR            0x8018  //32'hbfaf_8018 
#define CR7_ADDR            0x801c  //32'hbfaf_801c 
#define LED_ADDR            0xf000  //32'hbfaf_f000 
#define LED_RG0_ADDR        0xf004  //32'hbfaf_f004 
#define LED_RG1_ADDR        0xf008  //32'hbfaf_f008 
#define NUM_ADDR            0xf010  //32'hbfaf_f010 
#define SWITCH_ADDR         0xf020  //32'hbfaf_f020 
#define BTN_KEY_ADDR        0xf024  //32'hbfaf_f024
#define BTN_STEP_ADDR       0xf028  //32'hbfaf_f028
#define SW_INTER_ADDR       0xf02c  //32'hbfaf_f02c 
#define TIMER_ADDR          0xe000  //32'hbfaf_e000 
#define IO_SIMU_ADDR        0xffec  //32'hbfaf_ffec
#define VIRTUAL_UART_ADDR   0xfff0  //32'hbfaf_fff0
#define SIMU_FLAG_ADDR      0xfff4  //32'hbfaf_fff4 
#define OPEN_TRACE_ADDR     0xfff8  //32'hbfaf_fff8
#define NUM_MONITOR_ADDR    0xfffc  //32'hbfaf_fffc

// physical address = [0x1faf0000,0x1fafffff]
class nscscc_confreg : public memory {
public:
    nscscc_confreg() {
        timer = 0;
        memset(cr,0,sizeof(cr));
        led = 0;
        led_rg0 = 0;
        led_rg1 = 0;
        num = 0;
        simu_flag = 0xffffffffu;
        io_simu = 0;
        open_trace = 1;
        num_monitor = 1;
        virtual_uart = 0;
    }
    void tick() {
        timer ++;
    }
    bool do_read(unsigned long start_addr, unsigned long size, unsigned char* buffer) {
        assert(size == 4);
        switch (start_addr) {
            case CR0_ADDR:
                *(unsigned int*)buffer = cr[0];
                break;
            case CR1_ADDR:
                *(unsigned int*)buffer = cr[1];
                break;
            case CR2_ADDR:
                *(unsigned int*)buffer = cr[2];
                break;
            case CR3_ADDR:
                *(unsigned int*)buffer = cr[3];
                break;
            case CR4_ADDR:
                *(unsigned int*)buffer = cr[4];
                break;
            case CR5_ADDR:
                *(unsigned int*)buffer = cr[5];
                break;
            case CR6_ADDR:
                *(unsigned int*)buffer = cr[6];
                break;
            case CR7_ADDR:
                *(unsigned int*)buffer = cr[7];
                break;
            case LED_ADDR:
                *(unsigned int *)buffer = led;
                break;
            case LED_RG0_ADDR:
                *(unsigned int *)buffer = led_rg0;
                break;
            case LED_RG1_ADDR:
                *(unsigned int *)buffer = led_rg1;
                break;
            case NUM_ADDR:
                *(unsigned int *)buffer = num;
                break;
            case SWITCH_ADDR:
                *(unsigned int *)buffer = 0;
                break;
            case BTN_KEY_ADDR:
                *(unsigned int *)buffer = 0;
                break;
            case BTN_STEP_ADDR:
                *(unsigned int *)buffer = 0;
                break;
            case SW_INTER_ADDR:
                *(unsigned int *)buffer = 0;
                break;
            case TIMER_ADDR:
                *(unsigned int *)buffer = timer;
                break;
            case SIMU_FLAG_ADDR:
                *(unsigned int *)buffer = simu_flag;
                break;
            case IO_SIMU_ADDR:
                *(unsigned int *)buffer = io_simu;
                break;
            case VIRTUAL_UART_ADDR:
                *(unsigned int *)buffer = virtual_uart;
                break;
            case OPEN_TRACE_ADDR:
                *(unsigned int *)buffer = open_trace;
            case NUM_MONITOR_ADDR:
                *(unsigned int *)buffer = num_monitor;
            default:
                *(unsigned int *)buffer = 0;
                break;
        }
    }
    bool do_write(unsigned long start_addr, unsigned long size, const unsigned char* buffer) {
        assert(size == 4);
        switch (start_addr) {
            case CR0_ADDR:
                cr[0] = *(unsigned int*)buffer;
                break;
            case CR1_ADDR:
                cr[1] = *(unsigned int*)buffer;
                break;
            case CR2_ADDR:
                cr[2] = *(unsigned int*)buffer;
                break;
            case CR3_ADDR:
                cr[3] = *(unsigned int*)buffer;
                break;
            case CR4_ADDR:
                cr[4] = *(unsigned int*)buffer;
                break;
            case CR5_ADDR:
                cr[5] = *(unsigned int*)buffer;
                break;
            case CR6_ADDR:
                cr[6] = *(unsigned int*)buffer;
                break;
            case CR7_ADDR:
                cr[7] = *(unsigned int*)buffer;
                break;
            case TIMER_ADDR:
                timer = *(unsigned int*)buffer;
                break;
            case IO_SIMU_ADDR:
                io_simu = (((*(unsigned int*)buffer) & 0xffff) << 16) | ((*(unsigned int*)buffer) >> 16);
                break;
            case OPEN_TRACE_ADDR:
                open_trace = (*(unsigned int*)buffer) != 0;
                break;
            case NUM_MONITOR_ADDR:
                num_monitor = (*(unsigned int*)buffer) & 1;
                break;
            case VIRTUAL_UART_ADDR:
                virtual_uart = (*(unsigned int*)buffer) & 0xf;
                break;
            default:
                break;
        }
    }
private:
    uint32_t cr[8];
    unsigned int timer;
    unsigned int led;
    unsigned int led_rg0;
    unsigned int led_rg1;
    unsigned int num;
    unsigned int simu_flag;
    unsigned int io_simu;
    unsigned int virtual_uart;
    unsigned int open_trace;
    unsigned int num_monitor;
};

#endif