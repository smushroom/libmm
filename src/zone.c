/* virtual memory zone management */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include "debug.h"
#include "zone.h"
#include "mapping.h"
#include "page.h"
#include "slab.h"
#include "swap.h"
#include "pte.h"
#include "uio.h"
#include "pfra.h"

static struct pgdata *pg_data = NULL;

/* init pgdata */
static int pgdata_init(struct pgdata *pgdata)
{
    if(pgdata == NULL)
    {
        return -1;
    }

    /* link all zone list*/
    INIT_LIST_HEAD(&pgdata->zonelist);

    INIT_LIST_HEAD(&pgdata->active_list);
    INIT_LIST_HEAD(&pgdata->inactive_list);
    INIT_LIST_HEAD(&pgdata->shrink_list);

    if((pthread_mutex_init(&pgdata->mutex_lock, NULL)) != 0)
    {
        DD("pthread_mutex_init mutex_lock error : %s.", strerror(errno));
        return -2;
    }

    if((pthread_mutex_init(&pgdata->shrink_lock, NULL)) != 0)
    {
        DD("pthread_mutex_init shrink_lock error : %s.", strerror(errno));
        return -3;
    }

    if((pthread_mutex_init(&pgdata->swap_lock, NULL)) != 0)
    {
        DD("pthread_mutex_init swap_lock error : %s.", strerror(errno));
        return -3;
    }

    if((pthread_cond_init(&pgdata->swap_cond, NULL)) != 0)
    {
        DD("pthread_cond_init swap_cond  error : %s.", strerror(errno));
        return -4;
    }

    if((pthread_cond_init(&pgdata->shrink_cond, NULL)) != 0)
    {
        DD("pthread_cond_init shrink_cond error : %s.", strerror(errno));
        return -4;
    }


    pgdata->is_urgency = 0;
    pgdata->shrink_ratio = PGDATA_SHRINK_RATIO;
    pgdata->nr_pages = DEFAULT_PAGE_FNS;
    pgdata->nr_reserve_pages = RESERVE_PAGE_FNS;
    pgdata->nr_active = 0;
    pgdata->nr_inactive = 0;
    pgdata->nr_shrink = 0;

    if(kmem_mcb_init() != 0)
    {
        DD("kmem_mcb_init error.");
        return -3;
    }

    if(kmem_uio_init() != 0)
    {
        DD("kmem_uio_init error.");
        return -4;
    }


    return 0;
}

struct pgdata * pgdata_alloc()
{
    struct pgdata *pgdata = NULL;

    pgdata = (struct pgdata *)malloc(sizeof(struct pgdata));

    pgdata_init(pgdata);

    return pgdata;
}

/* get zone by id */
struct vzone * get_vzone(const unsigned int id)
{
    struct vzone *zone = NULL;
    struct list_head *pos, *n, *head;

    head = &pg_data->zonelist;

    list_for_each_safe(pos, n, head)
    {
        zone = list_entry(pos, struct vzone, zone_list);
        if(zone && (zone->id == id))
        {
            return zone;
        }
    }

    return NULL;
}

/* free zone  */
int vzone_free(const unsigned int id)
{
    struct vzone *zone = get_vzone(id);
    if(zone)
    {
        shrink_zone(pg_data, zone);
        free(zone->mapping);
        free(zone);
        return 0;
    }

    return -1;
}

/* alloc zone and add to zonelist */
int vzone_alloc(const unsigned int id)
{
    struct vzone * zone = (struct vzone *)malloc(sizeof(struct vzone));
    if(zone == NULL)
    {
        return -1;
    }

    struct address_mapping *mapping = mapping_alloc();
    if(mapping == NULL)
    {
        return -2;
    }
    mapping->zone = zone;

    zone->mapping = mapping;
    zone->rb_root.rb_node = NULL;
    zone->id = id;

    list_add(&zone->zone_list, &pg_data->zonelist);

    return 0;
}

void print_list_nr()
{
    DD("active page = %ld.", pg_data->nr_active);
    DD("inactive page = %ld.", pg_data->nr_inactive);
    DD("shrink page = %ld.", pg_data->nr_shrink);
    DD("free page nr = %d.", get_free_pages_nr());
}

/* add a page to active list */
static inline void add_active_list(struct pgdata *pgdata, struct list_head *item)
{
    struct page *page = list_entry(item, struct page, lru);
    page_sflags(page, PAGE_ACTIVE);
    list_add_tail(item, &pgdata->active_list);
    ++(pgdata->nr_active);
}

/* add a page to inactive list */
static inline void add_inactive_list(struct pgdata *pgdata, struct list_head *item)
{
    struct page *page = list_entry(item, struct page, lru);
    page_sflags(page, PAGE_INACTIVE);
    list_add_tail(item, &pgdata->inactive_list);
    ++(pgdata->nr_inactive);
}

/* move page to active list */
static inline void _move_to_active_list(struct pgdata *pgdata, struct list_head *item)
{
    list_del(item);
    --(pgdata->nr_inactive);
    struct page *page = list_entry(item, struct page, lru);
    page_cflags(page, PAGE_INACTIVE);
    add_active_list(pgdata, item);
}

