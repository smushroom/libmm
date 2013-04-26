#ifndef     _API_H_
#define     _API_H_

#include "page.h"
#include "pte.h"
#include "zone.h"

<<<<<<< HEAD
void * lmalloc(const int prio, const size_t size, unsigned long flags);
void * lzalloc(const int prio, const size_t size, unsigned long flags);
int lfree(void *address);
int llookup(const void * address);
//int lread(const void * address, void *buf, const size_t offset, const size_t size);
int lread(void ** address, void *buf, const size_t offset, const size_t size);
int lwrite(void **address, void *buf, const size_t offset, const size_t size);
int lfree_toswp(const void *address);
=======
extern void * lmalloc(const size_t size);
extern void * lzalloc(const size_t size);
extern int lfree(void *address);
extern int llookup(const void * address);
extern int lread(const void * address, void *buf, const size_t offset, const size_t size);
extern int lwrite(const void *address, void *buf, const size_t offset, const size_t size);
extern int lfree_toswp(const void *address);
>>>>>>> develop

#endif
