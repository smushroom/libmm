#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "debug.h"
#include "const.h"
#include "page.h"
#include "mapping.h"
#include "zone.h"
#include "slab.h"

static struct list_head kmem_cache_chain;

static inline uint32_t * slab_bufctl(struct slab *slabp)
{
    return (unsigned int *)(slabp + 1);
}

static inline uint32_t obj_to_index(kmem_cache_t *cachep, struct slab *slabp, void *objp)
{
    return ((objp - slabp->s_mem)/cachep->obj_size);
}

static inline void * index_to_obj(kmem_cache_t *cachep, struct slab *slabp, unsigned int free)
{
    return (slabp->s_mem + cachep->obj_size * free);
}

static inline void inc_cache_free(kmem_cache_t *cachep)
{
    ++(cachep->nr_frees);
}

static inline void dec_cache_free(kmem_cache_t *cachep)
{
    --(cachep->nr_frees);
}

static inline void inc_slab_use(struct slab *slabp)
{
    ++(slabp->inuse);
}

static inline void dec_slab_use(struct slab *slabp)
{
    --(slabp->inuse);
}

static inline void set_slab_free(struct slab *slabp, uint32_t next)
{
    slabp->free = next;
}

static void * slab_get_obj(kmem_cache_t *cachep, struct slab *slabp)
{
    if(cachep == NULL || slabp == NULL)
    {
        return NULL;
    }

    struct list_head *item = NULL;

    if(slabp->free == BUFCTL_END)
    {
        DD("not free obj.");
        return NULL;
    }
    void *objp = index_to_obj(cachep, slabp, slabp->free);
    uint32_t next = slab_bufctl(slabp)[slabp->free];
    inc_slab_use(slabp);
    set_slab_free(slabp, next);

    /* move to the full list*/
    if(slabp->inuse == cachep->obj_num)
    {
        item = &(slabp->list);
        list_del(item);
        list_add(item, &(cachep->kmem_lists.full));
    }

    dec_cache_free(cachep);

    return objp;
}

static int slab_free_obj(kmem_cache_t *cachep, struct slab *slabp, void *objp)
{
    if(cachep == NULL || slabp == NULL || objp == NULL)
    {
        return -1;
    }

    struct list_head *item = NULL;

    unsigned int objnr = obj_to_index(cachep, slabp, objp);
    if(objnr > cachep->obj_num)
    {
        return -2;
    }

    slab_bufctl(slabp)[objnr] = slabp->free;
    set_slab_free(slabp, objnr);
    dec_slab_use(slabp);

    /* move to the free list */
    if(slabp->inuse == 0)
    {
        item = &(slabp->list);
        list_del(item);
        list_add(item, &(cachep->kmem_lists.free));
    }

    inc_cache_free(cachep);

    return 0;
}

/* alloc slab pages default for 4 pages */
static inline void * slab_alloc(kmem_cache_t *cachep)
{
    if(cachep == NULL)
    {
        return NULL;
    }

    int i = 0;
    addr_t start_addr = 0, address = 0;
    struct page *page = NULL;

    start_addr = page_address(get_free_pages(MIGRATE_RESERVE, DEFAULT_SLAB_PAGES));
    address = start_addr;

    for(i = 0;i < DEFAULT_SLAB_PAGES;++i)
    {
        page = pfn_to_page(start_addr); 
        page->slab_list.next = (void *)cachep;
        start_addr += PAGE_SIZE;
    }

    return (void *)address;
}

/* estimate the slab parameters */
static inline int slab_estimate(const int obj_size, int *obj_num)
{
    *obj_num = ((pow(2,DEFAULT_SLAB_PAGES) * PAGE_SIZE) - sizeof(struct slab))/obj_size;

    return 0;
}

/* init slab 
 * struct slab + obj_num * (unsigned int)*/
