#include <stdio.h>
#include <pthread.h>
#include "debug.h"
#include "lthread.h"
#include "zone.h"

static pthread_t kswap_tid;
static pthread_t kreclaim_tid;

static pthread_t lthread_create(lthread_fn fn)
{
    int ret;
    pthread_t tid;

    ret = pthread_create(&tid, NULL,fn, NULL);

    DD("tid = %ld.",tid);

    return tid;
}

int lthread_init()
{
    /* kswap thread */
    kswap_tid = lthread_create(kswp_thread_fn);

    /* kreclaim thread */
    kreclaim_tid = lthread_create(kreclaim_thread_fn);
}
