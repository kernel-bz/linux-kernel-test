/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_SLAB_H
#define __LINUX_SLAB_H

#include "test/user-types.h"
#include "test/config.h"

#include <linux/gfp.h>
#include <linux/compiler_types.h>
#include <linux/percpu.h>
#include <linux/percpu-defs.h>
#include <linux/kernel.h>
#include "linux/mm.h"

#define ___GFP_RECLAIMABLE	0x10u
#define ___GFP_ZERO		0x100u

/*
 * Flags to pass to kmem_cache_create().
 * The ones marked DEBUG are only valid if CONFIG_DEBUG_SLAB is set.
 */
/* DEBUG: Perform (expensive) checks on alloc/free */
#define SLAB_CONSISTENCY_CHECKS ((slab_flags_t __force)0x00000100U)
/* DEBUG: Red zone objs in a cache */
#define SLAB_RED_ZONE           ((slab_flags_t __force)0x00000400U)
/* DEBUG: Poison objects */
#define SLAB_POISON             ((slab_flags_t __force)0x00000800U)
/* Align objs on cache lines */
#define SLAB_HWCACHE_ALIGN      ((slab_flags_t __force)0x00002000U)
/* Use GFP_DMA memory */
#define SLAB_CACHE_DMA          ((slab_flags_t __force)0x00004000U)
/* Use GFP_DMA32 memory */
#define SLAB_CACHE_DMA32        ((slab_flags_t __force)0x00008000U)
/* DEBUG: Store the last owner for bug hunting */
#define SLAB_STORE_USER         ((slab_flags_t __force)0x00010000U)
/* Panic if kmem_cache_create() fails */
#define SLAB_PANIC              ((slab_flags_t __force)0x00040000U)

/* Defer freeing slabs to RCU */
#define SLAB_TYPESAFE_BY_RCU    ((slab_flags_t __force)0x00080000U)
/* Spread some memory over cpuset */
#define SLAB_MEM_SPREAD         ((slab_flags_t __force)0x00100000U)
/* Trace allocations and frees */
#define SLAB_TRACE              ((slab_flags_t __force)0x00200000U)

/* Flag to prevent checks on free */
#ifdef CONFIG_DEBUG_OBJECTS
# define SLAB_DEBUG_OBJECTS     ((slab_flags_t __force)0x00400000U)
#else
# define SLAB_DEBUG_OBJECTS     0
#endif

/* Avoid kmemleak tracing */
#define SLAB_NOLEAKTRACE        ((slab_flags_t __force)0x00800000U)

/* Fault injection mark */
#ifdef CONFIG_FAILSLAB
# define SLAB_FAILSLAB          ((slab_flags_t __force)0x02000000U)
#else
# define SLAB_FAILSLAB          0
#endif
/* Account to memcg */
#ifdef CONFIG_MEMCG_KMEM
# define SLAB_ACCOUNT           ((slab_flags_t __force)0x04000000U)
#else
# define SLAB_ACCOUNT           0
#endif

#ifdef CONFIG_KASAN
#define SLAB_KASAN              ((slab_flags_t __force)0x08000000U)
#else
#define SLAB_KASAN              0
#endif

/* The following flags affect the page allocator grouping pages by mobility */
/* Objects are reclaimable */
#define SLAB_RECLAIM_ACCOUNT    ((slab_flags_t __force)0x00020000U)
#define SLAB_TEMPORARY          SLAB_RECLAIM_ACCOUNT    /* Objects are short-lived */

/* Slab deactivation flag */
#define SLAB_DEACTIVATED        ((slab_flags_t __force)0x10000000U)

/*
 * ZERO_SIZE_PTR will be returned for zero sized kmalloc requests.
 *
 * Dereferencing ZERO_SIZE_PTR will lead to a distinct access fault.
 *
 * ZERO_SIZE_PTR can be passed to kfree though in the same way that NULL can.
 * Both make kfree a no-op.
 */
#define ZERO_SIZE_PTR ((void *)16)

#define ZERO_OR_NULL_PTR(x) ((unsigned long)(x) <= \
                                (unsigned long)ZERO_SIZE_PTR)

