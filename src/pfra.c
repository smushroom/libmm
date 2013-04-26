/* 
 * page frame reclaiming algorithm, PFRA
 * */

#include <stdio.h>
#include "list.h"
#include "pte.h"
#include "zone.h"

int init_pfra_page(struct vzone *zone, pte_t *pte, struct page *page, unsigned long flags)
{
    if(zone == NULL || pte == NULL || page == NULL)
    {
        return -1;
    }

    INIT_LIST_HEAD(&page->zone_list);
    list_add(&zone->pfra_list, &page->zone_list);
    page->pgd_index = pgd_index(&zone->pgd, pte);

    page->_mapping = (BIO_private(flags)?0:1);
    page_sflags(page, (BIO_private(flags)?PAGE_PRIVATE:PAGE_SHARED));

    return 0;
}
