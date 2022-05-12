/* SPDX-License-Identifier: GPL-2.0 */
#ifndef MM_SLAB_H
#define MM_SLAB_H

#include "test/config.h"


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


/*
 * The slab lists for all objects.
 */
struct kmem_cache_node {
        spinlock_t list_lock;

#ifdef CONFIG_SLAB
        struct list_head slabs_partial; /* partial list first, better asm code */
        struct list_head slabs_full;
        struct list_head slabs_free;
        unsigned long total_slabs;      /* length of all slab lists */
        unsigned long free_slabs;       /* length of free slab list only */
        unsigned long free_objects;
        unsigned int free_limit;
        unsigned int colour_next;       /* Per-node cache coloring */
        struct array_cache *shared;     /* shared per node */
        struct alien_cache **alien;     /* on other nodes */
        unsigned long next_reap;        /* updated without locking */
        int free_touched;               /* updated without locking */
#endif

#ifdef CONFIG_SLUB
        unsigned long nr_partial;
        struct list_head partial;
#ifdef CONFIG_SLUB_DEBUG
        atomic_long_t nr_slabs;
        atomic_long_t total_objects;
        struct list_head full;
#endif
#endif

};


//user_common.c
bool slab_is_available(void);
void slab_state_set(enum slab_state state);

#endif
