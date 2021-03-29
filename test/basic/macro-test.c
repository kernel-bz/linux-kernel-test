// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/macro-test.c
 *  Macro Test
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#include "test/debug.h"
#include <linux/compiler.h>
#include <linux/compiler-gcc.h>
#include <linux/compiler_types.h>
#include <asm-generic/int-ll64.h>
#include <asm-generic/barrier.h>

# define __force	__attribute__((force))
# define __kernel	__attribute__((address_space(0)))
# define __rcu		__attribute__((noderef, address_space(4)))

static inline void pr_msg(const char *s)
{
    printf("%s: %d, %s\n", __FILE__, __LINE__, s);
}

#define __WARN(c, s)			\
    do {								\
        if ((c))						\
            pr_msg(s);					\
    } while (0)

#define __check_sparse(p, space) \
        ((void)(((typeof(*p) space *)p) == p))

#define __access_pointer(p, space) \
({ \
    typeof(*p) *_________p1 = (typeof(*p) *__force)READ_ONCE(p); \
    rcu_check_sparse(p, space); \
    ((typeof(*p) __force __kernel *)(_________p1)); \
})
#define __dereference_check(p, c, space) \
({ \
    /* Dependency order vs. p above. */ \
    typeof(*p) *________p1 = (typeof(*p) *__force)READ_ONCE(p); \
    __WARN(!(c), "suspicious rcu_dereference_check() usage"); \
    __check_sparse(p, space); \
    ((typeof(*p) __force __kernel *)(________p1)); \
})
#define __dereference_protected(p, c, space) \
({ \
    __WARN(!(c), "suspicious rcu_dereference_protected() usage"); \
    rcu_check_sparse(p, space); \
    ((typeof(*p) __force __kernel *)(p)); \
})
#define __dereference_raw(p) \
({ \
    /* Dependency order vs. p above. */ \
    typeof(p) ________p1 = READ_ONCE(p); \
    ((typeof(*p) __force __kernel *)(________p1)); \
})
typedef __u8  __attribute__((__may_alias__))  __u8_alias_t;
typedef __u16 __attribute__((__may_alias__)) __u16_alias_t;
typedef __u32 __attribute__((__may_alias__)) __u32_alias_t;
typedef __u64 __attribute__((__may_alias__)) __u64_alias_t;

#ifndef smp_store_release
#define smp_store_release(p, v) __smp_store_release(p, v)
#endif

#ifndef __smp_store_release
#define __smp_store_release(p, v)                                       \
do {                                                                    \
        compiletime_assert_atomic_type(*p);                             \
        WRITE_ONCE(*p, v);                                              \
} while (0)
#endif

typedef unsigned long           uintptr_t;

#define _INITIALIZER(v) (typeof(*(v)) __force __rcu *)(v)
#define _assign_pointer(p, v)                                              \
do {                                                                          \
        uintptr_t _r_a_p__v = (uintptr_t)(v);                                 \
        __check_sparse(p, __rcu);                                           \
                                                                              \
        if (__builtin_constant_p(v) && (_r_a_p__v) == (uintptr_t)NULL)        \
                WRITE_ONCE((p), (typeof(p))(_r_a_p__v));                      \
        else                                                                  \
                smp_store_release(&p, _INITIALIZER((typeof(p))_r_a_p__v)); \
} while (0)

#define _read_lock_held()	1

#define _dereference_check(p, c) \
    __dereference_check((p), (c) || _read_lock_held(), __rcu)

#define _dereference(p) _dereference_check(p, 0)

void basic_macro_test(void)
{
    int a = 10;
    int b = 20;
    int __rcu *arp;
    int *bp = &b;

    pr_fn_start(stack_depth);

    _assign_pointer(arp, &a);	//arp = &a
#if 0
    do {
        uintptr_t _r_a_p__v = (uintptr_t)(&a);
        ((void)(((typeof(*arp) __attribute__((noderef, address_space(4))) *)arp) == arp));
        if (__builtin_constant_p(&a) && (_r_a_p__v) == (uintptr_t) ((void *)0) )
            ({ union { typeof((arp)) __val; char __c[1]; } __u = { .__val = ((typeof(arp))(_r_a_p__v)) };
                __write_once_size(&((arp)), __u.__c, sizeof((arp)));
                __u.__val; });
        else
            do {
                do { } while (0);
                __asm__ __volatile__("": : :"memory");
                ({ union { typeof(*&arp) __val; char __c[1]; } __u =
                    { .__val = ((typeof(*((typeof(arp))_r_a_p__v)) __attribute__((force))
                                 __attribute__((noderef, address_space(4))) *)((typeof(arp))_r_a_p__v)) };
                    __write_once_size(&(*&arp), __u.__c, sizeof(*&arp));
                    __u.__val; });
            } while (0);
    } while (0);
#endif

    a = 100;
    printf("*bp = %d\n", *bp);

    bp = _dereference(arp);
    printf("*bp = %d\n", *bp);

    pr_fn_end(stack_depth);
}