static int slab_init(kmem_cache_t *cachep, struct slab *slabp, void *args)
{
    if(cachep == NULL || slabp == NULL || args == NULL)
    {
        return -1;
    }

    int i = 0;
    void *ptr = args;

    slabp->free = 0;
    slabp->inuse = 0;
    INIT_LIST_HEAD(&(slabp->list));

    ptr += (cachep->obj_num + 1) * sizeof(unsigned int);

    slabp->s_mem = ptr;
    for(i = 0;i < cachep->obj_num; ++i)
    {
        slab_bufctl(slabp)[i] = i + 1;
    }
    slab_bufctl(slabp)[i - 1] =  BUFCTL_END;

    return 0;
}

static inline int slab_free(struct list_head *head)
{
    if(head == NULL)
    {
        return -1;
    }

    struct slab *slabp = list_entry(head, struct slab, list);
    addr_t address = (addr_t)slabp;
    _free_pages(address, DEFAULT_SLAB_PAGES);

    return 0;
}

static struct slab * kmem_get_slab(kmem_cache_t *cachep)
{
    if(cachep == NULL)
    {
        return NULL;
    }

    struct slab *slabp = NULL;
    struct kmem_list *lists = NULL;
    struct list_head *head = NULL, *item = NULL, *pos = NULL;

    lists = &(cachep->kmem_lists);

    /* first find the partial slab*/
    head = &(lists->partial);
    if(list_empty_careful(head))
    {
        head = &(lists->free);
        if(list_empty_careful(head))
        {
            return NULL;
        }
        else
        {
            if(list_get_del(head, &item) != 0)
            {
                return NULL;
            }
            list_add(item, &(cachep->kmem_lists.partial));
        }
    }

    head = &(lists->partial);
    if(list_get(head, &pos) != 0)
    {
        return NULL;
    }

    slabp = list_entry(pos, struct slab, list);
    return slabp;
}

/* add a slab to kmem_cache_t */
static int kmem_add_slab(kmem_cache_t *cachep, struct slab *slabp)
{
    if(cachep == NULL || slabp == NULL)
    {
        return -1;
    }

    cachep->nr_frees += cachep->obj_num;
    list_add_tail(&(slabp->list), &(cachep->kmem_lists.free));

    return 0;
}

/* create a slab and add to kmem_cache_t */
int cache_grow(kmem_cache_t *cachep)
{
    if(cachep == NULL || cachep->obj_size == 0)
    {
        return -1;
    }

    /*DD("----- cache %s grow -----", cachep->name);*/
    void *ptr = NULL;
    struct slab *slabp = NULL;
    int obj_num = 0;

    ptr = slab_alloc(cachep);

    slabp = (struct slab *)ptr;
    slab_estimate(cachep->obj_size, &obj_num);
    cachep->obj_num = obj_num;
    cachep->nr_pages += pow(2, DEFAULT_SLAB_PAGES);

    ptr += sizeof(struct slab);
    slab_init(cachep, slabp, ptr);

    kmem_add_slab(cachep, slabp);

    return 0;
}


void * kmem_get_obj(kmem_cache_t *cachep)
{
    struct slab *slabp = NULL;
    void *obj = NULL;

repeat:
    slabp = kmem_get_slab(cachep);
    obj = slab_get_obj(cachep, slabp);
    if(obj)
    {
        return obj;
    }

    /* get obj is NULL but this kmem_cache_t has free objs */
    else
    {
        if(obj == NULL && cachep->nr_frees > 0)
        {
            return NULL;
        }
        else
        {
            if(cachep->nr_pages >= pow(2,MAX_SLAB_PAGES))
            {
                return NULL;
            }
            else
            {
                cache_grow(cachep);
                goto repeat;
            }
        }
    }

    return obj;
}

static int _kmem_free_obj(kmem_cache_t *cachep, void *objp)
{
    if(cachep == NULL || objp == NULL)
    {
        return -1;
    }

    unsigned long address = ((unsigned long )objp & VALUE_MASK(DEFAULT_SLAB_PAGES * PAGE_SIZE));
    struct slab *slabp = (struct slab *)address;

    return slab_free_obj(cachep,slabp,objp);
}

