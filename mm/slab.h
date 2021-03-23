/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MM_SLAB_H
#define MM_SLAB_H
/*
 * Internal slab definitions
 */

enum slab_state {
    DOWN,			/* No slab functionality yet */
    PARTIAL,		/* SLUB: kmem_cache_node available */
    PARTIAL_NODE,	/* SLAB: kmalloc size for node struct available */
    UP,				/* Slab caches usable but not all extras yet */
    FULL			/* Everything is working */
};


//user_common.c
bool slab_is_available(void);
void slab_state_set(enum slab_state state);

#endif
