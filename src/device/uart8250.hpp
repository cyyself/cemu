#ifndef UART8250_HPP
#define UART8250_HPP

#include "mmio_dev.hpp"
#include <mutex>
#include <queue>
#include <assert.h>

#define UART8250_TX_RX_DLL  0
#define UART8250_IER_DLM    1
#define UART8250_IIR_FCR    2
#define UART8250_LCR        3
#define UART8250_MCR        4
#define UART8250_LSR        5
#define UART8250_MSR        6
#define UART8250_SCR        7

#define UART8250_IER_RDA    1   // Received data available
#define UART8250_IER_THRE   2   // Transmitter holding register empty
#define UART8250_IER_LSRC   4   // Receiver line status register chagne
#define UART8250_IER_MSRC   8   // Modem status register change

class uart8250 : public mmio_dev {
public:
    uart8250() {
        thr_empty = false;
        DLL = 0;
        DLM = 0;
        IER = 0;
        LCR = 0b00000011; // 8in1
        IIR = 0;
        MCR = 0;
    }
    bool do_read(uint64_t start_addr, uint64_t size, unsigned char* buffer) {
        std::unique_lock<std::mutex> lock(rx_lock);
        assert(size == 1);
        switch (start_addr) {
            case UART8250_TX_RX_DLL: {
                if (DLAB()) {
                    // DLL
                    *buffer = DLL;
                }
                else {
                    // RX
                    if (!rx.empty()) {
                        *buffer = rx.front();
                        rx.pop();
                    }
                }
                break;
            }
            case UART8250_IER_DLM: {
                if (DLAB()) {
                    // DLM
                    *buffer = DLM;
                }
                else {
                    // IER
                    *buffer = IER;
                }
                break;
            }
            case UART8250_IIR_FCR: {
                // IIR
                update_IIR();
                *buffer = IIR;
                thr_empty = false;
                break;
            }
            case UART8250_LCR: {
                *buffer = LCR;
                break;
            }
            case UART8250_MCR: {
                *buffer = MCR;
                break;
            }
            case UART8250_LSR: {
                *buffer = (!rx.empty()) | (1<<5) | (1<<6);
                break;
            }
            case UART8250_MSR: {
                // Carrier detect = 1
                // Clear to send = 1
                *buffer = (1<<7) | (1<<4);
                break;
            }
            case UART8250_SCR: {
                *buffer = 0;
                break;
            }
            default:
                assert(false);
        }
        return true;
    }
    bool do_write(uint64_t start_addr, uint64_t size, const unsigned char* buffer) {
        std::unique_lock<std::mutex> lock_tx(tx_lock);
        std::unique_lock<std::mutex> lock_rx(rx_lock);
        assert(size == 1);
        switch (start_addr) {
            case UART8250_TX_RX_DLL: {
                if (DLAB()) {
                    DLL = *buffer;
                }
                else {
                    tx.push(static_cast<char>(*buffer));
                    thr_empty = false;
                }
                break;
            }
            case UART8250_IER_DLM: {
                if (DLAB()) {
                    DLM = *buffer;
                }
                else {
                    IER = *buffer;
                }
                break;
            }
            case UART8250_IIR_FCR: {
                // FCR allow clear FIFO
                if ( (*buffer) & (1 << 1)) {
                    while (!rx.empty()) rx.pop();
                }
                if ( (*buffer) & (1 << 2)) {
                    while (!tx.empty()) tx.pop();
                }
                break;
            }
            case UART8250_LCR: {
                LCR = *buffer;
                break;
            }
            case UART8250_MCR: {
                MCR = *buffer;
                break;
            }
            case UART8250_LSR: {
                // error
                break;
            }
            case UART8250_MSR: {
                // error
                break;
            }
            case UART8250_SCR: {
                // error
                break;
            }
            default:
                assert(false);
        }
        return true;
    }
    void putc(char c) {
        std::unique_lock<std::mutex> lock(rx_lock);
        rx.push(c);
    }
    char getc() {
        std::unique_lock<std::mutex> lock(tx_lock);
        if (!tx.empty()) {
            char res = tx.front();
            tx.pop();
            if (tx.empty()) thr_empty = true;
            return res;
        }
        else return EOF;
    }
    bool irq() {
        update_IIR();
        return !(IIR & 1);
    }
    bool exist_tx() {
        std::unique_lock<std::mutex> lock(tx_lock);
        return !tx.empty();
    }
private:
    bool DLAB() {
        return (LCR >> 7) != 0;
    }
    void update_IIR() {
        IIR = 3 << 6; // FIFO Enabled
        bool no_int = true;
        if ( (IER & UART8250_IER_THRE) && thr_empty) {
            IIR |= (1<<1);
            no_int = false;
        }
        if ( (IER & UART8250_IER_RDA) && !rx.empty()) {
            IIR |= (2<<1);
            no_int = false;
        }
        IIR |= no_int;
    }
    const static uint64_t UART_RX = 0;
    const static uint64_t UART_TX = 0;
    std::queue <char> rx;
    std::queue <char> tx;
    std::mutex rx_lock;
    std::mutex tx_lock;

    bool thr_empty;
    // regs
    unsigned char DLL;
    unsigned char DLM;
    unsigned char IER;
    unsigned char LCR;
    unsigned char IIR;
    unsigned char MCR;
};

#endif