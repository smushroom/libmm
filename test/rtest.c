/*                     VTEST.C test program.                        */
/*  This is a simple test program which creates two index files.    */
/*  100 keys are added to each file, some keys are deleted and      */
/*  then the keys are added back to the file.  The keys are listed  */
/*  in ascending and decending order.                               */

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include "bplus.h"

#define MIN_TEST_START 0 
#define MAX_TEST_END   10000

IX_DESC name1, name2;        /* index file variables */

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
        printf("%s\n",ee.key);
        ++count;
    }
    printf("down list total key is %d.\n", count);
    /*printf("\nPress any key to continue \n");*/
    /*getchar();*/
}

int main(int argc, char **argv)
{
    printf("read index files\n");
    open_index("test1.idx", &name1,0);

    printf("List the keys in each index file in ascending order:\n\n");
    uplist(&name1);

    printf("List the keys for each index file in decending order\n\n");
    downlist(&name1);

    /* always close all files */
    close_index(&name1);

    return 0;
}
