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

struct task_group *root_tg;

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

struct task_group *sched_test_tg_select(void)
{
    int tg_max, idx, cnt=0;
    struct task_group *tg;

    if (root_tg) {
        tg_max = pr_sched_tg_view_only();
        __fpurge(stdin);
        printf("Enter Task Group Index Number[0,%d]: ", tg_max-1);
        scanf("%d", &idx);
        list_for_each_entry_rcu(tg, &task_groups, list) {
            if (cnt == idx) {
                return tg;
            }
            cnt++;
        }
    }
    return &root_task_group;
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
    struct task_group *parent = &root_task_group;
    struct task_group *tg;
    int cnt=0, idx, tg_max;

    pr_fn_start_on(stack_depth);

    parent = sched_test_tg_select();

    tg = sched_create_group(parent);
    if (IS_ERR(tg)) {
        pr_err("sched_create_group() error!\n");
        goto _end;
    } else {
        sched_online_group(tg, parent);
        if (!root_tg) {
            root_tg = tg;
            //root_tg->parent = &root_task_group;
        }
    }

    pr_sched_tg_view_only();

    //pr_sched_se_pelt_info(root_tg->se[0]);
    //pr_sched_se_pelt_info(tg->se[0]);

_end:
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
    //rq = this_rq();
    rq = cpu_rq(0);
    if (rq->curr) {
        memcpy(p, rq->curr, sizeof(*p));
    } else {
        memcpy(p, &init_task, sizeof(init_task));
        rq->curr = p;
    }
    p->sched_task_group = sched_test_tg_select();

    pr_out_on(stack_depth, "\n");
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)root_tg);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->sched_task_group);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->prio);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->on_rq);
    pr_out_on(stack_depth, "\n");

    if (sched_fork(0, p) == 0) {
        //kernel/sched/core.c
        activate_task(rq, p, flags);
    }
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)root_tg);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->sched_task_group);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->prio);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->on_rq);
    pr_out_on(stack_depth, "\n");

    pr_sched_se_pelt_info(&p->se);

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
    //struct rq_flags rf;
    struct rq *rq;
    int cpu;

    pr_fn_start_on(stack_depth);

    //cpu = smp_processor_id();
    cpu = 0;
    rq = cpu_rq(cpu);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    rq = this_rq();
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    prev = rq->curr;

    deactivate_task(rq, prev, DEQUEUE_SLEEP | DEQUEUE_NOCLOCK);

    pr_sched_se_pelt_info(&rq->curr->se);

    pr_fn_end_on(stack_depth);
}

#if 0
//cat /proc/loadavg
//fs/proc/loadavg.c
static int loadavg_proc_show(struct seq_file *m, void *v)
{
    unsigned long avnrun[3];

    get_avenrun(avnrun, FIXED_1/200, 0);

    seq_printf(m, "%lu.%02lu %lu.%02lu %lu.%02lu %ld/%d %d\n",
        LOAD_INT(avnrun[0]), LOAD_FRAC(avnrun[0]),
        LOAD_INT(avnrun[1]), LOAD_FRAC(avnrun[1]),
        LOAD_INT(avnrun[2]), LOAD_FRAC(avnrun[2]),
        nr_running(), nr_threads,
        idr_get_cursor(&task_active_pid_ns(current)->idr) - 1);
    return 0;
}
#endif //0

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
static void _calc_global_load_test(void)
{
    //u32 tick = 1 / HZ;	//1/100 == 0.01s == 10ms
    int i, dbase, dlevel, loop_cnt=20;
    int step=1;

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
    pr_info_view_on(stack_depth, "%30s : %d\n", HZ);

    for (i=0; i<loop_cnt; i++) {
        pr_out_on(stack_depth, "\n");
        pr_info_view_on(stack_depth, "%20s : %d\n", i);
        pr_info_view_on(stack_depth, "%20s : %lu\n", jiffies);
        pr_info_view_on(stack_depth, "%20s : %u\n", this_rq->nr_running);

        //kernel/sched/loadavg.c
        calc_global_load(0);	//void

        //kernel/sched/core.c
        //scheduler_tick();
        if(this_rq->nr_running >= 10) step = -1;
        else if(this_rq->nr_running <= 0) step = 1;
        this_rq->nr_running += step;
        calc_global_load_tick(this_rq);

        //usleep(tick * 1000);

        //jiffies++;		//10ms
        jiffies += HZ;		//1s
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
    printf("2: calc_global_load test.\n");
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
        , pr_sched_tg_info
        , _calc_global_load_test
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
    printf("1: sched task group info.\n");
    printf("2: sched task group info(detail).\n");
    printf("3: sched_init test.\n");
    printf("4: sched_create_group test.\n");
    printf("5: activate_task test.\n");
    printf("6: deactivate_task test.\n");
    printf("7: Statistics Test -->\n");
    printf("8: exit.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

void sched_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , pr_sched_tg_info, pr_sched_tg_info_all
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
