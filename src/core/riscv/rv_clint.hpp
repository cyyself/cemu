#ifndef RV_CLINT_HPP
#define RV_CLINT_HPP

#include "mmio_dev.hpp"
#include <cstdint>

template <unsigned int nr_hart=1>
class rv_clint : public mmio_dev {
public:
    rv_clint() {
        mtime = 0;
        for (int i=0;i<nr_hart;i++) {
            mtimecmp[i] = 0;
            msip[i] = 0;
        }
    }
    bool do_read(uint64_t start_addr, uint64_t size, unsigned char* buffer) {
        if (start_addr >= 0x4000) {
            // mtimecmp, mtime
            if (start_addr >= 0xbff8 && start_addr + size <= 0xc000) {
                // mtime
                memcpy(buffer,((char*)(&mtime))+start_addr-0xbff8,size);
                // printf("read mtime\n");
            }
            else if (start_addr >= 0x4000 && start_addr + size <= 0x4000 + 8 * nr_hart) {
                memcpy(buffer,((char*)(&mtimecmp))+start_addr-0x4000,size);
                // printf("read mtimecmp\n");
            }
            else return false;
        }
        else {
            // msip
            if (start_addr+size <= 4*nr_hart) {
                memcpy(buffer,((char*)(&msip))+start_addr,size);
                // printf("read msip\n");
            }
            else return false;
        }
        return true;
    }
    bool do_write(uint64_t start_addr, uint64_t size, const unsigned char* buffer) {
        if (start_addr >= 0x4000) {
            // mtimecmp, mtime
            if (start_addr >= 0xbff8 && start_addr + size <= 0xc000) {
                // mtime
                memcpy(((char*)(&mtime))+start_addr-0xbff8,buffer,size);
                // printf("write mtime\n");
            }
            else if (start_addr >= 0x4000 && start_addr + size <= 0x4000 + 8 * nr_hart) {
                memcpy(((char*)(&mtimecmp))+start_addr-0x4000,buffer,size);
                // printf("write mtimecmp %lx mtime %lx size %d diff %lx\n",mtimecmp[0],mtime,(int)size,mtimecmp[0]-mtime);
            }
            else return false;
        }
        else {
            // msip
            if (start_addr+size <= 4*nr_hart) {
                memcpy(((char*)(&msip))+start_addr,buffer,size);
                for (int i=0;i<nr_hart;i++) msip[i] &= 1;
                // printf("write msip[0] %x\n",msip[0]);
                // printf("write msip[0] %x\n",msip[1]);
            }
            else return false;
        }
        return true;
    }
    void tick() {
        mtime ++;
    }
    bool m_s_irq(unsigned int hart_id) { // machine software irq
        if (hart_id >= nr_hart) assert(false);
        else return (msip[hart_id] & 1);
    }
    bool m_t_irq(unsigned int hart_id) { // machine timer irq
        if (hart_id >= nr_hart) assert(false);
        else return mtime > mtimecmp[hart_id];
    }
    void set_cmp(uint64_t new_val) {
        mtimecmp = new_val;
    }
private:
    uint64_t mtime;
    uint64_t mtimecmp[nr_hart];
    uint32_t msip[nr_hart];
};

#endif