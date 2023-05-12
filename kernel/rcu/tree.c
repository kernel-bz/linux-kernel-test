// SPDX-License-Identifier: GPL-2.0+
/*
 * Read-Copy Update mechanism for mutual exclusion
 *
 * Copyright IBM Corporation, 2008
 *
 * Authors: Dipankar Sarma <dipankar@in.ibm.com>
 *	    Manfred Spraul <manfred@colorfullife.com>
 *	    Paul E. McKenney <paulmck@linux.ibm.com> Hierarchical version
 *
 * Based on the original work by Paul McKenney <paulmck@linux.ibm.com>
 * and inputs from Rusty Russell, Andrea Arcangeli and Andi Kleen.
 *
 * For detailed explanation of Read-Copy Update mechanism see -
 *	Documentation/RCU
 */

#define pr_fmt(fmt) "rcu: " fmt

#include "test/debug.h"
#include "test/user-types.h"

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/smp.h>
//#include <linux/rcupdate_wait.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/sched/debug.h>
#include <linux/nmi.h>
#include <linux/atomic.h>
#include <linux/bitops.h>
#include <linux/export.h>
#include <linux/completion.h>
#include <linux/moduleparam.h>
#include <linux/percpu.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/mutex.h>
#include <linux/time.h>
#include <linux/kernel_stat.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
//#include <linux/prefetch.h>
#include <linux/delay.h>
//#include <linux/stop_machine.h>
#include <linux/random.h>
//#include <linux/trace_events.h>
//#include <linux/suspend.h>
#include <linux/ftrace.h>
//#include <linux/tick.h>
//#include <linux/sysrq.h>
//#include <linux/kprobes.h>
#include <linux/gfp.h>
#include <linux/oom.h>
//#include <linux/smpboot.h>
#include <linux/jiffies.h>
#include <linux/sched/isolation.h>
#include <linux/sched/clock.h>
//#include "../time/tick-internal.h"

#include "tree.h"
#include "rcu.h"

#include <linux/rcu_node_tree.h>

#ifdef MODULE_PARAM_PREFIX
#undef MODULE_PARAM_PREFIX
#endif
#define MODULE_PARAM_PREFIX "rcutree."

/* Data structures. */

/*
 * Steal a bit from the bottom of ->dynticks for idle entry/exit
 * control.  Initially this is for TLB flushing.
 */
#define RCU_DYNTICK_CTRL_MASK 0x1
#define RCU_DYNTICK_CTRL_CTR  (RCU_DYNTICK_CTRL_MASK + 1)
#ifndef rcu_eqs_special_exit
#define rcu_eqs_special_exit() do { } while (0)
#endif

static DEFINE_PER_CPU_SHARED_ALIGNED(struct rcu_data, rcu_data) = {
    .dynticks_nesting = 1,
    .dynticks_nmi_nesting = DYNTICK_IRQ_NONIDLE,
    .dynticks = ATOMIC_INIT(RCU_DYNTICK_CTRL_CTR),
};
struct _rcu_state _rcu_state = {
    .level = { &_rcu_state.node[0] },
    .gp_state = RCU_GP_IDLE,
    .gp_seq = (0UL - 300UL) << RCU_SEQ_CTR_SHIFT,
    .barrier_mutex = __MUTEX_INITIALIZER(_rcu_state.barrier_mutex),
    .name = RCU_NAME,
    .abbr = RCU_ABBR,
    .exp_mutex = __MUTEX_INITIALIZER(_rcu_state.exp_mutex),
    .exp_wake_mutex = __MUTEX_INITIALIZER(_rcu_state.exp_wake_mutex),
    .ofl_lock = __RAW_SPIN_LOCK_UNLOCKED(_rcu_state.ofl_lock),
};

