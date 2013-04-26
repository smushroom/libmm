#ifndef     _SWAP_H_
#define     _SWAP_H_

#include <stdio.h>
#include "page.h"
#include "rq.h"

#define     SWP_TYPE            5
#define     SWP_TYPE_MASK       ((1UL << SWP_TYPE) - 1)
#define     MAX_SWPFILES       (1<<SWP_TYPE) 

#define     SWP_SHIFT           12
#define     SWP_FLAGS_MASK      (((-1L) << SWP_TYPE ) & (((1UL) << SWP_SHIFT) -1))

/* page in the swap */
typedef struct 
{
    unsigned long val;
}swp_entry_t;

/*  swp_entry_t 
 *0       5             11                          64 
 *|--------------------------------------------------|
 *|  type |   flags      |              offset       | 
 *|--------------------------------------------------|
 * */

static inline unsigned long swp_flags(const swp_entry_t *entry)
{
    return (entry->val & SWP_FLAGS_MASK);
}

static inline unsigned int swp_type(const swp_entry_t *entry)
{
    return (entry->val & SWP_TYPE_MASK);
}

static inline unsigned long swp_offset(const swp_entry_t *entry)
{
    return (entry->val >> SWP_SHIFT); 
}

static inline void swp_entry(const unsigned long offset, const unsigned long flags, const unsigned long type, swp_entry_t *entry)
{
    entry->val = ((offset << SWP_SHIFT) | ((flags << SWP_TYPE) & SWP_FLAGS_MASK) | (type & SWP_TYPE_MASK));
}

static inline int empty_entry(const swp_entry_t *entry)
{
    return (entry->val == 0);
}

typedef struct swp_info_struct swp_info_s;
struct swp_info_operations
{
    int (*swp_readpage)(const swp_info_s *, const swp_entry_t *,struct page *);
    int (*swp_writepage)(swp_info_s *, const swp_entry_t *, struct page *);
    int (*swp_freepage)(swp_info_s *, swp_entry_t *);
    int (*swp_read_aio)(struct request_queue *rq, struct bio *bio);
    int (*swp_write_aio)(struct request_queue *rq, struct bio *bio);
};

typedef struct swp_info_struct
{
    unsigned int flags; /* state : USED WRITEOK */
    int prio;           /* priority */
    FILE *swp_file;
    struct list_head extent_list;
    unsigned short *swp_map;

    struct swp_info_operations *swp_ops;

    struct request_queue r_rq;    /* request read queue */
    struct request_queue w_rq;    /* request write queue */

    unsigned int slot_next; /* next slot  */
    unsigned int pages;     /* number of page slots */
    unsigned int max;       /* pages + reserve page */
    unsigned int inuse_pages;
    unsigned int type;
    int next;
}swp_info_t;

typedef struct swp_list
{
    int head;
    int next;
}swp_list_t;

enum
{
    SWP_USED =  (1 << 0),
    SWP_WRITEOK =  (1 << 1),
    SWP_DISCARDABLE =  (1 << 2), 
    SWP_DISCARDING =  (1 << 3),
    SWP_SLOIDSTATE =  (1 << 4),
    SWP_SCANNING = (1 << 8) 
};

#define  SWP_ACTIVE     (SWP_USED | SWP_WRITEOK)

#define     DEFAULT_SWP_CLUSTER_MAX    32
#define     DEFAULT_SWP_RESERVE         1
#define     DEFAULT_SWP_MAP_MAX        1024

int do_swp_readpage(swp_entry_t entry, struct page *page);
int do_swp_writepage(struct page *page, swp_entry_t *entry);
int do_swp_freepage(swp_entry_t entry);

inline swp_info_t *get_swp_info(struct page *page);
inline swp_info_t * _get_swp_info(unsigned int type);

int swp_on();
int swp_off();

#endif
