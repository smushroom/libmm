/* 
 * memory control block
 * */

#include <stdio.h>
#include "debug.h"
#include "list.h"
#include "slab.h"
#include "mcb.h"

#if defined(rb_entry)
#undef rb_entry
#define rb_entry(ptr, type, member) container_of(ptr, type, member)
#endif

static kmem_cache_t *mcb_cachep = NULL;

mem_control_block_t * mcb_search(struct rb_root *root, unsigned long address)
{
    if(root == NULL)
    {
        return NULL;
    }

    struct rb_node *node = root->rb_node;
    mem_control_block_t *mcb = NULL;
    int ret;

    while (node) 
    {
        mcb = container_of(node, mem_control_block_t, node);
        ret = address - mcb->address;

        if (ret < 0)
        {
            node = node->rb_left;
        }
        else if (ret > 0)
        {
            node = node->rb_right;
        }
        else
        {
            return mcb;
        }
    }

    return NULL;
}

static void mcb_free(mem_control_block_t *mcb)
{
    if(mcb)
    {
        mcb_cachep->kmem_ops->free_obj(mcb);
    }
}

int mcb_replace(struct rb_root *root, mem_control_block_t *mcb)
{
    rb_erase(&(mcb->node),root);
    /*mcb_free(mcb);*/

    return 0;
}

int mcb_earse(struct rb_root *root, mem_control_block_t *mcb)
{
    if(mcb)
    {
        rb_erase(&(mcb->node),root);
        mcb_free(mcb);

        return 0;
    }

    return -1;
}

int mcb_insert(struct rb_root *root, mem_control_block_t *mcb)
{
    if(root == NULL || mcb == NULL)
    {
        return -1;
    }

    int ret = 0;
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    /*  Figure out where to put new node */
    while (*new) 
    {
        mem_control_block_t *this = container_of(*new, mem_control_block_t, node);
        if(this)
        {
            ret = mcb->address - this->address;
        }
        else
        {
            break;
        }

        parent = *new;
        if (ret < 0)
        {
            new = &((*new)->rb_left);
        }
        else if (ret > 0)
        {
            new = &((*new)->rb_right);
        }
        else
        {
            return 0;
        }
    }

    /*  Add new node and rebalance tree. */
    rb_link_node(&mcb->node, parent, new);
    rb_insert_color(&mcb->node, root);

    return 0;
}

mem_control_block_t * mcb_leftmost(struct rb_root *root)
{
    struct rb_node *pnode = rb_first(root);
    mem_control_block_t *mcb = rb_entry(pnode, mem_control_block_t, node);

    return mcb;
}

void rb_print(struct rb_node *pnode)
{
    if(pnode) 
    {
        if (pnode->rb_left != NULL) 
        {
            rb_print(pnode->rb_left);
        }

        DD("address = %ld color = %lu\t", rb_entry(pnode, mem_control_block_t, node)->address, rb_color(pnode));

        if (pnode->rb_right != NULL) 
        {
            rb_print(pnode->rb_right);
        }
    }
}

void rb_front_print(struct rb_node *pnode)
{
    if(pnode) 
    {
        DD("address = %ld color = %lu\t ", rb_entry(pnode, mem_control_block_t, node)->address,rb_color(pnode));
        if (pnode->rb_left != NULL) 
        {
            rb_front_print(pnode->rb_left);
        }

        if (pnode->rb_right != NULL) 
        {
            rb_front_print(pnode->rb_right);
        }
    }
}

int kmem_mcb_init()
{
    mcb_cachep = kmem_cache_create("kmem_mcb", sizeof(mem_control_block_t), 0);
    if(mcb_cachep == NULL)
    {
        DD("kmem_cache_create error. ");
        return -1;
    }

    return 0;
}

mem_control_block_t * mcb_alloc()
{
    mem_control_block_t *mcb = (mem_control_block_t *)mcb_cachep->kmem_ops->get_obj(mcb_cachep);
    return mcb;
}

#if defined(rb_entry)
#undef rb_entry
#endif
