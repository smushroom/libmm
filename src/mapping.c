#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "page.h"
#include "slab.h"
#include "pte.h"
#include "mapping.h"

/* lookup a page */
static inline void * mapping_lookup_item(struct address_mapping *mapping, const unsigned long index)
{
    if(mapping == NULL)
    {
        return NULL;
    }

    return (struct page *)radix_tree_lookup(&(mapping->radix_root), index);
}

/* look up max_items pages from first_index */
static inline int mapping_lookup(struct address_mapping *mapping, void **results, unsigned long first_index, unsigned int max_items)
{
    if(mapping == NULL || results == NULL)
    {
        return -1;
    }

    return radix_tree_gang_lookup(&mapping->radix_root, results, first_index, max_items);
}

/* look up max_items pages from first_index with tags*/
static inline int mapping_lookup_tag(struct address_mapping *mapping, void **results, unsigned long first_index, unsigned int max_items,unsigned int tag)
{
    if(mapping == NULL || results == NULL)
    {
        return -1;
    }

    return radix_tree_gang_lookup_tag(&mapping->radix_root, results, first_index, max_items,tag);
}

/* insert a page */
static inline int mapping_insert(struct address_mapping *mapping, const unsigned long index, void* page)
{
    if(mapping == NULL || page == NULL)
    {
        return -1;
    }

    if(radix_tree_insert(mapping->rtn_cachep, &(mapping->radix_root), index, page) != 0)
    {
        DD("radix_tree_insert error.");
        return -2;
    }

    ++(mapping->nr_pages);
    return 0;
}

static inline int mapping_insert_tag(struct address_mapping *mapping, unsigned long index, void *page, unsigned int tag)
{
    if(mapping == NULL || page == NULL)
    {
        return -1;
    }

    radix_tree_root_tag(mapping, index, tag);
    if(radix_tree_insert(mapping->rtn_cachep, &(mapping->radix_root), index, page) != 0)
    {
        DD("radix_tree_insert tag error.");
        return -2;
    }

    ++(mapping->nr_pages);
    return 0;
}

/* remove a page */
static inline void* mapping_remove(struct address_mapping *mapping, const unsigned long index)
{
    if(mapping == NULL)
    {
        return NULL;
    }

    void *ptr = NULL;
    if((ptr = radix_tree_delete(mapping->rtn_cachep, &(mapping->radix_root),index)) == NULL)
    {
        DD("radix_tree_delete error.");
        return NULL;
    }

    --(mapping->nr_pages);
    return ptr;
}

/* read a page */
int mapping_readpage(struct address_mapping *mapping, const unsigned long index, const unsigned long flags)
{
    if(mapping == NULL)
    {
        return -1;
    }

    /*int ret;*/
    struct page *page = NULL;

    /* if page is in memory */
    if((page = mapping_lookup_item(mapping,index)) == NULL)
    {
        DD("page is not in mapping .");
        return -2;
    }

    return 0;
}

/* write a page */
int mapping_writepage(struct address_mapping *mapping, const unsigned long index)
{
    if(mapping == NULL)
    {
        return -1;
    }

    struct page *page = mapping_lookup_item(mapping,index);
    if(!page)
    {
        return -2;
    }

    return 0;
}

/* address_mapping operations  */
struct address_mapping_operation mops = 
{
    .readpage = mapping_readpage,
    .writepage = mapping_writepage,
    .lookup_page = mapping_lookup_item,
    .lookup_pages = mapping_lookup,
    .lookup_pages_tags = mapping_lookup_tag,
    .insertpage = mapping_insert,
    .removepage = mapping_remove,
};

static int mapping_init(struct address_mapping *mapping)
{
    radix_tree_init(&mapping->rtn_cachep);
}

/* mapping alloc */
struct address_mapping* mapping_alloc()
{
    struct address_mapping *mapping = (struct address_mapping *)malloc(sizeof(struct address_mapping));
    if(mapping == NULL)
    {
        DD("malloc mapping error.");
        return NULL;
    }

    mapping->a_ops = &mops;

    mapping_init(mapping);
    /* radix tree page mapping */

    return mapping;
}

/* mapping free */
void mapping_free(struct address_mapping *mapping)
{
    if(mapping)
    {
        free(mapping);
    }
}