/* move page to inactive list */
static inline void _move_to_inactive_list(struct pgdata *pgdata, struct list_head *item)
{
    list_del(item);
    --(pgdata->nr_active);
    struct page *page = list_entry(item, struct page, lru);
    page_cflags(page, PAGE_ACTIVE);
    add_inactive_list(pgdata, item);
}

/* move page to active list */
void move_to_active_list(struct pgdata *pgdata, struct page *page)
{
    if(PAGE_inactive(page->flags))
    {
        DD("111111111111111111");
        _move_to_active_list(pgdata, &page->lru);
    }
    else if(PAGE_active(page->flags))
    {
        DD("2222222222222222222");
    }
    else
    {
        DD("3333333333333333333");
        add_active_list(pgdata, &page->lru);
    }
}

/* move page to inactive list */
void move_to_inactive_list(struct pgdata *pgdata, struct page *page)
{
    if(PAGE_active(page->flags))
    {
        page_cflags(page, PAGE_ACTIVE);
        _move_to_inactive_list(pgdata, &page->lru);
    }
    else if(PAGE_inactive(page->flags))
    {
    }
    else
    {
        page_sflags(page, PAGE_INACTIVE);
        add_inactive_list(pgdata, &page->lru);
    }
}

/* move page to shrink list */
static inline void move_to_shrink_list(struct pgdata *pgdata, struct list_head *item)
{
    list_del(item);
    list_add_tail(item, &pgdata->shrink_list);
    ++(pgdata->nr_shrink);
}

static inline void del_active_list(struct pgdata *pgdata, struct list_head *item)
{
    if(item->next == NULL && item->prev == NULL)
    {
        DD("del from active list already.");
    }
    else
    {
        list_del(item);
        --(pgdata->nr_active);
    }
}

static inline void del_inactive_list(struct pgdata *pgdata, struct list_head *item)
{
    if(item->next == NULL && item->prev == NULL)
    {
        DD("del from inactive list already.");
    }
    else
    {
        list_del(item);
        --(pgdata->nr_inactive);
    }
}

/* del shrink list */
static inline void del_shrink_list(struct pgdata *pgdata, struct list_head *item)
{
    /*list_del(item);*/
    --(pgdata->nr_shrink);
}

/* get pte from memory control block */
static pte_t * mcb_get_pte(struct vzone *zone, unsigned long address)
{
    if(zone == NULL || address <= 0)
    {
        return NULL;
    }

    mem_control_block_t *mcb = mcb_search(&zone->rb_root, (unsigned long)address);
    if(mcb == NULL)
    {
        DD("can't find node : address = 0x%lx", (unsigned long)address);
        return NULL;
    }

    return mcb->pte;
}

/* get address size from memory control block */
static size_t mcb_get_size(struct vzone *zone, unsigned long address)
{
    if(zone == NULL || address <= 0)
    {
        return -1;
    }

    mem_control_block_t *mcb = mcb_search(&zone->rb_root, (unsigned long)address);
    if(mcb == NULL)
    {
        DD("can't find node : address = 0x%lx", (unsigned long)address);
        return -2;
    }

    return mcb->size;
}

/* get address flags from memory control block */
static unsigned long mcb_get_flags(struct vzone *zone, unsigned long address)
{
    if(zone == NULL || address <= 0)
    {
        return -1;
    }

    mem_control_block_t *mcb = mcb_search(&zone->rb_root, (unsigned long)address);
    if(mcb == NULL)
    {
        DD("can't find node : address = 0x%lx", (unsigned long)address);
        return -2;
    }

    return mcb->flags;
}

/* alloc page */
unsigned long v_alloc_page(struct vzone *zone, const uint32_t prio, const size_t size, unsigned long flags)
{
    if(zone == NULL)
    {
        return 0;
    }

    print_list_nr();

    struct page *origin_page = get_free_pages(prio, size_to_order(size));

    int i = 0, ret;
    int nr_pages = size >> PAGE_SHIFT;
    struct address_mapping *mapping = zone->mapping;

    /* check bio flags */
    chk_bio_flags(&flags);
    DD("alloc page flags = 0x%lx", flags);

    pgd_t *pgd = &(zone->pgd);
    pte_t *pte = NULL;

    struct page *page = origin_page;

    while(page && i < nr_pages)
    {
        if(mapping->a_ops->insertpage)
        {
            if((ret = mapping->a_ops->insertpage(mapping, page_to_index(page),page)) != 0)
            {
                DD("mapping_insert error. ret = %d.", ret);
                return 0;
            }
        }

        /* set pte and page */
        page_sflags(page, PAGE_PRESENT);

        BIO_private(flags)?page_sflags(page, PAGE_PRIVATE):page_sflags(page, PAGE_SHARED);
        BIO_sync(flags)?page_sflags(page, PAGE_SYNC):page_sflags(page, PAGE_ASYNC);
        DD("flags = 0x%lx", flags);
        DD("page flags = 0x%lx", page->flags);
        BIO_lock(flags)?page_sflags(page, PAGE_LOCK):page_sflags(page, PAGE_UNLOCK);
        DD("page flags = 0x%lx", page->flags);

        add_active_list(pg_data,&page->lru);


        page->mapping = mapping;
        DD("page address = 0x%lx", page_address(page));
        DD("page index = 0x%lx", page_to_index(page));

        pte = set_pte(pgd, page_to_index(page), 0);

        /*init for pfra */
        init_pfra_page(zone, pte, page, flags);

        ++page;
        ++i;
    }

    pte = get_pte(pgd, page_to_index(origin_page));

    mem_control_block_t *mcb = mcb_alloc();
    mcb->vruntime = time(NULL);
    mcb->size = size;
    mcb->flags = flags;
    mcb->address = page_address(origin_page);
    mcb->pte = pte;
    mcb_insert(&zone->rb_root, mcb);

    DD("origin_page address = 0x%lx", page_address(origin_page));

    print_list_nr();

    if(BIO_lock(flags))
    {
        mlock(page_address(origin_page), size);
    }

    return page_address(origin_page);
}

