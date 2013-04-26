#ifndef     _SLAB_H_
#define     _SLAB_H_

#include "page.h"

#define     BYTES_PER_WORD      sizeof(void *)
#define     DEFAULT_SLAB_PAGES   2
#define     MAX_SLAB_PAGES       8

typedef unsigned int kmem_bufctl_t;

#define     BUFCTL_END      (((kmem_bufctl_t)(~0U))-0)
#define     BUFCTL_FREE     (((kmem_bufctl_t)(~0U))-1)
#define     SLAB_LIMIT      (((kmem_bufctl_t)(~0U))-2)


struct slab
{
    struct list_head list; 
    //unsigned int obj;   [> object in slab <] 
    void * s_mem;   /* the first object of slab */
    unsigned int free;  /* the next free object of slab */
    unsigned int inuse;  /* number of objs inused in slab */
};

struct kmem_list
{
    struct list_head partial;   /* partial used */
    struct list_head full;      /* full used */
    struct list_head free;      /* free */
};

typedef struct kmem_cache kmem_cache_t_s;

struct kmem_cache_operation
{
    void* (*get_obj)(kmem_cache_t_s *);
    int (*free_obj)(void *obj);
};

typedef struct kmem_cache
{
    unsigned int flags;         /* flags */
    unsigned int nr_frees;      /* numbers of free */
    unsigned long free_limit;   /* free number limit */
    unsigned int nr_pages;

    int obj_size;               /* each slab object size */
    int obj_num;                /* each slab object number */
    unsigned long obj_offset;   /* object offset */

    char name[16];              /* name */
    struct list_head next;      /* next */
    struct kmem_list kmem_lists;     /* kmem lists */

    struct kmem_cache_operation *kmem_ops;
}kmem_cache_t;

struct kmem_cache_list
{
    char *name;
    struct list_head next, list;
};

/* mm node */
#define     MAX_NUMNODES 1
#define     NUM_INIT_LISTS  (3*MAX_NUMNODES)

extern struct list_head cache_chain;

extern void kmem_cache_init();
extern kmem_cache_t * kmem_cache_create(char *name, size_t obj_size, \
        unsigned long flags);
extern int     kmem_cache_destroy(kmem_cache_t *cachep);
extern int     print_kmem_info(kmem_cache_t *cachep);
extern void    print_kmem_cache_chain();
extern int     print_slab_info(struct slab *slabp);

#endif
