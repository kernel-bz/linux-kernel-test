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

struct task_struct *current_task;

struct task_group *sched_test_tg_select(void)
{
    int tg_max, idx, cnt=0;
    struct task_group *tg;

    tg_max = pr_sched_tg_view_only();
    if (tg_max == 0) {
        return &root_task_group;
    } else if (tg_max > 0) {
        __fpurge(stdin);
        printf("Enter Task Group Index Number[0,%d]: ", tg_max);
        scanf("%d", &idx);
        list_for_each_entry_rcu(tg, &task_groups, list) {
            if (cnt == idx)
                return tg;
            cnt++;
        }
    } else {
        pr_err("Please run sched_init() first!\n");
    }
    return NULL;
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
    if (!parent) return;

    tg = sched_create_group(parent);
    if (IS_ERR(tg)) {
        pr_err("sched_create_group() error!\n");
        goto _end;
    } else {
        sched_online_group(tg, parent);
    }

    pr_sched_tg_view_only();

    //pr_sched_pelt_info(root_tg->se[0]);
    //pr_sched_pelt_info(tg->se[0]);

_end:
    pr_fn_end_on(stack_depth);
}

/*
_do_fork()
  struct task_struct *p;
  p = copy_process()
    sched_fork(flags, p);
  wake_up_new_task(p)
    update_rq_clock()
      rq->clock; //10ms++
        rq->clock_task;
          rq->clock_pelt;
    post_init_entity_util_avg()
        attach_entity_cfs_rq();
    activate_task()
        enqueue_task();
*/
static void _sched_new_task_test(void)
{
    struct task_struct *p;
    struct rq *rq;
    unsigned int cpu;

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    pr_fn_start_on(stack_depth);
    //1280 Bytes
    pr_info_view_on(stack_depth, "%30s : %d\n", sizeof(struct task_struct));
    pr_info_view_on(stack_depth, "%30s : %d\n", sizeof(*p));
    pr_info_view_on(stack_depth, "%30s : %d\n", sizeof(init_task));
    p = (struct task_struct *)malloc(sizeof(*p));

    if (current_task) {
        memcpy(p, current_task, sizeof(*p));
    } else {
        memcpy(p, &init_task, sizeof(init_task));
        current_task = p;
    }

    pr_info_view_on(stack_depth, "%30s : %u\n", cpu);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cpu_rq(cpu));
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)task_rq(p));
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p);

    p->sched_task_group = sched_test_tg_select();
    p->cpu = cpu;
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->sched_task_group);
    pr_info_view_on(stack_depth, "%30s : %u\n", p->cpu);

    //kernel/sched/core.c
    if (sched_fork(0, p) == 0) {
        wake_up_new_task(p);
    }
    rq = task_rq(p);
    rq->curr = p;
    rq->cfs.curr = &p->se;
    rq->cfs.curr->exec_start = rq_clock(rq) - (sysctl_sched_wakeup_granularity * 2);
    current_task = p;

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p->sched_task_group);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->cfs.curr);
    pr_info_view_on(stack_depth, "%30s : %d\n", task_cpu(p));
    pr_info_view_on(stack_depth, "%30s : %d\n", cpu_of(rq));
    pr_info_view_on(stack_depth, "%30s : %d\n", p->prio);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->on_rq);
    pr_out_on(stack_depth, "\n");

    //pr_sched_pelt_info(&p->se);

    pr_fn_end_on(stack_depth);
}

static void _sched_current_task_info(void)
{
    struct rq *rq;
    unsigned int cpu;

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    pr_fn_start_on(stack_depth);

    rq = cpu_rq(cpu);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    pr_sched_curr_task_info(rq->curr);

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)current);
    pr_sched_curr_task_info(current);	//current_task

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
static void _sched_deactivate_task_test(void)
{
    struct task_struct *prev, *next;
    struct rq *rq;
    int cpu;

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    pr_fn_start_on(stack_depth);

    rq = cpu_rq(cpu);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    prev = rq->curr;

    deactivate_task(rq, prev, DEQUEUE_SLEEP | DEQUEUE_NOCLOCK);

    //pr_sched_pelt_info(&rq->curr->se);

    pr_fn_end_on(stack_depth);
}

/*
   queued = task_on_rq_queued(p);
   running = task_current(rq, p);
   if (queued) {
        dequeue_task(rq, p, queue_flags);
        __setscheduler(rq, p, attr, pi);
        enqueue_task(rq, p, queue_flags);
   }
   if (running) {
        put_prev_task(rq, p);
        __setscheduler(rq, p, attr, pi);
        set_next_task(rq, p);
   }
 */
