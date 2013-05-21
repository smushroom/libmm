#ifndef     _ZONE_H_
#define     _ZONE_H_

#include <stdint.h>
#include <pthread.h>
#include "types.h"
#include "pte.h"
#include "bio.h"
#include "swap.h"
#include "mcb.h"
#include "uio.h"
#include "mapping.h"

typedef struct vzone 
{
    /* page mapping */
    struct address_mapping *mapping;
    pgd_t pgd;
    /* mcb */
    struct rb_root rb_root;
    unsigned int id;
    struct list_head zone_list;     /* for zonelist */
    struct list_head pfra_list;     /* for pfra */

}vzone_t;

extern int vzone_alloc(const unsigned int id);
extern int vzone_free(const unsigned int id);
extern struct vzone * get_vzone(const unsigned int id);

#define     PAGEVEC_SIZE    14
/* LRU CACHE */
typedef struct pagevec
{
    unsigned long nr;
    unsigned long cold;
    struct list_head *list[PAGEVEC_SIZE]; 
}pagevec_t;

typedef struct pgdata
{
    /* buddy info */
    unsigned long nr_pages;
    unsigned long start_mem;
    unsigned long reserve_mem;
    unsigned long nr_reserve_pages;
    //unsigned long reserve_room;

    struct list_head zonelist;

    pthread_mutex_t mutex_lock;

    /* reclaim  info */

    /* active page */
    struct list_head active_list;
    unsigned long nr_active;
    /* inactive page */
    struct list_head inactive_list;
    unsigned long nr_inactive;

    pthread_mutex_t shrink_lock;
    pthread_cond_t shrink_cond;
    pthread_mutex_t swap_lock;
    pthread_cond_t swap_cond;

    struct list_head shrink_list;
    unsigned long  nr_shrink;

    uint8_t shrink_ratio;

    /* is pfra urgency ? */
    uint8_t is_urgency;

}pgdata_t;

#define     PGDATA_SHRINK_NR           32
#define     PGDATA_SHRINK_RATIO        99
#define     PGDATA_URGENCY_RATIO       1
#define     PGDATA_URGENCY(pgdata)    (pgdata->is_urgency == 1)

extern unsigned long v_alloc_page(struct vzone *zone, const uint32_t prio, const size_t size, unsigned long flags);
extern int v_free_page(struct vzone *zone, void *address);
extern int v_lookup_page(struct vzone *zone, const void * address);
extern int v_readpage(struct vzone *zone, void **address, uio_t *uio);
extern int v_writepage(struct vzone *zone, void **address, uio_t *uio);
extern int v_free_swap(swp_entry_t entry);

extern void * kswp_thread_fn(void *args);
extern void *kreclaim_thread_fn(void *args);
extern void * idle_thread_fn(void *args);

extern int init_mm();

extern struct pgdata *pg_data ;

#endif
