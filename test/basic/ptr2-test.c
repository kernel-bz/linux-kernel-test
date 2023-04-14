// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/ptr2-test.c
 *  Pointer2 Test
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

struct data {
    int a;
    struct data *next;
};

struct data *head0 = NULL;
struct data *head1 = NULL;
struct data *head2 = NULL;
struct data *head3 = NULL;
struct data **tails = &head0;

//include/linux/rcu_segcblist.h
struct dslist {
    int len;
    struct data *head;
    struct data **tails[4];
};

#define DSLIST_INIT(n) \
{ \
    .head = NULL, \
    .tails[0] = &n.head, \
    .tails[1] = &n.head, \
    .tails[2] = &n.head, \
    .tails[3] = &n.head, \
}
struct dslist dslist = DSLIST_INIT(dslist);

static void data_add(struct dslist *ds, struct data *head, int idx, int a)
{
    struct data *dp;

    dp = malloc(sizeof(struct data));
    dp->next = NULL;
    dp->a = a;

    *tails = dp;
    tails = &dp->next;
    pr_out(stack_depth, "idx: %d, dp: %p, %p, size: %u, %u\n",
                            idx, dp, &dp->next, sizeof(*dp), sizeof(*ds));
    ds->head = head;
    *ds->tails[idx] = dp;
    ds->tails[idx] = &dp->next;
}

static void data_print(struct data *head)
{
    pr_fn_start(stack_depth);
    struct data *d;

    for (d = head; d; d = d->next)
        pr_out(stack_depth, "data: %04d, %p\n", d->a, d);

    pr_fn_end(stack_depth);
}

void basic_ptr2_test(void)
{
    struct dslist *ds = &dslist;

    ds->tails[0] = &head0;
    data_add(ds, head0, 0, 0);
    data_add(ds, head0, 0, 1);
    data_add(ds, head0, 0, 2);

    ds->tails[1] = &head1;
    data_add(ds, head1, 1, 10);
    data_add(ds, head1, 1, 20);
    data_add(ds, head1, 1, 30);

    ds->tails[2] = &head2;
    data_add(ds, head2, 2, 100);
    data_add(ds, head2, 2, 200);
    data_add(ds, head2, 2, 300);

    ds->tails[3] = &head3;
    data_add(ds, head3, 3, 1000);
    data_add(ds, head3, 3, 2000);
    data_add(ds, head3, 3, 3000);

    data_print(head0);
    data_print(head1);
    data_print(head2);
    data_print(head3);

    ds = container_of(head2, struct dslist, head);
    //data_print(ds->head);
    data_print(&ds->head);
}
