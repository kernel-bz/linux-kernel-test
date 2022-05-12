
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

#include <mm/slab.h>
#include <linux/slub_def.h>
#include <linux/gfp.h>

/*
 * calculate_sizes() determines the order and the distribution of data within
 * a slab object.
 */
static int calculate_sizes(struct kmem_cache_slub *s, int forced_order)
{
        slab_flags_t flags = s->flags;
        unsigned int size = s->object_size;
        unsigned int order;

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

        kasan_cache_create(s, &size, &s->flags);
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

        return !!oo_objects(s->oo);
}
