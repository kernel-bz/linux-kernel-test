
// SPDX-License-Identifier: GPL-2.0
/*
 * SLUB: A slab allocator that limits cache line use instead of queuing
 * objects in per cpu and per node lists.
 *
 * The allocator synchronizes using per slab locks or atomic operatios
 * and only uses a centralized lock to manage a pool of partial slabs.
 *
 * (C) 2007 SGI, Christoph Lameter
 * (C) 2011 Linux Foundation, Christoph Lameter
 */

#include <linux/mm.h>
//#include <linux/swap.h> /* struct reclaim_state */
#include <linux/module.h>
//#include <linux/bit_spinlock.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include "slab.h"
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
//#include <linux/kasan.h>
#include <linux/cpu.h>
#include <linux/cpuset.h>
#include <linux/mempolicy.h>
#include <linux/ctype.h>
//#include <linux/debugobjects.h>
#include <linux/kallsyms.h>
//#include <linux/memory.h>
#include <linux/math64.h>
//#include <linux/fault-inject.h>
#include <linux/stacktrace.h>
//#include <linux/prefetch.h>
//#include <linux/memcontrol.h>
#include <linux/random.h>

//#include <trace/events/kmem.h>

//#include "internal.h"

#include <stdio.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/debug.h"
#include "test/test.h"

#include <mm/slab.h>
#include <linux/slub_def.h>
#include <linux/gfp.h>


struct kmem_cache_slub *kmem_cache_slub;

//include/linux/mmzone.h
#define PAGE_ALLOC_COSTLY_ORDER 3


#define OO_SHIFT        16
#define OO_MASK         ((1 << OO_SHIFT) - 1)
#define MAX_OBJS_PER_PAGE       32767 /* since page.objects is u15 */

/* Internal SLUB flags */
/* Poison object */
#define __OBJECT_POISON         ((slab_flags_t __force)0x80000000U)
/* Use cmpxchg_double */
#define __CMPXCHG_DOUBLE        ((slab_flags_t __force)0x40000000U)


/* Loop over all objects in a slab */
#define for_each_object(__p, __s, __addr, __objects) \
        for (__p = fixup_red_left(__s, __addr); \
                __p < (__addr) + (__objects) * (__s)->size; \
                __p += (__s)->size)

/* Determine object index from a given position */
static inline unsigned int slab_index(void *p, struct kmem_cache_slub *s, void *addr)
{
        //return (kasan_reset_tag(p) - addr) / s->size;
        return ((void *)p - addr) / s->size;
}

static inline unsigned int order_objects(unsigned int order, unsigned int size)
{
        return ((unsigned int)PAGE_SIZE << order) / size;
}

static inline struct kmem_cache_order_objects oo_make(unsigned int order,
                unsigned int size)
{
        struct kmem_cache_order_objects x = {
                (order << OO_SHIFT) + order_objects(order, size)
        };

        return x;
}

static inline unsigned int oo_order(struct kmem_cache_order_objects x)
{
        return x.x >> OO_SHIFT;
}

static inline unsigned int oo_objects(struct kmem_cache_order_objects x)
{
        return x.x & OO_MASK;
}

/*
 * Mininum / Maximum order of slab pages. This influences locking overhead
 * and slab fragmentation. A higher order reduces the number of partial slabs
 * and increases the number of allocations possible without having to
 * take the list_lock.
 */
static unsigned int slub_min_order;
static unsigned int slub_max_order = PAGE_ALLOC_COSTLY_ORDER;
static unsigned int slub_min_objects;


static inline unsigned int slab_order(unsigned int size,
                unsigned int min_objects, unsigned int max_order,
                unsigned int fract_leftover)
{
        unsigned int min_order = slub_min_order;
        unsigned int order;

        if (order_objects(min_order, size) > MAX_OBJS_PER_PAGE)
                return get_order(size * MAX_OBJS_PER_PAGE) - 1;

        for (order = max(min_order, (unsigned int)get_order(min_objects * size));
                        order <= max_order; order++) {

                unsigned int slab_size = (unsigned int)PAGE_SIZE << order;
                unsigned int rem;

                rem = slab_size % size;

                if (rem <= slab_size / fract_leftover)
                        break;
        }

        return order;
}

static inline int calculate_order(unsigned int size)
{
        unsigned int order;
        unsigned int min_objects;
        unsigned int max_objects;

        pr_fn_start_enable(stack_depth);

        /*
         * Attempt to find best configuration for a slab. This
         * works by first attempting to generate a layout with
         * the best configuration and backing off gradually.
         *
         * First we increase the acceptable waste in a slab. Then
         * we reduce the minimum objects required in a slab.
         */
        min_objects = slub_min_objects;
        if (!min_objects)
                min_objects = 4 * (fls(nr_cpu_ids) + 1);
        max_objects = order_objects(slub_max_order, size);
        min_objects = min(min_objects, max_objects);

        pr_view_enable(stack_depth, "%20s : %u\n", min_objects);
        pr_view_enable(stack_depth, "%20s : %u\n", max_objects);

        while (min_objects > 1) {
                unsigned int fraction;

                fraction = 16;
                while (fraction >= 4) {
                        order = slab_order(size, min_objects,
                                        slub_max_order, fraction);
                        if (order <= slub_max_order)
                                return order;
                        fraction /= 2;
                }
                min_objects--;
        }

        /*
         * We were unable to place multiple objects in a slab. Now
         * lets see if we can place a single object there.
         */
        order = slab_order(size, 1, slub_max_order, 1);
        if (order <= slub_max_order)
                return order;

        /*
         * Doh this slab cannot be placed using slub_max_order.
         */
        order = slab_order(size, 1, MAX_ORDER, 1);

        pr_view_enable(stack_depth, "%20s : %u\n", size);
        pr_view_enable(stack_depth, "%20s : %u\n", order);
        pr_fn_end_enable(stack_depth);

        if (order < MAX_ORDER)
                return order;
        return -ENOSYS;
}

