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
#include <unistd.h>

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
    pr_fn_start_on(stack_depth);
    sched_init();
    pr_fn_end_on(stack_depth);
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
            pr_info_view_on(stack_depth, "%20s : %d\n", cpu);
            pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
            pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
            pr_info_view_on(stack_depth, "%20s : %u\n", rq->nr_running);

            struct cfs_rq *cfs_rq = tg->cfs_rq[cpu_of(rq)];
            struct sched_entity *se;
            pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
            if(cfs_rq) {
                pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->curr);
                pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->next);
                pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->nr_running);
                pr_info_view_on(stack_depth, "%30s : %d\n", cfs_rq->on_list);
                if (cfs_rq->on_list > 0) {
                    int i=0;
                    struct rb_node *next = rb_first_cached(&cfs_rq->tasks_timeline);
                    while (next) {
                        se = rb_entry(next, struct sched_entity, run_node);
                        pr_info_view_on(stack_depth, "%40s : %d\n", i++);
                        pr_info_view_on(stack_depth, "%40s : %p\n", (void*)se->parent);
                        pr_info_view_on(stack_depth, "%40s : %p\n", (void*)se->cfs_rq);
                        pr_info_view_on(stack_depth, "%40s : %p\n", (void*)se->my_q);
                        pr_info_view_on(stack_depth, "%40s : %llu\n", se->vruntime);
                        pr_info_view_on(stack_depth, "%40s : %d\n", se->on_rq);
                        next = rb_next(&se->run_node);
                    }
                }
            }
            se = tg->se[cpu_of(rq)];
            pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
            if (se) {
                pr_info_view_on(stack_depth, "%30s : %d\n", se->depth);
                pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->parent);
                pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->cfs_rq);
                pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->my_q);
                pr_info_view_on(stack_depth, "%30s : %d\n", se->on_rq);
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

    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&root_task_group);
    parent = (parent_tg) ? parent_tg : &root_task_group;
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)parent);

    tg = sched_create_group(parent);
    if (IS_ERR(tg)) {
        pr_err("sched_create_group() error!\n");
    } else {
        sched_online_group(tg, parent);
        if (!parent_tg) parent_tg = tg;
        //parent_tg = tg;
    }

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)parent_tg);

    pr_fn_end_on(stack_depth);
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

    pr_fn_start_on(stack_depth);
    //1280 Bytes
    pr_info_view_on(stack_depth, "%30s : %d\n", sizeof(struct task_struct));
    pr_info_view_on(stack_depth, "%30s : %d\n", sizeof(*p));
    pr_info_view_on(stack_depth, "%30s : %d\n", sizeof(init_task));
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
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->sched_task_group);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->prio);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->on_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&p->se);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->se.cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->se.parent);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->se.on_rq);
    printf("\n");

    if (sched_fork(0, p) == 0) {
        //kernel/sched/core.c
        activate_task(rq, p, flags);
    }
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->sched_task_group);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->prio);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->on_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&p->se);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->se.cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->se.parent);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->se.on_rq);
    printf("\n");
    pr_info_view_on(stack_depth, "%30s : %u\n", p->se.cfs_rq->nr_running);
    pr_info_view_on(stack_depth, "%30s : %u\n", p->se.cfs_rq->h_nr_running);
    pr_info_view_on(stack_depth, "%30s : %u\n", rq->nr_running);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->rd);
    printf("\n");
    pr_fn_end_on(stack_depth);
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

    pr_fn_start_on(stack_depth);

    //cpu = smp_processor_id();
    cpu = 0;
    rq = cpu_rq(cpu);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    rq = this_rq();
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr);
    pr_info_view_on(stack_depth, "%30s : %u\n", rq->nr_running);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr->se.cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr->se.cfs_rq->curr);
    prev = rq->curr;

    deactivate_task(rq, prev, DEQUEUE_SLEEP | DEQUEUE_NOCLOCK);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr);
    pr_info_view_on(stack_depth, "%30s : %u\n", rq->nr_running);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr->se.cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr->se.cfs_rq->curr);

   pr_fn_end_on(stack_depth);
}

//tools/testing/selftests/timers/
/*
 * kernel/time/tick_common.c
 * 	tick_handle_periodic()
 * 	  tick_periodic(cpu)
 * 		if (timer_cpu == cpu)
 *        do_timer(ticks) //ticks==1
 *          jiffies += ticks;
 * 	        calc_global_load();
 * 	    update_process_times()
 *        scheduler_tick()
 *			task_tick_fair();
 * 			calc_global_load_tick();
 */
static void _scheduler_tick_test(void)
{
    //u32 tick = 1 / HZ;	//1/100 == 0.01s == 10ms
    int i, dbase, dlevel, loop_cnt=10;

    __fpurge(stdin);
    printf("Enter Debug Base Number[0,%d]: ", DebugBase);
    scanf("%d", &dbase);
    printf("Enter Debug Level Number[0,%d]: ", DebugLevel);
    scanf("%d", &dlevel);
    printf("Enter tick loop counter[1,%d]: ", loop_cnt);
    scanf("%d", &loop_cnt);

    DebugLevel = dlevel;
    DebugBase = dbase;

    pr_fn_start_on(stack_depth);

    struct rq *this_rq = this_rq();
    pr_info_view_on(stack_depth, "%30s : %lu\n", this_rq->calc_load_update);
    pr_info_view_on(stack_depth, "%30s : %lu\n", READ_ONCE(calc_load_update));
    this_rq->calc_load_update = READ_ONCE(calc_load_update);
    pr_info_view_on(stack_depth, "%30s : %lu\n", this_rq->calc_load_update);
    jiffies = this_rq->calc_load_update;
    pr_info_view_on(stack_depth, "%30s : %ld\n", atomic_long_read(&calc_load_tasks));	//delta active

    for (i=0; i<loop_cnt; i++) {
        pr_info_view_on(stack_depth, "%10s : %d\n", i);
        pr_info_view_on(stack_depth, "%10s : %lu\n", jiffies);

        //kernel/sched/loadavg.c
        calc_global_load(0);	//void

        //kernel/sched/core.c
        scheduler_tick();

        //usleep(tick * 1000);

        jiffies += HZ;	//1s
    }

    pr_fn_end_on(stack_depth);
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

static int _sched_statis_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> Statistics Test Menu\n");
    printf("0: help.\n");
    printf("1: sched task group view.\n");
    printf("2: scheduler_tick() test.\n");
    printf("3: decay_load() test.\n");
    printf("4: update_load_avg() test.\n");
    printf("5: exit.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

static void _sched_statis_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , _sched_task_group_view
        , _scheduler_tick_test
        , decay_load_test, update_load_avg_test
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    idx = _sched_statis_menu(asize);
    if (idx >= 0) {
        fn[idx]();
        goto _retry;
    }
}

static int _sched_test_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler Source Test Menu\n");
    printf("0: help.\n");
    printf("1: sched task group view.\n");
    printf("2: sched_init test.\n");
    printf("3: sched_create_group test.\n");
    printf("4: activate_task test.\n");
    printf("5: deactivate_task test.\n");
    printf("6: Statistics Test -->\n");
    printf("7: exit.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

void sched_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , _sched_task_group_view
        , _sched_init_test, _sched_create_group_test
        , _activate_task_test, _deactivate_task_test
        , _sched_statis_test
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    idx = _sched_test_menu(asize);
    if (idx >= 0) {
        fn[idx]();
        goto _retry;
    }
}
