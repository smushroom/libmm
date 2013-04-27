/*                     VTEST.C test program.                        */
/*  This is a simple test program which creates two index files.    */
/*  100 keys are added to each file, some keys are deleted and      */
/*  then the keys are added back to the file.  The keys are listed  */
/*  in ascending and decending order.                               */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "bplus.h"

#define MIN_TEST_START 0 
#define MAX_TEST_END   1000

int rand_num()
{
#define     RANG_LOW  MIN_TEST_START 
#define     RANG_HIGH MAX_TEST_END
    int num;
    /*srand((int)time(NULL));*/
    num =(int) (rand()%(RANG_HIGH - RANG_LOW + 1) + RANG_LOW);
#undef  RANG_LOW
#undef  RANG_HIGH 
    return num;
}

IX_DESC name1;        /* index file variables */

void uplist(IX_DESC *name)            /* list keys in ascending order */
{
    ENTRY ee;
    int count = 0;
    first_key(name);
    while (next_key(&ee, name) == IX_OK) 
    {
        printf("%s\n",ee.key);
        ++count;
    }
    printf("uplist total key is %d.\n", count);
    /*printf("\nPress any key to continue \n");*/
    /*getchar();*/
}

void downlist(IX_DESC *name)           /* list keys in decending order  */
{
    ENTRY ee;
    int count = 0;
    last_key(name);
    while (prev_key(&ee, name) == IX_OK) 
    {
        ++count;
        printf("%s\n",ee.key);
    }
    printf("down list total key is %d.\n", count);
    /*printf("\nPress any key to continue \n");*/
    /*getchar();*/
}

int main(int argc, char **argv)
{
    int i, num, ret;
    long ltime;
    ENTRY e;
    printf("Make one index files\n");
    make_index("test1.idx",&name1, 0);
    printf("Indexing 100 items in two index files:\n");

    /* note the time to index */
    time(&ltime);
    printf("%s",ctime(&ltime));

    /* add 100 keys to each index file */
    for (i = MIN_TEST_START; i < MAX_TEST_END; i++)
    {
        memset(&e, '\0', sizeof(ENTRY));
        e.recptr = i;
        sprintf(e.key, "%2d",i);
        if((ret = add_key(&e, &name1)) == IX_OK)
            printf("add key %s ok.\n", e.key);
        else if(ret = IX_EXISTED)
            printf("add key %s is existed.\n", e.key);
        else
            printf("add key %s error .\n", e.key);
    }
    printf("In Process: ADD %d key ok.\n", MAX_TEST_END);

    printf("Indexing is complete\n\n");
    printf("List the keys in each index file in ascending order:\n\n");
    uplist(&name1);

    /*[>list both files in decending order<]*/
    printf("List the keys for each index file in decending order\n\n");
    downlist(&name1);

    /* add key*/
    for(i = MIN_TEST_START; i < MAX_TEST_END/2; ++i)
    {
        memset(&e, '\0', sizeof(ENTRY));
        num = rand_num();
        e.recptr = num;
        sprintf(e.key, "%2d",num);
        if((ret = add_key(&e, &name1)) == IX_OK)
            printf("add key %s ok.\n", e.key);
        else if(ret = IX_EXISTED)
            printf("add key %s is existed.\n", e.key);
        else
            printf("add key %s error.\n", e.key);
    }

    /* delete some keys and list again */
    printf("\nNow delete half keys in each file\n\n");
    for (i = MIN_TEST_START; i < MAX_TEST_END/2 ; i++)
    {
        /*memset(&e, '\0', sizeof(ENTRY));*/
        num = rand_num();
        e.recptr = num;
        sprintf(e.key, "%2d",num);
            
        if((ret = delete_key(&e, &name1)) == IX_OK)
            printf("delete key %s ok.\n",e.key);
        else if(ret == IX_NOTEXISTED)
            printf("delete key %s not existed.\n",e.key);
        else
            printf("delete key %s error.\n", e.key);

    }

    printf("List the keys now for each index file in ascending order:\n\n");
    uplist(&name1);
    printf("List the keys now for each index file in ascending order:\n\n");
    downlist(&name1);

    printf("exception test.\n");
    printf("------------------------------------------------------------\n");
    for (i = MIN_TEST_START; i < MAX_TEST_END/2 ; i++)
    {
        /*memset(&e, '\0', sizeof(ENTRY));*/
        num = rand_num();
        e.recptr = num;
        sprintf(e.key, "%2d",num);
        if(num %2 == 0)
        {
            if((ret = delete_key(&e, &name1)) == IX_OK)
                printf("delete key %s ok.\n",e.key);
            else if(ret == IX_NOTEXISTED)
                printf("delete key %s not existed.\n",e.key);
            else
                printf("delete key %s error.\n", e.key);
        }
        else
        {
            if((ret = add_key(&e, &name1)) == IX_OK)
                printf("add key %s ok.\n", e.key);
            else if(ret = IX_EXISTED)
                printf("add key %s is existed.\n", e.key);
            else
                printf("add key %s error .\n", e.key);
        }
        if(num %17 == 0)
        {
            printf("exception break.\n");
            break;
        }
    }

    printf("List the keys now for each index file in ascending order:\n\n");
    uplist(&name1);
    printf("List the keys now for each index file in ascending order:\n\n");
    downlist(&name1);

    /* always close all files */
    close_index(&name1);
}
