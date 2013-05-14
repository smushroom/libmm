#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "chunk.h"

static struct malloc_zone  malloc_state;

static void * get_large_chunk(struct malloc_zone *mzone, const size_t size);
static int merge_lge_chunk(struct malloc_zone *mzone, void *address, struct list_head *pos, uint8_t idx);
/*static int merge_sml_chunk(struct malloc_zone *mzone, void *address, struct list_head *pos, uint8_t idx);*/
static void print_smlbins(struct malloc_zone *mzone);
static void print_lgebins(struct malloc_zone *mzone);
static void print_unsortbin(struct malloc_zone *mzone);
static void print_mmapbin(struct malloc_zone *mzone);
static inline uint8_t size_to_idx(const size_t size);

static void _init_malloc_zone(struct malloc_zone *mzone)
{
    if(mzone == NULL)
    {
        DD("init malloc zone error.");
        return;
    }

    int i = 0;

    mzone->flags = 0;
    mzone->next = NULL;

#ifdef  UNSORT_BIN
    INIT_LIST_HEAD(&mzone->unsort_bin);
#endif

#ifdef  MMAP_BIN
    INIT_LIST_HEAD(&mzone->mmap_bin);
#endif

    for(i = 0;i < NSMALLBINS; ++i)
    {
        INIT_LIST_HEAD(&mzone->smlbins[i]);
    }

    for(i = 0;i < NLARGEBINS; ++i)
    {
        INIT_LIST_HEAD(&mzone->lgebins[i]);
    }
}

int init_malloc_zone()
{
    _init_malloc_zone(&malloc_state);

    return 0;
}

static inline void set_chunk_size(chunk_ptr chk_ptr, size_t size)
{
    if(chk_ptr == NULL)
    {
        DD("chunk is NULL");
        return;
    }

    int index = size_to_idx(size);
    DD("size = %ld index = %d", size, index);

    if(size >= MMAP_SIZE)
    {
        chk_ptr->big_size = size>>MMAP_SIZE_SHIFT;
        /*DD("big_size = 0x%x", chk_ptr->big_size);*/
        chk_ptr->size = (size & MMAP_MASK) | MMAP_CHUNK;
        /*DD("small size = 0x%x", chk_ptr->size);*/
        DD("mmap chunk = 0x%x. size = 0x%x flags = 0x%x", chk_ptr->big_size, MMAP_CHUNK_SIZE(chk_ptr), chunk_flags(chk_ptr));
    }
    else if(size < LARGE_CHUNK_SIZE)
    {
        /*chk_ptr->size = small_size(index) >> SMALL_CHUNK_SHIFT | SMALL_CHUNK;*/
        chk_ptr->size = small_size(index) | SMALL_CHUNK;
        /*chk_ptr->flags = SMALL_CHUNK; */
        /*DD("small chunk = 0x%x. size = 0x%x flags = 0x%x", chk_ptr->big_size, CHUNK_SIZE(chk_ptr), chunk_flags(chk_ptr));*/
    }
    else if(size >= LARGE_CHUNK_SIZE)
    {
        DD("large size = 0x%x", large_size(index));
        /*chk_ptr->size = large_size(index) >> SMALL_CHUNK_SHIFT | LARGE_CHUNK;*/
        chk_ptr->size = large_size(index) | LARGE_CHUNK;
        /*chk_ptr->flags = LARGE_CHUNK; */
        DD("large chunk = 0x%x. size = 0x%x flags = 0x%x", chk_ptr->big_size, CHUNK_SIZE(chk_ptr), chunk_flags(chk_ptr));
    }
}