//include/linux/mmzone.h
/* Free memory management - zoned buddy allocator.  */
#ifndef CONFIG_FORCE_MAX_ZONEORDER
#define MAX_ORDER 11
#else
#define MAX_ORDER CONFIG_FORCE_MAX_ZONEORDER
#endif
#define MAX_ORDER_NR_PAGES (1 << (MAX_ORDER - 1))
//

/*
 * Kmalloc array related definitions
 */

#ifdef CONFIG_SLAB
/*
 * The largest kmalloc size supported by the SLAB allocators is
 * 32 megabyte (2^25) or the maximum allocatable page order if that is
 * less than 32 MB.
 *
 * WARNING: Its not easy to increase this value since the allocators have
 * to do various tricks to work around compiler limitations in order to
 * ensure proper constant folding.
 */
#define KMALLOC_SHIFT_HIGH      ((MAX_ORDER + PAGE_SHIFT - 1) <= 25 ? \
                                (MAX_ORDER + PAGE_SHIFT - 1) : 25)
#define KMALLOC_SHIFT_MAX       KMALLOC_SHIFT_HIGH
#ifndef KMALLOC_SHIFT_LOW
#define KMALLOC_SHIFT_LOW       5
#endif
#endif

#ifdef CONFIG_SLUB
/*
 * SLUB directly allocates requests fitting in to an order-1 page
 * (PAGE_SIZE*2).  Larger requests are passed to the page allocator.
 */
#define KMALLOC_SHIFT_HIGH      (PAGE_SHIFT + 1)
#define KMALLOC_SHIFT_MAX       (MAX_ORDER + PAGE_SHIFT - 1)
#ifndef KMALLOC_SHIFT_LOW
#define KMALLOC_SHIFT_LOW       3
#endif
#endif

#ifdef CONFIG_SLOB
/*
 * SLOB passes all requests larger than one page to the page allocator.
 * No kmalloc array is necessary since objects of different sizes can
 * be allocated from the same page.
 */
#define KMALLOC_SHIFT_HIGH      PAGE_SHIFT
#define KMALLOC_SHIFT_MAX       (MAX_ORDER + PAGE_SHIFT - 1)
#ifndef KMALLOC_SHIFT_LOW
#define KMALLOC_SHIFT_LOW       3
#endif
#endif

/* Maximum allocatable size */
#define KMALLOC_MAX_SIZE        (1UL << KMALLOC_SHIFT_MAX)
/* Maximum size for which we actually use a slab cache */
#define KMALLOC_MAX_CACHE_SIZE  (1UL << KMALLOC_SHIFT_HIGH)
/* Maximum order allocatable via the slab allocagtor */
#define KMALLOC_MAX_ORDER       (KMALLOC_SHIFT_MAX - PAGE_SHIFT)

/*
 * Kmalloc subsystem.
 */
#ifndef KMALLOC_MIN_SIZE
#define KMALLOC_MIN_SIZE (1 << KMALLOC_SHIFT_LOW)
#endif

/*
 * This restriction comes from byte sized index implementation.
 * Page size is normally 2^12 bytes and, in this case, if we want to use
 * byte sized index which can represent 2^8 entries, the size of the object
 * should be equal or greater to 2^12 / 2^8 = 2^4 = 16.
 * If minimum size of kmalloc is less than 16, we use it as minimum object
 * size and give up to use byte sized index.
 */
#define SLAB_OBJ_MIN_SIZE      (KMALLOC_MIN_SIZE < 16 ? \
                               (KMALLOC_MIN_SIZE) : 16)
/*
 * Whenever changing this, take care of that kmalloc_type() and
 * create_kmalloc_caches() still work as intended.
 */
enum kmalloc_cache_type {
        KMALLOC_NORMAL = 0,
        KMALLOC_RECLAIM,
#ifdef CONFIG_ZONE_DMA
        KMALLOC_DMA,
#endif
        NR_KMALLOC_TYPES
};

#ifndef CONFIG_SLOB
extern struct kmem_cache *
kmalloc_caches[NR_KMALLOC_TYPES][KMALLOC_SHIFT_HIGH + 1];

