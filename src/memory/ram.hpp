#ifndef RAM_HPP
#define RAM_HPP

#include "memory.hpp"
#include <cstring>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <assert.h>

class ram: public memory {
public:
    ram(unsigned long size_bytes) {
        mem = new unsigned char[size_bytes];
        mem_size = size_bytes;
    }
    ram(unsigned long size_bytes, const unsigned char *init_binary, unsigned long init_binary_len):ram(size_bytes) {
        assert(init_binary_len <= size_bytes);
        memcpy(mem,init_binary,init_binary_len);
    }
    ram(unsigned long size_bytes, const char *init_file):ram(size_bytes) {
        unsigned long file_size = std::filesystem::file_size(init_file);
        if (file_size > mem_size) {
            std::cerr << "ram size is not big enough for init file." << std::endl;
            file_size = size_bytes;
        }
        std::ifstream file(init_file,std::ios::in | std::ios::binary);
        file.read((char*)mem,file_size);
    }
    bool do_read (unsigned long start_addr, unsigned long size, unsigned char* buffer) {
        if (start_addr + size <= mem_size) {
            memcpy(buffer,&mem[start_addr],size);
            return true;
        }
        else return false;
    }
    bool do_write(unsigned long start_addr, unsigned long size, const unsigned char* buffer) {
        if (start_addr + size <= mem_size) {
            memcpy(&mem[start_addr],buffer,size);
            return true;
        }
        else return false;
    }
private:
    unsigned char *mem;
    unsigned long mem_size;
};

#endif