/* Dump rcu_node combining tree at boot to verify correct setup. */
static bool dump_tree;
//module_param(dump_tree, bool, 0444);
/* By default, use RCU_SOFTIRQ instead of rcuc kthreads. */
static bool use_softirq = 1;
//module_param(use_softirq, bool, 0444);
/* Control rcu_node-tree auto-balancing at boot time. */
static bool rcu_fanout_exact;
//module_param(rcu_fanout_exact, bool, 0444);
/* Increase (but not decrease) the RCU_FANOUT_LEAF at boot time. */
static int rcu_fanout_leaf = RCU_FANOUT_LEAF;
//module_param(rcu_fanout_leaf, int, 0444);
int rcu_num_lvls __read_mostly = RCU_NUM_LVLS;
/* Number of rcu_nodes at specified level. */
int num_rcu_lvl[] = NUM_RCU_LVL_INIT;
int rcu_num_nodes __read_mostly = NUM_RCU_NODES; /* Total # rcu_nodes in use. */

/*
 * The rcu_scheduler_active variable is initialized to the value
 * RCU_SCHEDULER_INACTIVE and transitions RCU_SCHEDULER_INIT just before the
 * first task is spawned.  So when this variable is RCU_SCHEDULER_INACTIVE,
 * RCU can assume that there is but one task, allowing RCU to (for example)
 * optimize synchronize_rcu() to a simple barrier().  When this variable
 * is RCU_SCHEDULER_INIT, RCU must actually do all the hard work required
 * to detect real grace periods.  This variable is also used to suppress
 * boot-time false positives from lockdep-RCU error checking.  Finally, it
 * transitions from RCU_SCHEDULER_INIT to RCU_SCHEDULER_RUNNING after RCU
 * is fully initialized, including all of its kthreads having been spawned.
 */
int rcu_scheduler_active __read_mostly;
EXPORT_SYMBOL_GPL(rcu_scheduler_active);

/*
 * The rcu_scheduler_fully_active variable transitions from zero to one
 * during the early_initcall() processing, which is after the scheduler
 * is capable of creating new tasks.  So RCU processing (for example,
 * creating tasks for RCU priority boosting) must be delayed until after
 * rcu_scheduler_fully_active transitions from zero to one.  We also
 * currently delay invocation of any RCU callbacks until after this point.
 *
 * It might later prove better for people registering RCU callbacks during
 * early boot to take responsibility for these callbacks, but one step at
 * a time.
 */
static int rcu_scheduler_fully_active __read_mostly;

static void rcu_report_qs_rnp(unsigned long mask, struct rcu_node *rnp,
                  unsigned long gps, unsigned long flags);
static void rcu_init_new_rnp(struct rcu_node *rnp_leaf);
static void rcu_cleanup_dead_rnp(struct rcu_node *rnp_leaf);
static void rcu_boost_kthread_setaffinity(struct rcu_node *rnp, int outgoingcpu);
static void invoke_rcu_core(void);
static void rcu_report_exp_rdp(struct rcu_data *rdp);
static void sync_sched_exp_online_cleanup(int cpu);

/* rcuc/rcub kthread realtime priority */
static int kthread_prio = IS_ENABLED(CONFIG_RCU_BOOST) ? 1 : 0;
//module_param(kthread_prio, int, 0444);

/* Delay in jiffies for grace-period initialization delays, debug only. */

static int gp_preinit_delay;
//module_param(gp_preinit_delay, int, 0444);
static int gp_init_delay;
//module_param(gp_init_delay, int, 0444);
static int gp_cleanup_delay;
//module_param(gp_cleanup_delay, int, 0444);
//166



//404 lines
#define DEFAULT_RCU_BLIMIT 10     /* Maximum callbacks per rcu_do_batch ... */
#define DEFAULT_MAX_RCU_BLIMIT 10000 /* ... even during callback flood. */
static long blimit = DEFAULT_RCU_BLIMIT;
#define DEFAULT_RCU_QHIMARK 10000 /* If this many pending, ignore blimit. */
static long qhimark = DEFAULT_RCU_QHIMARK;
#define DEFAULT_RCU_QLOMARK 100   /* Once only this many pending, use blimit. */
static long qlowmark = DEFAULT_RCU_QLOMARK;

