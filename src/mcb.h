#ifndef     _MCB_H_
#define     _MCB_H_

#include <time.h>
#include "rbtree.h"
#include "pte.h"

typedef struct mem_control_block 
{
    struct rb_node node;
    time_t vruntime;
    unsigned long address;
    unsigned long flags;        /* flags */
    pte_t *pte;
    size_t size;
}mem_control_block_t;

extern mem_control_block_t *mcb_search(struct rb_root *root, unsigned long address);
extern int mcb_earse(struct rb_root *root, mem_control_block_t *mcb);
extern int mcb_replace(struct rb_root *root, mem_control_block_t *mcb);
extern int mcb_insert(struct rb_root *root, mem_control_block_t *mcb);
extern mem_control_block_t * ts_leftmost(struct rb_root *root);
extern void rb_front_print(struct rb_node *pnode);
extern mem_control_block_t * mcb_alloc();
extern int kmem_mcb_init();

#endif
