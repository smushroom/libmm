#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "api.h"
#include "buddy.h"
#include "swap.h"

#define     PAGE_NR     2

int main(int argc, char **argv)
{
    init_mm();
    if(vzone_alloc(pthread_self()) < 0)
    {
        printf("vzone_alloc error. ");
        return -1;
    }


    printf("0 ----------------------------------------------\n");
    void * address;
    address = lzalloc(PAGE_NR * 4096);
    printf("address = 0x%x.\n", (unsigned long)address);

    int ret;
    printf("5 write ----------------------------------------------\n");
    char buf[PAGE_NR * 4096] = "wo shi softirq. ni shi shui?";
    if((ret = lwrite((void **)&address, buf, 0, 8192))  < 8192)
    {
        printf("lwrite error.ret = %d.\n", ret);
    }
    else
    {
        printf("lwrite page 0x%lx.\n", address);
    }

    printf("1 vzone free ----------------------------------------------\n");
    if(vzone_free(pthread_self()) < 0)
    {
        printf("vzone_freeerror. ");
        return -2;
    }

    return 0;
}
