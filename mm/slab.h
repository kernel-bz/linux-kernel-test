/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MM_SLAB_H
#define MM_SLAB_H
/*
 * Internal slab definitions
 */

enum slab_state {
    DOWN,			/* No slab functionality yet */
    PARTIAL,		/* SLUB: kmem_cache_node available */
    PARTIAL_NODE,		/* SLAB: kmalloc size for node struct available */
    UP,			/* Slab caches usable but not all extras yet */
    FULL			/* Everything is working */
};


#endif
