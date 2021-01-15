// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/ptr-test.c
 *  Pointer Test
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdlib.h>

#include "test/debug.h"

#ifndef offsetof
//include/linux/stddef.h
//typedef unsigned long size_t

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#ifndef container_of
//include/linux/kernel.h

#define container_of(ptr, type, member) ({			\
    const typeof(((type *)0)->member) * __mptr = (ptr);	\
    (type *)((char *)__mptr - offsetof(type, member)); })
#endif

struct sample {
    char 		a;
    int 		b;
    long 		c;
    long long 	d;
    char		e;
};

static void _ptr_offsetof_test(void)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%30s : %d\n", offsetof(struct sample, a));
    pr_info_view_on(stack_depth, "%30s : %d\n", offsetof(struct sample, b));
    pr_info_view_on(stack_depth, "%30s : %d\n", offsetof(struct sample, c));
    pr_info_view_on(stack_depth, "%30s : %d\n", offsetof(struct sample, d));
    pr_info_view_on(stack_depth, "%30s : %d\n", offsetof(struct sample, e));

    pr_fn_end_on(stack_depth);
}

static void _ptr_container_of_test(void)
{
    pr_fn_start_on(stack_depth);

    struct sample sample_st;

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&sample_st);
    pr_info_view_on(stack_depth, "%30s : %p\n"
                    , container_of(&sample_st.a, struct sample, a));
    pr_info_view_on(stack_depth, "%30s : %p\n"
                    , container_of(&sample_st.b, struct sample, b));
    pr_info_view_on(stack_depth, "%30s : %p\n"
                    , container_of(&sample_st.c, struct sample, c));
    pr_info_view_on(stack_depth, "%30s : %p\n"
                    , container_of(&sample_st.d, struct sample, d));

    pr_fn_end_on(stack_depth);
}

void basic_ptr_test(void)
{
    _ptr_offsetof_test();
    _ptr_container_of_test();
}
