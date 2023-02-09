/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LINUX_PERCPU_H
#define __LINUX_PERCPU_H

#include "test/user-types.h"

#include <linux/percpu-defs.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/cpumask.h>

//#define DECLARE_PER_CPU(type, val) extern type val
#define DECLARE_PER_CPU(type, val) type val
#define DEFINE_PER_CPU(type, val) type val

//include/linux/percpu-defs.h
#define DECLARE_PER_CPU_SHARED_ALIGNED(type, name)	type name
#define DEFINE_PER_CPU_SHARED_ALIGNED(type, name)	type name
#define DECLARE_PER_CPU_ALIGNED(type, name)		type name
#define DEFINE_PER_CPU_ALIGNED(type, name)		type name
#define DECLARE_PER_CPU_PAGE_ALIGNED(type, name)	type name
#define DEFINE_PER_CPU_PAGE_ALIGNED(type, name)		type name

#define per_cpu_ptr(ptr, cpu)	((typeof(ptr))(ptr) + cpu)
#define per_cpu(var, cpu)	(*per_cpu_ptr(&(var), cpu))

#define __get_cpu_var(var)	var
#define this_cpu_ptr(var)	var
#define this_cpu_read(var)	var
#define this_cpu_xchg(var, val)		uatomic_xchg(&var, val)
#define this_cpu_cmpxchg(var, old, new)	uatomic_cmpxchg(&var, old, new)

/* Ignore alignment, as CBMC doesn't care about false sharing. */
#define alloc_percpu(type) __alloc_percpu(sizeof(type), 1)

static inline void *__alloc_percpu(size_t size, size_t align)
{
    //gfp_t flags = (gfp_t)align;
    //return kmalloc(nr_cpu_ids * size, GFP_NOWAIT);
    //return kmalloc(nr_cpu_ids * size, flags);
    return malloc(nr_cpu_ids * size);
}

static inline void free_percpu(void *ptr)
{
    kfree(ptr);
}

#define per_cpu_ptr(ptr, cpu)	((typeof(ptr))(ptr) + cpu)

#define __this_cpu_inc(pcp) __this_cpu_add(pcp, 1)
#define __this_cpu_dec(pcp) __this_cpu_sub(pcp, 1)
#define __this_cpu_sub(pcp, n) __this_cpu_add(pcp, -(typeof(pcp)) (n))

#define this_cpu_inc(pcp) this_cpu_add(pcp, 1)
#define this_cpu_dec(pcp) this_cpu_sub(pcp, 1)
#define this_cpu_sub(pcp, n) this_cpu_add(pcp, -(typeof(pcp)) (n))


#endif
