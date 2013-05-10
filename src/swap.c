/* 
 * swap
 * */
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "swap.h"
#include "rq.h"
#include "uio.h"
#include "bio.h"
#include "swapfile.h"

static swp_info_t swp_infos[MAX_SWPFILES];
static swp_list_t swp_list;

static int cur_swp_file = 0;

inline swp_info_t * _get_swp_info(unsigned int type)
{
    return swp_infos + type;
}

inline swp_info_t *get_swp_info(struct page *page)
{
    swp_entry_t entry;
    entry.val = page->virtual;

    unsigned int type = swp_type(&entry); 
    return _get_swp_info(type);
}

static int swp_info_init(swp_info_t *swp_info)
{
    if(swp_info == NULL)
    {
        return -1;
    }

    swp_info->flags = SWP_ACTIVE;
    swp_info->swp_file = NULL;
    swp_info->pages =  DEFAULT_SWP_MAP_MAX;
    swp_info->swp_map = (unsigned short *)malloc(swp_info->pages * sizeof(unsigned short));;
    memset(swp_info->swp_map, 0, sizeof(unsigned short) * swp_info->pages);
    swp_info->slot_next = 1;
    swp_info->max = swp_info->pages + DEFAULT_SWP_RESERVE;
    swp_info->inuse_pages = 0;
    swp_info->swp_ops = &swpfile_ops;

    init_rq(&swp_info->r_rq, swp_info->type);
    init_rq(&swp_info->w_rq, swp_info->type);

    return 0;
}

/* lookup the free slot */
static int lookup_freeslot(swp_entry_t *entry)
{
    if(entry == NULL)
    {
        return -1;
    }

    int i;
    int pos = swp_list.head;
    swp_info_t *swp_info = NULL;

    while(pos >= 0 || pos < MAX_SWPFILES)
    {
        swp_info = swp_infos + pos;
        for(i = swp_info->slot_next; i < swp_info->pages; ++i)
        {
            DD("swp_map[%d] = %d.", i, swp_info->swp_map[i]);
            if(swp_info->swp_map[i] == 0)
            {
                ++(swp_info->swp_map[i]);
                DD("offset = %d type = %d", i << PAGE_SHIFT, pos);
                swp_entry(i << PAGE_SHIFT, 0,pos, entry);
                return 0;
            }
        }
        pos = swp_info->next; 
    }

    return -2;
}

int do_swp_readpage(swp_entry_t entry, struct page *page)
{
    if(page == NULL)
    {
        return -1;
    }

    swp_info_t *swp_info = NULL;
    unsigned int type;
    type = swp_type(&entry);

    swp_info = _get_swp_info(type); 
    if(swp_info && swp_info->swp_ops)
    {
        return swp_info->swp_ops->swp_readpage(swp_info,&entry, page);
    }

    return -2;
}

int do_swp_readpages(uio_t *uio, unsigned int flags)
{
    int i = 0;
    unsigned long index;
    struct page *page;
    swp_entry_t entry;

    bio_t *bio = make_bio(); 
    for(i = 0;i < uio->u_size; ++i)
    {
        page = (struct page *)uio->u_base;
        bio_add_page(bio, page, BIO_READ);
    }

    return 0;
}

int do_swp_writepage(struct page *page, swp_entry_t *entry)
{
    if(page == NULL || entry == NULL)
    {
        return -1;
    }

    int ret;
    unsigned int type;
    swp_info_t *swp_info = NULL;

    lookup_freeslot(entry);
    type = swp_type(entry);

    swp_info = _get_swp_info(type); 
    if(swp_info && swp_info->swp_ops)
    {
        if((ret = swp_info->swp_ops->swp_writepage(swp_info,entry, page)) < 0)
        {
            DD("swp writepage error. ret = %d.", ret);
            return -2;
        }
    }

    return 0; 
}

/* free a slot */
int do_swp_freepage(swp_entry_t entry)
{
    unsigned int type;
    swp_info_t *swp_info = NULL;

    type = swp_type(&entry);
    swp_info = _get_swp_info(type); 
    if(swp_info && swp_info->swp_ops)
    {
        return swp_info->swp_ops->swp_freepage(swp_info,&entry);
    }

    return -1;
}

int do_swpread_aio(struct request_queue *rq, struct bio *bio)
{
    DD("do_swpread_aio.");
    swp_info_t *swp_info = _get_swp_info(rq->swp_type);
    return swp_info->swp_ops->swp_read_aio(rq, bio);
}

int swp_on()
{
    uint32_t i;

    swp_list.head = 0;
    swp_list.next = 1;

    cur_swp_file = swp_list.head;
    swp_info_t *swp_info = NULL;

    for(i = 0;i < MAX_SWPFILES; ++i)
    {
        if(i == 5)
        {
            DD("i = %d", i);
        }
        swp_info = swp_infos + i;
        swp_info->prio = i;
        swp_info->type = i;
        swp_info_init(swp_info);
        swp_info->next = i + 1;
    }

    kmem_request_init();

    kmem_bio_init();

    return 0;
}

/* turn off the swp */
int swp_off()
{
    swp_list.head = -1;
    swp_list.next = -1;

    return 0;
}
