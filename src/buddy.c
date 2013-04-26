#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "debug.h"
#include "page.h"
#include "buddy.h"
#include "radix-tree.h"

buddy_list_t buddy_list;

inline addr_t buddy_start()
{
    return buddy_list.start;
}

inline addr_t buddy_end()
{
    return buddy_list.end;
}

inline void init_buddy_range(const addr_t start, const addr_t end)
{
    buddy_list.start = start;
    buddy_list.end = end;
}

inline free_area_t * buddy_freearea(const uint32_t order)
{
    return buddy_list.free_area + order;
}

inline void inc_freelist_pagenr(const uint32_t order)
{
    ++(buddy_list.free_area[order].nr_free_pages);
}

inline void dec_freelist_pagenr(const uint32_t order)
{
    --(buddy_list.free_area[order].nr_free_pages);
}

inline void zero_freelist_pagenr(const uint32_t order)
{
    buddy_list.free_area[order].nr_free_pages = 0;
}

/*is page address is adjacency*/
static inline int is_pageadj(const struct list_head *prev, const struct list_head *next, const uint32_t order)
{
    int index = 1;
    struct page *prev_page = NULL,*next_page = NULL;

    if(prev == NULL || next == NULL)
    {
        return 0;
    }

    prev_page = list_entry(prev, struct page, list);
    next_page = list_entry(next, struct page, list);

    index = pow(2,order);

    return ((page_address(prev_page) + index * PAGE_SIZE == page_address(next_page)) || (page_address(prev_page) - index * PAGE_SIZE == page_address(next_page)));
}

inline int free_list_del(struct list_head *head, const uint32_t order, struct list_head **item)
{
    if(head == NULL || order >= NR_MEM_LISTS || item == NULL)
    {
        return -1;
    }

    if(list_get_del(head, item) != 0)
    {
        return -2;
    }

    dec_freelist_pagenr(order);
    return 0;
}

inline int free_list_add(struct page *page, const uint32_t prio, const uint32_t order)
{
    if(page == NULL || prio >= MIGRATE_TYPES || order >= NR_MEM_LISTS)
    {
        return -1;
    }

    free_area_t *header = buddy_freearea(order);
    list_add(&(page->list),&(header->lists[prio]));
    inc_freelist_pagenr(order);

    return 0;
}

int free_list_add_line(struct page *page, const uint32_t prio, const uint32_t order)
{
    if(page == NULL || prio >= MIGRATE_TYPES || order >= NR_MEM_LISTS )
    {
        return -1;
    }

    page->order = order;
    free_area_t *header = buddy_freearea(order);
    struct list_head *head, *pos, *n;
    struct page *item_page = NULL;

    head = &(header->lists[prio]);

    if(list_empty_careful(head))
    {
        list_add(&(page->list),&(header->lists[prio]));
        inc_freelist_pagenr(order);
        return 0;
    }
    else
    {
        list_for_each_safe(pos, n, head)
        {
            item_page = list_entry(pos, struct page, list);
            if(page_address(page) < page_address(item_page))
            {
                __list_add(&(page->list), pos->prev, pos);
                inc_freelist_pagenr(order);
                return 0;
            }

            if(pos->next == head)
            {
                __list_add(&(page->list), pos, n);
                inc_freelist_pagenr(order);
                return 0;
            }
        }
    }

    return -1;
}

void buddy_list_init ()
{
    int i,j;
    free_area_t *free_area = NULL;
    struct list_head *head = NULL;

    for(i = 0;i < NR_MEM_LISTS; i++)
    {
        
        free_area = buddy_list.free_area + i;

        for(j = 0; j < MIGRATE_TYPES; ++j)
        {
            head = &(free_area->lists[j]);
            INIT_LIST_HEAD(head);
            zero_freelist_pagenr(i);
        }
    }
}

/*tidy the buddy list*/
int buddy_list_tidy(const addr_t start_page_mem, const uint32_t fns)
{
    int i ;
    free_area_t *free_list = NULL;
    struct page *page = NULL;
    struct list_head *head, *pos ,*n;

    buddy_list.start = start_page_mem;
    buddy_list.end = start_page_mem + fns * PAGE_SIZE;

    for(i = 0;i < NR_MEM_LISTS - 1; i++)
    {
        free_list = buddy_freearea(i);

        if(free_list->nr_free_pages <= 0)
        {
            return 0;
        }

        head = &(free_list->lists[MIGRATE_MOVABLE]);
        pos = head->next;

        while(pos && pos != head)
        {
            n = pos->next;
            if(pos->next != head)
            {
                if(is_pageadj(pos,pos->next, i))
                {
                    n = pos->next->next;
                    list_del(pos->next);
                    list_del(pos);
                    free_list->nr_free_pages -= 2;
                    page = list_entry(pos, struct page, list);
                    ++(page->order);
                    /*DD("page->order = %d. order = %d", page->order, i+1);*/
                    list_add(&(page->list),&((free_list+1)->lists[MIGRATE_MOVABLE]));
                    ++(free_list+1)->nr_free_pages;
                }
                else
                {
                }
            }
            pos = n;
        }
    }

    return 0;
}

int print_buddy_list()
{
    int i ,j;
    free_area_t *free_list = NULL;

    for(i = 0;i < NR_MEM_LISTS; i++)
    {
        free_list = buddy_freearea(i); 
        for(j = 0; j < MIGRATE_TYPES; ++j)
        {
            /*DD("[list = %d type = %d  %d]", i,j, free_list->nr_free_pages);*/
        }
    }

    return 0;
}
