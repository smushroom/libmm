#ifndef     _BUDDY_H_
#define     _BUDDY_H_

#include "page.h"

#define NR_MEM_LISTS   12 

typedef struct 
{
    struct list_head lists[MIGRATE_TYPES];
    uint32_t nr_free_pages;
}free_area_t;

typedef struct 
{
    unsigned long start;
    unsigned long end;

    free_area_t free_area[NR_MEM_LISTS];
    unsigned int nr_free_pages;

}buddy_list_t;

extern buddy_list_t buddy_list;

extern void buddy_list_init ();
extern int buddy_list_tidy(const addr_t start_page_mem, const uint32_t fns);
extern int print_buddy_list();
extern int free_list_add(struct page *page, const uint32_t prio, const uint32_t order);
extern int free_list_del(struct list_head *head, const uint32_t order, struct list_head **item);
extern int free_list_add_line(struct page *page, const uint32_t prio, const uint32_t order);

extern inline addr_t buddy_start();
extern inline addr_t buddy_end();
extern inline unsigned int get_free_pages_nr();

#endif
