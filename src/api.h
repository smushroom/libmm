#ifndef     _API_H_
#define     _API_H_

#include "page.h"
#include "pte.h"
#include "zone.h"

extern void * lmalloc(const size_t size);
extern void * lzalloc(const size_t size);
extern int lfree(void *address);
extern int llookup(const void * address);
extern int lread(void **address, void *buf, const size_t offset, const size_t size);
extern int lwrite(void **address, void *buf, const size_t offset, const size_t size);
extern int lfree_toswp(const void *address);

#endif
