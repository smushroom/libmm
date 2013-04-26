#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "swap.h"
#include "slab.h"
#include "bio.h"

static kmem_cache_t *bio_cachep = NULL;

void kmem_bio_init()
{
	bio_cachep = kmem_cache_create("bio_struct ",sizeof(bio_t) + BIO_INLINE_VECS * sizeof(struct bio_vec), 0);
}

static int bio_init(bio_t *bio)
{
    int i;
    struct bio_vec *bv = NULL;

    memset(bio, 0, sizeof(bio_t) + BIO_INLINE_VECS * sizeof(struct bio_vec *));

    bio->bi_vcnt = BIO_INLINE_VECS;
    bio->bi_max_vecs = BIO_INLINE_VECS;
    bio->bi_idex = 0;
    bio->bi_vec = (struct bio_vec *)((void *)bio + sizeof(bio_t));
    bio->bi_private = NULL;

    return 0;
}

void chk_bio_flags(unsigned long *flags)
{
    if((*flags) == 0)
    {
        (*flags) |= (O_SYNC | O_PRIVATE);
    }
    if(((*flags) & O_ASYNC == 0) || ((*flags) & O_SYNC == 0))
    {
        (*flags) |= O_SYNC;
    }
    if(((*flags) & O_PRIVATE == 0) || ((*flags) & O_SHARED == 0))
    {
        (*flags) |= O_PRIVATE;
    }
}

bio_t* make_bio()
{
    int i = 0;

    bio_t *bio = NULL;

    bio = bio_cachep->kmem_ops->get_obj(bio_cachep);
    bio_init(bio);

    return bio;
}

int bio_add_page(bio_t *bio, struct page *page, unsigned int rw)
{
    struct bio_vec *bv = bio->bi_vec;
    bv = &bio->bi_vec[bio->bi_idex++];
    bv->bv_page = page;
    bio->bi_idex++;
    bio->bi_size += PAGE_SIZE;
    bio->bi_rw = rw ;
    bio->bi_front_nr = page_to_index(page);
    bio->bi_back_nr = bio->bi_front_nr;

    return 0;
}

struct bio * _do_readpage_aio(struct page *page)
{
    DD("_do_readpage_aio.");

    if(page == NULL)
    {
        return NULL;
    }

    struct bio *bio = make_bio();
    bio_add_page(bio, page, BIO_READ);

    return bio;
}