static int _do_free_to_swap(pte_t *pte, swp_entry_t *entry)
{
    if(PTE_present(pte))
    {
        struct page *page = index_to_page(pte_index(pte));
        /* is page is lock ? */
        DD("page flags = 0x%lx", page->flags);
        if(PAGE_unlock(page->flags))
        /*if(PAGE_lock(page->flags))*/
        {
            if(PAGE_present(page->flags))
            {
                struct address_mapping *mapping = (struct address_mapping *)page->mapping;
                if(mapping->a_ops->removepage)
                {
                    DD("mapping removepage.");
                    if(mapping->a_ops->removepage(mapping, page_to_index(page)) == NULL)
                    {
                        DD("remove page error.");
                        return -3;
                    }
                }
                DD("revove page index = %ld", page_to_index(page));

                page->mapping = NULL;

                page_cflags(page, PAGE_PRESENT);
                page_sflags(page, PAGE_SWAPOUT);
                page_cflags(page, PAGE_ACTIVE);
                page_cflags(page, PAGE_REFERENCED);

                DD("free to swap page flags = 0x%lx.", page->flags);
                do_swp_writepage(page, entry);

                DD("free to swap page->index = %ld.", page_to_index(page))
                    del_active_list(pg_data, &page->lru);

                make_pte(swp_offset(entry),0, swp_type(entry),pte);
                DD("free to swap pte = 0x%lx.", pte->val);

                /* free page */
                if(free_pages(page, 0) != 0)
                {
                    DD("free_pages error.");
                    return -3;
                }
            }
        }
        else
        {
            DD("page is locked.");
            return -4;
        }
    }
    else
    {
        DD("page is not in memory");
        return -5;
    }

    return 0;
}

/* free page to swap */
static int do_free_to_swap(struct vzone *zone, const void *address, swp_entry_t *entry)
{
    if(zone == NULL || address == NULL || entry == NULL)
    {
        return -1;
    }

    int i = 0;

    pte_t *origin_pte = mcb_get_pte(zone, (unsigned long)address);
    pte_t *pte = origin_pte;
    unsigned int nr_pages = (mcb_get_size(zone, (unsigned long)address) >> PAGE_SHIFT);

    while(pte && i < nr_pages)
    {
        if(_do_free_to_swap(pte, entry) != 0)
        {
            DD("_do_free_to_swap error.");
            return -3;
        }

        ++pte;
        ++i;
    }

    print_list_nr();

    return 0;
}

/* free page to swap wrapper*/
int v_free_to_swap(struct vzone *zone, const void *address)
{
    swp_entry_t entry;
    return do_free_to_swap(zone, address, &entry);
}

/* free page */
int v_free_page(struct vzone *zone, void *address)
{
    DD("v_free_page.");

    if(zone == NULL || address == NULL)
    {
        return -1;
    }

    int i = 0;
    pte_t *origin_pte = mcb_get_pte(zone, (unsigned long)address);
    pte_t *pte = origin_pte;
    unsigned int nr_pages = (mcb_get_size(zone, (unsigned long)address) >> PAGE_SHIFT);

    while(pte && i < nr_pages)
    {
        /* in memrory */
        if(PTE_present(pte))
        {
            struct page *page = index_to_page(pte_index(pte));
            if(PAGE_present(page->flags))
            {
                struct address_mapping *mapping = zone->mapping;
                if(mapping->a_ops->removepage)
                {
                    DD("mapping removepage.");
                    if(mapping->a_ops->removepage(mapping, page_to_index(page)) == NULL)
                    {
                        DD("remove page error.");
                        return -3;
                    }
                }

                page_cflags(page, PAGE_PRESENT);
                page_cflags(page, PAGE_ACTIVE);
                del_active_list(pg_data, &page->lru);
                page->mapping = NULL;
            }

            /* free page */
            if(free_pages(page, page->order) != 0)
            {
                DD("free_pages error.");
                return -3;
            }
        }

        /* in swap */
        else if(!PTE_present(pte))
        {
            swp_entry_t entry;
            swp_entry(pte_index(pte), 0,pte_type(pte), &entry);

            if(do_swp_freepage(entry) != 0)
            {
                DD("v_free_swap error.");
                return -2;
            }
        }

        DD("put_pte.");
        /* free pte */
        put_pte(pte);

        ++pte;
        ++i;
    }

    return 0;
}