//module_param(blimit, long, 0444);
//module_param(qhimark, long, 0444);
//module_param(qlowmark, long, 0444);

static ulong jiffies_till_first_fqs = ULONG_MAX;
static ulong jiffies_till_next_fqs = ULONG_MAX;
static bool rcu_kick_kthreads;
static int rcu_divisor = 7;
//module_param(rcu_divisor, int, 0644);

/* Force an exit from rcu_do_batch() after 3 milliseconds. */
static long rcu_resched_ns = 3 * NSEC_PER_MSEC;
//module_param(rcu_resched_ns, long, 0644);

/*
 * How long the grace period must be before we start recruiting
 * quiescent-state help from rcu_note_context_switch().
 */
static ulong jiffies_till_sched_qs = ULONG_MAX;
//module_param(jiffies_till_sched_qs, ulong, 0444);
static ulong jiffies_to_sched_qs; /* See adjust_jiffies_till_sched_qs(). */
//module_param(jiffies_to_sched_qs, ulong, 0444); /* Display only! */
//435

/*
 * Make sure that we give the grace-period kthread time to detect any
 * idle CPUs before taking active measures to force quiescent states.
 * However, don't go below 100 milliseconds, adjusted upwards for really
 * large systems.
 */
static void adjust_jiffies_till_sched_qs(void)
{
    unsigned long j;

    /* If jiffies_till_sched_qs was specified, respect the request. */
    if (jiffies_till_sched_qs != ULONG_MAX) {
        WRITE_ONCE(jiffies_to_sched_qs, jiffies_till_sched_qs);
        return;
    }
    /* Otherwise, set to third fqs scan, but bound below on large system. */
    j = READ_ONCE(jiffies_till_first_fqs) +
              2 * READ_ONCE(jiffies_till_next_fqs);
    if (j < HZ / 10 + nr_cpu_ids / RCU_JIFFIES_FQS_DIV)
        j = HZ / 10 + nr_cpu_ids / RCU_JIFFIES_FQS_DIV;
    pr_info("RCU calculated value of scheduler-enlistment delay is %ld jiffies.\n", j);
    WRITE_ONCE(jiffies_to_sched_qs, j);
}
//459




//3333 lines
/*
 * Helper function for rcu_init() that initializes the _rcu_state structure.
 */
