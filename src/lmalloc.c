#include <stdio.h>
#include "lmmap.h"
#include "lbrk.h"
#include "lmalloc.h"

void * _lmalloc(size_t length)
{
    if(length < M_MMAP_THRESHOLD)
    {
        return lbrk(length);
    }
    else
    {
        int prot = PROT_READ | PROT_WRITE;
        int flags = MAP_ANON | MAP_LOCKED;
        return lmmap(length, prot, flags);
    }
}