/* get the size of chunk */
static inline uint32_t chunk_size(void *address)
{
    chunk_ptr chk_ptr = (chunk_ptr)address; 
    chk_size_t flags = chunk_flags(chk_ptr);
    chk_size_t sml_size = chk_ptr->size;

    if((flags & SMALL_CHUNK) || (flags & LARGE_CHUNK))
    {
        /*return (sml_size<<SMALL_CHUNK_SHIFT);*/
        /*DD("sml_size = 0x%x, mask = %x", sml_size, ~CHUNK_FLAGS_MASK);*/
        return (sml_size & (~CHUNK_FLAGS_MASK));
    }
    /*if(flags & MMAP_CHUNK)*/
    else if(flags & MMAP_CHUNK)
    {
        chk_size_t big_chk_size = chk_ptr->big_size;
        DD("mmap big size = 0x%x. smal size = 0x%x flags = 0x%x", chk_ptr->big_size, chk_ptr->size & ~CHUNK_FLAGS_MASK, chunk_flags(chk_ptr));
        DD("mmap size = %d", MMAP_CHUNK_SIZE(chk_ptr));
        return MMAP_CHUNK_SIZE(chk_ptr);
    }
}

static inline void * chunk2mem(chunk_ptr chk_ptr)
{
    if(chk_ptr == NULL)
    {
        DD("chunk is NULL");
        return NULL;
    }

    chk_size_t flags = chunk_flags(chk_ptr);
    if((flags & SMALL_CHUNK) || (flags & LARGE_CHUNK))
    {
        return ((void *)chk_ptr + sizeof(chk_size_t));
    }
    /*if(flags & MMAP_CHUNK)*/
    else
    {
        return ((void *)chk_ptr + 2 * sizeof(chk_size_t));
    }
}

static inline chunk_ptr mem2mmap_chunk(void *address)
{
    if(address == NULL)
    {
        DD("chunk is NULL");
        return NULL;
    }

    DD("111111111111111");
    return ((void *)address- 2 * sizeof(chk_size_t));
}

static inline chunk_ptr mem2chunk(void *address)
{
    if(address == NULL)
    {
        DD("chunk is NULL");
        return NULL;
    }

    chk_size_t * tmp_size = (chk_size_t *)(address - sizeof(chk_size_t));
    DD("tmp_size = 0x%x", *tmp_size);
    /*DD("small chunk flags = 0x%x", *tmp_size & SMALL_CHUNK);*/
    /*DD("large chunk flags = 0x%x", *tmp_size & LARGE_CHUNK);*/
    /*if((flags & SMALL_CHUNK) || (flags & LARGE_CHUNK))*/
    if(((*tmp_size) & SMALL_CHUNK) || ((*tmp_size) & LARGE_CHUNK))
    {
        DD("222222222222222");
        return ((void *)address- sizeof(chk_size_t));
    }
    /*if(flags & MMAP_CHUNK)*/
}