static int generic_writepage(struct vzone *zone, struct page *page)
{
    if(zone == NULL || page == NULL)
    {
        return -1;
    }

    if(PAGE_present(page->flags))
    {
        struct address_mapping *mapping = zone->mapping;
        if(mapping->a_ops->readpage(mapping, page_to_index(page), page->flags) != 0)
        {
            DD("mapping writepage error.");
            return -2;
        }

        /* move to active list */
        page_sflags(page, PAGE_REFERENCED);
        move_to_active_list(pg_data, page);
    }
    else
    {
        return -3;
    }

    return 0;
}

static int generic_readpage(struct vzone *zone, struct page *page)
{
    if(zone == NULL || page == NULL)
    {
        return -1;
    }

    if(PAGE_present(page->flags))
    {
        struct address_mapping *mapping = zone->mapping;
        if(mapping->a_ops->readpage(mapping, page_to_index(page), page->flags) != 0)
        {
            DD("mapping readpage error.");
            return -2;
        }

        /* move to active list */
        DD("move to active list.");
        page_sflags(page, PAGE_REFERENCED);
        move_to_active_list(pg_data, page);
    }
    else
    {
        return -3;
    }

    return 0;
}

/* write page slow */
static unsigned long do_writepage_slow(struct vzone *zone, pte_t *pte, unsigned long flags)
{
    if(zone == NULL || pte == NULL)
    {
        return 0;
    }

    DD("do_writepage_slow.");
    int ret;
    swp_entry_t entry;
    swp_entry(pte_index(pte), 0,pte_type(pte), &entry);
    struct page *page = get_free_page(0);
    DD("new page address = 0x%lx.", page_address(page));
    DD("new page index = 0x%lx.", page_to_index(page));

    if((ret = do_swp_readpage(entry, page)) != 0)
    {
        DD("do_swp_readpage error. ret = %d.",ret);
        return 0;
    }

    struct address_mapping *mapping = zone->mapping;
    if((ret = mapping->a_ops->insertpage(mapping, page_to_index(page), page)) != 0)
    {
        DD("radix_tree_insert error. ret = %d.", ret);
        return 0;
    }

    /* set page and pte */
    BIO_private(flags)?page_sflags(page, PAGE_PRIVATE):page_sflags(page, PAGE_SHARED);
    BIO_sync(flags)?page_sflags(page, PAGE_SYNC):page_sflags(page, PAGE_ASYNC);

    page_cflags(page, PAGE_SWAPOUT);
    page_sflags(page, PAGE_PRESENT);

    DD("do_writepage flags = 0x%lx", page->flags);

    page->mapping = mapping;
    make_pte(page_to_index(page), PTE_PRESENT,0, pte);

    /* move to active list */
    page_sflags(page, PAGE_REFERENCED);
    DD("move to active list");
    move_to_active_list(pg_data, page);

    DD("new address = 0x%lx", page_address(page));
    return page_address(page);
}

/* read page slow path */
static unsigned long do_readpage_slow(struct vzone *zone, pte_t *pte, unsigned long flags)
{
    if(zone == NULL || pte == NULL)
    {
        return 0;
    }

    int ret;
    swp_entry_t entry;
    swp_entry(pte_index(pte), 0,pte_type(pte), &entry);

    /* alloc a new page */
    struct page *page = get_free_page(0);
    if(page == NULL)
    {
        return 0;
    }

    if((ret = do_swp_readpage(entry, page)) != 0)
    {
        DD("do_swp_readpage error. ret = %d.",ret);
        return 0;
    }

    struct address_mapping *mapping = zone->mapping;
    if((ret = mapping->a_ops->insertpage(mapping, page_to_index(page), page)) != 0)
    {
        DD("radix_tree_insert error. ret = %d.", ret);
        return 0;
    }

    DD("do_readpage_slow flags = 0x%lx", page->flags);

    BIO_private(flags)?page_sflags(page, PAGE_PRIVATE):page_sflags(page, PAGE_SHARED);
    BIO_sync(flags)?page_sflags(page, PAGE_SYNC):page_sflags(page, PAGE_ASYNC);
    /* set page and pte */
    DD("do_readpage_slow mapping = 0x%p", mapping);
    page->mapping = mapping;
    page->pgd_index = pgd_index(&zone->pgd, pte);
    page_cflags(page, PAGE_SWAPOUT);
    page_sflags(page, PAGE_PRESENT);
    DD("do_readpage_slow flags = 0x%lx", page->flags);
    make_pte(page_to_index(page), PTE_PRESENT,0, pte);

    /* move to active list */
    page_sflags(page, PAGE_REFERENCED);
    DD("move to active list");
    move_to_active_list(pg_data, page);

    DD("new address = 0x%lx", page_address(page));
    return page_address(page);
}


