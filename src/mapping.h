#ifndef     _MAPPING_H_
#define     _MAPPING_H_

#include "list.h"
#include "slab.h"
#include "zone.h"
#include "radix-tree.h"

typedef struct address_mapping address_mapping_s;

struct address_mapping_operation 
{
    int (*readpage)(address_mapping_s*, const unsigned long, const unsigned long);
    int (*writepage)(address_mapping_s*, const unsigned long);
    void *(*lookup_page)(address_mapping_s*, const unsigned long);
    int (*lookup_pages)(address_mapping_s *, void **, unsigned long, unsigned int);
    int (*lookup_pages_tags)(address_mapping_s *, void **, unsigned long, unsigned int, unsigned int);
    int (*insertpage)(address_mapping_s*, const unsigned long, void *);
    void *(*removepage)(address_mapping_s*, const unsigned long);
};

/* max shrink page number */
#define     SHRINK_NR_PAGES     128

typedef struct rw_lock
{
    volatile unsigned int slock;
} rw_lock_t;

struct address_mapping 
{
    struct vzone *zone;
    struct radix_tree_root radix_root;
    kmem_cache_t *rtn_cachep;
    struct address_mapping_operation *a_ops;
    unsigned int nr_pages;
    //rw_lock_t a_lock;
};

extern struct address_mapping* mapping_alloc();

#endif
