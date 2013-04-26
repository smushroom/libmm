/*
 * request queue
 */
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "slab.h"
#include "rq.h"

static kmem_cache_t *req_cachep = NULL;

static void kmem_request_init()
{
    req_cachep = kmem_cache_create("request_cachep",sizeof(struct request), 0 );
}

void init_rq(struct request_queue *rq, const unsigned int type)
{
    if(rq == NULL)
    {
        return;
    }

    memset((void *)rq, 0, sizeof(*rq));
    INIT_LIST_HEAD(&rq->queue_head);
    rq->swp_type = type;
    kmem_request_init();
}

struct request *request_alloc()
{
    return req_cachep->kmem_ops->get_obj(req_cachep);
}

/* init request */
int request_init(struct request_queue *rq, struct request *req,struct bio *bio)
{
    if(rq == NULL || req == NULL || bio == NULL)
    {
        return -1;
    }

    if(req->bio_tail == NULL)
    {
        req->bio_tail = bio;
    }
    else
    {
        req->bio_tail->bi_next = bio;
        req->bio_tail = bio;
    }

    req->start_page_nr = bio->bi_front_nr;
    req->nr_pages = bio->bi_back_nr - bio->bi_front_nr + 1;
    req->data_len += bio->bi_size;
    req->bio = bio;
    req->q = rq;
    req->next = NULL;

    return 0;
}

/* get a request from request queue */
struct request * rq_get_fn(struct request_queue *rq)
{
    struct request *req = NULL;
    struct list_head *item = NULL;

    if(list_get_del(&rq->queue_head, &item) != 0)
    {
        return NULL;
    }

    req = list_entry(item, struct request, queuelist);

    return req;
}

/* from requet_queue find the best request  */
int rq_merge_fn(struct request_queue *rq, struct request **req, struct bio *bio)
{

    if(rq == NULL || req == NULL || bio == NULL)
    {
        return -1;
    }

    struct request *r = NULL;
    struct list_head *head, *pos , *n;

    head = &rq->queue_head; 
    list_for_each_safe(pos, n, head)
    {
        r = list_entry(pos, struct request, queuelist);
        if(r->start_page_nr < bio->bi_front_nr)
        {
            continue;
        }
        else if(r->start_page_nr  == bio->bi_back_nr)
        {
            r->nr_pages += (bio->bi_back_nr - bio->bi_front_nr + 1);
            *req = r;
            return NO_MERGE;
        }
        else if((r->start_page_nr + r->nr_pages) == bio->bi_front_nr)
        {
            r->start_page_nr = bio->bi_front_nr;
            r->nr_pages += (bio->bi_back_nr - bio->bi_front_nr + 1);
        }
    }

    r = request_alloc();
    if(!r)
    {
        return -2;
    }

    request_init(rq, r, bio);
    *req = r;

    return BACK_MERGE;
}

int print_request(const struct request *req)
{
    if(req == NULL)
    {
        return -1;
    }

    printf("--------------------------------------------\n");
    printf("start_page_nr = %d.\n", req->start_page_nr);
    printf("nr_pages = %d.\n", req->nr_pages);
    printf("datalen = %d.\n", req->data_len);
    printf("--------------------------------------------\n");

    return 0;
}

int print_request_queue(struct request_queue *rq)
{
    DD("print_request_queue .");
    if(rq == NULL)
    {
        return -1;
    }

    struct request *req;

    while((req = rq_get_fn(rq)))
    {
        print_request(req);
    }

    return 0;
}

int merge_request(struct request_queue *rq, struct bio *bio)
{
    DD("merge_request.");
    int ret;
    struct request *req;
    ret = rq_merge_fn(rq, &req, bio);

    switch(ret)
    {
        case NO_MERGE:
            break;
        case FRONT_MERGE:
            list_add(&(req->queuelist),&(rq->queue_head));
            break;
        case BACK_MERGE:
            list_add_tail(&(req->queuelist),&(rq->queue_head));
            break;
        default:
            break;
    }

    print_request_queue(rq);

    return 0;
}
