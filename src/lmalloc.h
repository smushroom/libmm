#ifndef     _LMALLOC_H_
#define     _LMALLOC_H_

#include <stdio.h>

#define     _LMALLOC_

#define     M_MMAP_THRESHOLD    (128*1024)

extern void * _lmalloc(size_t length);

#endif
