// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/sched/sched-test.c
 *  Linux Kernel Schedular Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <string.h>

#include "test/test.h"
#include "test/debug.h"
#include <linux/sched/init.h>
#include <linux/sched/task.h>
#include <linux/sched.h>

#include <kernel/sched/sched.h>

#if 0
/* Walk up scheduling entities hierarchy */
#define for_each_sched_entity(se) \
        for (; se; se = se->parent)

/* Iterate thr' all leaf cfs_rq's on a runqueue */
#define for_each_leaf_cfs_rq_safe(rq, cfs_rq, pos)			\
    list_for_each_entry_safe(cfs_rq, pos, &rq->leaf_cfs_rq_list,	\
                 leaf_cfs_rq_list)
    list_for_each_entry_safe(pos, n, head, member)			\
        for (pos = list_first_entry(head, typeof(*pos), member),	\
            n = list_next_entry(pos, member);			\
             &pos->member != (head); 					\
             pos = n, n = list_next_entry(n, member))
/*
 * Iterates the task_group tree in a bottom up fashion, see
 * list_add_leaf_cfs_rq() for details.
 */

#define for_each_domain(cpu, __sd) \
        for (__sd = rcu_dereference_check_sched_domain(cpu_rq(cpu)->sd); \
                        __sd; __sd = __sd->parent)

#define for_each_lower_domain(sd) for (; sd; sd = sd->child)
#endif

static void _sched_init_test(void)
{
    pr_fn_start();
    sched_init();
    pr_fn_end();
}

/*
_do_fork()
  struct task_struct *p;
  p = copy_process()
    sched_fork(flags, p);
  wake_up_new_task(p)
    activate_task(rq, p, ENQUEUE_NOCLOCK)
      enqueue_task(rq, p, flags)
        p->sched_class->enqueue_task(rq, p, flags)
          enqueue_task_fair(struct rq *rq, struct task_struct *p, int flags)
*/
static void _activate_task_test(void)
{
    struct task_struct *p;
    struct rq *rq;
    int flags = ENQUEUE_NOCLOCK;

    pr_fn_start();
    //1280 Bytes
    pr_info_view("%30s : %d\n", sizeof(struct task_struct));
    pr_info_view("%30s : %d\n", sizeof(*p));
    pr_info_view("%30s : %d\n", sizeof(init_task));
    p = (struct task_struct *)malloc(sizeof(init_task));

    //rq = task_rq(p);
    rq = this_rq();
    if (rq->curr) {
        memcpy(p, rq->curr, sizeof(*p));
    } else {
        memcpy(p, &init_task, sizeof(init_task));
        //p = &init_task;
        rq->curr = p;
    }

    printf("\n");
    pr_info_view("%30s : %p\n", (void*)rq);
    pr_info_view("%30s : %p\n", (void*)rq->curr);
    pr_info_view("%30s : %d\n", p->prio);
    pr_info_view("%30s : %d\n", p->on_rq);
    pr_info_view("%30s : %p\n", (void*)&p->se);
    pr_info_view("%30s : %p\n", (void*)p->se.cfs_rq);
    pr_info_view("%30s : %p\n", (void*)p->se.parent);
    pr_info_view("%30s : %d\n", p->se.on_rq);
    printf("\n");

    if (sched_fork(0, p) == 0) {
        //kernel/sched/core.c
        activate_task(rq, p, flags);
    }
    pr_info_view("%30s : %p\n", (void*)rq);
    pr_info_view("%30s : %p\n", (void*)rq->curr);
    pr_info_view("%30s : %d\n", p->prio);
    pr_info_view("%30s : %d\n", p->on_rq);
    pr_info_view("%30s : %p\n", (void*)&p->se);
    pr_info_view("%30s : %p\n", (void*)p->se.cfs_rq);
    pr_info_view("%30s : %p\n", (void*)p->se.parent);
    pr_info_view("%30s : %d\n", p->se.on_rq);
    printf("\n");
    pr_info_view("%30s : %u\n", p->se.cfs_rq->nr_running);
    pr_info_view("%30s : %u\n", p->se.cfs_rq->h_nr_running);
    pr_info_view("%30s : %u\n", rq->nr_running);
    pr_info_view("%30s : %p\n", (void*)rq->rd);
    printf("\n");
    pr_fn_end();
}

/*
 * kernel/sched/core.c
 * schedule()
 *   __schedule(false)
 *     struct task_struct *prev, *next;
 *     prev = rq->curr;
 *     deactivate_task(rq, prev, DEQUEUE_SLEEP | DEQUEUE_NOCLOCK);
 * 	   next = pick_next_task(rq, prev, &rf);
 *     rq = context_switch(rq, prev, next, &rf);
 */
static void _deactivate_task_test(void)
{
    struct task_struct *prev, *next;
    struct rq_flags rf;
    struct rq *rq;
    int cpu;

    pr_fn_start();

    //cpu = smp_processor_id();
    cpu = 0;
    rq = cpu_rq(cpu);
    pr_info_view("%30s : %p\n", (void*)rq);
    rq = this_rq();
    pr_info_view("%30s : %p\n", (void*)rq);
    pr_info_view("%30s : %p\n", (void*)rq->curr);
    prev = rq->curr;

    deactivate_task(rq, prev, DEQUEUE_SLEEP | DEQUEUE_NOCLOCK);

   pr_fn_end();
}

static void _sched_test_menu(void)
{
    printf("\n");
    printf("---------- Schedular Source Test --------------\n");
    printf("1: decay load test.\n");
    printf("2: update load_avg test.\n");
    printf("3: sched_init test.\n");
    printf("4: activate_task test.\n");
    printf("5: deactivate_task test.\n");
    printf("other: exit.\n");
    printf("\n");
}

void sched_test(void)
{
    void (*fn[])(void) = { _sched_test_menu
        , decay_load_test, update_load_avg_test
        , _sched_init_test, _activate_task_test, _deactivate_task_test
    };
    int idx = -1;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    __fpurge(stdin);
    _sched_test_menu();
    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    if(idx >= 0 && idx < asize) {
        fn[idx]();
        goto _retry;
    }
}
