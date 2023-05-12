#ifndef __TEST_USER_COMMON_H
#define __TEST_USER_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "test/user-define.h"
#include <linux/atomic.h>

#include <sys/time.h>
#include <time.h>

//include/linux/atomic-fallback.h
//#ifdef atomic_try_cmpxchg
#define atomic_try_cmpxchg_acquire atomic_try_cmpxchg
#define atomic_try_cmpxchg_release atomic_try_cmpxchg
#define atomic_try_cmpxchg_relaxed atomic_try_cmpxchg
//#endif /* atomic_try_cmpxchg */

#ifndef atomic_try_cmpxchg
static inline bool
atomic_try_cmpxchg(atomic_t *v, int *old, int new)
{
    int r, o = *old;
    r = atomic_cmpxchg(v, o, new);
    if (unlikely(r != o))
        *old = r;
    return likely(r == o);
}
#define atomic_try_cmpxchg atomic_try_cmpxchg
#endif


//include/linux/atomic-fallback.h
static inline int
atomic_fetch_add_unless(atomic_t *v, int a, int u)
{
    int c = atomic_read(v);

    do {
        if (unlikely(c == u))
            break;
    } while (!atomic_try_cmpxchg(v, &c, c + a));

    return c;
}
#define atomic_fetch_add_unless atomic_fetch_add_unless

//include/linux/atomic-fallback.h
static inline bool
atomic_add_unless_(atomic_t *v, int a, int u)
{
    return atomic_fetch_add_unless(v, a, u) != u;
}
#define atomic_add_unless atomic_add_unless_

static inline unsigned long get_time_nsec()
{
    struct timespec time;
    unsigned long nsec;

    clock_gettime(CLOCK_MONOTONIC, &time);
    nsec = time.tv_sec * 1000000000UL + time.tv_nsec;

    return nsec;
}

#ifdef __cplusplus
}
#endif

#endif // __TEST_USER_COMMON_H
