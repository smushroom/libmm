/*
 * Copyright (C) 2001 Momchil Velikov
 * Portions Copyright (C) 2001 Christoph Hellwig
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _RADIX_TREE_H_
#define _RADIX_TREE_H_

#include "slab.h"

typedef     unsigned    gfp_t;
#define BITS_PER_LONG 64

struct radix_tree_root {
	unsigned int		height;
	gfp_t			gfp_mask;
	struct radix_tree_node	*rnode;
};

#define RADIX_TREE_INIT(mask)	{					\
	.height = 0,							\
	.gfp_mask = (mask),						\
	.rnode = NULL,							\
}

#define RADIX_TREE(name, mask) \
	struct radix_tree_root name = RADIX_TREE_INIT(mask)

#define INIT_RADIX_TREE(root, mask)					\
do {									\
	(root)->height = 0;						\
	(root)->gfp_mask = (mask);					\
	(root)->rnode = NULL;						\
} while (0)

//int radix_tree_insert(struct radix_tree_root *, unsigned long, void *);
extern int radix_tree_insert(kmem_cache_t *, struct radix_tree_root *,unsigned long, void *);
void *radix_tree_lookup(struct radix_tree_root *, unsigned long);
void **radix_tree_lookup_slot(struct radix_tree_root *, unsigned long);
extern void *radix_tree_delete(kmem_cache_t *, struct radix_tree_root *, unsigned long);
unsigned int
radix_tree_gang_lookup(struct radix_tree_root *root, void **results,
			unsigned long first_index, unsigned int max_items);
int radix_tree_preload(gfp_t gfp_mask);
//void radix_tree_init(void);
void radix_tree_init(kmem_cache_t **);
void *radix_tree_tag_set(struct radix_tree_root *root,
			unsigned long index, int tag);
void *radix_tree_tag_clear(struct radix_tree_root *root,
			unsigned long index, int tag);
int radix_tree_tag_get(struct radix_tree_root *root,
			unsigned long index, int tag);
unsigned int
radix_tree_gang_lookup_tag(struct radix_tree_root *root, void **results,
		unsigned long first_index, unsigned int max_items, int tag);
int radix_tree_tagged(struct radix_tree_root *root, int tag);


#endif /* _LINUX_RADIX_TREE_H */
