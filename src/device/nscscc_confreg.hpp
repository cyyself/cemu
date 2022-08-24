#ifndef NSCSCC_CONFREG
#define NSCSCC_CONFREG

#include "mmio_dev.hpp"
#include <cstring>
#include <cassert>
#include <queue>
#include <fstream>

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
class nscscc_confreg : public mmio_dev {
public:
    nscscc_confreg(bool simulation = false) {
        timer = 0;
        memset(cr,0,sizeof(cr));
        led = 0;
        led_rg0 = 0;
        led_rg1 = 0;
        num = 0;
        simu_flag = simulation ? 0xffffffffu : 0;
        io_simu = 0;
        open_trace = 1;
        num_monitor = 1;
        virtual_uart = 0;
        set_switch(0);
    }
    void tick() {
        timer ++;
    }
    bool do_read(uint64_t start_addr, uint64_t size, unsigned char* buffer) {
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
                *(unsigned int *)buffer = switch_data;
                break;
            case BTN_KEY_ADDR:
                *(unsigned int *)buffer = 0;
                break;
            case BTN_STEP_ADDR:
                *(unsigned int *)buffer = 0;
                break;
            case SW_INTER_ADDR:
                *(unsigned int *)buffer = switch_inter_data;
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
                break;
            case NUM_MONITOR_ADDR:
                *(unsigned int *)buffer = num_monitor;
                break;
            default:
                *(unsigned int *)buffer = 0;
                break;
        }
        return true;
    }
    bool do_write(uint64_t start_addr, uint64_t size, const unsigned char* buffer) {
        assert(size == 4 || (size == 1 && start_addr == VIRTUAL_UART_ADDR));
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
            case OPEN_TRACE_ADDR: {
                open_trace = (*(unsigned int*)buffer) != 0;
                break;
            }
            case NUM_MONITOR_ADDR:
                num_monitor = (*(unsigned int*)buffer) & 1;
                break;
            case VIRTUAL_UART_ADDR:
                virtual_uart = (*(char*)buffer) & 0xff;
                uart_queue.push(virtual_uart);
                break;
            case NUM_ADDR:
                num = *(unsigned int*)buffer;
                break;
            default:
                break;
        }
        return true;
    }
    bool trace_on() {
        return open_trace != 0;
    }
    void set_switch(uint8_t value) {
        switch_data = value ^ 0xf;
        switch_inter_data = 0;
        for (int i=0;i<=7;i++) {
            if ( ((value >> i) & 1) == 0) {
                switch_inter_data |= 2<<(2*i);
            }
        }
    }
    bool has_uart() {
        return !uart_queue.empty();
    }
    char get_uart() {
        char res = uart_queue.front();
        uart_queue.pop();
        return res;
    }
    uint32_t get_num() {
        return num;
    }
    // trace
    void set_trace_file(std::string filename) {
        trace_file.open(filename);
    }
    bool do_trace(uint32_t pc, uint8_t wen, uint8_t wnum, uint32_t wdata, bool output = true) {
        uint32_t trace_cmp_flag;
        uint32_t ref_pc;
        uint32_t ref_wnum;
        uint32_t ref_wdata;
        if (trace_on() && wen && wnum) {
            while (trace_file >> std::hex >> trace_cmp_flag >> ref_pc >> ref_wnum >> ref_wdata && !trace_cmp_flag) {

            }
            if (pc != ref_pc || wnum != ref_wnum || wdata != ref_wdata) {
                if (output) {
                    printf("Error!\n");
                    printf("reference: PC = 0x%08x, wb_rf_wnum = 0x%02x, wb_rf_wdata = 0x%08x\n", ref_pc, ref_wnum, ref_wdata);
                    printf("mycpu    : PC = 0x%08x, wb_rf_wnum = 0x%02x, wb_rf_wdata = 0x%08x\n", pc, wnum, wdata);
                }
                return false;
            }
        }
        return true;
    }
private:
    std::ifstream trace_file;
    uint32_t cr[8];
    unsigned int switch_data;
    unsigned int switch_inter_data;
    unsigned int timer;
    unsigned int led;
    unsigned int led_rg0;
    unsigned int led_rg1;
    unsigned int num;
    unsigned int simu_flag;
    unsigned int io_simu;
    char virtual_uart;
    unsigned int open_trace;
    unsigned int num_monitor;
    std::queue <char> uart_queue;
};

#endif