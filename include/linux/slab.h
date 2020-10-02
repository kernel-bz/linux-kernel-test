/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SLAB_H
#define SLAB_H

#include <linux/types-user.h>
#include <linux/gfp.h>
#include "linux/mm.h"

#define SLAB_HWCACHE_ALIGN 1
#define SLAB_PANIC 2
#define SLAB_RECLAIM_ACCOUNT    0x00020000UL            /* Objects are reclaimable */

void *kmalloc(size_t size, gfp_t);
void kfree(void *);

static inline void *kzalloc(size_t size, gfp_t gfp)
{
        return kmalloc(size, gfp | __GFP_ZERO);
}

static inline int kcalloc(int nr, size_t size, gfp_t gfp)
{
        kmalloc(nr*size, gfp | __GFP_ZERO);
        return nr;
}

void *kmem_cache_alloc(struct kmem_cache *cachep, int flags);
void kmem_cache_free(struct kmem_cache *cachep, void *objp);

struct kmem_cache *
kmem_cache_create(const char *name, size_t size, size_t offset,
	unsigned long flags, void (*ctor)(void *));


/*
 * Please use this macro to create slab caches. Simply specify the
 * name of the structure and maybe some flags that are listed above.
 *
 * The alignment of the struct determines object alignment. If you
 * f.e. add ____cacheline_aligned_in_smp to the struct declaration
 * then the objects will be properly aligned in SMP configurations.
 */
#define KMEM_CACHE(__struct, __flags)					\
        kmem_cache_create(#__struct, sizeof(struct __struct),	\
            __alignof__(struct __struct), (__flags), NULL)

/*
 * To whitelist a single field for copying to/from usercopy, use this
 * macro instead for KMEM_CACHE() above.
 */
#define KMEM_CACHE_USERCOPY(__struct, __flags, __field)			\
        kmem_cache_create_usercopy(#__struct,			\
            sizeof(struct __struct),			\
            __alignof__(struct __struct), (__flags),	\
            offsetof(struct __struct, __field),		\
            sizeof_field(struct __struct, __field), NULL)



#endif		/* SLAB_H */
