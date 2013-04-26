#ifndef     _BITOPS_H_
#define     _BITOPS_H_

#include <stdint.h>

static __inline__ void __set_bit (int nr, volatile void *addr)
{
    *((uint32_t *) addr + (nr >> 5)) |= (1 << (nr & 31));
}

static __inline__ void __clear_bit (int nr, volatile void *addr)
{
        volatile uint32_t *p = (uint32_t *) addr + (nr >> 5); 
        uint32_t m = 1 << (nr & 31);
        *p &= ~m; 
}

static __inline__ int test_bit (int nr, const volatile void *addr)
{
    return 1 & (((const volatile uint32_t *) addr)[nr >> 5] >> (nr & 31));
}

#endif
