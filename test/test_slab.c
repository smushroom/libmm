#include <stdio.h>
#include "debug.h"
#include "page.h"
#include "slab.h"
#include "zone.h"

struct Test
{
    int i;
    char name[12];
    long l;
};

int main(int argc, char **argv)
{
    init_mm();

    kmem_cache_t *cachep_2 = kmem_cache_create("test_2", sizeof(struct Test), 0);
    kmem_cache_t *cachep_3 = kmem_cache_create("test_3", sizeof(struct Test), 0);
    kmem_cache_t *cachep_4 = kmem_cache_create("test_4", sizeof(struct Test), 0);
    kmem_cache_t *cachep_5 = kmem_cache_create("test_5", sizeof(struct Test), 0);

    /*print_kmem_info(cachep);*/
    DD("----------------------------------------");
    /*struct Test *t = (struct Test *)kmem_get_obj(cachep);*/
    int i = 0;
    while(i < 10000)
    {
        struct Test *t = (struct Test *)cachep_2->kmem_ops->get_obj(cachep_2);
        if(!t)
        {
            DD("get object error.");
            return -2;
        }
        ++i;
        cachep_2->kmem_ops->free_obj((void *)t);
        print_kmem_info(cachep_2);
    }
    /*print_kmem_info(cachep);*/
    /*DD("----------------------------------------");*/
    /*t->i = 0;*/
    /*t->l = 10;*/
    /*print_kmem_info(cachep_2);*/

    DD("----------------------------------------");
    print_kmem_cache_chain();

    DD("----------------------------------------");
    DD("----------------------------------------");
    DD("----------------------------------------");
    kmem_cache_destroy(cachep_2);
    kmem_cache_destroy(cachep_3);
    print_kmem_cache_chain();

    return 0;
}