static void _sched_setscheduler_test(void)
{
    struct rq *rq;
    struct task_struct *p;
    int i, policy, cpu, prio, user;
    int ret;
    struct sched_param param;
    char *spolicy[] = { "SCHED_NORMAL", "SCHED_FIFO", "SCHED_RR"
            , "SCHED_BATCH", "SCHED_ISO", "SCHED_IDLE", "SCHED_DEADLINE" };

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    rq = cpu_rq(cpu);
    p = rq->curr;
    if (!p) {
        pr_warn("Please run sched_init and wake_up_new_task first!\n");
        return;
    }

    for (i=SCHED_NORMAL; i <= SCHED_DEADLINE; i++)
        printf("%d: %s\n", i, spolicy[i]);
    printf("Input Policy Number[%d,%d]: ", SCHED_NORMAL, SCHED_DEADLINE);
    scanf("%d", &policy);

    //if policy==RT then prio>=0 and prio<=99
    //if policy==CFS then prio=0
    printf("Input Priority Number[0,%d]: ", MAX_RT_PRIO-1);
    scanf("%d", &prio);

    printf("Select User Check[0,1]: ");
    scanf("%d", &user);

    pr_fn_start_on(stack_depth);

    param.sched_priority = prio;
    if (user) {
        ret = sched_setscheduler_check(p, policy, &param);
        //_sched_setscheduler(p, policy, param, true);	//user check
    } else {
        ret = sched_setscheduler_nocheck(p, policy, &param);
        //_sched_setscheduler(p, policy, param, false);	//user nocheck
    }

    pr_info_view_on(stack_depth, "%20s : %d\n", ret);

    pr_fn_end_on(stack_depth);
}

static void _sched_schedule_test(void)
{
    int cpu;
    struct rq *rq;

#if 0
_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;
#endif //0

    cpu = smp_processor_id();

    rq = cpu_rq(cpu);
    if (!rq->curr) {
        pr_warn("Please run sched_init and wake_up_new_task first!\n");
        return;
    }

    //kernel/sched/core.c
    schedule();
}

static void _sched_wake_up_process_test(void)
{
    struct rq *rq;
    struct task_struct *p;
    int cpu;

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    rq = cpu_rq(cpu);
    p = rq->curr;
    if (!p) {
        pr_warn("Please run sched_init and wake_up_new_task first!\n");
        return;
    }

    wake_up_process(p);
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
    int i, loop_cnt=20;
    int step=1;

    __fpurge(stdin);
    printf("Enter tick loop counter[1,%d]: ", loop_cnt);
    scanf("%d", &loop_cnt);

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

static void _sched_pelt_info(void)
{
    struct task_group *tg;
    struct sched_entity *se;
    struct rq *rq;
    int cpu;

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    pr_fn_start_on(stack_depth);

    tg = sched_test_tg_select();
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)tg);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)tg->se[cpu]);
    se = tg->se[cpu];
    if (!se) {
        rq = cpu_rq(cpu);
        if (!rq->curr) {
            pr_warn("Please run sched_init and wake_up_new_task first!\n");
            return;
        }
        se = &rq->curr->se;
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)&rq->curr->se);
    }
    pr_sched_pelt_info(se);

    pr_fn_end_on(stack_depth);
}

static void _sched_set_user_nice_test(void)
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
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);

    set_user_nice(rq->curr, nice);

    pr_fn_end_on(stack_depth);
}

/*
 *	dl_task_timer(*hrtimer)
 *		enqueue_task_dl(rq, p, ENQUEUE_REPLENISH);
*/
static void sched_dl_enqueue_test(void)
{
    int cpu;
    struct rq *rq;
    struct task_struct *p;

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    rq = cpu_rq(cpu);
    p = rq->curr;
    if (!p) {
        pr_warn("Please run sched_init and wake_up_new_task first!\n");
        return;
    }
    p->sched_class = &dl_sched_class;
    p->prio = -1;
    p->normal_prio = -1;
    p->dl.dl_boosted = 0;
    p->dl.dl_throttled = 1;
    p->dl.dl_period = def_dl_bandwidth.dl_period;
    p->dl.dl_runtime = def_dl_bandwidth.dl_runtime;

    update_rq_clock(rq);

    //enqueue_task_dl(rq, p, ENQUEUE_REPLENISH);
    p->sched_class->enqueue_task(rq, p, ENQUEUE_REPLENISH);

    p->sched_class = &fair_sched_class;
    p->normal_prio = 120;
    p->prio = p->normal_prio;
}

