/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __TOOLS_LINUX_ATOMIC_H
#define __TOOLS_LINUX_ATOMIC_H

#include <asm/atomic.h>

/* atomic_cmpxchg_relaxed */
#ifndef atomic_cmpxchg_relaxed
#define  atomic_cmpxchg_relaxed			atomic_cmpxchg
#define  atomic_cmpxchg_release         atomic_cmpxchg
#endif /* atomic_cmpxchg_relaxed */


static inline void atomic_add(long i, atomic_t *v)
{
    v->counter += i;
}

static inline void
atomic_long_add(long i, atomic_long_t *v)
{
    atomic_add(i, (atomic_t *)v);
}

static inline long
atomic_long_read(atomic_long_t *v)
{
    return atomic_read((atomic_t *)v);
}


#endif /* __TOOLS_LINUX_ATOMIC_H */
