#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "mapping.h"
#include "page.h"
#include "slab.h"
#include "pte.h"

/* lookup a page */
static inline void * mapping_lookup(struct address_mapping *mapping, const unsigned long index)
{
    if(mapping == NULL)
    {
        return NULL;
    }

    return (struct page *)radix_tree_lookup(&(mapping->radix_root), index);
}

/* insert a page */
static inline int mapping_insert(struct address_mapping *mapping, const unsigned long index, void* page)
{
    if(mapping == NULL || page == NULL)
    {
        return -1;
    }

    return radix_tree_insert(&(mapping->radix_root), index, page);
}

/* remove a page */
static inline void* mapping_remove(struct address_mapping *mapping, const unsigned long index)
{
    if(mapping == NULL)
    {
        return NULL;
    }

    return radix_tree_delete(&(mapping->radix_root),index);
};

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
    if((page = mapping_lookup(mapping,index)) == NULL)
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

    struct page *page = mapping_lookup(mapping,index);
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
    .lookuppage = mapping_lookup,
    .insertpage = mapping_insert,
    .removepage = mapping_remove,
};

/* mapping alloc */
struct address_mapping* mapping_alloc()
{
    struct address_mapping *mapping = (struct address_mapping *)malloc(sizeof(struct address_mapping));
    mapping->a_ops = &mops;

    /* radix tree page mapping */
    radix_tree_init();

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
