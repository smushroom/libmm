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

    int i = 0;
    unsigned long address;
    /*unsigned int type;*/
    /*swp_entry_t entry;*/
    /*char buf[4096] = {0};*/

    while(1)
    {
        int ret;
        printf("0 ----------------------------------------------\n");
        address = lzalloc(PAGE_NR * 4096);
        printf("address = 0x%x.index = %d\n", address, pfn_to_index(address));
        /*if(address)*/
        /*{*/
        /*[>snprintf((char *)address, PAGE_SIZE, "wo shi softirq - %d", i);<]*/
        /*[>printf("address = %s.\n", address);<]*/
        /*lfree((void *)address);*/
        /*}*/


        printf("1 lookup ----------------------------------------------\n");
        if(llookup((void *)address) < 0)
        {
            printf("find page : 0x%lx error.\n", address);
        }
        else
        {
            printf("find page : 0x%lx.\n", address);
        }

        printf("2 free_----------------------------------------------\n");

        /*if(lfree((void *)address) < 0)*/
        /*{*/
        /*printf("lfree page error.\n");*/
        /*}*/

        if(lfree_toswp((void *)address) < 0)
        {
            printf("free page to swap error.\n");
        }
        printf("lfreeswp address = 0x%x.\n", address);

        /*[>printf("4 ----------------------------------------------\n");<]*/
        /*[>if(llookup((void *)address) < 0)<]*/
        /*{*/
        /*printf("find page : 0x%x error.\n", address);*/
        /*}*/
        /*else*/
        /*{*/
        /*printf("find page : 0x%x.\n", address);*/
        /*}*/

        printf("5 write ----------------------------------------------\n");
        /*snprintf((void *)address, 1024, "-------%s-------", "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk");*/
        char buf[PAGE_NR * 4096] = "wo shi softirq. ni shi shui?";
        if((ret = lwrite((void **)&address, buf, 0, PAGE_NR * 4096))  < PAGE_NR * 4096)
        {
            printf("lwrite error.ret = %d.\n", ret);
        }
        else
        {
            printf("lwrite page 0x%lx.\n", address);
        }
        printf("************************************************\n");
        /*printf("address = 0x%lx.\n", address);*/

        printf("5 free_to_swap ----------------------------------------------\n");
        if(lfree_toswp((void *)address) < 0)
        {
            printf("free page to swap error.\n");
        }
        printf("lfreeswp address = 0x%x.\n", address);

        /*printf("4 ----------------------------------------------\n");*/
        printf("6 read ----------------------------------------------\n");
        char data[PAGE_NR * 4096] = {0};
        if((ret = lread((void **)&address, data, 0, PAGE_NR * 4096)) < PAGE_NR * 4096)
        {
            printf("read page error. ret = %d\n", ret);
        }
        printf("data = %s.\n", data);

        printf("000000000000000000000000000000000000000000000000000\n");
        i++;

        sleep(100);
    }


    /*print_buddy_list();*/
    return 0;
}