int kmem_free_obj(void *objp)
{
    uint64_t page_addr = (uint64_t)objp & PAGE_MASK;
    struct page *page = pfn_to_page(page_addr);

    kmem_cache_t *cachep = (kmem_cache_t *)page->slab_list.next;

    return _kmem_free_obj(cachep, objp);
}

/* init kmem_list  */
static void kmem_list_init(struct kmem_list *parent)
{
    INIT_LIST_HEAD(&(parent->partial));
    INIT_LIST_HEAD(&(parent->full));
    INIT_LIST_HEAD(&(parent->free));
}

/* the slab system start  */
void kmem_cache_init()
{
    INIT_LIST_HEAD(&kmem_cache_chain);
}

/* cache list 
 * list --- list --- list  
 *  |       |
 *  cachep  cachep
 * */

/* init kmem cache list */
static struct list_head *init_kmem_cache_list(kmem_cache_t *cachep)
{
    if(cachep == NULL)
    {
        return NULL;
    }

    struct kmem_cache_list *cache_listp = (struct kmem_cache_list *)malloc(sizeof(struct kmem_cache_list));
    if(cache_listp == NULL)
    {
        return NULL;
    }

    cache_listp->name = cachep->name;
    INIT_LIST_HEAD(&(cache_listp->next));
    list_add(&(cachep->next),&(cache_listp->next));

    return &(cache_listp->list);
}

/* find the cachep in the cache list */
static int find_kmem_cache_list(kmem_cache_t *cachep)
{
    if(cachep == NULL)
    {
        return -1;
    }

    struct list_head *head = NULL, *pos, *n;
    struct kmem_cache_list *item_listp = NULL;
    head = &(kmem_cache_chain);
    if(list_empty_careful(head))
    {
        return -2;
    }
    else
    {
        list_for_each_safe(pos, n, head)
        {
            item_listp = list_entry(pos, struct kmem_cache_list, list);
            if(strncmp(cachep->name,item_listp->name, strlen(cachep->name))  == 0)
            {
                return 0;
            }
        }
    }

    return -3;
}

static int kmem_list_add(kmem_cache_t *cachep)
{
    if(cachep == NULL)
    {
        return -1;
    }

    struct list_head *list = NULL;

    if(find_kmem_cache_list(cachep) != 0)
    {
        list = init_kmem_cache_list(cachep);
        list_add(list, &kmem_cache_chain);
    }

    return 0;
}

static int kmem_list_del(kmem_cache_t *cachep)
{
    if(cachep == NULL)
    {
        return -1;
    }

    struct list_head *head, *pos, *n;
    struct kmem_cache_list *item_listp = NULL;
    head = &(kmem_cache_chain);

    if(list_empty_careful(head))
    {
        return -2;
    }
    else
    {
        list_for_each_safe(pos, n, head)
        {
            item_listp = list_entry(pos, struct kmem_cache_list, list);
            if(strncmp(cachep->name,item_listp->name, strlen(cachep->name))  == 0)
            {
                list_del(&(cachep->next));
                if(list_empty_careful(&(item_listp->next)))
                {
                    list_del(&(item_listp->list));
                }

                return 0;
            }
        }
    }

    return -3;
}

/* alloc kmem_cache_t */
static void * kmem_cache_alloc()
{
    /*get a free page from buddy list
     * kmem_cache_t struct must in one page*/
    if(sizeof(kmem_cache_t)> PAGE_SIZE)
    {
        return NULL;
    }

    int order = (sizeof(kmem_cache_t) >>PAGE_SHIFT);
    addr_t address = page_address(get_free_pages(MIGRATE_RESERVE, order));

    return (void *)address;
}

struct kmem_cache_operation kmem_ops = 
{
    .get_obj = kmem_get_obj,
    .free_obj = kmem_free_obj,
};

/* just only create kmem_cache_t 
 * and add to kmem_cache_chain*/
