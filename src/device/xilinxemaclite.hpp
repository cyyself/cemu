#ifndef XILINX_EMACLITE
#define XILINX_EMACLITE

#include "mmio_dev.hpp"
#include <cstring>
#include <cassert>
#include <queue>
#include <utility>
#include <mutex>

// No MDIO, No ping-pong buffer(Set both number of transmit and receive buffer to 0).

// TODO: ping pong buffer support

enum axiemac_regmap {
    XEL_TXBUFF_OFFST    = 0x0,	    /* Transmit Buffer */
    XEL_TPLR_OFFSET     = 0x07F4,   /* Tx packet length */
    XEL_GIER_OFFSET     = 0x07F8,   /* GIE Register */
    XEL_TSR_OFFSET      = 0x07FC,   /* Tx status */
    XEL_RXBUFF_OFFSET   = 0x1000,   /* Receive Buffer */
    XEL_RPLR_OFFSET     = 0x100C,   /* Rx packet length */
    XEL_RSR_OFFSET      = 0x17FC    /* Rx status */
};

struct xel_tx_ctrl_reg {
    unsigned int status : 1;
    unsigned int program : 1; // program new mac addr
    unsigned int r0 : 1;
    unsigned int int_en : 1; // interrupt enable
    // no loopback support
    unsigned int r1 : 28;
};

struct xel_rx_ctrl_reg {
    unsigned int status : 1;
    unsigned int r0 : 2;
    unsigned int int_en : 1;
    unsigned int r1 : 28;
};

struct xel_gie_reg {
    unsigned int r0 : 31;
    unsigned int gie : 1;
};

