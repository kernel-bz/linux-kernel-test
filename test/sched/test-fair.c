// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/sched/test-fair.c
 *  Linux Kernel Fair Schedular Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test/test.h"
#include "test/debug.h"
#include <linux/sched/init.h>
#include <linux/sched/task.h>
#include <linux/sched.h>

#include <kernel/sched/sched.h>

void test_fair_set_user_nice(void)
{
    struct rq *rq;
    unsigned int cpu;
    long nice;

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    printf("Input Nice Number[%d,%d]: ", MIN_NICE, MAX_NICE);
    scanf("%ld", &nice);

    pr_fn_start_on(stack_depth);

    rq = cpu_rq(cpu);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);

    set_user_nice(rq->curr, nice);

    pr_fn_end_on(stack_depth);
}

void test_fair_tg_set_cfs_bandwidth(void)
{
    struct task_group *tg;
    u64 period, quota;
    int ret;

    pr_fn_start_on(stack_depth);

    tg = sched_test_tg_select();

    printf("Input period(ms) [1,1000]: ");
    scanf("%llu", &period);
    printf("Input quota(ms) [1,1000]: ");
    scanf("%llu", &quota);

    //period = (u64)period * NSEC_PER_USEC;  //us to ns
    period = (u64)period * NSEC_PER_MSEC;  //ms to ns
    quota = (u64)quota * NSEC_PER_MSEC;  //ms to ns

    ret = tg_set_cfs_bandwidth(tg, period, quota);

    pr_view_on(stack_depth, "%10s : %d\n", ret);
    pr_fn_end_on(stack_depth);
}

//kernel/sched/core.c
void test_fair_walk_tg_tree_from(struct task_group *from)
{
    struct task_group *parent, *child;

    pr_fn_start_on(stack_depth);

    parent = from;
down:
    pr_view_on(stack_depth, "%10s : down: %p\n", parent);
    //(*down)()
    list_for_each_entry_rcu(child, &parent->children, siblings) {
        //child = parent->children->next
        //child = child->siblings->next
        parent = child;
        goto down;
up:
        continue;
    }
    //(*up)()
    if (parent == from)
        goto out;

    child = parent;
    pr_view_on(stack_depth, "%20s : up: %p\n", parent->parent);
    parent = parent->parent;
    pr_view_on(stack_depth, "%20s : up: %p\n", child);
    if (parent)
        goto up;

out:
    pr_fn_end_on(stack_depth);
}

void test_fair_walk_tg_tree_from_new(struct task_group *from)
{
    struct task_group *parent, *child, *sib;
    pr_fn_start_on(stack_depth);

    parent = from;
again:
    pr_view_on(stack_depth, "%20s : children: %p\n", parent);
    list_for_each_entry_rcu(child, &parent->children, siblings) {
        //list_for_each_entry_prev_rcu(child2, &child->parent->children, siblings)
        list_for_each_entry_rcu(sib, &child->parent->children, siblings)
            if (child != sib)
                pr_view_on(stack_depth, "%20s : siblings: %p\n", sib);
        parent = child;
        goto again;
    }

    pr_fn_end_on(stack_depth);
}
