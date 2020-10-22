/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __TOOLS_LINUX_ATOMIC_H
#define __TOOLS_LINUX_ATOMIC_H

#include <asm/atomic.h>
//#include <linux/spinlock.h>

/* atomic_cmpxchg_relaxed */
#ifndef atomic_cmpxchg_relaxed
#define  atomic_cmpxchg_relaxed			atomic_cmpxchg
#define  atomic_cmpxchg_release         atomic_cmpxchg
#endif /* atomic_cmpxchg_relaxed */


static inline void atomic_add(long i, atomic_t *v)
{
    v->counter += i;
}

//lib/atomic64.c
static inline void atomic64_add(s64 a, atomic64_t *v)
{
    v->counter += a;
}

static inline s64 atomic64_read(const atomic64_t *v)
{
    return  v->counter;
}

static inline s64 atomic64_xchg(atomic64_t *v, s64 new)
{
    s64 val;
    val = v->counter;
    v->counter = new;
    return val;
}


#endif /* __TOOLS_LINUX_ATOMIC_H */
