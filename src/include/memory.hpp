#ifndef MEMORY_HPP
#define MEMORY_HPP

class memory {
public:
    virtual bool do_read (unsigned long start_addr, unsigned long size, unsigned char* buffer) = 0;
    virtual bool do_write(unsigned long start_addr, unsigned long size, const unsigned char* buffer) = 0;
};

#endif