#ifndef     _BIO_H_
#define     _BIO_H_

#include "list.h"

#define     BIO_INLINE_VECS     8 

struct bio_vec 
{
    struct page *bv_page;
    unsigned int bv_len;
    unsigned int bv_offset;
};

typedef struct bio
{
    //struct list_head next;
    struct bio *bi_next;   /* list in the same request */

    unsigned int   bi_size;     /* I/O count */
    unsigned long  bi_flags;    /* status, command */
    unsigned long  bi_rw;       /* READ/WRITE */
    unsigned short bi_vcnt;     /* how many bio vec's */

    unsigned int bi_front_nr;     /* the first page nr*/
    unsigned int bi_back_nr;      /* the last page nr*/

    unsigned int bi_max_vecs;   /* max vec can hold */

    unsigned short bi_idex;     /* current index of vec */
    struct bio_vec *bi_vec;
    void *bi_private;

}bio_t;

enum
{
    BIO_READ = 0,
    BIO_WRITE = 1,
};

enum
{
    O_SYNC = (1 << 0),
    O_ASYNC = (1 << 1),
    O_PRIVATE = ( 1<< 2), 
    O_SHARED = ( 1 << 3), 
    O_LOCK = (1<<4),
    O_UNLOCK = (1<<5),
};

#define     BIO_sync(flags)    (flags & O_SYNC)
#define     BIO_async(flags)    (flags & O_ASYNC)
#define     BIO_private(flags)    (flags & O_PRIVATE)
#define     BIO_shared(flags)    (flags & O_SHARED)
#define     BIO_lock(flags)    (flags & O_LOCK)
#define     BIO_unlock(flags)    (flags & O_UNLOCK)

extern void    kmem_bio_init();
extern bio_t*  make_bio();
extern struct bio * _do_readpage_aio(struct page *page);
extern int bio_add_page(bio_t *bio, struct page *page, unsigned int rw);
extern void chk_bio_flags(unsigned long *flags);

#endif
