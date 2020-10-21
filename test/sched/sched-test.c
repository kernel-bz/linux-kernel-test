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

struct task_group *parent_tg;

/*
init/main.c:585:        sched_init();
init/main.c:1068:       sched_init_smp();
*/
static void _sched_init_test(void)
{
    pr_fn_start();
    sched_init();
    pr_fn_end();
}

static void _sched_task_group_view(void)
{
    struct task_group *tg;
    int cpu;

    printf("\n==> all of the task_group\n");
    printf("--> root_task_group : %p\n", (void*)&root_task_group);
    printf("--> parent_tg : %p\n", (void*)parent_tg);
    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        printf("--> tg : %p\n", (void*)tg);
        for_each_possible_cpu(cpu) {
            struct rq *rq;
            rq = cpu_rq(cpu);
            pr_info_view("%20s : %d\n", cpu);
            pr_info_view("%20s : %p\n", (void*)rq);

            struct cfs_rq *cfs_rq = tg->cfs_rq[cpu_of(rq)];
            struct sched_entity *se;
            pr_info_view("%20s : %p\n", (void*)cfs_rq);
            if(cfs_rq) {
                pr_info_view("%30s : %p\n", (void*)cfs_rq->curr);
                pr_info_view("%30s : %p\n", (void*)cfs_rq->next);
                pr_info_view("%30s : %d\n", cfs_rq->nr_running);
                pr_info_view("%30s : %d\n", cfs_rq->on_list);
                if (cfs_rq->on_list > 0) {
                    int i=0;
                    struct rb_node *next = rb_first_cached(&cfs_rq->tasks_timeline);
                    while (next) {
                        se = rb_entry(next, struct sched_entity, run_node);
                        pr_info_view("%40s : %d\n", i++);
                        pr_info_view("%40s : %p\n", (void*)se->parent);
                        pr_info_view("%40s : %p\n", (void*)se->cfs_rq);
                        pr_info_view("%40s : %p\n", (void*)se->my_q);
                        pr_info_view("%40s : %llu\n", se->vruntime);
                        pr_info_view("%40s : %d\n", se->on_rq);
                        next = rb_next(&se->run_node);
                    }
                }
            }
            se = tg->se[cpu_of(rq)];
            pr_info_view("%20s : %p\n", (void*)se);
            if (se) {
                pr_info_view("%30s : %d\n", se->depth);
                pr_info_view("%30s : %p\n", (void*)se->parent);
                pr_info_view("%30s : %p\n", (void*)se->cfs_rq);
                pr_info_view("%30s : %p\n", (void*)se->my_q);
                pr_info_view("%30s : %d\n", se->on_rq);
            }

        } //cpu
        printf("\n");
    } //tg
    rcu_read_unlock();
}

/*
 * kernel/sys.c
 * 	ksys_setsid(void)
 * 		kernel/sched/autogroup.c
 * 		sched_autogroup_create_attach(struct task_struct *p)
 *	 		autogroup_create();
 */
static void _sched_create_group_test(void)
{
    struct task_group *parent;
    struct task_group *tg;

    pr_fn_start();

    pr_info_view("%30s : %p\n", (void*)&root_task_group);
    parent = (parent_tg) ? parent_tg : &root_task_group;
    pr_info_view("%30s : %p\n", (void*)parent);

    tg = sched_create_group(parent);
    if (IS_ERR(tg)) {
        pr_err("sched_create_group() error!\n");
    } else {
        sched_online_group(tg, parent);
        if (!parent_tg) parent_tg = tg;
        //parent_tg = tg;
    }

    pr_info_view("%30s : %p\n", (void*)parent_tg);

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
    p = (struct task_struct *)malloc(sizeof(*p));

    //rq = task_rq(p);
    rq = this_rq();
    if (rq->curr) {
        memcpy(p, rq->curr, sizeof(*p));
    } else {
        memcpy(p, &init_task, sizeof(init_task));
        //p = &init_task;
        p->sched_task_group = parent_tg;
        rq->curr = p;
    }

    printf("\n");
    pr_info_view("%30s : %p\n", (void*)p->sched_task_group);
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
    pr_info_view("%30s : %p\n", (void*)p->sched_task_group);
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
    pr_info_view("%30s : %u\n", rq->nr_running);
    pr_info_view("%30s : %p\n", (void*)rq->curr->se.cfs_rq);
    pr_info_view("%30s : %p\n", (void*)rq->curr->se.cfs_rq->curr);
    prev = rq->curr;

    deactivate_task(rq, prev, DEQUEUE_SLEEP | DEQUEUE_NOCLOCK);

    pr_info_view("%30s : %p\n", (void*)rq->curr);
    pr_info_view("%30s : %u\n", rq->nr_running);
    pr_info_view("%30s : %p\n", (void*)rq->curr->se.cfs_rq);
    pr_info_view("%30s : %p\n", (void*)rq->curr->se.cfs_rq->curr);

   pr_fn_end();
}

static int _sched_test_menu(int pidx, int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[%d]--> Scheduler Source Test Menu\n", pidx);
    printf("0: help.\n");
    printf("1: sched task group view.\n");
    printf("2: decay load test.\n");
    printf("3: update load_avg test.\n");
    printf("4: sched_init test.\n");
    printf("5: sched_create_group test.\n");
    printf("6: activate_task test.\n");
    printf("7: deactivate_task test.\n");
    printf("8: exit.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

static void _sched_test_help(void)
{
    //help messages...
    printf("\n");
    printf("You can test the Linux kernel scheduler \n");
    printf("  at the user level as follows:\n");
    printf("* PELT execution procedure test\n");
    printf("* sched_init()\n");
    printf("* activate_task()\n");
    printf("* deactivate_task()\n");
    printf("\n");
    return;
}

void sched_test(int pidx)
{
    void (*fn[])(void) = { _sched_test_help
        , _sched_task_group_view
        , decay_load_test, update_load_avg_test
        , _sched_init_test
        , _sched_create_group_test
        , _activate_task_test, _deactivate_task_test
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    idx = _sched_test_menu(pidx, asize);
    if (idx >= 0) {
        fn[idx]();
        goto _retry;
    }
}