static void __init rcu_init_one(void)
{
    pr_fn_start_on(stack_depth);

    static const char * const buf[] = RCU_NODE_NAME_INIT;
    static const char * const fqs[] = RCU_FQS_NAME_INIT;
    static struct lock_class_key rcu_node_class[RCU_NUM_LVLS];
    static struct lock_class_key rcu_fqs_class[RCU_NUM_LVLS];

    int levelspread[RCU_NUM_LVLS];		/* kids/node in each level. */
    int cpustride = 1;
    int i;
    int j;
    struct rcu_node *rnp;

    BUILD_BUG_ON(RCU_NUM_LVLS > ARRAY_SIZE(buf));  /* Fix buf[] init! */

    pr_view_on(stack_depth, "%20s : %d\n", RCU_NUM_LVLS);
    pr_view_on(stack_depth, "%20s : %d\n", ARRAY_SIZE(buf));
    pr_view_on(stack_depth, "%20s : %d\n", rcu_num_lvls);

    /* Silence gcc 4.8 false positive about array index out of range. */
    if (rcu_num_lvls <= 0 || rcu_num_lvls > RCU_NUM_LVLS)
        panic("rcu_init_one: rcu_num_lvls out of range");

    /* Initialize the level-tracking arrays. */

    for (i = 1; i < rcu_num_lvls; i++)
        _rcu_state.level[i] =
            _rcu_state.level[i - 1] + num_rcu_lvl[i - 1];
    rcu_init_levelspread(levelspread, num_rcu_lvl);

    /* Initialize the elements themselves, starting from the leaves. */

    for (i = rcu_num_lvls - 1; i >= 0; i--) {
        cpustride *= levelspread[i];
        rnp = _rcu_state.level[i];
        pr_view_on(stack_depth, "%25s : %d\n", i);
        pr_view_on(stack_depth, "%25s : %d\n", cpustride);
        for (j = 0; j < num_rcu_lvl[i]; j++, rnp++) {
            pr_view_on(stack_depth, "%30s : %d\n", j);

            raw_spin_lock_init(&ACCESS_PRIVATE(rnp, lock));
            //lockdep_set_class_and_name(&ACCESS_PRIVATE(rnp, lock),
            //               &rcu_node_class[i], buf[i]);
            raw_spin_lock_init(&rnp->fqslock);
            //lockdep_set_class_and_name(&rnp->fqslock,
            //               &rcu_fqs_class[i], fqs[i]);
            rnp->gp_seq = _rcu_state.gp_seq;
            rnp->gp_seq_needed = _rcu_state.gp_seq;
            rnp->completedqs = _rcu_state.gp_seq;
            rnp->qsmask = 0;
            rnp->qsmaskinit = 0;
            rnp->grplo = j * cpustride;
            rnp->grphi = (j + 1) * cpustride - 1;
            if (rnp->grphi >= nr_cpu_ids)
                rnp->grphi = nr_cpu_ids - 1;
            if (i == 0) {
                rnp->grpnum = 0;
                rnp->grpmask = 0;
                rnp->parent = NULL;
            } else {
                rnp->grpnum = j % levelspread[i - 1];
                rnp->grpmask = BIT(rnp->grpnum);
                rnp->parent = _rcu_state.level[i - 1] +
                          j / levelspread[i - 1];
            }
            rnp->level = i;
            INIT_LIST_HEAD(&rnp->blkd_tasks);
            //rcu_init_one_nocb(rnp);
            init_waitqueue_head(&rnp->exp_wq[0]);
            init_waitqueue_head(&rnp->exp_wq[1]);
            init_waitqueue_head(&rnp->exp_wq[2]);
            init_waitqueue_head(&rnp->exp_wq[3]);
            spin_lock_init(&rnp->exp_lock);
        }
    }

    //init_swait_queue_head(&_rcu_state.gp_wq);
    //init_swait_queue_head(&_rcu_state.expedited_wq);
    rnp = rcu_first_leaf_node();
    for_each_possible_cpu(i) {
        while (i > rnp->grphi)
            rnp++;
        per_cpu_ptr(&rcu_data, i)->mynode = rnp;
        //rcu_boot_init_percpu_data(i);
    }

    pr_fn_end_on(stack_depth);
}
//3415
/*
 * Compute the rcu_node tree geometry from kernel parameters.  This cannot
 * replace the definitions in tree.h because those are needed to size
 * the ->node array in the _rcu_state structure.
 */
