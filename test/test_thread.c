#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
/*static pthread_rwlock_t rwlock;*/
static pthread_cond_t ready = PTHREAD_COND_INITIALIZER;
static int i = 0;

void * kswap_fn(void *args)
{
    while(1)
    {
        pthread_mutex_trylock(&lock);
        /*++i;*/
        /*pthread_rwlock_rdlock(&rwlock);*/
        /*while(i < 10)*/
            pthread_cond_wait(&ready, &lock);
        printf("kswap thread is running.i = %d.\n", i);
        pthread_mutex_unlock(&lock);
        /*pthread_rwlock_unlock(&rwlock);*/
        sleep(1);
    }
}

void *krecalim_fn(void *args)
{
    while(1)
    {
        pthread_mutex_trylock(&lock);
        /*pthread_rwlock_wrlock(&rwlock);*/
        ++i;
        pthread_mutex_unlock(&lock);
        /*pthread_rwlock_unlock(&rwlock);*/
        /*pthread_mutex_unlock(&lock);*/
        if(i < 10)
            pthread_cond_signal(&ready);
        printf("kreclaim thread is running.i = %d.\n", i);
        sleep(1);
    }
}

void thread_init()
{
    int ret;
    pthread_t kswap_id;
    pthread_t kreclaim_id;

    /*if(pthread_mutex_init(&lock, NULL) < 0)*/
    /*{*/
        /*return;*/
    /*}*/
    /*if(pthread_rwlock_init(&rwlock, NULL) < 0)*/
    /*{*/
    /*return;*/
    /*}*/

    if((ret = pthread_create(&kswap_id, NULL, kswap_fn, NULL)) < 0)
    {
        printf("pthread_create error: %s.\n", strerror(errno));
    }
    if((ret = pthread_create(&kreclaim_id, NULL, krecalim_fn, NULL)) < 0)
    {
        printf("pthread_create error: %s.\n", strerror(errno));
    }
}

int main(int argc, char **argv)
{
    thread_init();

    while(1)
    {
        printf("main thread is running.\n");
        sleep(1);
        pthread_exit(NULL);
    }
    return 0;
}