kmem_cache_t * kmem_cache_create(char *name, size_t obj_size, unsigned long flags)
{
    void *ptr;
    kmem_cache_t *cachep = NULL;

    if(name == NULL || obj_size == 0)
    {
        return NULL;
    }

    if((ptr = kmem_cache_alloc()) == NULL)
    {
        return NULL;
    }

    cachep = (kmem_cache_t *)ptr;
    strncpy(cachep->name, name, sizeof(cachep->name));
    cachep->obj_size = obj_size;
    cachep->nr_frees = 0;
    cachep->obj_num = 0;
    cachep->flags = flags;
    cachep->nr_pages = 0;
    cachep->kmem_ops = &kmem_ops;

    kmem_list_init(&(cachep->kmem_lists));
    cache_grow(cachep);
    kmem_list_add(cachep);

    return cachep;
    /*print_kmem_info(cachep);*/
}

/* get cache size */
static inline uint32_t kmem_cache_size(const kmem_cache_t *cachep)
{
    return cachep?cachep->obj_size:0;
}

/* get cache num */
static inline uint32_t kmem_cache_num(const kmem_cache_t *cachep)
{
    return cachep?cachep->obj_num:0;
}

/* get cahce name */
static inline const char* kmem_cache_name(const kmem_cache_t *cachep)
{
    return cachep?cachep->name:NULL;
}

/* get cache frees */
static inline uint32_t kmem_cache_frees(kmem_cache_t *cachep)
{
    return cachep?cachep->nr_frees:0;
}

int kmem_cache_destroy(kmem_cache_t *cachep)
{
    if(cachep == NULL)
    {
        return -1;
    }

    struct list_head *partial, *full, *free, *pos, *n;
    addr_t address = 0;

    kmem_list_del(cachep);

    partial = &(cachep->kmem_lists.partial);
    full = &(cachep->kmem_lists.full);
    free = &(cachep->kmem_lists.free);

    list_for_each_safe(pos, n, partial)
    {
        slab_free(pos);
    }
    list_for_each_safe(pos, n, full)
    {
        slab_free(pos);
    }
    list_for_each_safe(pos, n, free)
    {
        slab_free(pos);
    }

    address = (addr_t)cachep;
    _free_pages(address, sizeof(kmem_cache_t) >> PAGE_SHIFT);

    cachep = NULL;

    return 0;
}

int print_slab_info(struct slab *slabp)
{
    if(slabp == NULL)
    {
        return -1;
    }

    /*DD("s_mem = %lx.", slabp->s_mem);*/
    DD("free = %d.", slabp->free);
    DD("inuse = %d.", slabp->inuse);

    uint32_t tmp = slabp->free;
    while(tmp != BUFCTL_END)
    {
        DD("next = %d.",tmp);
        tmp = slab_bufctl(slabp)[tmp];
    }

    return 0;
}

int print_kmem_info(kmem_cache_t *cachep)
{
    if(cachep == NULL)
    {
        return -1;
    }

    DD("name = %s.", cachep->name);
    DD("obj_size = %d.", cachep->obj_size);
    DD("partial num = %d.",list_num(&(cachep->kmem_lists.partial)));
    DD("full num = %d.",list_num(&(cachep->kmem_lists.full)));
    DD("free num = %d.",list_num(&(cachep->kmem_lists.free)));
    DD("nr_frees = %d.", cachep->nr_frees);

    return 0;
}

void print_kmem_cache_chain()
{
    struct kmem_cache_list *cache_listp = NULL;
    kmem_cache_t *cachep = NULL;
    struct list_head *head, *pos, *n, *next, *pos2, *n2;

    head = &(kmem_cache_chain);

    list_for_each_safe(pos, n, head)
    {
        cache_listp = list_entry(pos, struct kmem_cache_list, list);
        DD("***********************************");

        next = &(cache_listp->next);
        list_for_each_safe(pos2, n2, next)
        {
            cachep = list_entry(pos2, kmem_cache_t, next);
            print_kmem_info(cachep);
        }
    }
}
