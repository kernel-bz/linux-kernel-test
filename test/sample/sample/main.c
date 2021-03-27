#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#if 0
typedef unsigned long           uintptr_t;

#ifdef __CHECKER__
#define rcu_check_sparse(p, space) \
        ((void)(((typeof(*p) space *)p) == p))
#else /* #ifdef __CHECKER__ */
#define rcu_check_sparse(p, space)
#endif /* #else #ifdef __CHECKER__ */


static __always_inline void __write_once_size(volatile void *p, void *res, int size)
{
        switch (size) {
        case 1: *(volatile __u8 *)p = *(__u8 *)res; break;
        case 2: *(volatile __u16 *)p = *(__u16 *)res; break;
        case 4: *(volatile __u32 *)p = *(__u32 *)res; break;
        case 8: *(volatile __u64 *)p = *(__u64 *)res; break;
        default:
                barrier();
                __builtin_memcpy((void *)p, (const void *)res, size);
                barrier();
        }
}

#define WRITE_ONCE(x, val) \
({                                                      \
        union { typeof(x) __val; char __c[1]; } __u =   \
                { .__val = (__force typeof(x)) (val) }; \
        __write_once_size(&(x), __u.__c, sizeof(x));    \
        __u.__val;                                      \
})

#ifndef smp_store_release
#define smp_store_release(p, v) __smp_store_release(p, v)
#endif

#ifndef __smp_store_release
#define __smp_store_release(p, v)                                       \
do {                                                                    \
        compiletime_assert_atomic_type(*p);                             \
        __smp_mb();                                                     \
        WRITE_ONCE(*p, v);                                              \
} while (0)
#endif

#define rcu_assign_pointer(p, v)                                              \
do {                                                                          \
        uintptr_t _r_a_p__v = (uintptr_t)(v);                                 \
        rcu_check_sparse(p, __rcu);                                           \
                                                                              \
        if (__builtin_constant_p(v) && (_r_a_p__v) == (uintptr_t)NULL)        \
                WRITE_ONCE((p), (typeof(p))(_r_a_p__v));                      \
        else                                                                  \
                smp_store_release(&p, RCU_INITIALIZER((typeof(p))_r_a_p__v)); \
} while (0)
#endif


int main()
{
    int i, len=0;

    for (i=0; i<300; i++) {
        len++;
        if (len & 31) continue;
        printf("i=%d, len=%d\n", i, len);
    }
    printf("i=%d, len=%d\n", i, len);

    printf("---------------\n");
    len = 0;
    for (i=0; i<300; i++) {
        len--;
        if (-len & 31) continue;
        printf("i=%d, len=%d\n", i, -len);
    }

    printf("i=%d, len=%d\n", i, len);
    return 0;
}