/* get large bins index */
static inline uint8_t _lge_idx(const size_t size, uint8_t flags)
{
    double d = log(2);
    double f = log((size));

    double e = f/d;
    int order = (int)e;

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

/* get the bigger index in large bins */
static inline uint8_t lge_idx_big(const size_t size)
{
    _lge_idx(size, IDX_BIG);
}

/* get the small index in large bins */
static inline uint8_t lge_idx_small(const size_t size)
{
    _lge_idx(size, IDX_SMALL);
}

static inline int is_sml_adjaddress(const void *address, const struct list_head *item, const uint8_t index)
{
    chunk_ptr chk_ptr = list_entry(item, struct chunk, list);
    DD("index = %d address = 0x%lx chk_ptr = 0x%lx", index, (unsigned long)address, (unsigned long)chk_ptr);

    int tmp_size = SMALL_CHUNK_STEP * index;

    if((address + tmp_size) == (void *)chk_ptr || (address - tmp_size) == (void *)chk_ptr)
    {
        return 0;
    }
    else if((address + tmp_size) < (void *)chk_ptr)
    {
        return -1;
    }
    else if((address - tmp_size) > (void *)chk_ptr)
    {
        return 1;
    }
}

static inline int is_lge_adjaddress(const void *address, const struct list_head *item, const uint8_t index)
{
    chunk_ptr chk_ptr = list_entry(item, struct chunk, list);
    DD("index = %d address = 0x%lx chk_ptr = 0x%lx", index, (unsigned long)address, (unsigned long)chk_ptr);

    int tmp_size = LARGE_CHUNK_SIZE << index;

    if((address + tmp_size) == (void *)chk_ptr || (address - tmp_size) == (void *)chk_ptr)
    {
        return 0;
    }
    else if((address + tmp_size) < (void *)chk_ptr)
    {
        return -1;
    }
    else if((address - tmp_size) > (void *)chk_ptr)
    {
        return 1;
    }
}

/* insert chunk into large bins */
static int insert_lge_chunk(struct malloc_zone *mzone, void *address, uint8_t idx)
{
    chunk_ptr chk = (chunk_ptr)address;
    chk->size = LARGE_CHUNK_SIZE<<idx;
    /*DD("LARGE_CHUNK_SIZE = %d idx = %d", LARGE_CHUNK_SIZE, idx);*/
    DD("chunk size = %d", chk->size);

    struct list_head *head = &mzone->lgebins[idx];
    if(list_empty_careful(head))
    {
        DD("11111");
        /*chk_ptr = chk;*/
        list_add(&chk->list, head);
    }
    else
    {
        DD("22222");
        list_add(&chk->list, head);
    }

    return 0;
}

static int insert_sml_chunk_line(struct malloc_zone *mzone, void *address, uint8_t idx)
{
    chunk_ptr tmp_chk = NULL;

    chunk_ptr chk = (chunk_ptr)address;
    /*chk->size = LARGE_CHUNK_SIZE<<idx;*/
    DD("chunk size = 0x%x", CHUNK_SIZE(chk));

    struct list_head *head, *n, *pos;
    head = &mzone->smlbins[idx];

    if(list_empty_careful(head))
    {
        DD("6666666666");
        /*chk_ptr = chk;*/
        DD("chunk = 0x%lx", (unsigned long)address);
        list_add(&chk->list, head);
    }
    else
    {
        DD("777777777");
        list_for_each_safe(pos, n, head)
        {
            int ret = is_sml_adjaddress(address, pos, idx);
            DD("ret = %d. idx = %d.", ret, idx);
            /*if(ret == 0)*/
            /*{*/
            /*DD("is adj address.");*/
            /*break;*/
            /*}*/
            if(ret == -1)
            {
                DD("small");
                __list_add(&chk->list, pos->prev, pos);
                break;
            }
        }

        if(pos == head)
        {
            DD("big");
            __list_add(&chk->list, pos, n);
        }
        /*list_add(&chk->list, head);*/
    }

    return 0;
}

static int insert_lge_chunk_line(struct malloc_zone *mzone, void *address, uint8_t idx)
{
    chunk_ptr tmp_chk = NULL;

    chunk_ptr chk = (chunk_ptr)address;
    size_t size = LARGE_CHUNK_SIZE<<idx;
    DD("chunk size = %ld",size);
    set_chunk_size(chk, size);

    struct list_head *head, *n, *pos;
    head = &mzone->lgebins[idx];

    if(list_empty_careful(head))
    {
        DD("333333");
        /*chk_ptr = chk;*/
        DD("chunk = 0x%lx", (unsigned long)address);
        list_add(&chk->list, head);
    }
    else
    {
        DD("4444444");
        list_for_each_safe(pos, n, head)
        {
            int ret = is_lge_adjaddress(address, pos, idx);
            DD("ret = %d. idx = %d.", ret, idx);
            if(ret == 0)
            {
                DD("is adj address.");
                merge_lge_chunk(mzone,address, pos, idx);
                break;
            }
            else if(ret == -1)
            {
                DD("small");
                __list_add(&chk->list, pos->prev, pos);
                break;
            }
        }

        if(pos == head)
        {
            DD("big");
            __list_add(&chk->list, pos->prev, pos);
            /*__list_add(&chk->list, pos, n);*/
        }

        /*list_add(&chk->list, head);*/
    }

    return 0;
}

#ifdef  UNSORT_BIN
static int merge_unsort_bin(struct malloc_zone *mzone)
{
    int index = 0;
    struct list_head *head, *pos, *n, *tmp;
    head = &mzone->unsort_bin;
    chunk_ptr prev_chk = NULL, next_chk = NULL;
    chk_size_t tmp_size = 0;

    pos = head->next;
    n = pos->next;
    while(pos != head && n != head)
        /*list_for_each_safe(pos, n, head)*/
    {
        prev_chk = list_entry(pos, struct chunk, list);
        next_chk = list_entry(n, struct chunk, list);
        DD("unsort bin prev address = 0x%lx size = %d next address = 0x%lx size = %d", (unsigned long)prev_chk, CHUNK_SIZE(prev_chk), (unsigned long)next_chk, CHUNK_SIZE(next_chk));

        if((chunk_flags(prev_chk) & LARGE_CHUNK) && (chunk_flags(next_chk) & LARGE_CHUNK))
        {
            DD("---------------------------------");
            tmp_size = CHUNK_SIZE(prev_chk); 
            if((unsigned long)prev_chk + tmp_size == (unsigned long)next_chk)
            {
                /*prev_chk->size += next_chk->size;*/
                tmp_size += CHUNK_SIZE(next_chk);
                index = size_to_idx(tmp_size);
                /* size is equal the large bins*/
                if(tmp_size == large_size(index))
                {
                    DD("merge size = %d", tmp_size);
                    set_chunk_size(prev_chk, tmp_size);
                    tmp = n->next;
                    /*list_del(&next_chk->list);*/
                    list_del(n);
                    n = tmp;
                    DD("unsort bin is adj.");
                    continue;
                }
            }
        }

        pos = n;
        n = n ->next;
    }
    return 0;
}

int wrapper_merge_unsort_bin()
{
    merge_unsort_bin(&malloc_state);
    print_unsortbin(&malloc_state);
}
#endif

/* merge chunk */
static int merge_lge_chunk(struct malloc_zone *mzone, void *address, struct list_head *pos, uint8_t idx)
{
    struct list_head *head;
    /*chunk_ptr chk_ptr = (chunk_ptr)address;*/
    chunk_ptr chk_ptr = list_entry(pos, struct chunk, list);

    list_del(&(((chunk_ptr)address)->list));
    list_del(pos);

    DD("address = 0x%lx chk_ptr = 0x%lx", (unsigned long)address, (unsigned long)chk_ptr);

    if((address + (LARGE_CHUNK_SIZE<<idx)) == (void *)chk_ptr)
    {
        DD("aaaaaaaaaaaaaaaaaaaaaaaaaa");
        ++idx;
        chk_ptr->size = LARGE_CHUNK_SIZE<<idx;
        insert_lge_chunk_line(mzone, address, idx);
    }
    else if((address - (LARGE_CHUNK_SIZE<<idx)) == (void *)chk_ptr)
    {
        DD("bbbbbbbbbbbbbbbbbbbbbbbbbb");
        ++idx;
        chk_ptr = list_entry(pos, struct chunk, list);
        chk_ptr->size = LARGE_CHUNK_SIZE<<idx; 
        insert_lge_chunk_line(mzone, (void *)chk_ptr, idx);
    }
    else
    {
        DD("ccccccccccccccccccccccccccc");
    }

    return 0;
}

/* split a big chunk to small chunks*/
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
        /*DD("split_chunk idx = %d. size = %ld", idx, size);*/
        insert_lge_chunk_line(mzone, address,idx);

        tmp_size = (LARGE_CHUNK_SIZE<<idx);
        size -= tmp_size; 
        address += tmp_size;
        /*DD("address = 0x%lx", (unsigned long)address);*/
    }

    return 0;
}

