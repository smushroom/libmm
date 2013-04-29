#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include "debug.h"
#include "page.h"
#include "buddy.h"
#include "slab.h"

static struct page **mem_map = NULL;
static unsigned long start_page_mem = 0;

static int fallbacks[MIGRATE_TYPES][MIGRATE_TYPES - 1] = 
{
    [MIGRATE_MOVABLE] = {MIGRATE_RECLAIMABLE, MIGRATE_UNMOVABLE, MIGRATE_RESERVE, MIGRATE_IOSOLATE},
    [MIGRATE_RECLAIMABLE] = {MIGRATE_MOVABLE, MIGRATE_UNMOVABLE, MIGRATE_RESERVE, MIGRATE_IOSOLATE},
    [MIGRATE_UNMOVABLE] = {MIGRATE_MOVABLE, MIGRATE_RECLAIMABLE, MIGRATE_RESERVE, MIGRATE_IOSOLATE},
    [MIGRATE_RESERVE] = {MIGRATE_MOVABLE, MIGRATE_RECLAIMABLE, MIGRATE_UNMOVABLE, MIGRATE_IOSOLATE},
    [MIGRATE_IOSOLATE] = {MIGRATE_MOVABLE, MIGRATE_RECLAIMABLE, MIGRATE_UNMOVABLE, MIGRATE_RESERVE},
};

inline struct page * index_to_page(unsigned long index)
{
    return mem_map[index];
}

inline struct page * pfn_to_page(const addr_t address)
{
    return mem_map[(address - start_page_mem ) >> PAGE_SHIFT];
}

inline unsigned long pfn_to_index(const addr_t address)
{
    return (address - start_page_mem) >> PAGE_SHIFT;
}

inline unsigned long index_to_pfn(unsigned long index)
{
    return (start_page_mem + (index << PAGE_SHIFT));
}

inline unsigned long page_to_index(const struct page *page)
{
    return (page - *mem_map);
}

inline unsigned long page_address(const struct page *page)
{
    return (start_page_mem + ((page - *mem_map) << PAGE_SHIFT));
}

static inline int free_pages_ok(struct page *page, const uint32_t order)
{
    if(page == NULL || order >= NR_MEM_LISTS)
    {
        return -1;
    }

    /* insert into buddy list by address sort */
    if(free_list_add_line(page, page->prio, order) < 0)
    {
        DD("free_list_add_line error. ");
        return -3;
    }

    buddy_list.nr_free_pages  += 1 << order;

    addr_t address = page_address(page);
    memset((void *)address, 0, order << PAGE_SHIFT);

    return 0;
}

int free_pages(struct page *page, const uint32_t order)
{
    /*DD("free_pages");*/
    if(page == NULL || order >= NR_MEM_LISTS)
    {
        return -1;
    }

    if(PAGE_reserved(PAGE_RESERVED))
    {
        int ret;
        if((ret = free_pages_ok(page, order)) < 0)
        {
            DD("free_pages_ok error. ret = %d.",ret);
            return -2;
        }
    }

    return 0;
}

/*free one page*/
int _free_page(addr_t address)
{
    if(address < buddy_start() || address > buddy_end())
    {
        return -1;
    }

    struct page *page = pfn_to_page(address);

    return free_pages(page, 0);
}

int _free_pages(addr_t address, const uint32_t order)
{
    if(address < buddy_start() || address > buddy_end())
    {
        DD("address out range.");
        return -1;
    }

    struct page *page = pfn_to_page(address);

    return free_pages(page, order);
}

static struct page * get_page_from_list(const uint32_t prio, const uint32_t order)
{
    free_area_t *queue = NULL;
    struct list_head *head = NULL, *item = NULL;
    struct page *page = NULL;

    queue = buddy_list.free_area + order;
    head = queue->lists + prio;

    if(!list_empty_careful(head) && queue->nr_free_pages > 0)
    {
        if(free_list_del(head, order, &item) != 0)
        {
            DD("free_list_del error.");
            return NULL;
        }
        page = list_entry(item,struct page,list);
        return page;
    }

