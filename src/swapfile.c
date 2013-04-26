/* 
 * swapfile functions
 * */

#include <errno.h>
#include <string.h>
#include "debug.h"
#include "swap.h"
#include "rq.h"

int aio_readpage(swp_info_t *swp_info, struct request *req)
{
    unsigned int nr_pages = req->nr_pages;
    unsigned int start_page_nr = req->start_page_nr;

    FILE *fp = swp_info->swp_file;

    if(fread((void *)index_to_pfn(start_page_nr), PAGE_SIZE, req->nr_pages, fp) < nr_pages)
    {
        DD("aio_reapage error. : %s.", strerror(errno));
        return -2;
    }

    return 0;
}

int aio_writepage(swp_info_t *swp_info, struct request *rq)
{
    return 0;
}

/* read from page */
static int generic_readpage(const swp_info_t *swp_info, const swp_entry_t *entry, struct page *page)
{
    if(swp_info == NULL || entry == NULL || page == NULL)
    {
        return -1;
    }

    int ret;
    unsigned long offset;
    FILE *fp = NULL;
    int size = PAGE_SIZE;

    offset = swp_offset(entry);

    fp = swp_info->swp_file;
    if(fp == NULL)
    {
        DD("swap file is not existed!");
        return -3;
    }

    fseek(fp, offset, SEEK_SET);
    DD("read page from swapfile.");

    DD("offset = %ld.", offset);

    if((ret = fread((void *)page_address(page), 1, size, fp)) < size)
    {
        DD("read swap error. %s.", strerror(errno));
        return -4;
    }

    return 0;
}

/* write to page */
static int generic_writepage(swp_info_t *swp_info, const swp_entry_t *entry, struct page *page)
{
    if(swp_info == NULL || entry == NULL || page == NULL)
    {
        return -1;
    }

    int ret;
    unsigned long offset;
    unsigned int type;
    int size = PAGE_SIZE;
    FILE *fp = NULL;

    offset = swp_offset(entry);
    type = swp_type(entry);

    fp = swp_info->swp_file;
    if(fp == NULL)
    {
        DD("swap file is not opend! type = %d.", type);
        char filename[32] = {0};
        snprintf(filename, sizeof(filename), "/data/swapfile_%d.swp", type);
        if((fp = fopen(filename, "w+")) == NULL)
        {
            DD("fopen error : %s.", strerror(errno));
            return -3;
        }

        swp_info->swp_file = fp;
    }

    fseek(fp, offset, SEEK_SET);
    DD("write page to swapfile.");
    /*DD("address = %s", page_address(page));*/
    if((ret = fwrite((void *)page_address(page), 1, size , fp)) < size )
    {
        DD("write swap error. %s.", strerror(errno));
        return -4;
    }

    return 0;
}

int generic_freepage(swp_info_t *swp_info, swp_entry_t *entry)
{
    unsigned long offset = swp_offset(entry); 

    --(swp_info->swp_map[(offset >> PAGE_SHIFT)]);

    return 0;
}

static int swpfile_readpage(const swp_info_t *swp_info, const swp_entry_t *entry , struct page *page)
{
    return generic_readpage(swp_info, entry, page);
}

static int swpfile_writepage(swp_info_t *swp_info, const swp_entry_t *entry, struct page *page)
{
    return generic_writepage(swp_info, entry, page);
}

static int swpfile_freepage(swp_info_t *swp_info, swp_entry_t *entry)
{
    return generic_freepage(swp_info, entry);
}

static int swpfile_read_aio(struct request_queue *rq, struct bio *bio)
{
    DD("swpfile_read_aio .");
    return merge_request(rq, bio);
}

static int swpfile_write_aio(struct request_queue *rq, struct bio *bio)
{
}

struct swp_info_operations swpfile_ops = 
{
    .swp_readpage = swpfile_readpage,
    .swp_writepage = swpfile_writepage, 
    .swp_read_aio = swpfile_read_aio,
    .swp_write_aio = swpfile_write_aio,
    .swp_freepage = swpfile_freepage,
};