static int _v_readpage(struct vzone *zone, pte_t *pte, unsigned long flags, uio_t *uio, unsigned long *new_address)
{
    if(zone == NULL ||  pte == NULL || uio == NULL)
    {
        return -1;
    } 

    struct page *page = NULL;

    if(empty_pte(pte))
    {
        DD("page is not in memory or swap. data is lost!");
        return -2;
    }
    /* in memrory */
    else if(PTE_present(pte))
    {
        /*unsigned long index = pte_index(pte);*/
        page = index_to_page(pte_index(pte));
        DD("read page flags = 0x%lx", page->flags);
        if(PAGE_present(page->flags))
        {
            if(generic_readpage(zone, page) != 0)
            {
                DD("read page is not in mapping.");
                return -3;
            }

            DD("page is in mapping.");
        }
    }
    /* in swapout */
    else if(!PTE_present(pte))
    {
        if(BIO_sync(flags))
        {
            if((*new_address = do_readpage_slow(zone, pte, flags)) <= 0)
            {
                DD("do_readpage_slow error.");
                return -4;
            }
        }

        else if(BIO_async(flags))
        {
            DD("read from async.");
        }
        else
        {
            return -5;
        }
    }

    /* copy data to dst */
    page = index_to_page(pte_index(pte));
    unsigned long address = page_address(page);
    memcpy(uio->u_base, (void *)(address + uio->u_offset), uio->u_size);
    DD("read to address size = %ld.", uio->u_size);

    return uio->u_size;
}

/* read page */
int v_readpage(struct vzone *zone, void **old_addr,uio_t *uio)
{
    if(zone == NULL || old_addr == NULL || uio == NULL)
    {
        return -1;
    }

    unsigned long *address = (unsigned long *)(*old_addr);
    size_t size = uio->u_size;
    mem_control_block_t *mcb = mcb_search(&zone->rb_root, (unsigned long)address);
    if(size > mcb->size)
    {
        return -2;
    }

    int ret,i = 0;
    unsigned long flags = mcb->flags;
    /*DD("flags = 0x%lx.", flags);*/
    unsigned long offset = uio->u_offset;
    unsigned int page_offset, count = 0;
    unsigned int nr_read = 0;
    unsigned long new_address;

    pte_t *origin_pte = mcb->pte;
    pte_t *pte = origin_pte;

    /*DD("size = %ld.", size);*/
    while(size > 0)
    {
        pte = origin_pte + (offset >> PAGE_SHIFT);
        DD("read pte = 0x%lx", pte->val);
        page_offset = offset % (PAGE_SIZE);
        count = PAGE_SIZE - page_offset;
        if(count > size)
        {
            count = size;
        }

        uio->u_offset = page_offset;
        uio->u_size = count;
        if((ret = _v_readpage(zone, pte, flags, uio, &new_address)) < 0)
        {
            DD("_v_readpage error. ret = %d.", ret);
            return -3;
        }

        nr_read += ret;
        uio->u_base += count;

        /*nr_write += count;*/
        offset += count;
        size -= count;
    }

    DD("old address = 0x%lx", (unsigned long)address);
    DD("new index = 0x%lx", pte_index(origin_pte));
    new_address = index_to_pfn(pte_index(origin_pte));
    DD("new address = 0x%lx", new_address);

    if((unsigned long)address != new_address)
    {
        DD("mcb replace.");
        mcb_replace(&zone->rb_root, mcb);
        mcb->address = new_address;
        mcb_insert(&zone->rb_root, mcb);
    }

    *old_addr = (void *)index_to_pfn(pte_index(origin_pte));

    print_list_nr();

    return nr_read;
}

static int _v_writepage(struct vzone *zone, pte_t *pte, unsigned long flags, uio_t *uio, unsigned long *new_address)
{
    if(zone == NULL || pte == NULL || uio == NULL || new_address == NULL)
    {
        return -1;
    } 

    *new_address = 0;
    struct page *page = NULL;

    /* if is in the mapping */
    if(empty_pte(pte))
    {
        DD("page is not in memory or swap. data is lost!");
        return -2;
    }
    else if(PTE_present(pte))
    {
        page = index_to_page(pte_index(pte));
        DD("_v_writepage : page pte_index = %ld.", page->pgd_index);
        DD("_v_writepage : page mapping = 0x%p", page->mapping);
        if(PAGE_present(page->flags))
        {
            if(generic_writepage(zone, page) != 0)
            {
                DD("mapping writepage error.");
                return -3;
            }
        }
    }
    else if(!PTE_present(pte))
    {
        if(BIO_sync(flags))
        {
            /*unsigned long new_address;*/
            if((*new_address = do_writepage_slow(zone, pte, flags)) <= 0)
            {
                DD("do_writepage_slow error.");
                return -4;
            }
        }
        else if(BIO_async(flags))
        {
            DD("write from async");
        }
        else
        {
            return -5;
        }
    }

    /* copy data to page */
    page = index_to_page(pte_index(pte));
    unsigned long address = page_address(page);
    memcpy((void *)(address + uio->u_offset), uio->u_base, uio->u_size);
    DD("write to address size = %ld.", uio->u_size);

    return uio->u_size;
}

