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

#define     SMALL_CHUNK_SHIFT   3
//#define     SMALL_CHUNK_SIZE    (1<<SMALL_CHUNK_SHIFT) 
#define     SMALL_CHUNK_SIZE    32 
#define     SMALL_CHUNK_STEP    8
#define     LARGE_CHUNK_SHIFT   10 
#define     LARGE_CHUNK_SIZE    ((1<<LARGE_CHUNK_SHIFT)) 
#define     LARGE_CHUNK_MULTI   2
#define     MMAP_SIZE_SHIFT     16
#define     MMAP_SIZE           ((1<<MMAP_SIZE_SHIFT))
#define     MMAP_MASK           ((MMAP_SIZE)- 1)
#define     CHUNK_FLAGS_SHIFT   SMALL_CHUNK_SHIFT
#define     CHUNK_FLAGS_MASK    ((1<<SMALL_CHUNK_SHIFT) - 1)

#define     M_MMAP_THRESHOLD    MMAP_SIZE 

#define     NSMALLBINS     ((LARGE_CHUNK_SIZE - SMALL_CHUNK_SIZE)/ SMALL_CHUNK_STEP) 
#define     NLARGEBINS      9 

#ifndef     UNSORT_BIN
#define     UNSORT_BIN
#endif

#ifndef     MMAP_BIN
#define     MMAP_BIN
#endif

#define     SMALL_SIZE_SHIFT    (8 * sizeof(chk_size_t)- SMALL_CHUNK_SHIFT)
typedef struct chunk
{
    //chk_size_t size:SMALL_SIZE_SHIFT;    [> size in bytes, includeing overhead <]
    //chk_size_t flags:SMALL_CHUNK_SHIFT;
    chk_size_t size;  // |--------------------size---------------|-------flags-----|
    chk_size_t big_size;
    struct list_head  list;     //used only if free
}chunk_t, *chunk_ptr;

enum
{
    SMALL_CHUNK = 0x01,
    LARGE_CHUNK = 0x02,
    MMAP_CHUNK = 0x04,
};

#define     CHUNK_SIZE(chk_ptr)     ((chk_ptr->size & (~CHUNK_FLAGS_MASK)))
#define     MMAP_CHUNK_SIZE(chk_ptr)    ((chk_ptr->big_size << MMAP_SIZE_SHIFT) | CHUNK_SIZE(chk_ptr))
#define     chunk_flags(chk_ptr)    (chk_ptr->size & CHUNK_FLAGS_MASK)
//#define     chunk2mem(chunk_ptr)    ((void *)chunk_ptr + sizeof(chk_size_t))
//#define     mem2chunk(address)      ((chunk_ptr)((void *)address - sizeof(chk_size_t)))

typedef struct malloc_zone
{
    uint32_t flags;

#ifdef UNSORT_BIN 
    struct list_head unsort_bin;
#endif

    struct list_head smlbins[NSMALLBINS];
    struct list_head lgebins[NLARGEBINS];

#ifdef  MMAP_BIN
    struct list_head mmap_bin;
#endif

    struct malloc_zone *next;
}malloc_zone_t;

#define     sml_idx(size)     ((size - SMALL_CHUNK_SIZE)/SMALL_CHUNK_STEP)
#define     small_size(index)   (SMALL_CHUNK_SIZE + SMALL_CHUNK_STEP * index)
#define     large_size(index)   (LARGE_CHUNK_SIZE<<index)
extern int init_malloc_zone();
extern void* get_chunk(size_t size);
extern void* get_mmap(size_t size);
extern int free_mmap(void *address);
extern int free_chunk(void *address);
extern int wrapper_merge_unsort_bin();

#endif
