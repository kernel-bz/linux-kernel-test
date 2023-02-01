// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/ptr-test.c
 *  Pointer Test
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

/**
$ cat /proc/self/maps
55cc223a6000-55cc223ae000 r-xp 00000000 08:06 6029337            /bin/cat
55cc225ad000-55cc225ae000 r--p 00007000 08:06 6029337            /bin/cat
55cc225ae000-55cc225af000 rw-p 00008000 08:06 6029337            /bin/cat
55cc22b22000-55cc22b43000 rw-p 00000000 00:00 0                  [heap]
7f54d3d59000-7f54d486a000 r--p 00000000 08:06 18487918           /usr/
7f54d486a000-7f54d4a51000 r-xp 00000000 08:06 4854359            /lib/
7f54d4e84000-7f54d4e85000 rw-p 00000000 00:00 0
7fffe60ce000-7fffe60ef000 rw-p 00000000 00:00 0                  [stack]
7fffe612d000-7fffe6130000 r--p 00000000 00:00 0                  [vvar]
7fffe6130000-7fffe6132000 r-xp 00000000 00:00 0                  [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0          [vsyscall]
*/

#include <stdio.h>
#include <stdlib.h>

#include <linux/compiler.h>

#include "test/debug.h"

#ifndef offsetof
//include/linux/stddef.h
//typedef unsigned long size_t
//#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)
#endif

#ifndef sizeof_field
#define sizeof_field(TYPE, MEMBER) sizeof((((TYPE *)0)->MEMBER))
#endif

#ifndef offsetofend
#define offsetofend(TYPE, MEMBER) \
        (offsetof(TYPE, MEMBER) + sizeof_field(TYPE, MEMBER))
#endif

#ifndef container_of
//include/linux/kernel.h
#define container_of(ptr, type, member) ({			\
    const typeof(((type *)0)->member) * __mptr = (ptr);	\
    (type *)((char *)__mptr - offsetof(type, member)); })
#endif

struct sample {
    char 		a;		//4( 0)
    int 		b;		//4( 4)
    long 		c;		//8( 8)
    long long 	d;		//8(16)
    char		e;		//8(24)
};

struct sample2 {
    char 		a;		//1( 0)
    int 		b;		//4( 1)
    long 		c;		//8( 5)
    long long 	d;		//8(13)
    char		e;		//1(21)
} __packed;


struct attr {
    char		*a;
    char		*b;
    char		*c;
    char 		*d;
    char		*e;
};

struct attr2 {
    char __kernel	*a;
    char __user		*b;
    char __iomem	*c;
    char __percpu	*d;
    char __rcu		*e;
};

struct attr attr_struct;
struct attr2 attr2_struct;

static void _ptr_offsetof_test(void)
{
    pr_fn_start(stack_depth);

    pr_view(stack_depth, "%30s : %d\n", sizeof(struct sample));
    pr_view(stack_depth, "%30s : %d\n", offsetof(struct sample, a));
    pr_view(stack_depth, "%30s : %d\n", offsetof(struct sample, b));
    pr_view(stack_depth, "%30s : %d\n", offsetof(struct sample, c));
    pr_view(stack_depth, "%30s : %d\n", offsetof(struct sample, d));
    pr_view(stack_depth, "%30s : %d\n\n", offsetof(struct sample, e));

    pr_view(stack_depth, "%30s : %d\n", offsetofend(struct sample, a));
    pr_view(stack_depth, "%30s : %d\n", offsetofend(struct sample, b));
    pr_view(stack_depth, "%30s : %d\n", offsetofend(struct sample, c));
    pr_view(stack_depth, "%30s : %d\n", offsetofend(struct sample, d));
    pr_view(stack_depth, "%30s : %d\n\n", offsetofend(struct sample, e));

    pr_view(stack_depth, "%30s : %d\n", sizeof(struct sample2));
    pr_view(stack_depth, "%30s : %d\n", offsetof(struct sample2, a));
    pr_view(stack_depth, "%30s : %d\n", offsetof(struct sample2, b));
    pr_view(stack_depth, "%30s : %d\n", offsetof(struct sample2, c));
    pr_view(stack_depth, "%30s : %d\n", offsetof(struct sample2, d));
    pr_view(stack_depth, "%30s : %d\n\n", offsetof(struct sample2, e));

    pr_view(stack_depth, "%30s : %d\n", offsetofend(struct sample2, a));
    pr_view(stack_depth, "%30s : %d\n", offsetofend(struct sample2, b));
    pr_view(stack_depth, "%30s : %d\n", offsetofend(struct sample2, c));
    pr_view(stack_depth, "%30s : %d\n", offsetofend(struct sample2, d));
    pr_view(stack_depth, "%30s : %d\n\n", offsetofend(struct sample2, e));

    pr_fn_end(stack_depth);
}

static void _ptr_container_of_test(void)
{
    pr_fn_start(stack_depth);

    struct sample sample_st;

    pr_view(stack_depth, "%30s : %d\n", sizeof(struct sample));
    pr_view(stack_depth, "%30s : %p\n", (void*)&sample_st);
    pr_view(stack_depth, "%30s : %p\n"
                    , container_of(&sample_st.a, struct sample, a));
    pr_view(stack_depth, "%30s : %p\n"
                    , container_of(&sample_st.b, struct sample, b));
    pr_view(stack_depth, "%30s : %p\n"
                    , container_of(&sample_st.c, struct sample, c));
    pr_view(stack_depth, "%30s : %p\n"
                    , container_of(&sample_st.d, struct sample, d));

    pr_fn_end(stack_depth);
}

static void _ptr_attribute_test(void)
{
    pr_fn_start(stack_depth);

    pr_view(stack_depth, "%30s : %d\n", sizeof(struct attr));
    pr_view(stack_depth, "%30s : %d\n", sizeof(struct attr2));

    pr_view(stack_depth, "%30s : %p\n", &attr_struct.a);
    pr_view(stack_depth, "%30s : %p\n", &attr_struct.b);
    pr_view(stack_depth, "%30s : %p\n", &attr_struct.c);
    pr_view(stack_depth, "%30s : %p\n", &attr_struct.d);
    pr_view(stack_depth, "%30s : %p\n\n", &attr_struct.e);

    pr_view(stack_depth, "%30s : %p\n", &attr2_struct.a);
    pr_view(stack_depth, "%30s : %p\n", &attr2_struct.b);
    pr_view(stack_depth, "%30s : %p\n", &attr2_struct.c);
    pr_view(stack_depth, "%30s : %p\n", &attr2_struct.d);
    pr_view(stack_depth, "%30s : %p\n\n", &attr2_struct.e);

    attr_struct.b = &attr_struct.a;
    pr_view(stack_depth, "%30s : %p\n", attr_struct.b);
    attr_struct.b = &attr_struct.c;
    pr_view(stack_depth, "%30s : %p\n", attr_struct.b);

    /*
     *  //https://kldp.org/node/96789
     * 	warning: dereference of noderef expression
     * 	warning: incorrect type in argument 1 (different address spaces)
     */
    attr2_struct.b = &attr2_struct.a;
    pr_view(stack_depth, "%30s : %p\n", attr2_struct.b);
    attr2_struct.b = &attr2_struct.c;
    pr_view(stack_depth, "%30s : %p\n", attr2_struct.b);

    pr_fn_end(stack_depth);
}

void basic_ptr_test(void)
{
    _ptr_offsetof_test();
    _ptr_container_of_test();
    _ptr_attribute_test();
}