static __always_inline enum kmalloc_cache_type kmalloc_type(gfp_t flags)
{
#ifdef CONFIG_ZONE_DMA
        /*
         * The most common case is KMALLOC_NORMAL, so test for it
         * with a single branch for both flags.
         */
        if (likely((flags & (__GFP_DMA | __GFP_RECLAIMABLE)) == 0))
                return KMALLOC_NORMAL;

        /*
         * At least one of the flags has to be set. If both are, __GFP_DMA
         * is more important.
         */
        return flags & __GFP_DMA ? KMALLOC_DMA : KMALLOC_RECLAIM;
#else
        return flags & ___GFP_RECLAIMABLE ? KMALLOC_RECLAIM : KMALLOC_NORMAL;
#endif
}

/*
 * Figure out which kmalloc slab an allocation of a certain size
 * belongs to.
 * 0 = zero alloc
 * 1 =  65 .. 96 bytes
 * 2 = 129 .. 192 bytes
 * n = 2^(n-1)+1 .. 2^n
 */
static __always_inline unsigned int kmalloc_index(size_t size)
{
        if (!size)
                return 0;

        if (size <= KMALLOC_MIN_SIZE)
                return KMALLOC_SHIFT_LOW;

        if (KMALLOC_MIN_SIZE <= 32 && size > 64 && size <= 96)
                return 1;
        if (KMALLOC_MIN_SIZE <= 64 && size > 128 && size <= 192)
                return 2;
        if (size <=          8) return 3;
        if (size <=         16) return 4;
        if (size <=         32) return 5;
        if (size <=         64) return 6;
        if (size <=        128) return 7;
        if (size <=        256) return 8;
        if (size <=        512) return 9;
        if (size <=       1024) return 10;
        if (size <=   2 * 1024) return 11;
        if (size <=   4 * 1024) return 12;
        if (size <=   8 * 1024) return 13;
        if (size <=  16 * 1024) return 14;
        if (size <=  32 * 1024) return 15;
        if (size <=  64 * 1024) return 16;
        if (size <= 128 * 1024) return 17;
        if (size <= 256 * 1024) return 18;
        if (size <= 512 * 1024) return 19;
        if (size <= 1024 * 1024) return 20;
        if (size <=  2 * 1024 * 1024) return 21;
        if (size <=  4 * 1024 * 1024) return 22;
        if (size <=  8 * 1024 * 1024) return 23;
        if (size <=  16 * 1024 * 1024) return 24;
        if (size <=  32 * 1024 * 1024) return 25;
        if (size <=  64 * 1024 * 1024) return 26;
        BUG();

        /* Will never be reached. Needed because the compiler may complain */
        return -1;
}
#endif /* !CONFIG_SLOB */


#define SLAB_HWCACHE_ALIGN 1
#define SLAB_PANIC 2
#define SLAB_RECLAIM_ACCOUNT    0x00020000UL            /* Objects are reclaimable */

void *kmalloc(size_t size, gfp_t);
void kfree(void *);

static inline void *kzalloc(size_t size, gfp_t gfp)
{
        return kmalloc(size, gfp | ___GFP_ZERO);
}

static inline void *kcalloc(int nr, size_t size, gfp_t gfp)
{
        return kmalloc(nr*size, gfp | ___GFP_ZERO);
}

void *kmem_cache_alloc(struct kmem_cache *cachep, int flags);
void kmem_cache_free(struct kmem_cache *cachep, void *objp);

void kmem_cache_free_bulk(struct kmem_cache *s, size_t size, void **p);
int kmem_cache_alloc_bulk(struct kmem_cache *s, gfp_t flags, size_t size, void **p);

struct kmem_cache *
kmem_cache_create(const char *name, unsigned int size, unsigned int align,
        unsigned int flags, void (*ctor)(void *));

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


static __always_inline void *__kmalloc_node(size_t size, gfp_t flags, int node)
{
    return kmalloc(size, flags);
}

static __always_inline void *kmalloc_node(size_t size, gfp_t flags, int node)
{
    return __kmalloc_node(size, flags, node);
}

static inline void *kzalloc_node(size_t size, gfp_t flags, int node)
{
    return kmalloc_node(size, flags | ___GFP_ZERO, node);
}

static inline void *kmalloc_array(unsigned n, size_t s, gfp_t gfp)
{
    return kmalloc(n * s, gfp);
}

extern void *__kmalloc_track_caller(size_t, gfp_t, unsigned long);
#define kmalloc_track_caller(size, flags) \
    __kmalloc_track_caller(size, flags, _RET_IP_)


#endif		/* SLAB_H */