/* write to page */
int v_writepage(struct vzone *zone, void **old_addr, uio_t *uio)
{
    if(zone == NULL || old_addr == NULL || uio == NULL)
    {
        return -1;
    }

    unsigned long *address = (unsigned long *)(*old_addr);
    size_t size = uio->u_size;
    mem_control_block_t *mcb = mcb_search(&zone->rb_root, (unsigned long)address);
    if(size > mcb->size)
    {
        return -2;
    }

    int ret,i = 0;
    unsigned long flags = mcb->flags;
    unsigned long offset = uio->u_offset;
    unsigned int page_offset, count = 0;
    unsigned int nr_write = 0;
    unsigned long new_address;

    /*pte_t *origin_pte = mcb_get_pte(zone, (unsigned long)address);*/
    pte_t *origin_pte = mcb->pte;

    pte_t *pte = origin_pte;

    DD("size = %ld.", size);
    while(size > 0)
    {
        pte = origin_pte + (offset >> PAGE_SHIFT);
        DD("read pte = 0x%lx", pte->val);
        page_offset = offset % (PAGE_SIZE);
        count = PAGE_SIZE - page_offset;
        DD("write page count = %d.",count);
        if(count > size)
        {
            count = size;
        }

        uio->u_offset = page_offset;
        uio->u_size = count;
        if((ret = _v_writepage(zone, pte, flags, uio, &new_address)) < 0)
        {
            DD("_v_writepage error. ret = %d.", ret);
            return -3;
        }

        if(new_address != 0)
        {
            DD("new address = 0x%lx", new_address);
            _set_pte(pte, pfn_to_index(new_address),pte_flags(pte), pte_type(pte));
        }

        nr_write += ret;

        uio->u_base += count;

        /*nr_write += count;*/
        offset += count;
        size -= count;
    }

    DD("old address = 0x%lx", (unsigned long)address);
    DD("new index = 0x%lx", pte_index(origin_pte));
    new_address = index_to_pfn(pte_index(origin_pte));
    DD("new address = 0x%lx", new_address);

    if((unsigned long)address != new_address)
    {
        DD("mcb replace.");
        mcb_replace(&zone->rb_root, mcb);
        mcb->address = new_address;
        mcb_insert(&zone->rb_root, mcb);
    }

    /*DD("new address = 0x%lx", index_to_address(pte_index(origin_pte)));*/
    *old_addr = (void *)index_to_pfn(pte_index(origin_pte));

    print_list_nr();

    return nr_write;
}

static int _v_lookup_page(struct vzone *zone, pte_t *pte)
{
    if(zone == NULL || pte == NULL)
    {
        return -1;
    }

    struct address_mapping *mapping = zone->mapping;
    struct page *page = NULL;
    if(mapping->a_ops->lookup_page)
    {
        DD("lookup index = %ld.", pte_index(pte));
        if((page = mapping->a_ops->lookup_page(mapping, pte_index(pte))) == NULL)
        {
            DD("can't find page : index = %ld ", pte_index(pte));
            return -2;
        }
    }

    return 0;
}

/* search address */
int v_lookup_page(struct vzone *zone, const void *address)
{
    if(zone == NULL || address == NULL)
    {
        return -1;
    }

    int i = 0;
    pte_t *pte = mcb_get_pte(zone, (unsigned long)address);
    unsigned int nr_pages = (mcb_get_size(zone, (unsigned long)address) >> PAGE_SHIFT);

    while(pte && i < nr_pages)
    {
        if(_v_lookup_page(zone, pte) != 0)
        {
            DD("pte not in memory : 0x%lx", (unsigned long)pte);
            return -2;
        }

        ++pte;
        ++i;
    }

    return 0;
}

/* shrink pgdata inactive list */
static int shrink_inactive_list(struct pgdata *pgdata)
{
    if(pgdata == NULL)
    {
        return -1;
    }

    struct list_head *pos, *n, *head;
    head = &pgdata->inactive_list;
    DD("inactive_list nr = %ld", pgdata->nr_inactive);

    if(list_empty_careful(head))
    {
        DD("shrink inactive is empty");
    }
    else
    {
        list_for_each_safe(pos, n, head)
        {
            move_to_shrink_list(pgdata, pos);
            --(pgdata->nr_inactive);

            if(pgdata->nr_shrink >= PGDATA_SHRINK_NR)
            {
                return 1;
            }
        }
    }

    return 0;
}