/*
 * calculate_sizes() determines the order and the distribution of data within
 * a slab object.
 */
static int calculate_sizes(struct kmem_cache_slub *s, int forced_order)
{
        slab_flags_t flags = s->flags;
        unsigned int size = s->object_size;
        unsigned int order;

        pr_fn_start_enable(stack_depth);

        /*
         * Round up object size to the next word boundary. We can only
         * place the free pointer at word boundaries and this determines
         * the possible location of the free pointer.
         */
        size = ALIGN(size, sizeof(void *));

#ifdef CONFIG_SLUB_DEBUG
        /*
         * Determine if we can poison the object itself. If the user of
         * the slab may touch the object after free or before allocation
         * then we should never poison the object itself.
         */
        if ((flags & SLAB_POISON) && !(flags & SLAB_TYPESAFE_BY_RCU) &&
                        !s->ctor)
                s->flags |= __OBJECT_POISON;
        else
                s->flags &= ~__OBJECT_POISON;


        /*
         * If we are Redzoning then check if there is some space between the
         * end of the object and the free pointer. If not then add an
         * additional word to have some bytes to store Redzone information.
         */
        if ((flags & SLAB_RED_ZONE) && size == s->object_size)
                size += sizeof(void *);
#endif

        /*
         * With that we have determined the number of bytes in actual use
         * by the object. This is the potential offset to the free pointer.
         */
        s->inuse = size;

        if (((flags & (SLAB_TYPESAFE_BY_RCU | SLAB_POISON)) ||
                s->ctor)) {
                /*
                 * Relocate free pointer after the object if it is not
                 * permitted to overwrite the first word of the object on
                 * kmem_cache_free.
                 *
                 * This is the case if we do RCU, have a constructor or
                 * destructor or are poisoning the objects.
                 */
                s->offset = size;
                size += sizeof(void *);
        }

#ifdef CONFIG_SLUB_DEBUG
        if (flags & SLAB_STORE_USER)
                /*
                 * Need to store information about allocs and frees after
                 * the object.
                 */
                size += 2 * sizeof(struct track);
#endif

        //kasan_cache_create(s, &size, &s->flags);
#ifdef CONFIG_SLUB_DEBUG
        if (flags & SLAB_RED_ZONE) {
                /*
                 * Add some empty padding so that we can catch
                 * overwrites from earlier objects rather than let
                 * tracking information or the free pointer be
                 * corrupted if a user writes before the start
                 * of the object.
                 */
                size += sizeof(void *);

                s->red_left_pad = sizeof(void *);
                s->red_left_pad = ALIGN(s->red_left_pad, s->align);
                size += s->red_left_pad;
        }
#endif

        /*
         * SLUB stores one object immediately after another beginning from
         * offset 0. In order to align the objects we have to simply size
         * each object to conform to the alignment.
         */
        size = ALIGN(size, s->align);
        s->size = size;
        if (forced_order >= 0)
                order = forced_order;
        else
                order = calculate_order(size);

        pr_view_enable(stack_depth, "%20s : %u\n", size);
        pr_view_enable(stack_depth, "%20s : %u\n", order);

        if ((int)order < 0)
                return 0;

        s->allocflags = 0;
        if (order)
                s->allocflags |= __GFP_COMP;

        if (s->flags & SLAB_CACHE_DMA)
                s->allocflags |= GFP_DMA;

        if (s->flags & SLAB_CACHE_DMA32)
                s->allocflags |= GFP_DMA32;

        if (s->flags & SLAB_RECLAIM_ACCOUNT)
                s->allocflags |= __GFP_RECLAIMABLE;
        /*
         * Determine the number of objects per slab
         */
        s->oo = oo_make(order, size);
        s->min = oo_make(get_order(size), size);
        if (oo_objects(s->oo) > oo_objects(s->max))
                s->max = s->oo;

        pr_fn_end_enable(stack_depth);

        return !!oo_objects(s->oo);
}

void slub_calculate_sizes_test(void)
{
    static struct kmem_cache_slub test_kmem_cache;
    //unsigned int align = ARCH_KMALLOC_MINALIGN;
    unsigned int align = 128;
    unsigned int size;

    kmem_cache_slub = &test_kmem_cache;
    struct kmem_cache_slub *s = kmem_cache_slub;

    pr_fn_start_enable(stack_depth);

     __fpurge(stdin);
    printf("Input Size: ");
    scanf("%u", &size);

    s->name = "test";
    //s->size = s->object_size = sizeof(struct kmem_cache_node);
    s->size = s->object_size = size;
    s->align = ALIGN(align, sizeof(void *));
    s->useroffset = 0;
    s->usersize = 0;
    s->flags = SLAB_HWCACHE_ALIGN;

    mm_kmem_cache_slub_info(s);

    if(!calculate_sizes(s, -1))
        pr_err("%s\n", "error on the calculate_sizes().");

    mm_kmem_cache_slub_info(s);

    pr_fn_end_enable(stack_depth);
}