    return NULL;
}

static int sep_list_from_area(const uint32_t prio, uint32_t order)
{
    struct page *page = NULL;
    free_area_t *queue = NULL;
    struct list_head *head, *item = NULL;

    queue = buddy_list.free_area + order;

    while(queue && order < NR_MEM_LISTS)
    {
        if(queue->nr_free_pages > 0) 
        {
            head = &(queue->lists[prio]);
            if(list_empty_careful(head))
            {
                ++order;
                ++queue;
            }
            else
            {
                /*DD("sep list order = %d. prio = %d", order, prio);*/
                if(free_list_del(head, order, &item) != 0)
                {
                    return -1;
                }

                page = list_entry(item, struct page, list);
                --order;
                --(page->order);
                free_list_add(page, prio, order);
                page += (uint32_t)pow(2,order);
                free_list_add(page, prio,order);

                return order;
            }
        }

        ++order;
        ++queue;
    }

    return -2;
}

/* get page from list */
struct page * _get_free_pages(const uint32_t prio, const uint32_t order)
{
    if(order >= NR_MEM_LISTS)
    {
        return NULL;
    }

    int index = 0;
    struct page *page = NULL;
    int cur_order = order;
    int tmp_order = 0;
    uint32_t cur_prio = prio;

repeat:

    if((page = get_page_from_list(cur_prio, cur_order)))
    {
        buddy_list.nr_free_pages -= 1 << cur_order;
        return page;
    }
    else
    {
        if((tmp_order = sep_list_from_area(cur_prio, cur_order)) >= cur_order)
        {
            goto repeat;
        }
    }

    if(index < MIGRATE_TYPES)
    {
        cur_prio = fallbacks[prio][index];
        ++index;
        goto repeat;
    }

    DD("can't provider order = %d prio = %d memory request.", order, prio);

    return NULL;
}

struct page * get_free_pages(const uint32_t prio, const uint32_t order)
{
    /*if(priority == GFP_ATOMIC)*/
    DD("order = %d.", order);
    return _get_free_pages(prio, order);
}

struct page * get_free_page(const uint32_t prio)
{
    return get_free_pages(prio, 0);
}

/*inline unsigned long get_reserve_mem()*/
/*{*/
    /*return reserve_page_mem;*/
/*}*/

int init_buddy(unsigned long *start_mem, unsigned long *reserve_mem)
{
    int i = 0, ret;
    struct page *p = NULL;
    addr_t *page_ptr = NULL;
    addr_t address = 0;

    buddy_list_init();

    address = sbrk(DEFAULT_PAGE_FNS * sizeof(struct page *));
    /*mem_map = (struct page **)malloc(DEFAULT_PAGE_FNS * sizeof(struct page *));*/
    mem_map = (struct page **)address;
    page_ptr = (addr_t*)malloc(DEFAULT_PAGE_FNS * sizeof(struct page));
    /*start_page_mem = (addr_t)malloc((DEFAULT_PAGE_FNS + RESERVE_PAGE_FNS) << PAGE_SHIFT);*/
    start_page_mem = (addr_t)memalign(PAGE_SIZE, (DEFAULT_PAGE_FNS + RESERVE_PAGE_FNS) << PAGE_SHIFT);
    *start_mem = start_page_mem;

    p = (struct page *)page_ptr;
    address = start_page_mem;
    for(i = 0;p && i < DEFAULT_PAGE_FNS;++i, ++p)
    {
        /*DD("address = %x.",address);*/
        p->flags = PAGE_RESERVED;
        p->virtual = address;
        p->prio = MIGRATE_MOVABLE;
        address += PAGE_SIZE;
        *(mem_map + i) = p;;

        if((ret = free_page(p)) < 0)
        {
            DD("free_page error. ret = %d.", ret);
            return -1;
        }
    }

    *reserve_mem = start_page_mem + (DEFAULT_PAGE_FNS << PAGE_SHIFT);
    /* buddy  */
    buddy_list_tidy(start_page_mem, DEFAULT_PAGE_FNS);

    return 0;
}