/* shrink pgdata active list */
static int shrink_active_list(struct pgdata *pgdata)
{
    if(pgdata == NULL)
    {
        return -1;
    }

    struct list_head *pos, *n, *head;
    struct page *page = NULL;
    head = &pg_data->active_list;
    DD("active_list nr = %ld", pgdata->nr_active);

    if(list_empty_careful(head))
    {
        DD("shrink active list is empty");
    }
    else
    {
        DD("shrink_active_list.");
        list_for_each_safe(pos, n, head)
        {
            page = list_entry(pos, struct page, lru);
            DD("shrink active list : page pgd_index = %ld.", page->pgd_index);
            DD("shrink active list : page mapping = 0x%p.", page->mapping);
            if(!PAGE_referenced(page->flags))
            {
                DD("page is no referenced.");
                move_to_shrink_list(pg_data, pos);
                --(pgdata->nr_active);
                if(pgdata->nr_shrink >= PGDATA_SHRINK_NR)
                {
                    return 1;
                }
            }
            else
            {
                DD("page is referenced.");
                /* next time reclaim the page */
                page_cflags(page, PAGE_REFERENCED);
            }
        }
    }

    return 0;
}

static inline unsigned int get_free_ratio(struct pgdata *pgdata)
{
    return ((get_free_pages_nr() * 100) / pg_data->nr_pages);
}

/* shrink list */
int shrink_page_list(struct pgdata *pgdata)
{
    if(pgdata == NULL)
    {
        return -1;
    }

    struct page *page = NULL;
    struct vzone *zone = NULL;
    pte_t *pte = NULL;

    struct list_head *pos, *n, *head;
    struct list_head *pos2, *n2, *head2;
    /*struct vzone *zone = mapping->zone;*/
    head = &pgdata->shrink_list;

    if(list_empty_careful(head))
    {
        DD("shrink list is empty");
    }
    else
    {
        /* first wakeup kswap thread */
        PAGE_free(page->flags);
        DD("----------------------");
        DD("send signal to kswap.");
        pthread_cond_signal(&pgdata->swap_cond);
        DD("----------------------");
    }

    /* second  reclaim page*/
    list_for_each_safe(pos, n, head)
    {
        page = list_entry(pos, struct page, lru);
        zone = ((struct address_mapping *)page->mapping)->zone;

        /* private page */
        if(PAGE_private(page->flags))
        {
            pte = get_pte(&zone->pgd, page->pgd_index);
            DD("shrink page pte_index = %ld", page->pgd_index);
            DD("shrink pte = 0x%lx", pte->val);

            /* mapping memory is urgency */
            if(PGDATA_URGENCY(pg_data))
            {
                /* free page */
                if(free_pages(page, 0) != 0)
                {
                    DD("free_pages error.");
                }

                put_pte(pte);
                list_del(pos);
            }
        }
        /* shared page */
        else if(PAGE_shared(page->flags))
        {
            /* mapping memory is urgency */
            if(PGDATA_URGENCY(pgdata))
            {
                /* list for each shread zone and free page */
                head2 = &page->zone_list;
                list_for_each_safe(pos2, n2, head2)
                {
                    zone = list_entry(pos2, struct vzone, pfra_list);
                    pte = get_pte(&zone->pgd, page->pgd_index);

                    put_pte(pte);
                }
                if(free_pages(page, 0) != 0)
                {
                    DD("free_pages error.");
                }

                list_del(pos);
            }
        }
        else
        {
            DD("page flags : 0x%lx", page->flags);
            return -3;
        }
    }

    return 0;
}

/* shrink all page from one vzone*/
int shrink_zone(struct pgdata *pgdata, struct vzone *zone)
{
    if(pgdata == NULL || zone == NULL)
    {
        return -1;
    }

    int ret;
    int i, nr = 0;
    struct page *page = NULL;
    struct address_mapping *mapping = zone->mapping;

    pthread_mutex_lock(&pg_data->mutex_lock);
    if((mapping->nr_pages * sizeof(void *)) > (pg_data->nr_reserve_pages << PAGE_SHIFT))
    {
        DD("no enough reserve memory used!");
        return -2;
    }
    void **results = (void **)pg_data->reserve_mem;; 

    if(mapping->a_ops->lookup_pages)
    {
        nr = mapping->a_ops->lookup_pages(mapping,results, 1, mapping->nr_pages);
        for(i = 0;i < nr; ++i)
        {
            page = (struct page *)results[i];
            if(mapping->a_ops->removepage)
            {
                if(mapping->a_ops->removepage(mapping, page_to_index(page)) == NULL)
                {
                    DD("shrink_zone remove page error.");
                }

                del_active_list(pgdata, &page->lru);

                DD("shrink_zone free page : 0x%lx", page_address(page));
                if((ret = free_page(page)) < 0)
                {
                    DD("shrink_zone free_page error.");
                    return -2;
                }
            }
            else
            {
                DD("shrink_zone no removepage functions.");
                return -3;
            }
        }
    }
    else
    {
        DD("shrink_zone no lookup_pages functions.");
        return -4;
    }

    pthread_mutex_unlock(&pg_data->mutex_lock);
}

