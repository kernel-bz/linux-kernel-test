// SPDX-License-Identifier: GPL-2.0+
/*
 *	mt-debug.h: debug header for maple-tree
 * 	Copyright (c) 2022 JaeJoon Jung <rgbi3307@gmail.com>
 */

#ifndef MTDEBUG_H
#define MTDEBUG_H

#include <linux/maple_tree.h>

void mt_debug_node_print(struct maple_tree *mt, struct maple_node *node);
void mt_debug_mas_print(struct ma_state *mas);
void mt_debug_wr_mas_print(struct ma_wr_state *wr_mas);
void mt_debug_mas_alloc_print(struct maple_alloc *alloc);

//mm/slab_user.c
void mm_debug_kmem_cache_print(struct kmem_cache *cachep);

#endif // MTDEBUG_H
