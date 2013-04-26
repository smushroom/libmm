/* 
 * user api
 * */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include "debug.h"
#include "types.h"
#include "uio.h"
#include "api.h"

<<<<<<< HEAD
void * lmalloc(const int prio, const size_t size, unsigned long flags)
{
    unsigned int id = pthread_self();
    struct vzone *zone = get_vzone(id);

    return (void *)v_alloc_page(zone, prio, size, flags);
}

void * lzalloc(const int prio, const size_t size, unsigned long flags)
{
    unsigned int id = pthread_self();
    struct vzone *zone = get_vzone(id);
=======
static void * _lmalloc(const int prio, const size_t size, unsigned long flags)
{
    unsigned int id = pthread_self();
    struct vzone *zone = get_vzone(id);
    /*struct page *page = v_alloc_page(zone, prio, size); */
    return (void *)v_alloc_page(zone, prio, size, flags);
    /*DD("order = %d.",page->order);*/
    /*return page_address(page);*/
    /*return page_address(get_free_pages(prio,size_to_order(size)));*/
}

void *lmalloc(const size_t size)
{
    return _lmalloc(0, size, 0);
}

static void * _lzalloc(const int prio, const size_t size, unsigned long flags)
{
    unsigned int id = pthread_self();
    struct vzone *zone = get_vzone(id);
    /*struct page *page = v_alloc_page(zone, prio, size); */
    return (void *)v_alloc_page(zone, prio, size, flags); 
    /*addr_t address  = page_address(page);*/
    /*memset((void *)address, 0, size);*/
>>>>>>> develop

    return (void *)v_alloc_page(zone, prio, size, flags); 
}

void * lzalloc(const size_t size)
{
    return _lzalloc(0, size, 0);
}

int lfree(void *address)
{
    pthread_t id = pthread_self();
    struct vzone *zone = get_vzone(id);

    return v_free_page(zone, address);
}

int llookup(const void * address)
{
    pthread_t id = pthread_self();
    struct vzone *zone = get_vzone(id);

    if(v_lookup_page(zone, address) < 0)
    {
        DD("llookup_page error.");
        return -1;
    }

    return 0;
}

int lread(void ** address, void *buf, const size_t offset, const size_t size)
{
    if(address == NULL || buf == NULL || size <= 0)
    {
        return -1;
    }

    int ret;
    unsigned int id = pthread_self();
    struct vzone *zone = get_vzone(id);

    uio_t *uio = uio_alloc();

    if(uio_init(uio, buf, offset, size) < 0)
    {
        DD("uio_init error.");
        return -2;
    }

    if((ret = v_readpage(zone, address, uio)) < 0)
    {
        DD("reapage error. ret = %d.", ret);
        return -3;
    }

    uio_free(uio);

    return ret;
}

int lwrite(void **address, void *buf, const size_t offset, const size_t size)
{
    if(address == NULL || buf == NULL || size <= 0)
    {
        return -1;
    }

    int ret ;
    unsigned int id = pthread_self();
    struct vzone *zone = get_vzone(id);

    uio_t *uio = uio_alloc();

    if(uio_init(uio, buf, offset, size) < 0)
    {
        DD("uio_init error.");
        return -2;
    }

    if((ret = v_writepage(zone, address, uio)) < 0)
    {
        DD("v_writepage error.");
        return -3;
    }

    uio_free(uio);

    return ret;
}

/* free a page to swap */
int lfree_toswp(const void *address)
{
    int ret;
    unsigned int id = pthread_self();
    struct vzone *zone = get_vzone(id);

    if((ret = v_free_to_swap(zone, address)) < 0)
    {
        DD("v_free_swap error. ret = %d.", ret);
        return -1;
    }

    return 0;
}
