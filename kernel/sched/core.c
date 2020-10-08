// SPDX-License-Identifier: GPL-2.0-only
/*
 *  kernel/sched/core.c
 *
 *  Core kernel scheduler code and related syscalls
 *
 *  Copyright (C) 1991-2002  Linus Torvalds
 */
#include "sched.h"

#include <linux/nospec.h>
#include <linux/kcov.h>

#include <linux/topology.h>

//#include <asm/switch_to.h>
//#include <asm/tlb.h>

//#include "../workqueue_internal.h"
//#include "../smpboot.h"

#include "pelt.h"

#define CREATE_TRACE_POINTS
//#include <trace/events/sched.h>

DEFINE_PER_CPU_SHARED_ALIGNED(struct rq, runqueues);

#if defined(CONFIG_SCHED_DEBUG) && defined(CONFIG_JUMP_LABEL)
/*
 * Debugging: various feature bits
 *
 * If SCHED_DEBUG is disabled, each compilation unit has its own copy of
 * sysctl_sched_features, defined in sched.h, to allow constants propagation
 * at compile time and compiler optimization based on features default.
 */
#define SCHED_FEAT(name, enabled)	\
    (1UL << __SCHED_FEAT_##name) * enabled |
const_debug unsigned int sysctl_sched_features =
#include "features.h"
    0;
#undef SCHED_FEAT
#endif

/*
 * Number of tasks to iterate in a single balance run.
 * Limited because this is done with IRQs disabled.
 */
const_debug unsigned int sysctl_sched_nr_migrate = 32;

/*
 * period over which we measure -rt task CPU usage in us.
 * default: 1s
 */
unsigned int sysctl_sched_rt_period = 1000000;

__read_mostly int scheduler_running;

/*
 * part of the period that we allow rt tasks to run in us.
 * default: 0.95s
 */
int sysctl_sched_rt_runtime = 950000;
//74 lines





//140 lines
/*
 * RQ-clock updating methods:
 */

static void update_rq_clock_task(struct rq *rq, s64 delta)
{
/*
 * In theory, the compile should just see 0 here, and optimize out the call
 * to sched_rt_avg_update. But I don't trust it...
 */
    s64 __maybe_unused steal = 0, irq_delta = 0;

#ifdef CONFIG_IRQ_TIME_ACCOUNTING
    irq_delta = irq_time_read(cpu_of(rq)) - rq->prev_irq_time;

    /*
     * Since irq_time is only updated on {soft,}irq_exit, we might run into
     * this case when a previous update_rq_clock() happened inside a
     * {soft,}irq region.
     *
     * When this happens, we stop ->clock_task and only update the
     * prev_irq_time stamp to account for the part that fit, so that a next
     * update will consume the rest. This ensures ->clock_task is
     * monotonic.
     *
     * It does however cause some slight miss-attribution of {soft,}irq
     * time, a more accurate solution would be to update the irq_time using
     * the current rq->clock timestamp, except that would require using
     * atomic ops.
     */
    if (irq_delta > delta)
        irq_delta = delta;

    rq->prev_irq_time += irq_delta;
    delta -= irq_delta;
#endif
#ifdef CONFIG_PARAVIRT_TIME_ACCOUNTING
    if (static_key_false((&paravirt_steal_rq_enabled))) {
        steal = paravirt_steal_clock(cpu_of(rq));
        steal -= rq->prev_steal_time_rq;

        if (unlikely(steal > delta))
            steal = delta;

        rq->prev_steal_time_rq += steal;
        delta -= steal;
    }
#endif

    rq->clock_task += delta;

#ifdef CONFIG_HAVE_SCHED_AVG_IRQ
    if ((irq_delta + steal) && sched_feat(NONTASK_CAPACITY))
        update_irq_load_avg(rq, irq_delta + steal);
#endif
    update_rq_clock_pelt(rq, delta);
}

void update_rq_clock(struct rq *rq)
{
    s64 delta;

    lockdep_assert_held(&rq->lock);

    if (rq->clock_update_flags & RQCF_ACT_SKIP)
        return;

#ifdef CONFIG_SCHED_DEBUG
    if (sched_feat(WARN_DOUBLE_CLOCK))
        SCHED_WARN_ON(rq->clock_update_flags & RQCF_UPDATED);
    rq->clock_update_flags |= RQCF_UPDATED;
#endif

    delta = sched_clock_cpu(cpu_of(rq)) - rq->clock;
    if (delta < 0)
        return;
    rq->clock += delta;
    update_rq_clock_task(rq, delta);
}
//220 lines



//499 lines
/*
 * resched_curr - mark rq's current task 'to be rescheduled now'.
 *
 * On UP this means the setting of the need_resched flag, on SMP it
 * might also involve a cross-CPU call to trigger the scheduler on
 * the target CPU.
 */
void resched_curr(struct rq *rq)
{
    struct task_struct *curr = rq->curr;
    int cpu;

    lockdep_assert_held(&rq->lock);

    if (test_tsk_need_resched(curr))
        return;

    cpu = cpu_of(rq);

    if (cpu == smp_processor_id()) {
        set_tsk_need_resched(curr);
        //set_preempt_need_resched();
        return;
    }
#if 0
    if (set_nr_and_not_polling(curr))
        smp_send_reschedule(cpu);
    else
        trace_sched_wake_idle_without_ipi(cpu);
#endif //0
}

void resched_cpu(int cpu)
{
    struct rq *rq = cpu_rq(cpu);
    unsigned long flags;

    raw_spin_lock_irqsave(&rq->lock, flags);
    if (cpu_online(cpu) || cpu == smp_processor_id())
        resched_curr(rq);
    raw_spin_unlock_irqrestore(&rq->lock, flags);
}
//541 lines





//746 lines
static void set_load_weight(struct task_struct *p, bool update_load)
{
    int prio = p->static_prio - MAX_RT_PRIO;
    struct load_weight *load = &p->se.load;

    /*
     * SCHED_IDLE tasks get minimal weight:
     */
    if (task_has_idle_policy(p)) {
        load->weight = scale_load(WEIGHT_IDLEPRIO);
        load->inv_weight = WMULT_IDLEPRIO;
        p->se.runnable_weight = load->weight;
        return;
    }

    /*
     * SCHED_OTHER tasks have to update their load when changing their
     * weight
     */
    if (update_load && p->sched_class == &fair_sched_class) {
        reweight_task(p, prio);
    } else {
        load->weight = scale_load(sched_prio_to_weight[prio]);
        load->inv_weight = sched_prio_to_wmult[prio];
        p->se.runnable_weight = load->weight;
    }
}





//2758 lines
#ifdef CONFIG_SCHEDSTATS

DEFINE_STATIC_KEY_FALSE(sched_schedstats);
static bool __initdata __sched_schedstats = false;

static void set_schedstats(bool enabled)
{
    if (enabled)
        static_branch_enable(&sched_schedstats);
    else
        static_branch_disable(&sched_schedstats);
}

void force_schedstat_enabled(void)
{
    if (!schedstat_enabled()) {
        pr_info("kernel profiling enabled schedstats, disable via kernel.sched_schedstats.\n");
        static_branch_enable(&sched_schedstats);
    }
}

static int __init setup_schedstats(char *str)
{
    int ret = 0;
    if (!str)
        goto out;

    /*
     * This code is called before jump labels have been set up, so we can't
     * change the static branch directly just yet.  Instead set a temporary
     * variable so init_schedstats() can do it later.
     */
    if (!strcmp(str, "enable")) {
        __sched_schedstats = true;
        ret = 1;
    } else if (!strcmp(str, "disable")) {
        __sched_schedstats = false;
        ret = 1;
    }
out:
    if (!ret)
        pr_warn("Unable to parse schedstats=\n");

    return ret;
}
//__setup("schedstats=", setup_schedstats);

static void __init init_schedstats(void)
{
    set_schedstats(__sched_schedstats);
}

#ifdef CONFIG_PROC_SYSCTL
int sysctl_schedstats(struct ctl_table *table, int write,
             void __user *buffer, size_t *lenp, loff_t *ppos)
{
    struct ctl_table t;
    int err;
    int state = static_branch_likely(&sched_schedstats);

    if (write && !capable(CAP_SYS_ADMIN))
        return -EPERM;

    t = *table;
    t.data = &state;
    err = proc_dointvec_minmax(&t, write, buffer, lenp, ppos);
    if (err < 0)
        return err;
    if (write)
        set_schedstats(state);
    return err;
}
#endif /* CONFIG_PROC_SYSCTL */
#else  /* !CONFIG_SCHEDSTATS */
static inline void init_schedstats(void) {}
#endif /* CONFIG_SCHEDSTATS */
//2835 lines





//2919 lines
unsigned long to_ratio(u64 period, u64 runtime)
{
    if (runtime == RUNTIME_INF)
        return BW_UNIT;

    /*
     * Doing this here saves a lot of checks in all
     * the calling paths, and returning zero seems
     * safe for them anyway.
     */
    if (period == 0)
        return 0;

    return div64_u64(runtime << BW_SHIFT, period);
}





//6541 lines
#ifdef CONFIG_CGROUP_SCHED
/*
 * Default task group.
 * Every task in system belongs to this group at bootup.
 */
struct task_group root_task_group;
LIST_HEAD(task_groups);

/* Cacheline aligned slab cache for task_group */
static struct kmem_cache *task_group_cache __read_mostly;
#endif

DECLARE_PER_CPU(cpumask_var_t, load_balance_mask);
DECLARE_PER_CPU(cpumask_var_t, select_idle_mask);

void __init sched_init(void)
{
        unsigned long ptr = 0;
        int i;

        pr_fn_start();

        //wait_bit_init();

#ifdef CONFIG_FAIR_GROUP_SCHED
        ptr += 2 * nr_cpu_ids * sizeof(void **);
#endif
#ifdef CONFIG_RT_GROUP_SCHED
        ptr += 2 * nr_cpu_ids * sizeof(void **);
#endif
        pr_info("kzalloc size = %u\n", ptr);
        if (ptr) {
                ptr = (unsigned long)kzalloc(ptr, GFP_NOWAIT);
                pr_info("addr=0x%X\n", ptr);

#ifdef CONFIG_FAIR_GROUP_SCHED
                root_task_group.se = (struct sched_entity **)ptr;
                ptr += nr_cpu_ids * sizeof(void **);
                pr_info("root_task_group.se = %p\n", root_task_group.se);

                root_task_group.cfs_rq = (struct cfs_rq **)ptr;
                ptr += nr_cpu_ids * sizeof(void **);
                pr_info("root_task_group.cfs_rq = %p\n", root_task_group.cfs_rq);
#endif /* CONFIG_FAIR_GROUP_SCHED */
#ifdef CONFIG_RT_GROUP_SCHED
                root_task_group.rt_se = (struct sched_rt_entity **)ptr;
                ptr += nr_cpu_ids * sizeof(void **);

                root_task_group.rt_rq = (struct rt_rq **)ptr;
                ptr += nr_cpu_ids * sizeof(void **);

#endif /* CONFIG_RT_GROUP_SCHED */
        }
#ifdef CONFIG_CPUMASK_OFFSTACK
        for_each_possible_cpu(i) {
                per_cpu(load_balance_mask, i) = (cpumask_var_t)kzalloc_node(
                        cpumask_size(), GFP_KERNEL, cpu_to_node(i));
                per_cpu(select_idle_mask, i) = (cpumask_var_t)kzalloc_node(
                        cpumask_size(), GFP_KERNEL, cpu_to_node(i));
        }
#endif /* CONFIG_CPUMASK_OFFSTACK */

        init_rt_bandwidth(&def_rt_bandwidth, global_rt_period(), global_rt_runtime());
        init_dl_bandwidth(&def_dl_bandwidth, global_rt_period(), global_rt_runtime());

        pr_info_view("%30s : 0x%X\n", __cpu_possible_mask.bits[0]);

#ifdef CONFIG_SMP
        init_defrootdomain();
#endif

#ifdef CONFIG_RT_GROUP_SCHED
        init_rt_bandwidth(&root_task_group.rt_bandwidth,
                        global_rt_period(), global_rt_runtime());
#endif /* CONFIG_RT_GROUP_SCHED */

#ifdef CONFIG_CGROUP_SCHED
        task_group_cache = KMEM_CACHE(task_group, 0);

        list_add(&root_task_group.list, &task_groups);
        INIT_LIST_HEAD(&root_task_group.children);
        INIT_LIST_HEAD(&root_task_group.siblings);
        autogroup_init(&init_task);
#endif /* CONFIG_CGROUP_SCHED */

        pr_info_view("%30s : 0x%X\n", __cpu_possible_mask.bits[0]);

        for_each_possible_cpu(i) {
            struct rq *rq;

            rq = cpu_rq(i);
            pr_info("cpu=%d, rq=%p\n", i, rq);

            raw_spin_lock_init(&rq->lock);
            rq->nr_running = 0;
            rq->calc_load_active = 0;
            rq->calc_load_update = jiffies + LOAD_FREQ;
            init_cfs_rq(&rq->cfs);
            init_rt_rq(&rq->rt);
            init_dl_rq(&rq->dl);
    #ifdef CONFIG_FAIR_GROUP_SCHED
            root_task_group.shares = ROOT_TASK_GROUP_LOAD;	//1024
            INIT_LIST_HEAD(&rq->leaf_cfs_rq_list);
            rq->tmp_alone_branch = &rq->leaf_cfs_rq_list;
            /*
             * How much CPU bandwidth does root_task_group get?
             *
             * In case of task-groups formed thr' the cgroup filesystem, it
             * gets 100% of the CPU resources in the system. This overall
             * system CPU resource is divided among the tasks of
             * root_task_group and its child task-groups in a fair manner,
             * based on each entity's (task or task-group's) weight
             * (se->load.weight).
             *
             * In other words, if root_task_group has 10 tasks of weight
             * 1024) and two child groups A0 and A1 (of weight 1024 each),
             * then A0's share of the CPU resource is:
             *
             *	A0's bandwidth = 1024 / (10*1024 + 1024 + 1024) = 8.33%
             *
             * We achieve this by letting root_task_group's tasks sit
             * directly in rq->cfs (i.e root_task_group->se[] = NULL).
             */
            init_cfs_bandwidth(&root_task_group.cfs_bandwidth);
            init_tg_cfs_entry(&root_task_group, &rq->cfs, NULL, i, NULL);
    #endif /* CONFIG_FAIR_GROUP_SCHED */

            rq->rt.rt_runtime = def_rt_bandwidth.rt_runtime;
    #ifdef CONFIG_RT_GROUP_SCHED
            init_tg_rt_entry(&root_task_group, &rq->rt, NULL, i, NULL);
    #endif
    #ifdef CONFIG_SMP
            rq->sd = NULL;
            rq->rd = NULL;
            rq->cpu_capacity = rq->cpu_capacity_orig = SCHED_CAPACITY_SCALE;
            rq->balance_callback = NULL;
            rq->active_balance = 0;
            rq->next_balance = jiffies;
            rq->push_cpu = 0;
            rq->cpu = i;
            rq->online = 0;
            rq->idle_stamp = 0;
            rq->avg_idle = 2*sysctl_sched_migration_cost;
            rq->max_idle_balance_cost = sysctl_sched_migration_cost;

            INIT_LIST_HEAD(&rq->cfs_tasks);

            //rq_attach_root(rq, &def_root_domain);
    #ifdef CONFIG_NO_HZ_COMMON
            rq->last_load_update_tick = jiffies;
            rq->last_blocked_load_update_tick = jiffies;
            atomic_set(&rq->nohz_flags, 0);
    #endif
    #endif /* CONFIG_SMP */
            //hrtick_rq_init(rq);
            atomic_set(&rq->nr_iowait, 0);
        }

        set_load_weight(&init_task, false);

        /*
         * The boot idle thread does lazy MMU switching as well:
         */
        //mmgrab(&init_mm);
        //enter_lazy_tlb(&init_mm, current);

        /*
         * Make us the idle thread. Technically, schedule() should not be
         * called from this thread, however somewhere below it might be,
         * but because we are the idle thread, we just pick up running again
         * when this runqueue becomes "idle".
         */
        //init_idle(current, smp_processor_id());

        calc_load_update = jiffies + LOAD_FREQ;

    #ifdef CONFIG_SMP
        //idle_thread_set_boot_cpu();
    #endif
        init_sched_fair_class();

        //CONFIG_SCHEDSTATS
        init_schedstats();

        //kernel/sched/psi.c
        //psi_init();

        //init_uclamp();

        scheduler_running = 1;

        pr_fn_end();
}






//7907 lines
/*
 * Nice levels are multiplicative, with a gentle 10% change for every
 * nice level changed. I.e. when a CPU-bound task goes from nice 0 to
 * nice 1, it will get ~10% less CPU time than another CPU-bound task
 * that remained on nice 0.
 *
 * The "10% effect" is relative and cumulative: from _any_ nice level,
 * if you go up 1 level, it's -10% CPU usage, if you go down 1 level
 * it's +10% CPU usage. (to achieve that we use a multiplier of 1.25.
 * If a task goes up by ~10% and another task goes down by ~10% then
 * the relative distance between them is ~25%.)
 */
const int sched_prio_to_weight[40] = {
 /* -20 */     88761,     71755,     56483,     46273,     36291,
 /* -15 */     29154,     23254,     18705,     14949,     11916,
 /* -10 */      9548,      7620,      6100,      4904,      3906,
 /*  -5 */      3121,      2501,      1991,      1586,      1277,
 /*   0 */      1024,       820,       655,       526,       423,
 /*   5 */       335,       272,       215,       172,       137,
 /*  10 */       110,        87,        70,        56,        45,
 /*  15 */        36,        29,        23,        18,        15,
};

/*
 * Inverse (2^32/x) values of the sched_prio_to_weight[] array, precalculated.
 *
 * In cases where the weight does not change often, we can use the
 * precalculated inverse to speed up arithmetics by turning divisions
 * into multiplications:
 */
const u32 sched_prio_to_wmult[40] = {
 /* -20 */     48388,     59856,     76040,     92818,    118348,
 /* -15 */    147320,    184698,    229616,    287308,    360437,
 /* -10 */    449829,    563644,    704093,    875809,   1099582,
 /*  -5 */   1376151,   1717300,   2157191,   2708050,   3363326,
 /*   0 */   4194304,   5237765,   6557202,   8165337,  10153587,
 /*   5 */  12820798,  15790321,  19976592,  24970740,  31350126,
 /*  10 */  39045157,  49367440,  61356676,  76695844,  95443717,
 /*  15 */ 119304647, 148102320, 186737708, 238609294, 286331153,
};

#undef CREATE_TRACE_POINTS

//end of file
