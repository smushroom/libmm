#include "lbrk.h"

void * lbrk(size_t length)
{
    unsigned long *address = sbrk(length);

    return (void *)address;
}
