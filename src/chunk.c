#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "chunk.h"

static struct malloc_zone  malloc_state;

static inline uint8_t _lge_idx(const size_t size, uint8_t flags)
{
    double d = log(2);
    double f = log((size));

    double e = f/d;
    int order = (int)e;
    /*DD("order = %d.", order);*/

    if(e > order)
    {
        if(flags == IDX_BIG)
            return (order - LARGE_CHUNK_SHIFT + 1);
        else 
            return (order - LARGE_CHUNK_SHIFT);
    }
    else
    {
        return (order - LARGE_CHUNK_SHIFT);
    }
}

static inline uint8_t lge_idx_big(const size_t size)
{
    _lge_idx(size, IDX_BIG);
}

static inline uint8_t lge_idx_small(const size_t size)
{
    _lge_idx(size, IDX_SMALL);
}

static int insert_lge_chunk(struct malloc_zone *mzone, void *address, uint8_t idx)
{
    return 0;
}

static int split_chunk(struct malloc_zone *mzone, void *address, size_t size)
{
    DD("split_chunk size = %ld.", size);
    if(size < SMALL_CHUNK_SIZE || size >= MMAP_SIZE)
    {
        DD("split_chunk size = %ld.", size);
        return -1;
    }

    uint8_t idx = 0;
    size_t tmp_size = 0;
    while(size >= LARGE_CHUNK_SIZE)
    {
        idx = lge_idx_small(size);
        insert_lge_chunk(mzone, address,idx);

        tmp_size = LARGE_CHUNK_SIZE<<idx;
        size -= tmp_size; 
        address -= tmp_size;

        DD("split_chunk idx = %d. size = %ld", idx, size);
    }

    return 0;
}

static void * lbrk(struct malloc_zone *mzone, size_t size)
{
    if(size < BRK_MININUM)
    {
        size = BRK_MININUM;
    }

    void *address = __sbrk(size);

    if (address == (void *)-1 && errno == ENOMEM)
    {
        DD("sbrk error : %s.", strerror(errno));
        return NULL;
    }

    split_chunk(mzone, address, size);

    return address;
}

static void * lmmap(size_t size, int prot, int flags)
{
    void *address = __mmap(NULL, size, prot, flags, 0, 0);

    return address;
}

static void * _lmalloc(struct malloc_zone *mzone,size_t size)
{
    if(size < M_MMAP_THRESHOLD)
    {
        return lbrk(mzone, size);
    }
    else
    {
        int prot = PROT_READ | PROT_WRITE;
        int flags = MAP_ANONYMOUS | MAP_PRIVATE | MAP_LOCKED;
        return lmmap(size, prot, flags);
    }
}

static inline uint8_t size_to_idx(const size_t size)
{
    if(size < LARGE_CHUNK_SIZE)
    {
        return sml_idx(size);
    }
    else 
    {
        return lge_idx_big(size);
    }
}

void* get_chunk(size_t size)
{
    chunk_ptr chk_ptr = NULL;
    uint32_t index = 0;
    DD("get_chunk index = %d.", index);
    struct malloc_zone *mzone = NULL;

    mzone = &malloc_state;
    /*mzone = mzone->next;*/

repeat:
    index = size_to_idx(size);
    while(1)
    {
        if(size < LARGE_CHUNK_SIZE && index < NSMALLBINS)
        {
            chk_ptr = mzone->smlbins[index];
        }
        else  if(size >= LARGE_CHUNK_SIZE && index < NLARGEBINS)
        {
            chk_ptr = mzone->lgebins[index];
        }
        else
        {
            DD("no chunk ");
            break;
            /*return NULL;*/
        }

        if(chk_ptr == NULL)
        {
            DD("index = %d.",index);
            ++index;
            continue;
        }
    }

    if(chk_ptr == NULL)
    {
        DD("need to malloc.");
        _lmalloc(mzone, size);
    }
}