/* swap shrink page */
static int swap_page_list(struct pgdata *pgdata)
{
    DD("swap_page_list.");
    struct page *page = NULL;
    struct vzone *zone = NULL;
    pte_t *pte = NULL;

    struct list_head *pos, *n, *head;
    struct list_head *pos2, *n2, *head2;

    head = &pgdata->shrink_list;

    if(list_empty_careful(head))
    {
        DD("shrink list is empty");
    }
    else
    {
        list_for_each_safe(pos, n, head)
        {
            page = list_entry(pos, struct page, lru);
            zone = ((struct address_mapping *)page->mapping)->zone;
            DD("zone = 0x%p.", zone);

            DD("***************************************************");
            /* private page */
            if(PAGE_private(page->flags))
            {
                DD("*****************************");
                DD("active list nr = %ld", pgdata->nr_active);

                pte = get_pte(&zone->pgd, page->pgd_index);

                DD("shrink pte = 0x%lx", pte->val);
                DD("active list nr = %ld", pgdata->nr_active);

                swp_entry_t entry;
                if(_do_free_to_swap((void *)pte, &entry) == 0)
                {
                    /* fixed the bug */
                    ++(pgdata->nr_active);
                    del_shrink_list(pgdata, pos);
                }
                DD("******************");
                DD("active list nr = %ld", pgdata->nr_active);
            }

            /* shared page */
            else if(PAGE_shared(page->flags))
            {
                pte = get_pte(&zone->pgd, page->pgd_index);

                swp_entry_t entry;
                if(_do_free_to_swap((void *)pte, &entry) == 0)
                {
                    del_shrink_list(pgdata, pos);
                }

                head2 = &page->zone_list;
                list_for_each_safe(pos2, n2, head2)
                {
                    zone = list_entry(pos2, struct vzone, pfra_list);
                    pte = get_pte(&zone->pgd, page->pgd_index);
                    make_pte(swp_offset(&entry),0, swp_type(&entry),pte);
                }
            }
            else
            {
                return -2;
            }
            DD("***************************************************");
        }
    }

    return 0;
}

void * idle_thread_fn(void *args)
{
    while(1)
    {
        /* check global option */
        /*check_options();*/

        DD("free page_nr = %d.", get_free_pages_nr());
        DD("all page = %ld.", pg_data->nr_pages);
        DD("free ratio = %d.", get_free_ratio(pg_data));

        if(get_free_ratio(pg_data) < pg_data->shrink_ratio)
        {
            DD("----------------------");
            DD("send signal to kreclaim");
            DD("----------------------");
            pthread_cond_signal(&pg_data->shrink_cond);
        }

        sleep(3);
    }
}

/* kswap page thread */
void * kswp_thread_fn(void *args)
{
    struct list_head *pos, *n, *head;
    struct vzone *zone = NULL;
    struct address_mapping *mapping = NULL;

    while(1)
    {
        pthread_mutex_lock(&pg_data->swap_lock);
        pthread_cond_wait(&pg_data->swap_cond, &pg_data->swap_lock);
        pthread_mutex_unlock(&pg_data->swap_lock);

        DD("==============================");
        DD("nr_inactive = %ld.", pg_data->nr_inactive);
        DD("nr_active = %ld.", pg_data->nr_active);
        DD("nr_shrink = %ld.", pg_data->nr_shrink);
        DD("nr_pages = %ld.", pg_data->nr_pages);
        DD("wake up by kreclaim.");
        DD("==============================");
        head = &pg_data->zonelist;
        if(list_empty_careful(head))
        {
            DD("kswap zone list is empty");
        }
        else
        {
            DD("kswap_thread is running.");
            list_for_each_safe(pos, n, head)
            {
                zone = list_entry(pos, struct vzone, zone_list);
                mapping = zone->mapping;

                if(swap_page_list(pg_data) != 0)
                {
                    DD("swap_page_list error.");
                }
            }
        }

        sleep(1);
    }
}

/* reclaim page thread */
void *kreclaim_thread_fn(void *args)
{
    struct list_head *pos, *n, *head;
    struct vzone *zone = NULL;
    struct address_mapping *mapping = NULL;

    while(1)
    {
        pthread_mutex_lock(&pg_data->shrink_lock);
        pthread_cond_wait(&pg_data->shrink_cond, &pg_data->shrink_lock);
        pthread_mutex_unlock(&pg_data->shrink_lock);

        DD("+++++++++++++++++++++++++++++++++++++++");
        DD("wake up by idle thread.");
        DD("+++++++++++++++++++++++++++++++++++++++");

        if(shrink_inactive_list(pg_data) < 1)
        {
            if(shrink_active_list(pg_data) < 1)
            {
                /*shrink_zone();*/
            }
        }

        DD("shrink page list nr = %ld.", pg_data->nr_shrink);
        if(shrink_page_list(pg_data) != 0)
        {
            DD("shrink_list error.");
            return;
        }
        sleep(1);
    }

    return 0;
}

int init_mm()
{
    /* buddy */
    unsigned long buddy_start_mem;
    unsigned long buddy_reserve_mem;
    init_buddy(&buddy_start_mem, &buddy_reserve_mem);


    /* slab */
    kmem_cache_init();

    /* swap on */
    swp_on();

    pg_data = pgdata_alloc();
    pg_data->start_mem = buddy_start_mem;
    pg_data->reserve_mem = buddy_reserve_mem;

    print_list_nr();

    /* thread */
    lthread_init();

    return 0;
}
