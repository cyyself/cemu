#ifndef RV_CORE_HPP
#define RV_CORE_HPP

#include <core.hpp>
#include <memory.hpp>
#include <bitset>

class rvcore: public core {
public:
    void step(memory &mem, memory &mmio, std::bitset<4> irq);
};

#endif