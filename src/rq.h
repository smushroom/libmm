/* 
 * request queue 
 * */

#ifndef     _REQUEST_QUEUE_H_
#define     _REQUEST_QUEUE_H_

#include "bio.h"

typedef struct request_queue
{
    struct list_head    queue_head; /* each item is request */
    struct request      *boundary_rq;
    unsigned int        nr_requests;
    unsigned long       queue_flags;
    unsigned int        swp_type;   /* swap device  */
}rq_t;

typedef struct request
{
    struct list_head    queuelist;  /* link to request queue */
    struct list_head    donelist;   /* done list */
    struct request_queue *q;
    struct bio *bio;        /* the current bio of the request lists */
    struct bio *bio_tail;   /* the last bio of the request lists */
    struct request *next;
    
    unsigned int data_len;

    unsigned int start_page_nr;
    unsigned int nr_pages;

}req_t;

enum 
{
    NO_MERGE = 0,
    FRONT_MERGE = 1,
    BACK_MERGE = 2,
};

extern void init_rq(struct request_queue *rq, const unsigned int type);
extern int merge_request(struct request_queue *rq, struct bio *bio);
extern struct request * rq_get_fn(struct request_queue *rq);

#endif