/* wrpper brk syscall */
static int lbrk(struct malloc_zone *mzone, size_t size)
{
    if(size < BRK_MININUM)
    {
        size = BRK_MININUM;
    }

    /*void *address = __sbrk(size);*/
    void *address = malloc(size);

    if (address == (void *)-1 && errno == ENOMEM)
    {
        DD("sbrk error : %s.", strerror(errno));
        return -1;
    }
    DD("address = 0x%lx", (unsigned long)address);

    split_chunk(mzone, address, size);

    return 0;
}

/* wrapper mmap syscall */
static void * lmmap(struct malloc_zone *mzone, size_t size, int prot, int flags)
{
    void *address = __mmap(NULL, size, prot, flags, -1, 0);
    if(address == (void *)-1)
    {
        DD("mmap error : %s", strerror(errno));
        return NULL;
    }

    DD("mmap size = %ld.", size);
    /*chunk_ptr chk_ptr = (chunk_ptr)address;*/
    /*chk_ptr->size = size;*/
    /*DD("mmap address = 0x%lx. size = 0x%lx.",(unsigned long)address, chk_ptr->size );*/

    return address;
}

/* get the index of the bins  including large bins and small bins */
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

#ifdef MMAP_BIN
/* get from mmap chunk */
static void * get_mmap_chunk(struct malloc_zone *mzone, size_t size)
{
    struct list_head *head, *pos, *n;
    head = &mzone->mmap_bin;
    chunk_ptr chk_ptr = NULL; 

    list_for_each_safe(pos, n, head)
    {
        chk_ptr = list_entry(pos, struct chunk, list);
        if(chk_ptr->size == size)
        {
            list_del(pos);
            return chk_ptr;
        }
        else if(chk_ptr->size < size)
        {
            continue;
        }
        else 
        {
            return NULL;
        }
    }

    return NULL;
}
#endif