static void __init rcu_init_geometry(void)
{
    pr_fn_start_on(stack_depth);

    ulong d;
    int i;
    int rcu_capacity[RCU_NUM_LVLS];

    /*
     * Initialize any unspecified boot parameters.
     * The default values of jiffies_till_first_fqs and
     * jiffies_till_next_fqs are set to the RCU_JIFFIES_TILL_FORCE_QS
     * value, which is a function of HZ, then adding one for each
     * RCU_JIFFIES_FQS_DIV CPUs that might be on the system.
     */
    d = RCU_JIFFIES_TILL_FORCE_QS + nr_cpu_ids / RCU_JIFFIES_FQS_DIV;
    if (jiffies_till_first_fqs == ULONG_MAX)
        jiffies_till_first_fqs = d;
    if (jiffies_till_next_fqs == ULONG_MAX)
        jiffies_till_next_fqs = d;
    adjust_jiffies_till_sched_qs();

    pr_view_on(stack_depth, "%30s : %d\n", RCU_NUM_LVLS);
    pr_view_on(stack_depth, "%30s : %d\n", nr_cpu_ids);
    pr_view_on(stack_depth, "%30s : %d\n", RCU_JIFFIES_TILL_FORCE_QS);
    pr_view_on(stack_depth, "%30s : %d\n", RCU_JIFFIES_FQS_DIV);
    pr_view_on(stack_depth, "%30s : %d\n", d);
    pr_view_on(stack_depth, "%30s : %d\n", jiffies_till_next_fqs);

    /* If the compile-time values are accurate, just leave. */
    if (rcu_fanout_leaf == RCU_FANOUT_LEAF &&
        nr_cpu_ids == NR_CPUS)
        return;
    pr_info("Adjusting geometry for rcu_fanout_leaf=%d, nr_cpu_ids=%u\n",
        rcu_fanout_leaf, nr_cpu_ids);

    /*
     * The boot-time rcu_fanout_leaf parameter must be at least two
     * and cannot exceed the number of bits in the rcu_node masks.
     * Complain and fall back to the compile-time values if this
     * limit is exceeded.
     */
    if (rcu_fanout_leaf < 2 ||
        rcu_fanout_leaf > sizeof(unsigned long) * 8) {
        rcu_fanout_leaf = RCU_FANOUT_LEAF;
        WARN_ON(1);
        return;
    }

    /*
     * Compute number of Nodes that can be handled an rcu_node tree
     * with the given number of levels.
     */
    rcu_capacity[0] = rcu_fanout_leaf;
    for (i = 1; i < RCU_NUM_LVLS; i++)
        rcu_capacity[i] = rcu_capacity[i - 1] * RCU_FANOUT;

    /*
     * The tree must be able to accommodate the configured number of CPUs.
     * If this limit is exceeded, fall back to the compile-time values.
     */
    if (nr_cpu_ids > rcu_capacity[RCU_NUM_LVLS - 1]) {
        rcu_fanout_leaf = RCU_FANOUT_LEAF;
        WARN_ON(1);
        return;
    }

    /* Calculate the number of levels in the tree. */
    for (i = 0; nr_cpu_ids > rcu_capacity[i]; i++) {
    }
    rcu_num_lvls = i + 1;

    /* Calculate the number of rcu_nodes at each level of the tree. */
    for (i = 0; i < rcu_num_lvls; i++) {
        int cap = rcu_capacity[(rcu_num_lvls - 1) - i];
        num_rcu_lvl[i] = DIV_ROUND_UP(nr_cpu_ids, cap);
    }

    /* Calculate the total number of rcu_node structures. */
    rcu_num_nodes = 0;
    for (i = 0; i < rcu_num_lvls; i++)
        rcu_num_nodes += num_rcu_lvl[i];

    pr_fn_end_on(stack_depth);
}
//3495




//3520 lines
void __init kernel_rcu_init(void)
{
    pr_fn_start_on(stack_depth);

    int cpu;

    //rcu_early_boot_tests();

    //rcu_bootup_announce();
    rcu_init_geometry();
    rcu_init_one();

#if 0
    if (dump_tree)
        rcu_dump_rcu_node_tree();
    if (use_softirq)
        open_softirq(RCU_SOFTIRQ, rcu_core_si);

    /*
     * We don't need protection against CPU-hotplug here because
     * this is called early in boot, before either interrupts
     * or the scheduler are operational.
     */
    pm_notifier(rcu_pm_notify, 0);
    for_each_online_cpu(cpu) {
        rcutree_prepare_cpu(cpu);
        rcu_cpu_starting(cpu);
        rcutree_online_cpu(cpu);
    }

    /* Create workqueue for expedited GPs and for Tree SRCU. */
    rcu_gp_wq = alloc_workqueue("rcu_gp", WQ_MEM_RECLAIM, 0);
    WARN_ON(!rcu_gp_wq);
    rcu_par_gp_wq = alloc_workqueue("rcu_par_gp", WQ_MEM_RECLAIM, 0);
    WARN_ON(!rcu_par_gp_wq);
    srcu_init();
#endif

    pr_fn_end_on(stack_depth);
}
