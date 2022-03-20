#ifndef CORE_HPP
#define CORE_HPP

#include "cpu.hpp"

class core {
public:
    virtual void eval() = 0;
    friend class cpu;
};

#endif