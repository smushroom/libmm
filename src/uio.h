#ifndef     _UIO_H_
#define     _UIO_H_

#include <stdio.h>

typedef struct uio 
{
    void *u_base;
    size_t u_size;
    size_t u_offset;
}uio_t;

extern int kmem_uio_init();
extern uio_t * uio_alloc();
extern void uio_free(uio_t *uio);
extern int uio_init(uio_t *uio, void *buf, const size_t offset, const size_t size);

#endif
