#include <stdio.h>
#include "debug.h"
#include "slab.h"
#include "uio.h"

static kmem_cache_t *uio_cachep = NULL;

int kmem_uio_init()
{
    uio_cachep = kmem_cache_create("kmem_uio", sizeof(uio_t), 0);
    if(uio_cachep == NULL)
    {
        DD("kmem_uio_init error. ");
        return -1;
    }

    return 0;
}

void uio_free(uio_t *uio)
{
    if(uio)
    {
        uio_cachep->kmem_ops->free_obj(uio); 
    }
}

int uio_init(uio_t *uio, void *buf, const size_t offset, const size_t size)
{
    if(uio == NULL || buf == NULL || size <= 0)
    {
        return -1;
    }

    uio->u_base = buf;
    uio->u_offset = offset;
    uio->u_size = size;

    return 0;
}

uio_t * uio_alloc()
{
    uio_t *uio = (uio_t *)uio_cachep->kmem_ops->get_obj(uio_cachep);
    return uio;
}
