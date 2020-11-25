/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __TOOLS_LINUX_ATOMIC_H
#define __TOOLS_LINUX_ATOMIC_H

#include <asm/atomic.h>
//#include <linux/spinlock.h>
#include <asm-generic/atomic64.h>

/* atomic_cmpxchg_relaxed */
#ifndef atomic_cmpxchg_relaxed
#define  atomic_cmpxchg_relaxed			atomic_cmpxchg
#define  atomic_cmpxchg_release         atomic_cmpxchg
#endif /* atomic_cmpxchg_relaxed */


static inline void atomic_add(long i, atomic_t *v)
{
    v->counter += i;
}

#endif /* __TOOLS_LINUX_ATOMIC_H */
