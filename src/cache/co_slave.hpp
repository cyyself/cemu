#ifndef CO_SLAVE_HPP
#define CO_SLAVE_HPP

#include <cstdint>

class co_slave {
public:
    virtual void invalidate_exclusive(uint64_t start_addr) = 0;
    virtual void invalidate_shared(uint64_t start_addr) = 0;
};

enum l1_line_status {
    L1_INVALID, L1_SHARED, L1_EXCLUSIVE, L1_MODIFIED
};

#endif