static void sched_cpudl_test(void)
{
    int i;
    struct rq *rq;

    u64 dl[] = { 100, 200, 300, 400, 10, 20, 30, 40, 3000, 2000, 4000, 1000 };
    //u64 dl[] = { 450, 600, 250, 50, 200, 300 };
    int cpu[] = { 0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3 };
    //int cpu[] = { 6, 4, 3, 2, 1, 0 };	//NR_CPUS=8, include/linux/cpumask.h
    int asize = sizeof (dl) / sizeof (dl[0]);

    rq = cpu_rq(0);
    if (!rq->rd) {
        pr_warn("Please run sched_init first!\n");
        return;
    }

    for (i=0; i<asize; i++) {
        cpudl_set(&rq->rd->cpudl, cpu[i], dl[i]);
    }
}

static void _sched_test_help(void)
{
    //help messages...
    printf("\n");
    printf("You can test the Linux kernel scheduler \n");
    printf("  at the user level as follows:\n");
    printf("\n");
    return;
}

static int _sched_basic_pelt_test_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> Basic PELT Test Menu\n");
    printf("0: exit.\n");
    printf("1: calc_global_load test.\n");
    printf("2: decay_load() test.\n");
    printf("3: update_load_avg() test.\n");
    printf("4: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_basic_pelt_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , _calc_global_load_test
        , decay_load_test
        , update_load_avg_test
        , _sched_test_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    while (1) {
        idx = _sched_basic_pelt_test_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_cfs_test_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> CFS Test Menu\n");
    printf("0: exit.\n");
    printf("1: sched pelt info.\n");
    printf("2: leaf cfs_rq info.\n");
    printf("3: set_user_nice test.\n");
    printf("4: run rebalance test.\n");
    printf("5: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_cfs_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , _sched_pelt_info
        , pr_leaf_cfs_rq_info
        , _sched_set_user_nice_test
        , sched_fair_run_rebalance	//kernel/sched/fair.c
        , _sched_test_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

    while (1) {
        idx = _sched_cfs_test_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_rt_test_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> RT Test Menu\n");
    printf("0: exit.\n");
    printf("1: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_rt_test(void)
{
    void (*fn[])(void) = { _sched_test_help
           , _sched_test_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

    while (1) {
        idx = _sched_rt_test_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_dl_test_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler --> DeadLine Test Menu\n");
    printf("0: exit.\n");
    printf("1: deadline enqueue test.\n");
    printf("2: cpudl test.\n");
    printf("3: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx > 0 && idx < asize) ? idx : -1;
}

static void _sched_dl_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , sched_dl_enqueue_test
        , sched_cpudl_test
        , _sched_test_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

    while (1) {
        idx = _sched_dl_test_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}

static int _sched_test_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Scheduler Source Test Menu\n");
    printf(" 0: exit.\n");

    printf(" 1: wake_up_new_task test.\n");
    printf(" 2: current task info.\n");
    printf(" 3: deactivate_task test.\n");
    printf(" 4: setscheduler test.\n");
    printf(" 5: schedule test.\n");
    printf(" 6: create task group test.\n");
    printf(" 7: wake up process test.\n");
    printf(" 8: task group info.\n");
    printf(" 9: task group info(detail).\n");
    printf("10: Basic PELT Test -->\n");
    printf("11: CFS Test -->\n");
    printf("12: RT Test -->\n");
    printf("13: DeadLine Test -->\n");

    printf("14: help.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx > 0 && idx < asize) ? idx : -1;
}

void sched_test(void)
{
    void (*fn[])(void) = { _sched_test_help
        , _sched_new_task_test
        , _sched_current_task_info
        , _sched_deactivate_task_test
        , _sched_setscheduler_test
        , _sched_schedule_test
        , _sched_create_group_test
        , _sched_wake_up_process_test
        , pr_sched_tg_info
        , pr_sched_tg_info_all
        , _sched_basic_pelt_test
        , _sched_cfs_test
        , _sched_rt_test
        , _sched_dl_test
        , _sched_test_help
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

    while(1) {
        idx = _sched_test_menu(asize);
        if (idx < 0) break;
        fn[idx]();
    }
}
