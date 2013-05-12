#ifndef     _CHUNK_H_
#define     _CHUNK_H_

#include <stdio.h>
#include <stdint.h>
#include <sys/mman.h>
#include "types.h"
#include "list.h"

#define     __mmap   mmap
#define     __sbrk   sbrk 

#define     IDX_BIG     0x1
#define     IDX_SMALL   0x2
#define     BRK_MININUM     (28*1024)

typedef struct chunk
{
    chk_size_t size;    /* size in bytes, includeing overhead */
    struct list_head  list;     //used only if free
}chunk_t, *chunk_ptr;

#define     chunk2mem(chunk_ptr)    ((void *)chunk_ptr + sizeof(chk_size_t))
#define     mem2chunk(address)      ((chunk_ptr)((void *)address - sizeof(chk_size_t)))

#define     SMALL_CHUNK_SIZE    32 
#define     SMALL_CHUNK_STEP    8
#define     LARGE_CHUNK_SHIFT   10 
#define     LARGE_CHUNK_SIZE    (1<<LARGE_CHUNK_SHIFT) 
#define     LARGE_CHUNK_MULTI   2
#define     MMAP_SIZE           (128*1024)

#define     M_MMAP_THRESHOLD    MMAP_SIZE 

#define     NSMALLBINS     ((LARGE_CHUNK_SIZE - SMALL_CHUNK_SIZE)/ SMALL_CHUNK_STEP) 
#define     NLARGEBINS      9 

#ifndef     UNSORT_BIN
#define     UNSORT_BIN
#endif

typedef struct malloc_zone
{
    uint32_t flags;

#ifdef UNSORT_BIN 
    struct list_head unsort_bin;
#endif

    struct list_head smlbins[NSMALLBINS];
    struct list_head lgebins[NLARGEBINS];
    struct list_head mmap_bin;

    struct malloc_zone *next;
}malloc_zone_t;

#define     sml_idx(size)     ((size - SMALL_CHUNK_SIZE)/SMALL_CHUNK_STEP)

#define     small_size(index)   (SMALL_CHUNK_SIZE + SMALL_CHUNK_STEP * index)
extern int init_malloc_zone();
extern void* get_chunk(size_t size);
extern int free_chunk(void *address);

#endif
