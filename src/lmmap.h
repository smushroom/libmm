#ifndef     _L_MMAP_H_
#define     _L_MMAP_H_

#include <sys/mman.h>

#define     __mmap  mmap

extern void * lmmap(size_t length, int prot, int flags);

#endif
