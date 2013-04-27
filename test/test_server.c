#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "api.h"
#include "buddy.h"
#include "swap.h"

#define     PAGE_NR     1

int main(int argc, char **argv)
{
    init_mm();
    if(vzone_alloc(pthread_self()) < 0)
    {
        printf("vzone_alloc error. ");
        return -1;
    }

    void * address;

    while(1)
    {
        printf("0 ----------------------------------------------\n");
        address = lzalloc(PAGE_NR * 4096);
        printf("address = 0x%x.\n", (unsigned long)address);

        sleep(100);
    }


    /*print_buddy_list();*/
    return 0;
}