/* get from unsort first */
static void * get_unsort_chunk(struct malloc_zone *mzone, size_t size)
{
    struct list_head *head, *pos, *n;
    head = &mzone->unsort_bin;
    chunk_ptr chk_ptr = NULL; 

    list_for_each_safe(pos, n, head)
    {
        chk_ptr = list_entry(pos, struct chunk, list);
        if(chk_ptr->size == size)
        {
            list_del(pos);
            return chk_ptr;
        }
        else if(chk_ptr->size < size)
        {
            continue;
        }
        else 
        {
            return NULL;
        }
    }

    return NULL;
}

static void * _get_small_chunk(struct malloc_zone *mzone, uint8_t index)
{
    struct list_head *head, *item;
    chunk_ptr chk_ptr = NULL;
    int i = index;

    while(i < NSMALLBINS)
    {
        head = &mzone->smlbins[i];
        if(!list_empty_careful(head))
        {
            if(list_get_del(head, &item) != 0)
            {
                DD("free_list_del error.");
                return NULL;
            }

            chk_ptr = list_entry(item, struct chunk, list);
            DD("get chunk from index = %d. size = %d", index, small_size(index));
            break;
            /*return chk_ptr;*/
        }
        ++i;
    }
    if(chk_ptr)
    {
        /*chk_ptr->size = small_size(index);*/
        DD("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
        DD("_get_small_chunk chk_ptr size = %d.", small_size(index));
        set_chunk_size(chk_ptr, small_size(index));
        int count = small_size(i) - small_size(index);
        if(count > SMALL_CHUNK_SIZE)
        {
            int tmp_index = size_to_idx(count);
            chunk_ptr tmp_chk_ptr = (void *)chk_ptr + small_size(index);
            /*tmp_chk_ptr->size = small_size(tmp_index); */
            set_chunk_size(tmp_chk_ptr, small_size(tmp_index));
            insert_sml_chunk_line(mzone, tmp_chk_ptr, tmp_index);
        }
    }

    return chk_ptr;
}

/* get from small chunk */
static void * get_small_chunk(struct malloc_zone *mzone, const size_t size)
{
    struct list_head *head = NULL;
    int index = size_to_idx(size);
    chunk_ptr chk_ptr = NULL;


    if((chk_ptr = _get_small_chunk(mzone, index)))
    {
        return  chk_ptr;
    }

    DD("get from small chunk : could not get small chunk directly.");

    chk_ptr = get_large_chunk(mzone, LARGE_CHUNK_SIZE);
    if(chk_ptr == NULL)
    {
        DD("get from large chunk error.");
        return NULL;
    }

    /*chk_ptr->size = small_size(index);*/
    DD("set chunk size = %d.", small_size(index));
    set_chunk_size(chk_ptr, small_size(index));

    int count = LARGE_CHUNK_SIZE - small_size(index);
    DD("split from large bins : count = %d.", count);
    if(count < SMALL_CHUNK_SIZE)
    {
        return chk_ptr;
    }
    else
    {
        int tmp_index = size_to_idx(count);
        chunk_ptr tmp_chk_ptr = (void *)chk_ptr + size;
        set_chunk_size(tmp_chk_ptr, count);
        /*tmp_chk_ptr->size = small_size(tmp_index); */
        insert_sml_chunk_line(mzone, tmp_chk_ptr, tmp_index);
    }

    DD("get from small chunk 0x%lx size = %d", (unsigned long)chk_ptr, CHUNK_SIZE(chk_ptr));
    return chk_ptr;
}

static int sep_from_large_chunk(struct malloc_zone *mzone, uint32_t index)
{
    int tmp_size = 0;
    chunk_ptr chk_ptr = NULL;
    struct list_head *head, *item;

    while(index < NLARGEBINS)
    {
        head = &mzone->lgebins[index];
        if(list_empty_careful(head))
        {
            ++index;
        }
        else
        {
            DD("sep from index = %d.", index);
            list_get_del(head, &item);

            --index;
            head = &mzone->lgebins[index];
            tmp_size = LARGE_CHUNK_SIZE << index;
            DD("sep from tmp_size = %d.", tmp_size);

            chk_ptr = list_entry(item, struct chunk, list);
            /*chk_ptr->size = tmp_size;*/
            set_chunk_size(chk_ptr, tmp_size);
            list_add(&chk_ptr->list, head);

            chk_ptr = (void *)chk_ptr + tmp_size;
            /*chk_ptr->size = tmp_size;*/
            set_chunk_size(chk_ptr, tmp_size);
            list_add(&chk_ptr->list, head);

            return index;
        }
    }

    return -2;
}

static chunk_ptr _get_large_chunk(struct malloc_zone *mzone, const uint32_t index)
{
    struct list_head *head, *item;
    chunk_ptr chk_ptr = NULL;

    head = &mzone->lgebins[index];
    if(!list_empty_careful(head))
    {
        if(list_get_del(head, &item) != 0)
        {
            DD("free_list_del error.");
            return NULL;
        }

        chk_ptr = list_entry(item, struct chunk, list);
        DD("get chunk from index = %d.", index);
        return chk_ptr;
    }

    return NULL;
}

/* get chunk from large */
static void * get_large_chunk(struct malloc_zone *mzone, const size_t size)
{
    struct list_head *head = NULL;
    void *addr = NULL;
    int index = size_to_idx(size);
    chunk_ptr chk_ptr = NULL;
    int tmp_index = 0;

repeat_large:
    if((chk_ptr = _get_large_chunk(mzone, index)))
    {
        return chk_ptr;
    }
    else
    {
        if((tmp_index = sep_from_large_chunk(mzone, index)) >= index)
        {
            DD("tmp_index = %d.index = %d.", tmp_index, index);
            goto repeat_large;
        }
    }

    return NULL;
}

/* get a chunk from bins ,maybe needto split and merge */
void* get_chunk(const size_t size)
{
    struct malloc_zone *mzone = &malloc_state;
    void *addr = NULL;

    if(size < SMALL_CHUNK_SIZE)
    {
        DD("chunk size is too small.");
        return NULL;
    }
    else 
    {
        struct list_head *head = NULL;
        uint32_t index = 0;
        DD("get_chunk index = %d.", index);

        /* get chunk from unsort bin first */
        if(addr = get_unsort_chunk(mzone, size))
        {
            return chunk2mem(addr);
        }
        /* get chunk from sort bins second */
        else
        {

repeat_get:
            if(size < LARGE_CHUNK_SIZE)
            {
                addr = get_small_chunk(mzone,size);
                print_smlbins(mzone);
            }
            else 
            {
                addr = get_large_chunk(mzone, size);
                print_lgebins(mzone);
            }

            if(addr == NULL)
            {
                lbrk(mzone, size);
                /*lbrk(mzone, size);*/
                /*lbrk(mzone, size);*/
                /*lbrk(mzone, size);*/
                /*lbrk(mzone, size);*/
                /*lbrk(mzone, size);*/

                goto repeat_get;
            }

            return chunk2mem(addr);
        }
    }
}

void* get_mmap(size_t size)
{
    struct malloc_zone *mzone = &malloc_state;
    void *addr = NULL;

    if(size >= M_MMAP_THRESHOLD)
    {

#ifdef  MMAP_BIN

        addr = get_mmap_chunk(mzone, size);

#endif
        if(addr == NULL)
        {
            int prot = PROT_READ | PROT_WRITE;
            /*The use of MAP_ANONYMOUS in conjunction with MAP_SHARED is only supported on Linux since kernel 2.4.*/
            int flags = MAP_ANONYMOUS | MAP_SHARED;

            addr = lmmap(mzone, size, prot, flags);
            chunk_ptr chk_ptr = (chunk_ptr)addr;
            /*chk_ptr->size = size;*/
            set_chunk_size(chk_ptr, size);
        }

        return chunk2mem(addr);
    }

    return NULL;
}

/* freem mmap chunk */
static int free_mmap_chunk(struct malloc_zone *mzone, chunk_ptr chk_ptr)
{
    size_t size = chunk_size(chk_ptr);
    DD("free mmap chk_ptr = 0x%lx size = %ld", (unsigned long)chk_ptr, size);

#ifdef MMAP_BIN

    list_add(&chk_ptr->list, &mzone->mmap_bin);
    print_mmapbin(mzone);
    return 0;

#else

    return munmap((void *)chk_ptr, size);

#endif

}

/* free small chunk */
static int _free_small_chunk(struct malloc_zone *mzone, chunk_ptr chk_ptr)
{
    int idx = 0;
    idx = size_to_idx(CHUNK_SIZE(chk_ptr));

    insert_sml_chunk_line(mzone, (void *)chk_ptr,idx);

    return 0;
}

/* free large chunk */
static int _free_large_chunk(struct malloc_zone *mzone, chunk_ptr chk_ptr)
{
    int idx = 0;
    idx = size_to_idx(CHUNK_SIZE(chk_ptr));

    insert_lge_chunk_line(mzone, (void *)chk_ptr,idx);

    return 0;
}

#ifdef  UNSORT_BIN
static int free_to_unsortbin(struct malloc_zone *mzone, void *address)
{
    struct list_head *head, *n, *pos;
    head = &mzone->unsort_bin;
    chunk_ptr chk_ptr = mem2chunk(address);;
    chunk_ptr tmp_chk_ptr = NULL;

    DD("free size = %d.", CHUNK_SIZE(chk_ptr));
    list_for_each_safe(pos, n, head)
    {
        tmp_chk_ptr = list_entry(pos, struct chunk, list);
        DD("chk_ptr = 0x%lx tmp_chk_ptr = 0x%lx", (unsigned long)chk_ptr, (unsigned long)tmp_chk_ptr);
        if(chk_ptr < tmp_chk_ptr)
        {
            /*DD("111111111111111111");*/
            __list_add(&chk_ptr->list, pos->prev, pos);
            break;
        }
    }

    if(pos == head)
    {
        /*DD("22222222222222");*/
        __list_add(&chk_ptr->list, pos->prev, pos);
    }

    return 0;
}
#endif

static int _free_chunk(struct malloc_zone *mzone, chunk_ptr chk_ptr)
{
    if(chk_ptr->size < LARGE_CHUNK_SIZE)
    {
        _free_small_chunk(mzone, chk_ptr);
        print_smlbins(mzone);
    }
    else 
    {
        _free_large_chunk(mzone, chk_ptr);
        print_lgebins(mzone);
    }

    return 0;
}

/* free chunk */
int free_chunk(void *address)
{
    struct malloc_zone *mzone = &malloc_state;
    chunk_ptr chk_ptr = mem2chunk(address);

    size_t size = CHUNK_SIZE(chk_ptr);
    DD("free size = 0x%lx", size);

#ifdef  UNSORT_BIN

    free_to_unsortbin(mzone, address);
    print_unsortbin(mzone);

#else

    if(address == NULL)
    {
        return -1;
    }

    chunk_ptr chk_ptr = mem2chunk(address);
    DD("chk_ptr size = %d.", chk_ptr->size);

    _free_chunk(mzone, chk_ptr);

#endif

    return 0;
}

int free_mmap(void *address)
{
    struct malloc_zone *mzone = &malloc_state;
    chunk_ptr chk_ptr = mem2mmap_chunk(address);

    size_t size = chunk_size(chk_ptr);

    DD("size = %ld", size);
    if(size >= M_MMAP_THRESHOLD)
    {
        DD("000000000000000000");
        free_mmap_chunk(mzone, chk_ptr);
    }
}

/* print all lgebins */
static void print_lgebins(struct malloc_zone *mzone)
{
    int i = 0;
    struct list_head *head, *pos, *n;
    chunk_ptr chk_ptr = NULL;

    DD("***********************************************");
    for(i = 0;i < NLARGEBINS; ++i)
    {
        head = &mzone->lgebins[i];
        if(list_empty_careful(head))
        {
            DD("large bins index = %d is empty", i);
        }
        else
        {
            /*DD("***********************");*/
            DD("large bins index = %d is not empty", i);
            /*DD("***********************");*/
            list_for_each_safe(pos, n, head)
            {
                chk_ptr = list_entry(pos, struct chunk, list);
                DD("large bins chunk address = 0x%lx size = %d", (unsigned long)chk_ptr, CHUNK_SIZE(chk_ptr));

            }
        }
    }
}

/* print all lgebins */
static void print_smlbins(struct malloc_zone *mzone)
{
    int i = 0;
    struct list_head *head, *pos, *n;
    chunk_ptr chk_ptr = NULL;

    DD("***********************************************");
    for(i = 0;i < NSMALLBINS; ++i)
    {
        head = &mzone->smlbins[i];
        if(list_empty_careful(head))
        {
            DD("small bins index = %d is empty", i);
        }
        else
        {
            /*DD("***********************");*/
            /*DD("small bins index = %d is not empty", i);*/
            /*DD("***********************");*/
            list_for_each_safe(pos, n, head)
            {
                chk_ptr = list_entry(pos, struct chunk, list);
                DD("small bins chunk address = 0x%lx size = %d", (unsigned long)chk_ptr, CHUNK_SIZE(chk_ptr));

            }
        }
    }
}

#ifdef  UNSORT_BIN
/* print unsort bin */
static void print_unsortbin(struct malloc_zone *mzone)
{
    struct list_head *head, *pos, *n;
    head = &mzone->unsort_bin;
    chunk_ptr chk_ptr = NULL;

    DD("***********************************************");
    list_for_each_safe(pos, n, head)
    {
        chk_ptr = list_entry(pos, struct chunk, list);
        DD("unsort bin chunk address = 0x%lx size = %d", (unsigned long)chk_ptr, CHUNK_SIZE(chk_ptr));
    }
}
#endif

#ifdef  MMAP_BIN
/* print mmap bin */
static void print_mmapbin(struct malloc_zone *mzone)
{
    struct list_head *head, *pos, *n;
    head = &mzone->mmap_bin;
    chunk_ptr chk_ptr = NULL;

    DD("***********************************************");
    list_for_each_safe(pos, n, head)
    {
        chk_ptr = list_entry(pos, struct chunk, list);
        DD("mmap bin chunk address = 0x%lx size = %d", (unsigned long)chk_ptr, chunk_size(chk_ptr));
    }
}
#endif
