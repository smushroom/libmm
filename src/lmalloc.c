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
        int flags = MAP_ANONYMOUS | MAP_PRIVATE | MAP_LOCKED;
        return lmmap(length, prot, flags);
    }
}
