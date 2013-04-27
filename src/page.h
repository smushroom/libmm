#ifndef     _PAGE_H_
#define     _PAGE_H_

#include <stdio.h>
#include "list.h"
#include "types.h"

#define     DEFAULT_PAGE_FNS    (1024*16)
#define     RESERVE_PAGE_FNS    128 

#define     PAGE_SHIFT  12 
#define     PAGE_SIZE   (1 << PAGE_SHIFT) 
#define     PAGE_MASK   (~(PAGE_SIZE - 1))

struct page
{
    unsigned long flags;
    unsigned long virtual;
    unsigned int order;  /* used free page */
    uint32_t prio;

    struct list_head list, slab_list, lru;
    void *mapping;
    unsigned int _mapping;  /* 0 - private page 1 - shared page */

    /*pfra*/
    unsigned long pgd_index; /* pte in zone */
    struct list_head zone_list;  /* list all zone*/
};

/* page state */
enum PAGE_STATE
{
    PAGE_RESERVED = (1<<0),
    PAGE_FREE = (1<<1),
    PAGE_PRESENT = (1<<2),    /* in memory */
    PAGE_SWAPOUT = (1<<3),   /* in swapfile */
    PAGE_ACTIVE = (1<<4),
    PAGE_INACTIVE = (1<<5),
    PAGE_REFERENCED = (1<<6),
    PAGE_PRIVATE = (1<<7),
    PAGE_SHARED = (1<<8),
    PAGE_SYNC = (1 << 9),
    PAGE_ASYNC = (1 << 10),
};

#define     PAGE_present(flags)     (flags & PAGE_PRESENT)
#define     PAGE_free(flags)        (flags & PAGE_FREE)
#define     PAGE_reserved(flags)    (flags & PAGE_RESERVED)
#define     PAGE_swapout(flags)     (flags & PAGE_SWAPOUT)
#define     PAGE_active(flags)      (flags & PAGE_ACTIVE)
#define     PAGE_inactive(flags)    (flags & PAGE_INACTIVE)
#define     PAGE_referenced(flags)  (flags & PAGE_REFERENCED)
#define     PAGE_private(flags)     (flags & PAGE_PRIVATE)
#define     PAGE_shared(flags)      (flags & PAGE_SHARED)

static inline void page_sflags(struct page *page, unsigned long flags)
{
    page->flags |= flags;
}

static inline void page_cflags(struct page *page, unsigned long flags)
{
    page->flags &= ~flags;
}

/* page flags */
enum PAGE_MIGRATE
{
    MIGRATE_MOVABLE = 0,
    MIGRATE_RECLAIMABLE,
    MIGRATE_UNMOVABLE,
    MIGRATE_RESERVE,
    MIGRATE_IOSOLATE,
    MIGRATE_TYPES
};

extern int init_buddy();
extern struct page* get_free_pages(const uint32_t prio,const uint32_t order);
extern struct page * get_free_page(const uint32_t prio);
extern int free_pages(struct page *page, const uint32_t order);
extern int _free_page(const addr_t address);
extern int _free_pages(const addr_t address, const uint32_t order);

extern inline struct page *pfn_to_page(const addr_t addr);
extern inline unsigned long pfn_to_index(const addr_t address);
extern inline unsigned long index_to_pfn(unsigned long index);
extern inline struct page * index_to_page(unsigned long index);
extern inline unsigned long page_to_index(const struct page *page);
extern inline addr_t page_address(const struct page *page);
extern inline unsigned long get_reserve_mem();

static inline uint32_t size_to_order(const size_t size)
{
    return (size + PAGE_SIZE - 1) >> PAGE_SHIFT;
}

#define     free_page(page)     free_pages((page), 0)
#define  	__get_free_page(priority) 		__get_free_pages((priority),0)
#define     page_swp_entry(page)      (page?page->virtual:0)

#endif
