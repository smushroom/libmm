#include <stdio.h>
#include "lmmap.h"

void * lmmap(size_t length, int prot, int flags)
{
    unsigned long address = __map(NULL, length, prot, flags, 0, 0);

    return (void *)address;
}
