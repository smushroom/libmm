#ifndef     _TYPES_H_
#define     _TYPES_H_

#include <stdint.h>

//#define     BIT_32
#ifdef  BIT_32
typedef     uint32_t    addr_t;
#else
typedef     uint64_t    addr_t;
#endif

#endif