template <size_t tx_buffer_size = 16, size_t rx_buffer_size = 16>
class xilinx_emaclite : public mmio_dev {
public:
    xilinx_emaclite() {
        reset();
    }
    void reset() {
        tx_buffer_head = tx_buffer_tail = rx_buffer_head = rx_buffer_tail = 0;
        memset(tx_buffer_len, 0, sizeof(tx_buffer_len));
        memset(rx_buffer_len, 0, sizeof(rx_buffer_len));
        mac_addr[0] = 0x00; mac_addr[1] = 0x00; mac_addr[2] = 0x5E;
        mac_addr[3] = 0x00; mac_addr[4] = 0xFA; mac_addr[5] = 0xCE;
        need_irq = false;
        memset(&tx_ping_ctrl, 0, sizeof(tx_ping_ctrl));
        memset(&rx_ping_ctrl, 0, sizeof(tx_ping_ctrl));
        memset(&gie, 0, sizeof(tx_ping_ctrl));
        memset(tx_ping_buffer, 0, sizeof(tx_ping_buffer));
        tx_ping_buffer_len = 0;
    }
    bool do_read(uint64_t start_addr, uint64_t size, unsigned char* buffer) {
        if (XEL_TXBUFF_OFFST <= start_addr && start_addr+size <= XEL_TXBUFF_OFFST + 2036) { // tx buffer
            memcpy(buffer, tx_ping_buffer+start_addr-XEL_TXBUFF_OFFST, size);
        }
        else if (XEL_RXBUFF_OFFSET <= start_addr && start_addr+size <= XEL_RXBUFF_OFFSET + 2036) { // rx buffer
            memcpy(buffer, rx_buffer[rx_buffer_head]+start_addr-XEL_RXBUFF_OFFSET, size);
        }
        else if (size == 4 && (start_addr&0x3) == 0) { // reg
            // printf("emaclite read %lx size %d \n", start_addr, size);
            switch (start_addr) {
                case XEL_TPLR_OFFSET: {
                    std::unique_lock<std::mutex> lock(tx_lock);
                    memcpy(buffer, &tx_ping_buffer_len, size);
                    break;
                }
                case XEL_GIER_OFFSET:
                    memcpy(buffer, &gie, size);
                    break;
                case XEL_TSR_OFFSET:
                    memcpy(buffer, &tx_ping_ctrl, size);
                    break;
                case XEL_RPLR_OFFSET: {
                    std::unique_lock<std::mutex> lock(rx_lock);
                    memcpy(buffer, &rx_buffer_len[rx_buffer_head], size);
                    break;
                }
                case XEL_RSR_OFFSET:
                    memcpy(buffer, &rx_ping_ctrl, size);
                    break;
                default:
                    memset(buffer, 0x0, size);
                    break;
            }
        }
        else memset(buffer, 0x0, size);
        return true;
    }
    bool do_write(uint64_t start_addr, uint64_t size, const unsigned char* buffer) {
        if (XEL_TXBUFF_OFFST <= start_addr && start_addr+size <= XEL_TXBUFF_OFFST+2036) { // tx buffer
            memcpy(tx_ping_buffer+start_addr-XEL_TXBUFF_OFFST, buffer, size);
        }
        else if (size == 4 && (start_addr&0x3) == 0) { // reg
            // printf("emaclite write %lx size %d \n", start_addr, size);
            switch (start_addr) {
                case XEL_GIER_OFFSET: {
                    xel_gie_reg *to_write = (xel_gie_reg *)buffer;
                    gie.gie = to_write->gie;
                    // printf("gie set to %d\n", gie.gie);
                    break;
                }
                case XEL_TPLR_OFFSET: {
                    std::unique_lock<std::mutex> lock(tx_lock);
                    uint16_t *to_write = (uint16_t *)buffer;
                    tx_ping_buffer_len = *to_write <= 2048 ? *to_write : 0;
                    break;
                }
                case XEL_TSR_OFFSET: {
                    std::unique_lock<std::mutex> lock(tx_lock);
                    xel_tx_ctrl_reg *to_write = (xel_tx_ctrl_reg *)buffer;
                    tx_ping_ctrl.int_en = to_write->int_en;
                    // printf("tx ping int en = %d\n", tx_ping_ctrl.int_en);
                    if (to_write->status) {
                        // add to transmit buffer
                        if (to_write->program) { // program mac addr
                            memcpy(mac_addr, tx_ping_buffer, 6);
                        }
                        else { // transmit eth frame
                            if (tx_buffer_tail + 1 != tx_buffer_head) {
                                memcpy(tx_buffer[tx_buffer_tail], tx_ping_buffer, tx_buffer_len[tx_buffer_tail]);
                                tx_buffer_len[tx_buffer_tail] = tx_ping_buffer_len;
                                tx_buffer_tail ++;
                                if (tx_buffer_tail + 1 == tx_buffer_head) {
                                    tx_ping_ctrl.status = 1; // buffer full, waiting to pop
                                }
                                else {
                                    need_irq |= tx_ping_ctrl.int_en && gie.gie; // immediate irq
                                }
                            }
                        }
                    }
                    break;
                }
                case XEL_RSR_OFFSET: {
                    std::unique_lock<std::mutex> lock(rx_lock);
                    xel_rx_ctrl_reg *to_write = (xel_rx_ctrl_reg *)buffer;
                    rx_ping_ctrl.int_en = to_write->int_en;
                    // printf("rx ping int en = %d\n", rx_ping_ctrl.int_en);
                    if (!to_write->status) {
                        if (rx_buffer_head != rx_buffer_tail) {
                            rx_buffer_head ++;
                            need_irq |= rx_ping_ctrl.int_en && gie.gie; // immediate irq
                        }
                        else {
                            rx_ping_ctrl.status = 0;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return true;
    }
    bool tx_frame(size_t size, char *src) { // link partner side tx, cemu side rx
        assert(size <= 2048);
        std::unique_lock<std::mutex> lock(rx_lock);
        if ((rx_buffer_tail+1)%rx_buffer_size == rx_buffer_head) return false;
        if (size >= 14) { // illegal frame
            // check mac addr, must be the dst addr or broadcast
            if (strncmp(src, mac_addr, 6) == 0 || strncmp(src, "\xff\xff\xff\xff\xff\xff", 6) == 0) {
                memcpy(rx_buffer[rx_buffer_tail], src, size);
                rx_buffer_len[rx_buffer_tail] = size;
                rx_buffer_tail++;
                need_irq |= rx_ping_ctrl.int_en && gie.gie;
                rx_ping_ctrl.status = 1;
            }
            // else printf("warning packet filterd\n");
            return true;
            // return true even if filterd
            
        }
        else return false;
    }
    size_t rx_frame(char *dst) { // link partner side rx, cemu side tx
        // dst should not smaller than 2048 bytes, return 0 if no packet available
        std::unique_lock<std::mutex> lock(tx_lock);
        if (tx_buffer_head == tx_buffer_tail) return 0; // no new frame
        memcpy(dst, tx_buffer[tx_buffer_head], tx_buffer_len[tx_buffer_head]);
        uint16_t res = tx_buffer_len[tx_buffer_head];
        if (tx_buffer_head + 1 == tx_buffer_tail) {
            // full -> available
            tx_ping_ctrl.status = 0;
            need_irq |= tx_ping_ctrl.int_en && gie.gie;
        }
        tx_buffer_head ++;
        return res;
    }
    bool edge_irq() {
        bool res = need_irq;
        need_irq = false;
        return res;
    }
private:
    char mac_addr[6];
    const unsigned char bcast_addr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    xel_gie_reg     gie;
    xel_tx_ctrl_reg tx_ping_ctrl;
    xel_rx_ctrl_reg rx_ping_ctrl;
    uint16_t tx_buffer_len[tx_buffer_size], rx_buffer_len[rx_buffer_size];
    char tx_buffer[tx_buffer_size][2048], rx_buffer[rx_buffer_size][2048];
    char tx_ping_buffer[2048];
    uint16_t tx_ping_buffer_len;
    // rx zero copy
    unsigned int tx_buffer_head, tx_buffer_tail, rx_buffer_head, rx_buffer_tail;
    std::mutex tx_lock, rx_lock;
    bool need_irq;
};

#endif