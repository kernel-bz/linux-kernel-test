// SPDX-License-Identifier: GPL-2.0
/*
 * Completely Fair Scheduling (CFS) Class (SCHED_NORMAL/SCHED_BATCH)
 *
 *  Copyright (C) 2007 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 *  Interactivity improvements by Mike Galbraith
 *  (C) 2007 Mike Galbraith <efault@gmx.de>
 *
 *  Various enhancements by Dmitry Adamushko.
 *  (C) 2007 Dmitry Adamushko <dmitry.adamushko@gmail.com>
 *
 *  Group scheduling enhancements by Srivatsa Vaddagiri
 *  Copyright IBM Corporation, 2007
 *  Author: Srivatsa Vaddagiri <vatsa@linux.vnet.ibm.com>
 *
 *  Scaled math optimizations by Thomas Gleixner
 *  Copyright (C) 2007, Thomas Gleixner <tglx@linutronix.de>
 *
 *  Adaptive scheduling granularity, math enhancements by Peter Zijlstra
 *  Copyright (C) 2007 Red Hat, Inc., Peter Zijlstra
 */
#include <stdlib.h>
#include "sched.h"
#include "pelt.h"

//#include <trace/events/sched.h>

/*
 * Targeted preemption latency for CPU-bound tasks:
 *
 * NOTE: this latency value is not the same as the concept of
 * 'timeslice length' - timeslices in CFS are of variable length
 * and have no persistent notion like in traditional, time-slice
 * based scheduling concepts.
 *
 * (to see the precise effective timeslice length of your workload,
 *  run vmstat and monitor the context-switches (cs) field)
 *
 * (default: 6ms * (1 + ilog(ncpus)), units: nanoseconds)
 */
unsigned int sysctl_sched_latency			= 6000000ULL;
static unsigned int normalized_sysctl_sched_latency	= 6000000ULL;

/*
 * The initial- and re-scaling of tunables is configurable
 *
 * Options are:
 *
 *   SCHED_TUNABLESCALING_NONE - unscaled, always *1
 *   SCHED_TUNABLESCALING_LOG - scaled logarithmical, *1+ilog(ncpus)
 *   SCHED_TUNABLESCALING_LINEAR - scaled linear, *ncpus
 *
 * (default SCHED_TUNABLESCALING_LOG = *(1+ilog(ncpus))
 */
enum sched_tunable_scaling sysctl_sched_tunable_scaling = SCHED_TUNABLESCALING_LOG;

/*
 * Minimal preemption granularity for CPU-bound tasks:
 *
 * (default: 0.75 msec * (1 + ilog(ncpus)), units: nanoseconds)
 */
unsigned int sysctl_sched_min_granularity			= 750000ULL;
static unsigned int normalized_sysctl_sched_min_granularity	= 750000ULL;

/*
 * This value is kept at sysctl_sched_latency/sysctl_sched_min_granularity
 */
static unsigned int sched_nr_latency = 8;

/*
 * After fork, child runs first. If set to 0 (default) then
 * parent will (try to) run first.
 */
unsigned int sysctl_sched_child_runs_first __read_mostly;

/*
 * SCHED_OTHER wake-up granularity.
 *
 * This option delays the preemption effects of decoupled workloads
 * and reduces their over-scheduling. Synchronous workloads will still
 * have immediate wakeup/sleep latencies.
 *
 * (default: 1 msec * (1 + ilog(ncpus)), units: nanoseconds)
 */
unsigned int sysctl_sched_wakeup_granularity			= 1000000UL;
static unsigned int normalized_sysctl_sched_wakeup_granularity	= 1000000UL;

const_debug unsigned int sysctl_sched_migration_cost	= 500000UL;

#ifdef CONFIG_SMP
/*
 * For asym packing, by default the lower numbered CPU has higher priority.
 */
int __weak arch_asym_cpu_priority(int cpu)
{
    return -cpu;
}

/*
 * The margin used when comparing utilization with CPU capacity.
 *
 * (default: ~20%)
 */
#define fits_capacity(cap, max)	((cap) * 1280 < (max) * 1024)

#endif

#ifdef CONFIG_CFS_BANDWIDTH
/*
 * Amount of runtime to allocate from global (tg) to local (per-cfs_rq) pool
 * each time a cfs_rq requests quota.
 *
 * Note: in the case that the slice exceeds the runtime remaining (either due
 * to consumption or the quota being specified to be smaller than the slice)
 * we will always only issue the remaining available time.
 *
 * (default: 5 msec, units: microseconds)
 */
unsigned int sysctl_sched_cfs_bandwidth_slice		= 5000UL;
#endif

static inline void update_load_add(struct load_weight *lw, unsigned long inc)
{
    lw->weight += inc;
    lw->inv_weight = 0;
}

static inline void update_load_sub(struct load_weight *lw, unsigned long dec)
{
    lw->weight -= dec;
    lw->inv_weight = 0;
}

static inline void update_load_set(struct load_weight *lw, unsigned long w)
{
    lw->weight = w;
    lw->inv_weight = 0;
}
//138
/*
 * Increase the granularity value when there are more CPUs,
 * because with more CPUs the 'effective latency' as visible
 * to users decreases. But the relationship is not linear,
 * so pick a second-best guess by going with the log2 of the
 * number of CPUs.
 *
 * This idea comes from the SD scheduler of Con Kolivas:
 */
static unsigned int get_update_sysctl_factor(void)
{
    unsigned int cpus = min_t(unsigned int, num_online_cpus(), 8);
    unsigned int factor;

    switch (sysctl_sched_tunable_scaling) {
    case SCHED_TUNABLESCALING_NONE:
        factor = 1;
        break;
    case SCHED_TUNABLESCALING_LINEAR:
        factor = cpus;
        break;
    case SCHED_TUNABLESCALING_LOG:
    default:
        factor = 1 + ilog2(cpus);
        break;
    }

    return factor;
}

static void update_sysctl(void)
{
    unsigned int factor = get_update_sysctl_factor();

#define SET_SYSCTL(name) \
    (sysctl_##name = (factor) * normalized_sysctl_##name)
    SET_SYSCTL(sched_min_granularity);
    SET_SYSCTL(sched_latency);
    SET_SYSCTL(sched_wakeup_granularity);
#undef SET_SYSCTL
}

void sched_init_granularity(void)
{
    update_sysctl();
}
//185
#define WMULT_CONST	(~0U)
#define WMULT_SHIFT	32

static void __update_inv_weight(struct load_weight *lw)
{
    unsigned long w;

    if (likely(lw->inv_weight))
        return;

    w = scale_load_down(lw->weight);

    if (BITS_PER_LONG > 32 && unlikely(w >= WMULT_CONST))
        lw->inv_weight = 1;
    else if (unlikely(!w))
        lw->inv_weight = WMULT_CONST;
    else
        lw->inv_weight = WMULT_CONST / w;
}

/*
 * delta_exec * weight / lw.weight
 *   OR
 * (delta_exec * (weight * lw->inv_weight)) >> WMULT_SHIFT
 *
 * Either weight := NICE_0_LOAD and lw \e sched_prio_to_wmult[], in which case
 * we're guaranteed shift stays positive because inv_weight is guaranteed to
 * fit 32 bits, and NICE_0_LOAD gives another 10 bits; therefore shift >= 22.
 *
 * Or, weight =< lw.weight (because lw.weight is the runqueue weight), thus
 * weight/lw.weight <= 1, and therefore our shift will also be positive.
 */
static u64 __calc_delta(u64 delta_exec, unsigned long weight, struct load_weight *lw)
{
    u64 fact = scale_load_down(weight);
    int shift = WMULT_SHIFT;

    __update_inv_weight(lw);

    if (unlikely(fact >> 32)) {
        while (fact >> 32) {
            fact >>= 1;
            shift--;
        }
    }

    /* hint to use a 32x32->64 mul */
    fact = (u64)(u32)fact * lw->inv_weight;

    while (fact >> 32) {
        fact >>= 1;
        shift--;
    }

    return mul_u64_u32_shr(delta_exec, fact, shift);
}


const struct sched_class fair_sched_class;

/**************************************************************
 * CFS operations on generic schedulable entities:
 */

#ifdef CONFIG_FAIR_GROUP_SCHED
static inline struct task_struct *task_of(struct sched_entity *se)
{
    SCHED_WARN_ON(!entity_is_task(se));
    return container_of(se, struct task_struct, se);
}

/* Walk up scheduling entities hierarchy */
#define for_each_sched_entity(se) \
        for (; se; se = se->parent)

static inline struct cfs_rq *task_cfs_rq(struct task_struct *p)
{
    return p->se.cfs_rq;
}

/* runqueue on which this entity is (to be) queued */
static inline struct cfs_rq *cfs_rq_of(struct sched_entity *se)
{
    return se->cfs_rq;
}

/* runqueue "owned" by this group */
static inline struct cfs_rq *group_cfs_rq(struct sched_entity *grp)
{
    return grp->my_q;
}

static inline void cfs_rq_tg_path(struct cfs_rq *cfs_rq, char *path, int len)
{
    if (!path)
        return;

    if (cfs_rq && task_group_is_autogroup(cfs_rq->tg))
        autogroup_path(cfs_rq->tg, path, len);
    //else if (cfs_rq && cfs_rq->tg->css.cgroup)
    //    cgroup_path(cfs_rq->tg->css.cgroup, path, len);
    else
        strlcpy(path, "(null)", len);
}

static inline bool list_add_leaf_cfs_rq(struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    struct rq *rq = rq_of(cfs_rq);
    int cpu = cpu_of(rq);

    pr_info_view_on(stack_depth, "%30s : %d\n", cfs_rq->on_list);

    if (cfs_rq->on_list)
        return rq->tmp_alone_branch == &rq->leaf_cfs_rq_list;

    cfs_rq->on_list = 1;
    pr_info_view_on(stack_depth, "%30s : %d\n", cfs_rq->on_list);

    /*
     * Ensure we either appear before our parent (if already
     * enqueued) or force our parent to appear after us when it is
     * enqueued. The fact that we always enqueue bottom-up
     * reduces this to two cases and a special case for the root
     * cfs_rq. Furthermore, it also means that we will always reset
     * tmp_alone_branch either when the branch is connected
     * to a tree or when we reach the top of the tree
     */
    if (cfs_rq->tg->parent &&
        cfs_rq->tg->parent->cfs_rq[cpu]->on_list) {
        /*
         * If parent is already on the list, we add the child
         * just before. Thanks to circular linked property of
         * the list, this means to put the child at the tail
         * of the list that starts by parent.
         */
        list_add_tail_rcu(&cfs_rq->leaf_cfs_rq_list,
            &(cfs_rq->tg->parent->cfs_rq[cpu]->leaf_cfs_rq_list));
        /*
         * The branch is now connected to its tree so we can
         * reset tmp_alone_branch to the beginning of the
         * list.
         */
        rq->tmp_alone_branch = &rq->leaf_cfs_rq_list;
        return true;
    }

    if (!cfs_rq->tg->parent) {
        /*
         * cfs rq without parent should be put
         * at the tail of the list.
         */
        list_add_tail_rcu(&cfs_rq->leaf_cfs_rq_list,
            &rq->leaf_cfs_rq_list);
        /*
         * We have reach the top of a tree so we can reset
         * tmp_alone_branch to the beginning of the list.
         */
        rq->tmp_alone_branch = &rq->leaf_cfs_rq_list;
        return true;
    }

    /*
     * The parent has not already been added so we want to
     * make sure that it will be put after us.
     * tmp_alone_branch points to the begin of the branch
     * where we will add parent.
     */
    //error!
    //list_add_rcu(&cfs_rq->leaf_cfs_rq_list, rq->tmp_alone_branch);
    /*
     * update tmp_alone_branch to points to the new begin
     * of the branch
     */
    rq->tmp_alone_branch = &cfs_rq->leaf_cfs_rq_list;

    pr_fn_end_on(stack_depth);

    return false;
}

static inline void list_del_leaf_cfs_rq(struct cfs_rq *cfs_rq)
{
    if (cfs_rq->on_list) {
        struct rq *rq = rq_of(cfs_rq);

        /*
         * With cfs_rq being unthrottled/throttled during an enqueue,
         * it can happen the tmp_alone_branch points the a leaf that
         * we finally want to del. In this case, tmp_alone_branch moves
         * to the prev element but it will point to rq->leaf_cfs_rq_list
         * at the end of the enqueue.
         */
        if (rq->tmp_alone_branch == &cfs_rq->leaf_cfs_rq_list)
            rq->tmp_alone_branch = cfs_rq->leaf_cfs_rq_list.prev;

        list_del_rcu(&cfs_rq->leaf_cfs_rq_list);
        cfs_rq->on_list = 0;
    }
}

static inline void assert_list_leaf_cfs_rq(struct rq *rq)
{
    SCHED_WARN_ON(rq->tmp_alone_branch != &rq->leaf_cfs_rq_list);
}

/* Iterate thr' all leaf cfs_rq's on a runqueue */
#define for_each_leaf_cfs_rq_safe(rq, cfs_rq, pos)			\
    list_for_each_entry_safe(cfs_rq, pos, &rq->leaf_cfs_rq_list,	\
                 leaf_cfs_rq_list)

/* Do the two (enqueued) entities belong to the same group ? */
static inline struct cfs_rq *
is_same_group(struct sched_entity *se, struct sched_entity *pse)
{
    if (se->cfs_rq == pse->cfs_rq)
        return se->cfs_rq;

    return NULL;
}

static inline struct sched_entity *parent_entity(struct sched_entity *se)
{
    return se->parent;
}

static void
find_matching_se(struct sched_entity **se, struct sched_entity **pse)
{
    int se_depth, pse_depth;

    /*
     * preemption test can be made between sibling entities who are in the
     * same cfs_rq i.e who have a common parent. Walk up the hierarchy of
     * both tasks until we find their ancestors who are siblings of common
     * parent.
     */

    /* First walk up until both entities are at same depth */
    se_depth = (*se)->depth;
    pse_depth = (*pse)->depth;

    while (se_depth > pse_depth) {
        se_depth--;
        *se = parent_entity(*se);
    }

    while (pse_depth > se_depth) {
        pse_depth--;
        *pse = parent_entity(*pse);
    }

    while (!is_same_group(*se, *pse)) {
        *se = parent_entity(*se);
        *pse = parent_entity(*pse);
    }
}

#else	/* !CONFIG_FAIR_GROUP_SCHED */

static inline struct task_struct *task_of(struct sched_entity *se)
{
    return container_of(se, struct task_struct, se);
}

#define for_each_sched_entity(se) \
        for (; se; se = NULL)

static inline struct cfs_rq *task_cfs_rq(struct task_struct *p)
{
    return &task_rq(p)->cfs;
}

static inline struct cfs_rq *cfs_rq_of(struct sched_entity *se)
{
    struct task_struct *p = task_of(se);
    struct rq *rq = task_rq(p);

    return &rq->cfs;
}

/* runqueue "owned" by this group */
static inline struct cfs_rq *group_cfs_rq(struct sched_entity *grp)
{
    return NULL;
}

static inline void cfs_rq_tg_path(struct cfs_rq *cfs_rq, char *path, int len)
{
    if (path)
        strlcpy(path, "(null)", len);
}

static inline bool list_add_leaf_cfs_rq(struct cfs_rq *cfs_rq)
{
    return true;
}

static inline void list_del_leaf_cfs_rq(struct cfs_rq *cfs_rq)
{
}

static inline void assert_list_leaf_cfs_rq(struct rq *rq)
{
}

#define for_each_leaf_cfs_rq_safe(rq, cfs_rq, pos)	\
        for (cfs_rq = &rq->cfs, pos = NULL; cfs_rq; cfs_rq = pos)

static inline struct sched_entity *parent_entity(struct sched_entity *se)
{
    return NULL;
}

static inline void
find_matching_se(struct sched_entity **se, struct sched_entity **pse)
{
}

#endif	/* CONFIG_FAIR_GROUP_SCHED */
//498
static __always_inline
void account_cfs_rq_runtime(struct cfs_rq *cfs_rq, u64 delta_exec);

/**************************************************************
 * Scheduling class tree data structure manipulation methods:
 */

static inline u64 max_vruntime(u64 max_vruntime, u64 vruntime)
{
    s64 delta = (s64)(vruntime - max_vruntime);
    if (delta > 0)
        max_vruntime = vruntime;

    return max_vruntime;
}

static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
    s64 delta = (s64)(vruntime - min_vruntime);
    if (delta < 0)
        min_vruntime = vruntime;

    return min_vruntime;
}

static inline int entity_before(struct sched_entity *a,
                struct sched_entity *b)
{
    return (s64)(a->vruntime - b->vruntime) < 0;
}

static void update_min_vruntime(struct cfs_rq *cfs_rq)
{
    struct sched_entity *curr = cfs_rq->curr;
    struct rb_node *leftmost = rb_first_cached(&cfs_rq->tasks_timeline);

    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->curr);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)leftmost);

    u64 vruntime = cfs_rq->min_vruntime;

    if (curr) {
        if (curr->on_rq)
            vruntime = curr->vruntime;
        else
            curr = NULL;
    }

    if (leftmost) { /* non-empty tree */
        struct sched_entity *se;
        se = rb_entry(leftmost, struct sched_entity, run_node);

        if (!curr)
            vruntime = se->vruntime;
        else
            vruntime = min_vruntime(vruntime, se->vruntime);
    }

    /* ensure we never gain time by being placed backwards. */
    cfs_rq->min_vruntime = max_vruntime(cfs_rq->min_vruntime, vruntime);
#ifndef CONFIG_64BIT
    smp_wmb();
    cfs_rq->min_vruntime_copy = cfs_rq->min_vruntime;
#endif

    pr_fn_end_on(stack_depth);
}

/*
 * Enqueue an entity into the rb-tree:
 */
static void __enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    struct rb_node **link = &cfs_rq->tasks_timeline.rb_root.rb_node;
    struct rb_node *parent = NULL;
    struct sched_entity *entry;
    bool leftmost = true;

    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)*link);

    /*
     * Find the right place in the rbtree:
     */
    while (*link) {
        parent = *link;
        entry = rb_entry(parent, struct sched_entity, run_node);
        /*
         * We dont care about collisions. Nodes with
         * the same key stay together.
         */
        pr_info_view_on(stack_depth, "%30s : %llu\n", se->vruntime);
        pr_info_view_on(stack_depth, "%30s : %llu\n", entry->vruntime);
        if (entity_before(se, entry)) {		//se < entry
            link = &parent->rb_left;
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&parent->rb_left);
        } else {
            link = &parent->rb_right;		//se >= entry
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&parent->rb_right);
            leftmost = false;
        }
    }

    rb_link_node(&se->run_node, parent, link);
    rb_insert_color_cached(&se->run_node,
                   &cfs_rq->tasks_timeline, leftmost);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)link);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)*link);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&se->run_node);
    pr_fn_end_on(stack_depth);
}

static void __dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);
    rb_erase_cached(&se->run_node, &cfs_rq->tasks_timeline);
    pr_fn_end_on(stack_depth);
}

struct sched_entity *__pick_first_entity(struct cfs_rq *cfs_rq)
{
    struct rb_node *left = rb_first_cached(&cfs_rq->tasks_timeline);

    if (!left)
        return NULL;

    return rb_entry(left, struct sched_entity, run_node);
}

static struct sched_entity *__pick_next_entity(struct sched_entity *se)
{
    struct rb_node *next = rb_next(&se->run_node);

    if (!next)
        return NULL;

    return rb_entry(next, struct sched_entity, run_node);
}
//619 lines




//658 lines
/*
 * delta /= w
 */
static inline u64 calc_delta_fair(u64 delta, struct sched_entity *se)
{
    if (unlikely(se->load.weight != NICE_0_LOAD))
        delta = __calc_delta(delta, NICE_0_LOAD, &se->load);

    return delta;
}

/*
 * The idea is to set a period in which each task runs once.
 *
 * When there are too many tasks (sched_nr_latency) we have to stretch
 * this period because otherwise the slices get too small.
 *
 * p = (nr <= nl) ? l : l*nr/nl
 */
static u64 __sched_period(unsigned long nr_running)
{
    if (unlikely(nr_running > sched_nr_latency))
        return nr_running * sysctl_sched_min_granularity;
    else
        return sysctl_sched_latency;
}

/*
 * We calculate the wall-time slice from the period by taking a part
 * proportional to the weight.
 *
 * s = p*P[w/rw]
 */
static u64 sched_slice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    u64 slice = __sched_period(cfs_rq->nr_running + !se->on_rq);

    for_each_sched_entity(se) {
        struct load_weight *load;
        struct load_weight lw;

        cfs_rq = cfs_rq_of(se);
        load = &cfs_rq->load;

        if (unlikely(!se->on_rq)) {
            lw = cfs_rq->load;

            update_load_add(&lw, se->load.weight);
            load = &lw;
        }
        slice = __calc_delta(slice, se->load.weight, load);
    }
    return slice;
}

/*
 * We calculate the vruntime slice of a to-be-inserted task.
 *
 * vs = s/w
 */
static u64 sched_vslice(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    return calc_delta_fair(sched_slice(cfs_rq, se), se);
}
//723
//#include "pelt.h"
#ifdef CONFIG_SMP

static int select_idle_sibling(struct task_struct *p, int prev_cpu, int cpu);
static unsigned long task_h_load(struct task_struct *p);
static unsigned long capacity_of(int cpu);

/* Give new sched_entity start runnable values to heavy its load in infant time */
void init_entity_runnable_average(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    struct sched_avg *sa = &se->avg;

    memset(sa, 0, sizeof(*sa));

    /*
     * Tasks are initialized with full load to be seen as heavy tasks until
     * they get a chance to stabilize to their real load level.
     * Group entities are initialized with zero load to reflect the fact that
     * nothing has been attached to the task group yet.
     */
    if (entity_is_task(se))
        sa->runnable_load_avg = sa->load_avg = scale_load_down(se->load.weight);

    se->runnable_weight = se->load.weight;

    /* when this task enqueue'ed, it will contribute to its cfs_rq's load_avg */

    pr_sched_info(se);

    pr_fn_end_on(stack_depth);
}

static void attach_entity_cfs_rq(struct sched_entity *se);

/*
 * With new tasks being created, their initial util_avgs are extrapolated
 * based on the cfs_rq's current util_avg:
 *
 *   util_avg = cfs_rq->util_avg / (cfs_rq->load_avg + 1) * se.load.weight
 *
 * However, in many cases, the above util_avg does not give a desired
 * value. Moreover, the sum of the util_avgs may be divergent, such
 * as when the series is a harmonic series.
 *
 * To solve this problem, we also cap the util_avg of successive tasks to
 * only 1/2 of the left utilization budget:
 *
 *   util_avg_cap = (cpu_scale - cfs_rq->avg.util_avg) / 2^n
 *
 * where n denotes the nth task and cpu_scale the CPU capacity.
 *
 * For example, for a CPU with 1024 of capacity, a simplest series from
 * the beginning would be like:
 *
 *  task  util_avg: 512, 256, 128,  64,  32,   16,    8, ...
 * cfs_rq util_avg: 512, 768, 896, 960, 992, 1008, 1016, ...
 *
 * Finally, that extrapolated util_avg is clamped to the cap (util_avg_cap)
 * if util_avg > util_avg_cap.
 */
void post_init_entity_util_avg(struct task_struct *p)
{
    struct sched_entity *se = &p->se;
    struct cfs_rq *cfs_rq = cfs_rq_of(se);
    struct sched_avg *sa = &se->avg;
    long cpu_scale = arch_scale_cpu_capacity(cpu_of(rq_of(cfs_rq)));
    long cap = (long)(cpu_scale - cfs_rq->avg.util_avg) / 2;

    if (cap > 0) {
        if (cfs_rq->avg.util_avg != 0) {
            sa->util_avg  = cfs_rq->avg.util_avg * se->load.weight;
            sa->util_avg /= (cfs_rq->avg.load_avg + 1);

            if (sa->util_avg > cap)
                sa->util_avg = cap;
        } else {
            sa->util_avg = cap;
        }
    }

    if (p->sched_class != &fair_sched_class) {
        /*
         * For !fair tasks do:
         *
        update_cfs_rq_load_avg(now, cfs_rq);
        attach_entity_load_avg(cfs_rq, se, 0);
        switched_from_fair(rq, p);
         *
         * such that the next switched_to_fair() has the
         * expected state.
         */
        se->avg.last_update_time = cfs_rq_clock_pelt(cfs_rq);
        return;
    }

    attach_entity_cfs_rq(se);
}

#else /* !CONFIG_SMP */
void init_entity_runnable_average(struct sched_entity *se)
{
}
void post_init_entity_util_avg(struct task_struct *p)
{
}
static void update_tg_load_avg(struct cfs_rq *cfs_rq, int force)
{
}
#endif /* CONFIG_SMP */
//829
/*
 * Update the current task's runtime statistics.
 */
static void update_curr(struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    struct sched_entity *curr = cfs_rq->curr;

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->curr);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq_of(cfs_rq));
    if (!rq_of(cfs_rq)) return;

    u64 now = rq_clock_task(rq_of(cfs_rq));
    u64 delta_exec;

    pr_info_view_on(4, "%30s : %llu\n", now);
    pr_info_view_on(4, "%30s : %p\n", (void*)cfs_rq->curr);

    if (unlikely(!curr)) return;

    delta_exec = now - curr->exec_start;

    pr_info_view_on(4, "%30s : %llu\n", curr->exec_start);
    pr_info_view_on(4, "%30s : %lld\n", delta_exec);

    if (unlikely((s64)delta_exec <= 0))
        return;

    curr->exec_start = now;

    schedstat_set(curr->statistics.exec_max,
              max(delta_exec, curr->statistics.exec_max));

    curr->sum_exec_runtime += delta_exec;
    schedstat_add(cfs_rq->exec_clock, delta_exec);

    curr->vruntime += calc_delta_fair(delta_exec, curr);
    update_min_vruntime(cfs_rq);

    if (entity_is_task(curr)) {
        struct task_struct *curtask = task_of(curr);

        //trace_sched_stat_runtime(curtask, delta_exec, curr->vruntime);
        cgroup_account_cputime(curtask, delta_exec);
        account_group_exec_runtime(curtask, delta_exec);
    }

    account_cfs_rq_runtime(cfs_rq, delta_exec);

    pr_fn_end_on(stack_depth);
}

static void update_curr_fair(struct rq *rq)
{
    update_curr(cfs_rq_of(&rq->curr->se));
}
//872
static inline void
update_stats_wait_start(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    u64 wait_start, prev_wait_start;

    if (!schedstat_enabled())
        return;

    wait_start = rq_clock(rq_of(cfs_rq));
    prev_wait_start = schedstat_val(se->statistics.wait_start);

    if (entity_is_task(se) && task_on_rq_migrating(task_of(se)) &&
        likely(wait_start > prev_wait_start))
        wait_start -= prev_wait_start;

    __schedstat_set(se->statistics.wait_start, wait_start);
}

static inline void
update_stats_wait_end(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    struct task_struct *p;
    u64 delta;

    if (!schedstat_enabled())
        return;

    delta = rq_clock(rq_of(cfs_rq)) - schedstat_val(se->statistics.wait_start);

    if (entity_is_task(se)) {
        p = task_of(se);
        if (task_on_rq_migrating(p)) {
            /*
             * Preserve migrating task's wait time so wait_start
             * time stamp can be adjusted to accumulate wait time
             * prior to migration.
             */
            __schedstat_set(se->statistics.wait_start, delta);
            return;
        }
        //trace_sched_stat_wait(p, delta);
    }

    __schedstat_set(se->statistics.wait_max,
              max(schedstat_val(se->statistics.wait_max), delta));
    __schedstat_inc(se->statistics.wait_count);
    __schedstat_add(se->statistics.wait_sum, delta);
    __schedstat_set(se->statistics.wait_start, 0);
}

static inline void
update_stats_enqueue_sleeper(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    struct task_struct *tsk = NULL;
    u64 sleep_start, block_start;

    if (!schedstat_enabled())
        return;

    sleep_start = schedstat_val(se->statistics.sleep_start);
    block_start = schedstat_val(se->statistics.block_start);

    if (entity_is_task(se))
        tsk = task_of(se);

    if (sleep_start) {
        u64 delta = rq_clock(rq_of(cfs_rq)) - sleep_start;

        if ((s64)delta < 0)
            delta = 0;

        if (unlikely(delta > schedstat_val(se->statistics.sleep_max)))
            __schedstat_set(se->statistics.sleep_max, delta);

        __schedstat_set(se->statistics.sleep_start, 0);
        __schedstat_add(se->statistics.sum_sleep_runtime, delta);

        if (tsk) {
            //account_scheduler_latency(tsk, delta >> 10, 1);
            //trace_sched_stat_sleep(tsk, delta);
        }
    }
    if (block_start) {
        u64 delta = rq_clock(rq_of(cfs_rq)) - block_start;

        if ((s64)delta < 0)
            delta = 0;

        if (unlikely(delta > schedstat_val(se->statistics.block_max)))
            __schedstat_set(se->statistics.block_max, delta);

        __schedstat_set(se->statistics.block_start, 0);
        __schedstat_add(se->statistics.sum_sleep_runtime, delta);

        if (tsk) {
            if (tsk->in_iowait) {
                __schedstat_add(se->statistics.iowait_sum, delta);
                __schedstat_inc(se->statistics.iowait_count);
                //trace_sched_stat_iowait(tsk, delta);
            }

            //trace_sched_stat_blocked(tsk, delta);

            /*
             * Blocking time is in units of nanosecs, so shift by
             * 20 to get a milliseconds-range estimation of the
             * amount of time that the task spent sleeping:
             */
            if (unlikely(prof_on == SLEEP_PROFILING)) {
                profile_hits(SLEEP_PROFILING,
                        (void *)get_wchan(tsk),
                        delta >> 20);
            }
            //account_scheduler_latency(tsk, delta >> 10, 0);
        }
    }
}
//990
/*
 * Task is being enqueued - update stats:
 */
static inline void
update_stats_enqueue(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{
    if (!schedstat_enabled())
        return;

    /*
     * Are we enqueueing a waiting task? (for current tasks
     * a dequeue/enqueue event is a NOP)
     */
    if (se != cfs_rq->curr)
        update_stats_wait_start(cfs_rq, se);

    if (flags & ENQUEUE_WAKEUP)
        update_stats_enqueue_sleeper(cfs_rq, se);
}

static inline void
update_stats_dequeue(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{

    if (!schedstat_enabled())
        return;

    /*
     * Mark the end of the wait period if dequeueing a
     * waiting task:
     */
    if (se != cfs_rq->curr)
        update_stats_wait_end(cfs_rq, se);

    if ((flags & DEQUEUE_SLEEP) && entity_is_task(se)) {
        struct task_struct *tsk = task_of(se);

        if (tsk->state & TASK_INTERRUPTIBLE)
            __schedstat_set(se->statistics.sleep_start,
                      rq_clock(rq_of(cfs_rq)));
        if (tsk->state & TASK_UNINTERRUPTIBLE)
            __schedstat_set(se->statistics.block_start,
                      rq_clock(rq_of(cfs_rq)));
    }
}

/*
 * We are picking a new current task - update its stats:
 */
static inline void
update_stats_curr_start(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    /*
     * We are starting a new run period:
     */
    se->exec_start = rq_clock_task(rq_of(cfs_rq));
}

/**************************************************
 * Scheduling class queueing methods:
 */
//1052 lines





//2755 lines
static void
account_entity_enqueue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    update_load_add(&cfs_rq->load, se->load.weight);
#ifdef CONFIG_SMP
    if (entity_is_task(se)) {
        struct rq *rq = rq_of(cfs_rq);

        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq_of(cfs_rq));
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&se->group_node);
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&rq->cfs_tasks);

        //account_numa_enqueue(rq, task_of(se));
        //list_add(&se->group_node, &rq->cfs_tasks);	//error
    }
#endif
    cfs_rq->nr_running++;

    pr_fn_end_on(stack_depth);
}

static void
account_entity_dequeue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    update_load_sub(&cfs_rq->load, se->load.weight);
#ifdef CONFIG_SMP
    if (entity_is_task(se)) {
        //account_numa_dequeue(rq_of(cfs_rq), task_of(se));
        list_del_init(&se->group_node);
    }
#endif
    cfs_rq->nr_running--;
}

/*
 * Signed add and clamp on underflow.
 *
 * Explicitly do a load-store to ensure the intermediate value never hits
 * memory. This allows lockless observations without ever seeing the negative
 * values.
 */
#define add_positive(_ptr, _val) do {                           \
    typeof(_ptr) ptr = (_ptr);                              \
    typeof(_val) val = (_val);                              \
    typeof(*ptr) res, var = READ_ONCE(*ptr);                \
                                \
    res = var + val;                                        \
                                \
    if (val < 0 && res > var)                               \
        res = 0;                                        \
                                \
    WRITE_ONCE(*ptr, res);                                  \
} while (0)

/*
 * Unsigned subtract and clamp on underflow.
 *
 * Explicitly do a load-store to ensure the intermediate value never hits
 * memory. This allows lockless observations without ever seeing the negative
 * values.
 */
#define sub_positive(_ptr, _val) do {				\
    typeof(_ptr) ptr = (_ptr);				\
    typeof(*ptr) val = (_val);				\
    typeof(*ptr) res, var = READ_ONCE(*ptr);		\
    res = var - val;					\
    if (res > var)						\
        res = 0;					\
    WRITE_ONCE(*ptr, res);					\
} while (0)

/*
 * Remove and clamp on negative, from a local variable.
 *
 * A variant of sub_positive(), which does not use explicit load-store
 * and is thus optimized for local variable updates.
 */
#define lsub_positive(_ptr, _val) do {				\
    typeof(_ptr) ptr = (_ptr);				\
    *ptr -= min_t(typeof(*ptr), *ptr, _val);		\
} while (0)

#ifdef CONFIG_SMP
static inline void
enqueue_runnable_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    cfs_rq->runnable_weight += se->runnable_weight;

    cfs_rq->avg.runnable_load_avg += se->avg.runnable_load_avg;
    cfs_rq->avg.runnable_load_sum += se_runnable(se) * se->avg.runnable_load_sum;
}

static inline void
dequeue_runnable_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    cfs_rq->runnable_weight -= se->runnable_weight;

    sub_positive(&cfs_rq->avg.runnable_load_avg, se->avg.runnable_load_avg);
    sub_positive(&cfs_rq->avg.runnable_load_sum,
             se_runnable(se) * se->avg.runnable_load_sum);
}

static inline void
enqueue_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    cfs_rq->avg.load_avg += se->avg.load_avg;
    cfs_rq->avg.load_sum += se_weight(se) * se->avg.load_sum;
}

static inline void
dequeue_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    sub_positive(&cfs_rq->avg.load_avg, se->avg.load_avg);
    sub_positive(&cfs_rq->avg.load_sum, se_weight(se) * se->avg.load_sum);
}
#else
static inline void
enqueue_runnable_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se) { }
static inline void
dequeue_runnable_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se) { }
static inline void
enqueue_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se) { }
static inline void
dequeue_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se) { }
#endif

static void reweight_entity(struct cfs_rq *cfs_rq, struct sched_entity *se,
                unsigned long weight, unsigned long runnable)
{
    if (se->on_rq) {
        /* commit outstanding execution time */
        if (cfs_rq->curr == se)
            update_curr(cfs_rq);
        account_entity_dequeue(cfs_rq, se);
        dequeue_runnable_load_avg(cfs_rq, se);
    }
    dequeue_load_avg(cfs_rq, se);

    se->runnable_weight = runnable;
    update_load_set(&se->load, weight);

#ifdef CONFIG_SMP
    do {
        u32 divider = LOAD_AVG_MAX - 1024 + se->avg.period_contrib;

        se->avg.load_avg = div_u64(se_weight(se) * se->avg.load_sum, divider);
        se->avg.runnable_load_avg =
            div_u64(se_runnable(se) * se->avg.runnable_load_sum, divider);
    } while (0);
#endif

    enqueue_load_avg(cfs_rq, se);
    if (se->on_rq) {
        account_entity_enqueue(cfs_rq, se);
        enqueue_runnable_load_avg(cfs_rq, se);
    }
}

void reweight_task(struct task_struct *p, int prio)
{
    struct sched_entity *se = &p->se;
    //struct cfs_rq *cfs_rq = cfs_rq_of(se);
    struct load_weight *load = &se->load;
    unsigned long weight = scale_load(sched_prio_to_weight[prio]);

    //reweight_entity(cfs_rq, se, weight, weight);
    load->inv_weight = sched_prio_to_wmult[prio];
}

#ifdef CONFIG_FAIR_GROUP_SCHED
#ifdef CONFIG_SMP
/*
 * All this does is approximate the hierarchical proportion which includes that
 * global sum we all love to hate.
 *
 * That is, the weight of a group entity, is the proportional share of the
 * group weight based on the group runqueue weights. That is:
 *
 *                     tg->weight * grq->load.weight
 *   ge->load.weight = -----------------------------               (1)
 *			  \Sum grq->load.weight
 *
 * Now, because computing that sum is prohibitively expensive to compute (been
 * there, done that) we approximate it with this average stuff. The average
 * moves slower and therefore the approximation is cheaper and more stable.
 *
 * So instead of the above, we substitute:
 *
 *   grq->load.weight -> grq->avg.load_avg                         (2)
 *
 * which yields the following:
 *
 *                     tg->weight * grq->avg.load_avg
 *   ge->load.weight = ------------------------------              (3)
 *				tg->load_avg
 *
 * Where: tg->load_avg ~= \Sum grq->avg.load_avg
 *
 * That is shares_avg, and it is right (given the approximation (2)).
 *
 * The problem with it is that because the average is slow -- it was designed
 * to be exactly that of course -- this leads to transients in boundary
 * conditions. In specific, the case where the group was idle and we start the
 * one task. It takes time for our CPU's grq->avg.load_avg to build up,
 * yielding bad latency etc..
 *
 * Now, in that special case (1) reduces to:
 *
 *                     tg->weight * grq->load.weight
 *   ge->load.weight = ----------------------------- = tg->weight   (4)
 *			    grp->load.weight
 *
 * That is, the sum collapses because all other CPUs are idle; the UP scenario.
 *
 * So what we do is modify our approximation (3) to approach (4) in the (near)
 * UP case, like:
 *
 *   ge->load.weight =
 *
 *              tg->weight * grq->load.weight
 *     ---------------------------------------------------         (5)
 *     tg->load_avg - grq->avg.load_avg + grq->load.weight
 *
 * But because grq->load.weight can drop to 0, resulting in a divide by zero,
 * we need to use grq->avg.load_avg as its lower bound, which then gives:
 *
 *
 *                     tg->weight * grq->load.weight
 *   ge->load.weight = -----------------------------		   (6)
 *				tg_load_avg'
 *
 * Where:
 *
 *   tg_load_avg' = tg->load_avg - grq->avg.load_avg +
 *                  max(grq->load.weight, grq->avg.load_avg)
 *
 * And that is shares_weight and is icky. In the (near) UP case it approaches
 * (4) while in the normal case it approaches (3). It consistently
 * overestimates the ge->load.weight and therefore:
 *
 *   \Sum ge->load.weight >= tg->weight
 *
 * hence icky!
 */
static long calc_group_shares(struct cfs_rq *cfs_rq)
{
    long tg_weight, tg_shares, load, shares;
    struct task_group *tg = cfs_rq->tg;

    tg_shares = READ_ONCE(tg->shares);

    load = max(scale_load_down(cfs_rq->load.weight), cfs_rq->avg.load_avg);

    tg_weight = atomic_long_read(&tg->load_avg);

    /* Ensure tg_weight >= load */
    tg_weight -= cfs_rq->tg_load_avg_contrib;
    tg_weight += load;

    shares = (tg_shares * load);
    if (tg_weight)
        shares /= tg_weight;

    /*
     * MIN_SHARES has to be unscaled here to support per-CPU partitioning
     * of a group with small tg->shares value. It is a floor value which is
     * assigned as a minimum load.weight to the sched_entity representing
     * the group on a CPU.
     *
     * E.g. on 64-bit for a group with tg->shares of scale_load(15)=15*1024
     * on an 8-core system with 8 tasks each runnable on one CPU shares has
     * to be 15*1024*1/8=1920 instead of scale_load(MIN_SHARES)=2*1024. In
     * case no task is runnable on a CPU MIN_SHARES=2 should be returned
     * instead of 0.
     */
    return clamp_t(long, shares, MIN_SHARES, tg_shares);
}

/*
 * This calculates the effective runnable weight for a group entity based on
 * the group entity weight calculated above.
 *
 * Because of the above approximation (2), our group entity weight is
 * an load_avg based ratio (3). This means that it includes blocked load and
 * does not represent the runnable weight.
 *
 * Approximate the group entity's runnable weight per ratio from the group
 * runqueue:
 *
 *					     grq->avg.runnable_load_avg
 *   ge->runnable_weight = ge->load.weight * -------------------------- (7)
 *						 grq->avg.load_avg
 *
 * However, analogous to above, since the avg numbers are slow, this leads to
 * transients in the from-idle case. Instead we use:
 *
 *   ge->runnable_weight = ge->load.weight *
 *
 *		max(grq->avg.runnable_load_avg, grq->runnable_weight)
 *		-----------------------------------------------------	(8)
 *		      max(grq->avg.load_avg, grq->load.weight)
 *
 * Where these max() serve both to use the 'instant' values to fix the slow
 * from-idle and avoid the /0 on to-idle, similar to (6).
 */
static long calc_group_runnable(struct cfs_rq *cfs_rq, long shares)
{
    long runnable, load_avg;

    load_avg = max(cfs_rq->avg.load_avg,
               scale_load_down(cfs_rq->load.weight));

    runnable = max(cfs_rq->avg.runnable_load_avg,
               scale_load_down(cfs_rq->runnable_weight));

    runnable *= shares;
    if (load_avg)
        runnable /= load_avg;

    return clamp_t(long, runnable, MIN_SHARES, shares);
}
#endif /* CONFIG_SMP */

static inline int throttled_hierarchy(struct cfs_rq *cfs_rq);

/*
 * Recomputes the group entity based on the current state of its group
 * runqueue.
 */
static void update_cfs_group(struct sched_entity *se)
{
    struct cfs_rq *gcfs_rq = group_cfs_rq(se);
    long shares, runnable;

    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)gcfs_rq);

    if (!gcfs_rq)
        return;

    if (throttled_hierarchy(gcfs_rq))
        return;

#ifndef CONFIG_SMP
    runnable = shares = READ_ONCE(gcfs_rq->tg->shares);

    if (likely(se->load.weight == shares))
        return;
#else
    shares   = calc_group_shares(gcfs_rq);
    runnable = calc_group_runnable(gcfs_rq, shares);
#endif

    reweight_entity(cfs_rq_of(se), se, shares, runnable);

    pr_fn_end_on(stack_depth);
}

#else /* CONFIG_FAIR_GROUP_SCHED */
static inline void update_cfs_group(struct sched_entity *se)
{
}
#endif /* CONFIG_FAIR_GROUP_SCHED */

static inline void cfs_rq_util_change(struct cfs_rq *cfs_rq, int flags)
{
    struct rq *rq = rq_of(cfs_rq);

    if (&rq->cfs == cfs_rq || (flags & SCHED_CPUFREQ_MIGRATION)) {
        /*
         * There are a few boundary cases this might miss but it should
         * get called often enough that that should (hopefully) not be
         * a real problem.
         *
         * It will not get called when we go idle, because the idle
         * thread is a different class (!fair), nor will the utilization
         * number include things like RT tasks.
         *
         * As is, the util number is not freq-invariant (we'd have to
         * implement arch_scale_freq_capacity() for that).
         *
         * See cpu_util().
         */
        cpufreq_update_util(rq, flags);
    }
}
//3131 lines
#ifdef CONFIG_SMP
#ifdef CONFIG_FAIR_GROUP_SCHED
/**
 * update_tg_load_avg - update the tg's load avg
 * @cfs_rq: the cfs_rq whose avg changed
 * @force: update regardless of how small the difference
 *
 * This function 'ensures': tg->load_avg := \Sum tg->cfs_rq[]->avg.load.
 * However, because tg->load_avg is a global value there are performance
 * considerations.
 *
 * In order to avoid having to look at the other cfs_rq's, we use a
 * differential update where we store the last value we propagated. This in
 * turn allows skipping updates if the differential is 'small'.
 *
 * Updating tg's load_avg is necessary before update_cfs_share().
 */
static inline void update_tg_load_avg(struct cfs_rq *cfs_rq, int force)
{
    pr_fn_start_on(stack_depth);

    long delta = cfs_rq->avg.load_avg - cfs_rq->tg_load_avg_contrib;

    if (!cfs_rq->tg) {
        pr_err("cfs_rq->tg is NULL\n");
        return;
    }
    /*
     * No need to update load_avg for root_task_group as it is not used.
     */
    if (cfs_rq->tg == &root_task_group)
        return;

    if (force || abs(delta) > cfs_rq->tg_load_avg_contrib / 64) {
        atomic_long_add(delta, &cfs_rq->tg->load_avg);
        cfs_rq->tg_load_avg_contrib = cfs_rq->avg.load_avg;
    }

    pr_fn_end_on(stack_depth);
}

/*
 * Called within set_task_rq() right before setting a task's CPU. The
 * caller only guarantees p->pi_lock is held; no other assumptions,
 * including the state of rq->lock, should be made.
 */
void set_task_rq_fair(struct sched_entity *se,
              struct cfs_rq *prev, struct cfs_rq *next)
{
    u64 p_last_update_time;
    u64 n_last_update_time;

    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%30s : %p\n", se);
    pr_info_view_on(stack_depth, "%30s : %p\n", prev);
    pr_info_view_on(stack_depth, "%30s : %p\n", next);

    if (!sched_feat(ATTACH_AGE_LOAD))
        return;

    /*
     * We are supposed to update the task to "current" time, then its up to
     * date and ready to go to new CPU/cfs_rq. But we have difficulty in
     * getting what current time is, so simply throw away the out-of-date
     * time. This will result in the wakee task is less decayed, but giving
     * the wakee more load sounds not bad.
     */
    if (!(se->avg.last_update_time && prev))
        return;

#ifndef CONFIG_64BIT
    {
        u64 p_last_update_time_copy;
        u64 n_last_update_time_copy;

        do {
            p_last_update_time_copy = prev->load_last_update_time_copy;
            n_last_update_time_copy = next->load_last_update_time_copy;

            smp_rmb();

            p_last_update_time = prev->avg.last_update_time;
            n_last_update_time = next->avg.last_update_time;

        } while (p_last_update_time != p_last_update_time_copy ||
             n_last_update_time != n_last_update_time_copy);
    }
#else
    p_last_update_time = prev->avg.last_update_time;
    n_last_update_time = next->avg.last_update_time;
#endif
    __update_load_avg_blocked_se(p_last_update_time, se);
    se->avg.last_update_time = n_last_update_time;

    pr_fn_end_on(stack_depth);
}

/*
 * When on migration a sched_entity joins/leaves the PELT hierarchy, we need to
 * propagate its contribution. The key to this propagation is the invariant
 * that for each group:
 *
 *   ge->avg == grq->avg						(1)
 *
 * _IFF_ we look at the pure running and runnable sums. Because they
 * represent the very same entity, just at different points in the hierarchy.
 *
 * Per the above update_tg_cfs_util() is trivial and simply copies the running
 * sum over (but still wrong, because the group entity and group rq do not have
 * their PELT windows aligned).
 *
 * However, update_tg_cfs_runnable() is more complex. So we have:
 *
 *   ge->avg.load_avg = ge->load.weight * ge->avg.runnable_avg		(2)
 *
 * And since, like util, the runnable part should be directly transferable,
 * the following would _appear_ to be the straight forward approach:
 *
 *   grq->avg.load_avg = grq->load.weight * grq->avg.runnable_avg	(3)
 *
 * And per (1) we have:
 *
 *   ge->avg.runnable_avg == grq->avg.runnable_avg
 *
 * Which gives:
 *
 *                      ge->load.weight * grq->avg.load_avg
 *   ge->avg.load_avg = -----------------------------------		(4)
 *                               grq->load.weight
 *
 * Except that is wrong!
 *
 * Because while for entities historical weight is not important and we
 * really only care about our future and therefore can consider a pure
 * runnable sum, runqueues can NOT do this.
 *
 * We specifically want runqueues to have a load_avg that includes
 * historical weights. Those represent the blocked load, the load we expect
 * to (shortly) return to us. This only works by keeping the weights as
 * integral part of the sum. We therefore cannot decompose as per (3).
 *
 * Another reason this doesn't work is that runnable isn't a 0-sum entity.
 * Imagine a rq with 2 tasks that each are runnable 2/3 of the time. Then the
 * rq itself is runnable anywhere between 2/3 and 1 depending on how the
 * runnable section of these tasks overlap (or not). If they were to perfectly
 * align the rq as a whole would be runnable 2/3 of the time. If however we
 * always have at least 1 runnable task, the rq as a whole is always runnable.
 *
 * So we'll have to approximate.. :/
 *
 * Given the constraint:
 *
 *   ge->avg.running_sum <= ge->avg.runnable_sum <= LOAD_AVG_MAX
 *
 * We can construct a rule that adds runnable to a rq by assuming minimal
 * overlap.
 *
 * On removal, we'll assume each task is equally runnable; which yields:
 *
 *   grq->avg.runnable_sum = grq->avg.load_sum / grq->load.weight
 *
 * XXX: only do this for the part of runnable > running ?
 *
 */

static inline void
update_tg_cfs_util(struct cfs_rq *cfs_rq, struct sched_entity *se, struct cfs_rq *gcfs_rq)
{
    long delta = gcfs_rq->avg.util_avg - se->avg.util_avg;

    /* Nothing to update */
    if (!delta)
        return;

    /*
     * The relation between sum and avg is:
     *
     *   LOAD_AVG_MAX - 1024 + sa->period_contrib
     *
     * however, the PELT windows are not aligned between grq and gse.
     */

    /* Set new sched_entity's utilization */
    se->avg.util_avg = gcfs_rq->avg.util_avg;
    se->avg.util_sum = se->avg.util_avg * LOAD_AVG_MAX;

    /* Update parent cfs_rq utilization */
    add_positive(&cfs_rq->avg.util_avg, delta);
    cfs_rq->avg.util_sum = cfs_rq->avg.util_avg * LOAD_AVG_MAX;
}

static inline void
update_tg_cfs_runnable(struct cfs_rq *cfs_rq, struct sched_entity *se, struct cfs_rq *gcfs_rq)
{
    pr_fn_start_on(stack_depth);

    long delta_avg, running_sum, runnable_sum = gcfs_rq->prop_runnable_sum;
    unsigned long runnable_load_avg, load_avg;
    u64 runnable_load_sum, load_sum = 0;
    s64 delta_sum;

    if (!runnable_sum)
        return;

    gcfs_rq->prop_runnable_sum = 0;

    if (runnable_sum >= 0) {
        /*
         * Add runnable; clip at LOAD_AVG_MAX. Reflects that until
         * the CPU is saturated running == runnable.
         */
        runnable_sum += se->avg.load_sum;
        runnable_sum = min(runnable_sum, (long)LOAD_AVG_MAX);
    } else {
        /*
         * Estimate the new unweighted runnable_sum of the gcfs_rq by
         * assuming all tasks are equally runnable.
         */
        if (scale_load_down(gcfs_rq->load.weight)) {
            load_sum = div_s64(gcfs_rq->avg.load_sum,
                scale_load_down(gcfs_rq->load.weight));
        }

        /* But make sure to not inflate se's runnable */
        runnable_sum = min(se->avg.load_sum, load_sum);
    }

    /*
     * runnable_sum can't be lower than running_sum
     * Rescale running sum to be in the same range as runnable sum
     * running_sum is in [0 : LOAD_AVG_MAX <<  SCHED_CAPACITY_SHIFT]
     * runnable_sum is in [0 : LOAD_AVG_MAX]
     */
    running_sum = se->avg.util_sum >> SCHED_CAPACITY_SHIFT;
    runnable_sum = max(runnable_sum, running_sum);

    load_sum = (s64)se_weight(se) * runnable_sum;
    load_avg = div_s64(load_sum, LOAD_AVG_MAX);

    delta_sum = load_sum - (s64)se_weight(se) * se->avg.load_sum;
    delta_avg = load_avg - se->avg.load_avg;

    se->avg.load_sum = runnable_sum;
    se->avg.load_avg = load_avg;
    add_positive(&cfs_rq->avg.load_avg, delta_avg);
    add_positive(&cfs_rq->avg.load_sum, delta_sum);

    runnable_load_sum = (s64)se_runnable(se) * runnable_sum;
    runnable_load_avg = div_s64(runnable_load_sum, LOAD_AVG_MAX);
    delta_sum = runnable_load_sum - se_weight(se) * se->avg.runnable_load_sum;
    delta_avg = runnable_load_avg - se->avg.runnable_load_avg;

    se->avg.runnable_load_sum = runnable_sum;
    se->avg.runnable_load_avg = runnable_load_avg;

    if (se->on_rq) {
        add_positive(&cfs_rq->avg.runnable_load_avg, delta_avg);
        add_positive(&cfs_rq->avg.runnable_load_sum, delta_sum);
    }

    pr_fn_end_on(stack_depth);
}

static inline void add_tg_cfs_propagate(struct cfs_rq *cfs_rq, long runnable_sum)
{
    cfs_rq->propagate = 1;
    cfs_rq->prop_runnable_sum += runnable_sum;
}

/* Update task and its cfs_rq load average */
static inline int propagate_entity_load_avg(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    struct cfs_rq *cfs_rq, *gcfs_rq;

    if (entity_is_task(se))
        return 0;

    gcfs_rq = group_cfs_rq(se);
    if (!gcfs_rq->propagate)
        return 0;

    gcfs_rq->propagate = 0;

    cfs_rq = cfs_rq_of(se);

    add_tg_cfs_propagate(cfs_rq, gcfs_rq->prop_runnable_sum);

    update_tg_cfs_util(cfs_rq, se, gcfs_rq);
    update_tg_cfs_runnable(cfs_rq, se, gcfs_rq);

    //trace_pelt_cfs_tp(cfs_rq);
    //trace_pelt_se_tp(se);

    pr_fn_end_on(stack_depth);

    return 1;
}

/*
 * Check if we need to update the load and the utilization of a blocked
 * group_entity:
 */
static inline bool skip_blocked_update(struct sched_entity *se)
{
    struct cfs_rq *gcfs_rq = group_cfs_rq(se);

    /*
     * If sched_entity still have not zero load or utilization, we have to
     * decay it:
     */
    if (se->avg.load_avg || se->avg.util_avg)
        return false;

    /*
     * If there is a pending propagation, we have to update the load and
     * the utilization of the sched_entity:
     */
    if (gcfs_rq->propagate)
        return false;

    /*
     * Otherwise, the load and the utilization of the sched_entity is
     * already zero and there is no pending propagation, so it will be a
     * waste of time to try to decay it:
     */
    return true;
}

#else /* CONFIG_FAIR_GROUP_SCHED */

static inline void update_tg_load_avg(struct cfs_rq *cfs_rq, int force) {}

static inline int propagate_entity_load_avg(struct sched_entity *se)
{
    return 0;
}

static inline void add_tg_cfs_propagate(struct cfs_rq *cfs_rq, long runnable_sum) {}

#endif /* CONFIG_FAIR_GROUP_SCHED */
//3452
/**
 * update_cfs_rq_load_avg - update the cfs_rq's load/util averages
 * @now: current time, as per cfs_rq_clock_pelt()
 * @cfs_rq: cfs_rq to update
 *
 * The cfs_rq avg is the direct sum of all its entities (blocked and runnable)
 * avg. The immediate corollary is that all (fair) tasks must be attached, see
 * post_init_entity_util_avg().
 *
 * cfs_rq->avg is used for task_h_load() and update_cfs_share() for example.
 *
 * Returns true if the load decayed or we removed load.
 *
 * Since both these conditions indicate a changed cfs_rq->avg.load we should
 * call update_tg_load_avg() when this function returns true.
 */
static inline int
update_cfs_rq_load_avg(u64 now, struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    unsigned long removed_load = 0, removed_util = 0, removed_runnable_sum = 0;
    struct sched_avg *sa = &cfs_rq->avg;
    int decayed = 0;

    if (cfs_rq->removed.nr) {
        unsigned long r;
        u32 divider = LOAD_AVG_MAX - 1024 + sa->period_contrib;

        //raw_spin_lock(&cfs_rq->removed.lock);
        swap(cfs_rq->removed.util_avg, removed_util);
        swap(cfs_rq->removed.load_avg, removed_load);
        swap(cfs_rq->removed.runnable_sum, removed_runnable_sum);
        cfs_rq->removed.nr = 0;
        //raw_spin_unlock(&cfs_rq->removed.lock);

        r = removed_load;
        sub_positive(&sa->load_avg, r);
        sub_positive(&sa->load_sum, r * divider);

        r = removed_util;
        sub_positive(&sa->util_avg, r);
        sub_positive(&sa->util_sum, r * divider);

        add_tg_cfs_propagate(cfs_rq, -(long)removed_runnable_sum);

        decayed = 1;
    }

    decayed |= __update_load_avg_cfs_rq(now, cfs_rq);

#ifndef CONFIG_64BIT
    smp_wmb();
    cfs_rq->load_last_update_time_copy = sa->last_update_time;
#endif

    if (decayed)
        cfs_rq_util_change(cfs_rq, 0);

    pr_fn_end_on(stack_depth);

    return decayed;
}

/**
 * attach_entity_load_avg - attach this entity to its cfs_rq load avg
 * @cfs_rq: cfs_rq to attach to
 * @se: sched_entity to attach
 * @flags: migration hints
 *
 * Must call update_cfs_rq_load_avg() before this, since we rely on
 * cfs_rq->avg.last_update_time being current.
 */
static void attach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{
    u32 divider = LOAD_AVG_MAX - 1024 + cfs_rq->avg.period_contrib;

    /*
     * When we attach the @se to the @cfs_rq, we must align the decay
     * window because without that, really weird and wonderful things can
     * happen.
     *
     * XXX illustrate
     */
    se->avg.last_update_time = cfs_rq->avg.last_update_time;
    se->avg.period_contrib = cfs_rq->avg.period_contrib;

    /*
     * Hell(o) Nasty stuff.. we need to recompute _sum based on the new
     * period_contrib. This isn't strictly correct, but since we're
     * entirely outside of the PELT hierarchy, nobody cares if we truncate
     * _sum a little.
     */
    se->avg.util_sum = se->avg.util_avg * divider;

    se->avg.load_sum = divider;
    if (se_weight(se)) {
        se->avg.load_sum =
            div_u64(se->avg.load_avg * se->avg.load_sum, se_weight(se));
    }

    se->avg.runnable_load_sum = se->avg.load_sum;

    enqueue_load_avg(cfs_rq, se);
    cfs_rq->avg.util_avg += se->avg.util_avg;
    cfs_rq->avg.util_sum += se->avg.util_sum;

    add_tg_cfs_propagate(cfs_rq, se->avg.load_sum);

    cfs_rq_util_change(cfs_rq, flags);

    //trace_pelt_cfs_tp(cfs_rq);
}

/**
 * detach_entity_load_avg - detach this entity from its cfs_rq load avg
 * @cfs_rq: cfs_rq to detach from
 * @se: sched_entity to detach
 *
 * Must call update_cfs_rq_load_avg() before this, since we rely on
 * cfs_rq->avg.last_update_time being current.
 */
static void detach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    dequeue_load_avg(cfs_rq, se);
    sub_positive(&cfs_rq->avg.util_avg, se->avg.util_avg);
    sub_positive(&cfs_rq->avg.util_sum, se->avg.util_sum);

    add_tg_cfs_propagate(cfs_rq, -se->avg.load_sum);

    cfs_rq_util_change(cfs_rq, 0);

    //trace_pelt_cfs_tp(cfs_rq);
}

/*
 * Optional action to be done while updating the load average
 */
#define UPDATE_TG		0x1
#define SKIP_AGE_LOAD	0x2
#define DO_ATTACH		0x4

/* Update task and its cfs_rq load average */
static inline void update_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{
    pr_fn_start_on(stack_depth);

    u64 now = cfs_rq_clock_pelt(cfs_rq);
    int decayed;

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%20s : %d\n", flags);
    pr_info_view_on(stack_depth, "%20s : %llu\n", now);

    /*
     * Track task load average for carrying it to new CPU after migrated, and
     * track group sched_entity load average for task_h_load calc in migration
     */
    if (se->avg.last_update_time && !(flags & SKIP_AGE_LOAD))
        __update_load_avg_se(now, cfs_rq, se);

    decayed  = update_cfs_rq_load_avg(now, cfs_rq);
    decayed |= propagate_entity_load_avg(se);

    if (!se->avg.last_update_time && (flags & DO_ATTACH)) {

        /*
         * DO_ATTACH means we're here from enqueue_entity().
         * !last_update_time means we've passed through
         * migrate_task_rq_fair() indicating we migrated.
         *
         * IOW we're enqueueing a task on a new CPU.
         */
        attach_entity_load_avg(cfs_rq, se, SCHED_CPUFREQ_MIGRATION);
        update_tg_load_avg(cfs_rq, 0);

    } else if (decayed && (flags & UPDATE_TG))
        update_tg_load_avg(cfs_rq, 0);

    pr_fn_end_on(stack_depth);
}

#ifndef CONFIG_64BIT
static inline u64 cfs_rq_last_update_time(struct cfs_rq *cfs_rq)
{
    u64 last_update_time_copy;
    u64 last_update_time;

    do {
        last_update_time_copy = cfs_rq->load_last_update_time_copy;
        smp_rmb();
        last_update_time = cfs_rq->avg.last_update_time;
    } while (last_update_time != last_update_time_copy);

    return last_update_time;
}
#else
static inline u64 cfs_rq_last_update_time(struct cfs_rq *cfs_rq)
{
    return cfs_rq->avg.last_update_time;
}
#endif

/*
 * Synchronize entity load avg of dequeued entity without locking
 * the previous rq.
 */
static void sync_entity_load_avg(struct sched_entity *se)
{
    struct cfs_rq *cfs_rq = cfs_rq_of(se);
    u64 last_update_time;

    last_update_time = cfs_rq_last_update_time(cfs_rq);
    __update_load_avg_blocked_se(last_update_time, se);
}

/*
 * Task first catches up with cfs_rq, and then subtract
 * itself from the cfs_rq (task must be off the queue now).
 */
static void remove_entity_load_avg(struct sched_entity *se)
{
    struct cfs_rq *cfs_rq = cfs_rq_of(se);
    unsigned long flags;

    /*
     * tasks cannot exit without having gone through wake_up_new_task() ->
     * post_init_entity_util_avg() which will have added things to the
     * cfs_rq, so we can remove unconditionally.
     */

    sync_entity_load_avg(se);

    //raw_spin_lock_irqsave(&cfs_rq->removed.lock, flags);
    ++cfs_rq->removed.nr;
    cfs_rq->removed.util_avg	+= se->avg.util_avg;
    cfs_rq->removed.load_avg	+= se->avg.load_avg;
    cfs_rq->removed.runnable_sum	+= se->avg.load_sum; /* == runnable_sum */
    //raw_spin_unlock_irqrestore(&cfs_rq->removed.lock, flags);
}

static inline unsigned long cfs_rq_runnable_load_avg(struct cfs_rq *cfs_rq)
{
    return cfs_rq->avg.runnable_load_avg;
}

static inline unsigned long cfs_rq_load_avg(struct cfs_rq *cfs_rq)
{
    return cfs_rq->avg.load_avg;
}

static inline unsigned long task_util(struct task_struct *p)
{
    return READ_ONCE(p->se.avg.util_avg);
}

static inline unsigned long _task_util_est(struct task_struct *p)
{
    struct util_est ue = READ_ONCE(p->se.avg.util_est);

    return (max(ue.ewma, ue.enqueued) | UTIL_AVG_UNCHANGED);
}

static inline unsigned long task_util_est(struct task_struct *p)
{
    return max(task_util(p), _task_util_est(p));
}

static inline void util_est_enqueue(struct cfs_rq *cfs_rq,
                    struct task_struct *p)
{
    unsigned int enqueued;

    if (!sched_feat(UTIL_EST))
        return;

    /* Update root cfs_rq's estimated utilization */
    enqueued  = cfs_rq->avg.util_est.enqueued;
    enqueued += _task_util_est(p);
    WRITE_ONCE(cfs_rq->avg.util_est.enqueued, enqueued);
}

/*
 * Check if a (signed) value is within a specified (unsigned) margin,
 * based on the observation that:
 *
 *     abs(x) < y := (unsigned)(x + y - 1) < (2 * y - 1)
 *
 * NOTE: this only works when value + maring < INT_MAX.
 */
static inline bool within_margin(int value, int margin)
{
    return ((unsigned int)(value + margin - 1) < (2 * margin - 1));
}

static void
util_est_dequeue(struct cfs_rq *cfs_rq, struct task_struct *p, bool task_sleep)
{
    long last_ewma_diff;
    struct util_est ue;
    int cpu;

    if (!sched_feat(UTIL_EST))
        return;

    /* Update root cfs_rq's estimated utilization */
    ue.enqueued  = cfs_rq->avg.util_est.enqueued;
    ue.enqueued -= min_t(unsigned int, ue.enqueued, _task_util_est(p));
    WRITE_ONCE(cfs_rq->avg.util_est.enqueued, ue.enqueued);

    /*
     * Skip update of task's estimated utilization when the task has not
     * yet completed an activation, e.g. being migrated.
     */
    if (!task_sleep)
        return;

    /*
     * If the PELT values haven't changed since enqueue time,
     * skip the util_est update.
     */
    ue = p->se.avg.util_est;
    if (ue.enqueued & UTIL_AVG_UNCHANGED)
        return;

    /*
     * Skip update of task's estimated utilization when its EWMA is
     * already ~1% close to its last activation value.
     */
    ue.enqueued = (task_util(p) | UTIL_AVG_UNCHANGED);
    last_ewma_diff = ue.enqueued - ue.ewma;
    if (within_margin(last_ewma_diff, (SCHED_CAPACITY_SCALE / 100)))
        return;

    /*
     * To avoid overestimation of actual task utilization, skip updates if
     * we cannot grant there is idle time in this CPU.
     */
    cpu = cpu_of(rq_of(cfs_rq));
    if (task_util(p) > capacity_orig_of(cpu))
        return;

    /*
     * Update Task's estimated utilization
     *
     * When *p completes an activation we can consolidate another sample
     * of the task size. This is done by storing the current PELT value
     * as ue.enqueued and by using this value to update the Exponential
     * Weighted Moving Average (EWMA):
     *
     *  ewma(t) = w *  task_util(p) + (1-w) * ewma(t-1)
     *          = w *  task_util(p) +         ewma(t-1)  - w * ewma(t-1)
     *          = w * (task_util(p) -         ewma(t-1)) +     ewma(t-1)
     *          = w * (      last_ewma_diff            ) +     ewma(t-1)
     *          = w * (last_ewma_diff  +  ewma(t-1) / w)
     *
     * Where 'w' is the weight of new samples, which is configured to be
     * 0.25, thus making w=1/4 ( >>= UTIL_EST_WEIGHT_SHIFT)
     */
    ue.ewma <<= UTIL_EST_WEIGHT_SHIFT;
    ue.ewma  += last_ewma_diff;
    ue.ewma >>= UTIL_EST_WEIGHT_SHIFT;
    WRITE_ONCE(p->se.avg.util_est, ue);
}

static inline int task_fits_capacity(struct task_struct *p, long capacity)
{
    return fits_capacity(task_util_est(p), capacity);
}

static inline void update_misfit_status(struct task_struct *p, struct rq *rq)
{
    pr_fn_start_on(stack_depth);

    if (!static_branch_unlikely(&sched_asym_cpucapacity))
        return;

    if (!p) {
        rq->misfit_task_load = 0;
        return;
    }

    if (task_fits_capacity(p, capacity_of(cpu_of(rq)))) {
        rq->misfit_task_load = 0;
        return;
    }

    rq->misfit_task_load = task_h_load(p);

    pr_fn_end_on(stack_depth);
}

#else /* CONFIG_SMP */

#define UPDATE_TG	0x0
#define SKIP_AGE_LOAD	0x0
#define DO_ATTACH	0x0

static inline void update_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se, int not_used1)
{
    cfs_rq_util_change(cfs_rq, 0);
}

static inline void remove_entity_load_avg(struct sched_entity *se) {}

static inline void
attach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags) {}
static inline void
detach_entity_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se) {}

static inline int idle_balance(struct rq *rq, struct rq_flags *rf)
{
    return 0;
}

static inline void
util_est_enqueue(struct cfs_rq *cfs_rq, struct task_struct *p) {}

static inline void
util_est_dequeue(struct cfs_rq *cfs_rq, struct task_struct *p,
         bool task_sleep) {}
static inline void update_misfit_status(struct task_struct *p, struct rq *rq) {}

#endif /* CONFIG_SMP */
//3861 lines
static void check_spread(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
#ifdef CONFIG_SCHED_DEBUG
    s64 d = se->vruntime - cfs_rq->min_vruntime;

    if (d < 0)
        d = -d;

    if (d > 3*sysctl_sched_latency)
        schedstat_inc(cfs_rq->nr_spread_over);
#endif
}

static void
place_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int initial)
{
    u64 vruntime = cfs_rq->min_vruntime;

    pr_fn_start_on(stack_depth);

    /*
     * The 'current' period is already promised to the current tasks,
     * however the extra weight of the new task will slow them down a
     * little, place the new task so that it fits in the slot that
     * stays open at the end.
     */
    if (initial && sched_feat(START_DEBIT))
        vruntime += sched_vslice(cfs_rq, se);

    /* sleeps up to a single latency don't count. */
    if (!initial) {
        unsigned long thresh = sysctl_sched_latency;

        /*
         * Halve their sleep time's effect, to allow
         * for a gentler effect of sleepers:
         */
        if (sched_feat(GENTLE_FAIR_SLEEPERS))
            thresh >>= 1;

        vruntime -= thresh;
    }

    /* ensure we never gain time by being placed backwards. */
    se->vruntime = max_vruntime(se->vruntime, vruntime);

    pr_fn_end_on(stack_depth);
}

static void check_enqueue_throttle(struct cfs_rq *cfs_rq);

static inline void check_schedstat_required(void)
{
#ifdef CONFIG_SCHEDSTATS
    if (schedstat_enabled())
        return;
#if 0
    /* Force schedstat enabled if a dependent tracepoint is active */
    if (trace_sched_stat_wait_enabled()    ||
            trace_sched_stat_sleep_enabled()   ||
            trace_sched_stat_iowait_enabled()  ||
            trace_sched_stat_blocked_enabled() ||
            trace_sched_stat_runtime_enabled())  {
        printk_deferred_once("Scheduler tracepoints stat_sleep, stat_iowait, "
                 "stat_blocked and stat_runtime require the "
                 "kernel parameter schedstats=enable or "
                 "kernel.sched_schedstats=1\n");
    }
#endif //0
#endif
}
//3928 lines


//3959 lines
static void
enqueue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{
    bool renorm = !(flags & ENQUEUE_WAKEUP) || (flags & ENQUEUE_MIGRATED);
    bool curr = cfs_rq->curr == se;

    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->curr);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);

    /*
     * If we're the current task, we must renormalise before calling
     * update_curr().
     */
    if (renorm && curr)
        se->vruntime += cfs_rq->min_vruntime;

    update_curr(cfs_rq);

    /*
     * Otherwise, renormalise after, such that we're placed at the current
     * moment in time, instead of some random moment in the past. Being
     * placed in the past could significantly boost this task to the
     * fairness detriment of existing tasks.
     */
    if (renorm && !curr)
        se->vruntime += cfs_rq->min_vruntime;

    /*
     * When enqueuing a sched_entity, we must:
     *   - Update loads to have both entity and cfs_rq synced with now.
     *   - Add its load to cfs_rq->runnable_avg
     *   - For group_entity, update its weight to reflect the new share of
     *     its group cfs_rq
     *   - Add its new weight to cfs_rq->load.weight
     */
    update_load_avg(cfs_rq, se, UPDATE_TG | DO_ATTACH);
    update_cfs_group(se);
    enqueue_runnable_load_avg(cfs_rq, se);

    account_entity_enqueue(cfs_rq, se);

    if (flags & ENQUEUE_WAKEUP)
        place_entity(cfs_rq, se, 0);

    check_schedstat_required();
    update_stats_enqueue(cfs_rq, se, flags);
    check_spread(cfs_rq, se);
    if (!curr)
        __enqueue_entity(cfs_rq, se);
    se->on_rq = 1;

    if (cfs_rq->nr_running == 1) {
        list_add_leaf_cfs_rq(cfs_rq);	//error
        //check_enqueue_throttle(cfs_rq);
    }

    pr_fn_end_on(stack_depth);
}
//4012
static void __clear_buddies_last(struct sched_entity *se)
{
    for_each_sched_entity(se) {
        struct cfs_rq *cfs_rq = cfs_rq_of(se);
        if (cfs_rq->last != se)
            break;

        cfs_rq->last = NULL;
    }
}

static void __clear_buddies_next(struct sched_entity *se)
{
    for_each_sched_entity(se) {
        struct cfs_rq *cfs_rq = cfs_rq_of(se);
        if (cfs_rq->next != se)
            break;

        cfs_rq->next = NULL;
    }
}

static void __clear_buddies_skip(struct sched_entity *se)
{
    for_each_sched_entity(se) {
        struct cfs_rq *cfs_rq = cfs_rq_of(se);
        if (cfs_rq->skip != se)
            break;

        cfs_rq->skip = NULL;
    }
}

static void clear_buddies(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    if (cfs_rq->last == se)
        __clear_buddies_last(se);

    if (cfs_rq->next == se)
        __clear_buddies_next(se);

    if (cfs_rq->skip == se)
        __clear_buddies_skip(se);
}

static __always_inline void return_cfs_rq_runtime(struct cfs_rq *cfs_rq);

static void
dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
{
    pr_fn_start_on(stack_depth);
    /*
     * Update run-time statistics of the 'current'.
     */
    update_curr(cfs_rq);

    /*
     * When dequeuing a sched_entity, we must:
     *   - Update loads to have both entity and cfs_rq synced with now.
     *   - Subtract its load from the cfs_rq->runnable_avg.
     *   - Subtract its previous weight from cfs_rq->load.weight.
     *   - For group entity, update its weight to reflect the new share
     *     of its group cfs_rq.
     */
    update_load_avg(cfs_rq, se, UPDATE_TG);
    dequeue_runnable_load_avg(cfs_rq, se);

    update_stats_dequeue(cfs_rq, se, flags);

    clear_buddies(cfs_rq, se);

    if (se != cfs_rq->curr)
        __dequeue_entity(cfs_rq, se);
    se->on_rq = 0;
    account_entity_dequeue(cfs_rq, se);

    /*
     * Normalize after update_curr(); which will also have moved
     * min_vruntime if @se is the one holding it back. But before doing
     * update_min_vruntime() again, which will discount @se's position and
     * can move min_vruntime forward still more.
     */
    if (!(flags & DEQUEUE_SLEEP))
        se->vruntime -= cfs_rq->min_vruntime;

    /* return excess runtime on last dequeue */
    return_cfs_rq_runtime(cfs_rq);

    update_cfs_group(se);

    /*
     * Now advance min_vruntime if @se was the entity holding it back,
     * except when: DEQUEUE_SAVE && !DEQUEUE_MOVE, in this case we'll be
     * put back on, and if we advance min_vruntime, we'll be placed back
     * further than we started -- ie. we'll be penalized.
     */
    if ((flags & (DEQUEUE_SAVE | DEQUEUE_MOVE)) != DEQUEUE_SAVE)
        update_min_vruntime(cfs_rq);

    pr_fn_end_on(stack_depth);
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void
check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
    pr_fn_start_on(stack_depth);

    unsigned long ideal_runtime, delta_exec;
    struct sched_entity *se;
    s64 delta;

    ideal_runtime = sched_slice(cfs_rq, curr);
    delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
    if (delta_exec > ideal_runtime) {
        resched_curr(rq_of(cfs_rq));
        /*
         * The current task ran long enough, ensure it doesn't get
         * re-elected due to buddy favours.
         */
        clear_buddies(cfs_rq, curr);
        return;
    }

    /*
     * Ensure that a task that missed wakeup preemption by a
     * narrow margin doesn't have to wait for a full slice.
     * This also mitigates buddy induced latencies under load.
     */
    if (delta_exec < sysctl_sched_min_granularity)
        return;

    se = __pick_first_entity(cfs_rq);
    delta = curr->vruntime - se->vruntime;

    if (delta < 0)
        return;

    if (delta > ideal_runtime)
        resched_curr(rq_of(cfs_rq));

    pr_fn_end_on(stack_depth);
}
//4151
static void
set_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    /* 'current' is not kept within the tree. */
    if (se->on_rq) {
        /*
         * Any task has to be enqueued before it get to execute on
         * a CPU. So account for the time it spent waiting on the
         * runqueue.
         */
        update_stats_wait_end(cfs_rq, se);
        __dequeue_entity(cfs_rq, se);
        update_load_avg(cfs_rq, se, UPDATE_TG);
    }

    update_stats_curr_start(cfs_rq, se);
    cfs_rq->curr = se;
    pr_info_view_on(stack_depth, "%30s : %p\n", cfs_rq->curr);

    /*
     * Track our maximum slice length, if the CPU's load is at
     * least twice that of our own weight (i.e. dont track it
     * when there are only lesser-weight tasks around):
     */
    if (schedstat_enabled() &&
        rq_of(cfs_rq)->cfs.load.weight >= 2*se->load.weight) {
        schedstat_set(se->statistics.slice_max,
            max((u64)schedstat_val(se->statistics.slice_max),
                se->sum_exec_runtime - se->prev_sum_exec_runtime));
    }

    se->prev_sum_exec_runtime = se->sum_exec_runtime;

    pr_fn_end_on(stack_depth);
}
//4184
static int
wakeup_preempt_entity(struct sched_entity *curr, struct sched_entity *se);

/*
 * Pick the next process, keeping these things in mind, in this order:
 * 1) keep things fair between processes/task groups
 * 2) pick the "next" process, since someone really wants that to run
 * 3) pick the "last" process, for cache locality
 * 4) do not run the "skip" process, if something else is available
 */
static struct sched_entity *
pick_next_entity(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
    struct sched_entity *left = __pick_first_entity(cfs_rq);
    struct sched_entity *se;

    /*
     * If curr is set we have to see if its left of the leftmost entity
     * still in the tree, provided there was anything in the tree at all.
     */
    if (!left || (curr && entity_before(curr, left)))
        left = curr;

    se = left; /* ideally we run the leftmost entity */

    /*
     * Avoid running the skip buddy, if running something else can
     * be done without getting too unfair.
     */
    if (cfs_rq->skip == se) {
        struct sched_entity *second;

        if (se == curr) {
            second = __pick_first_entity(cfs_rq);
        } else {
            second = __pick_next_entity(se);
            if (!second || (curr && entity_before(curr, second)))
                second = curr;
        }

        if (second && wakeup_preempt_entity(second, left) < 1)
            se = second;
    }

    /*
     * Prefer last buddy, try to return the CPU to a preempted task.
     */
    if (cfs_rq->last && wakeup_preempt_entity(cfs_rq->last, left) < 1)
        se = cfs_rq->last;

    /*
     * Someone really wants this to run. If it's not unfair, run it.
     */
    if (cfs_rq->next && wakeup_preempt_entity(cfs_rq->next, left) < 1)
        se = cfs_rq->next;

    clear_buddies(cfs_rq, se);

    return se;
}

static bool check_cfs_rq_runtime(struct cfs_rq *cfs_rq);

static void put_prev_entity(struct cfs_rq *cfs_rq, struct sched_entity *prev)
{
    /*
     * If still on the runqueue then deactivate_task()
     * was not called and update_curr() has to be done:
     */
    if (prev->on_rq)
        update_curr(cfs_rq);

    /* throttle cfs_rqs exceeding runtime */
    check_cfs_rq_runtime(cfs_rq);

    check_spread(cfs_rq, prev);

    if (prev->on_rq) {
        update_stats_wait_start(cfs_rq, prev);
        /* Put 'current' back into the tree. */
        __enqueue_entity(cfs_rq, prev);
        /* in !on_rq case, update occurred at dequeue */
        update_load_avg(cfs_rq, prev, 0);
    }
    cfs_rq->curr = NULL;
}
//4271
static void
entity_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr, int queued)
{
    pr_fn_start_on(stack_depth);
    /*
     * Update run-time statistics of the 'current'.
     */
    update_curr(cfs_rq);

    /*
     * Ensure that runnable average is periodically updated.
     */
    update_load_avg(cfs_rq, curr, UPDATE_TG);
    update_cfs_group(curr);

#ifdef CONFIG_SCHED_HRTICK
    /*
     * queued ticks are scheduled to match the slice, so don't bother
     * validating it and just reschedule.
     */
    if (queued) {
        resched_curr(rq_of(cfs_rq));
        return;
    }
    /*
     * don't let the period tick interfere with the hrtick preemption
     */
    if (!sched_feat(DOUBLE_TICK) &&
            hrtimer_active(&rq_of(cfs_rq)->hrtick_timer))
        return;
#endif

    if (cfs_rq->nr_running > 1)
        check_preempt_tick(cfs_rq, curr);

    pr_fn_end_on(stack_depth);
}
//4306
//4307
/**************************************************
 * CFS bandwidth control machinery
 */

#ifdef CONFIG_CFS_BANDWIDTH

#ifdef CONFIG_JUMP_LABEL
static struct static_key __cfs_bandwidth_used;

static inline bool cfs_bandwidth_used(void)
{
    return static_key_false(&__cfs_bandwidth_used);
}

void cfs_bandwidth_usage_inc(void)
{
    static_key_slow_inc_cpuslocked(&__cfs_bandwidth_used);
}

void cfs_bandwidth_usage_dec(void)
{
    static_key_slow_dec_cpuslocked(&__cfs_bandwidth_used);
}
#else /* CONFIG_JUMP_LABEL */
static bool cfs_bandwidth_used(void)
{
    return true;
}

void cfs_bandwidth_usage_inc(void) {}
void cfs_bandwidth_usage_dec(void) {}
#endif /* CONFIG_JUMP_LABEL */

/*
 * default period for cfs group bandwidth.
 * default: 0.1s, units: nanoseconds
 */
static inline u64 default_cfs_period(void)
{
    return 100000000ULL;
}

static inline u64 sched_cfs_bandwidth_slice(void)
{
    return (u64)sysctl_sched_cfs_bandwidth_slice * NSEC_PER_USEC;
}

/*
 * Replenish runtime according to assigned quota. We use sched_clock_cpu
 * directly instead of rq->clock to avoid adding additional synchronization
 * around rq->lock.
 *
 * requires cfs_b->lock
 */
void __refill_cfs_bandwidth_runtime(struct cfs_bandwidth *cfs_b)
{
    if (cfs_b->quota != RUNTIME_INF)
        cfs_b->runtime = cfs_b->quota;
}

static inline struct cfs_bandwidth *tg_cfs_bandwidth(struct task_group *tg)
{
    return &tg->cfs_bandwidth;
}
//4372
/* returns 0 on failure to allocate runtime */
static int assign_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
    struct task_group *tg = cfs_rq->tg;
    struct cfs_bandwidth *cfs_b = tg_cfs_bandwidth(tg);
    u64 amount = 0, min_amount;

    /* note: this is a positive sum as runtime_remaining <= 0 */
    min_amount = sched_cfs_bandwidth_slice() - cfs_rq->runtime_remaining;

    raw_spin_lock(&cfs_b->lock);
    if (cfs_b->quota == RUNTIME_INF)
        amount = min_amount;
    else {
        start_cfs_bandwidth(cfs_b);

        if (cfs_b->runtime > 0) {
            amount = min(cfs_b->runtime, min_amount);
            cfs_b->runtime -= amount;
            cfs_b->idle = 0;
        }
    }
    raw_spin_unlock(&cfs_b->lock);

    cfs_rq->runtime_remaining += amount;

    return cfs_rq->runtime_remaining > 0;
}

static void __account_cfs_rq_runtime(struct cfs_rq *cfs_rq, u64 delta_exec)
{
    /* dock delta_exec before expiring quota (as it could span periods) */
    cfs_rq->runtime_remaining -= delta_exec;

    if (likely(cfs_rq->runtime_remaining > 0))
        return;

    if (cfs_rq->throttled)
        return;
    /*
     * if we're unable to extend our runtime we resched so that the active
     * hierarchy can be throttled
     */
    if (!assign_cfs_rq_runtime(cfs_rq) && likely(cfs_rq->curr))
        resched_curr(rq_of(cfs_rq));
}

static __always_inline
void account_cfs_rq_runtime(struct cfs_rq *cfs_rq, u64 delta_exec)
{
    if (!cfs_bandwidth_used() || !cfs_rq->runtime_enabled)
        return;

    __account_cfs_rq_runtime(cfs_rq, delta_exec);
}
//4428
static inline int cfs_rq_throttled(struct cfs_rq *cfs_rq)
{
    return cfs_bandwidth_used() && cfs_rq->throttled;
}

/* check whether cfs_rq, or any parent, is throttled */
static inline int throttled_hierarchy(struct cfs_rq *cfs_rq)
{
    return cfs_bandwidth_used() && cfs_rq->throttle_count;
}
//4439 lines






//4731 lines
/* a cfs_rq won't donate quota below this amount */
static const u64 min_cfs_rq_runtime = 1 * NSEC_PER_MSEC;
/* minimum remaining period time to redistribute slack quota */
static const u64 min_bandwidth_expiration = 2 * NSEC_PER_MSEC;
/* how long we wait to gather additional slack before distributing */
static const u64 cfs_bandwidth_slack_period = 5 * NSEC_PER_MSEC;
//4738
/*
 * Are we near the end of the current quota period?
 *
 * Requires cfs_b->lock for hrtimer_expires_remaining to be safe against the
 * hrtimer base being cleared by hrtimer_start. In the case of
 * migrate_hrtimers, base is never cleared, so we are fine.
 */
static int runtime_refresh_within(struct cfs_bandwidth *cfs_b, u64 min_expire)
{
        struct hrtimer *refresh_timer = &cfs_b->period_timer;
        u64 remaining;
#if 0
        /* if the call-back is running a quota refresh is already occurring */
        if (hrtimer_callback_running(refresh_timer))
                return 1;

        /* is a quota refresh about to occur? */
        remaining = ktime_to_ns(hrtimer_expires_remaining(refresh_timer));
        if (remaining < min_expire)
                return 1;
#endif //0
        return 0;
}

static void start_cfs_slack_bandwidth(struct cfs_bandwidth *cfs_b)
{
        u64 min_left = cfs_bandwidth_slack_period + min_bandwidth_expiration;

        /* if there's a quota refresh soon don't bother with slack */
        if (runtime_refresh_within(cfs_b, min_left))
                return;

        /* don't push forwards an existing deferred unthrottle */
        if (cfs_b->slack_started)
                return;
        cfs_b->slack_started = true;

        hrtimer_start(&cfs_b->slack_timer,
                        ns_to_ktime(cfs_bandwidth_slack_period),
                        HRTIMER_MODE_REL);
}
//4780
/* we know any runtime found here is valid as update_curr() precedes return */
static void __return_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
        struct cfs_bandwidth *cfs_b = tg_cfs_bandwidth(cfs_rq->tg);
        s64 slack_runtime = cfs_rq->runtime_remaining - min_cfs_rq_runtime;

        if (slack_runtime <= 0)
                return;

        raw_spin_lock(&cfs_b->lock);
        if (cfs_b->quota != RUNTIME_INF) {
                cfs_b->runtime += slack_runtime;

                /* we are under rq->lock, defer unthrottling using a timer */
                if (cfs_b->runtime > sched_cfs_bandwidth_slice() &&
                    !list_empty(&cfs_b->throttled_cfs_rq))
                        start_cfs_slack_bandwidth(cfs_b);
        }
        raw_spin_unlock(&cfs_b->lock);

        /* even if it's not valid for return we don't want to try again */
        cfs_rq->runtime_remaining -= slack_runtime;
}

static __always_inline void return_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
        if (!cfs_bandwidth_used())
                return;

        if (!cfs_rq->runtime_enabled || cfs_rq->nr_running)
                return;

        __return_cfs_rq_runtime(cfs_rq);
}
//4819 lines





//4878 lines
/* conditionally throttle active cfs_rq's from put_prev_entity() */
static bool check_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
    if (!cfs_bandwidth_used())
        return false;

    if (likely(!cfs_rq->runtime_enabled || cfs_rq->runtime_remaining > 0))
        return false;

    /*
     * it's possible for a throttled entity to be forced into a running
     * state (e.g. set_curr_task), in this case we're finished.
     */
    if (cfs_rq_throttled(cfs_rq))
        return true;

    //throttle_cfs_rq(cfs_rq);
    return true;
}
//4898 lines





//4964 lines
void init_cfs_bandwidth(struct cfs_bandwidth *cfs_b)
{
    raw_spin_lock_init(&cfs_b->lock);
    cfs_b->runtime = 0;
    cfs_b->quota = RUNTIME_INF;
    //cfs_b->period = ns_to_ktime(default_cfs_period());

    INIT_LIST_HEAD(&cfs_b->throttled_cfs_rq);
    hrtimer_init(&cfs_b->period_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS_PINNED);
    //cfs_b->period_timer.function = sched_cfs_period_timer;
    hrtimer_init(&cfs_b->slack_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    //cfs_b->slack_timer.function = sched_cfs_slack_timer;
    cfs_b->distribute_running = 0;
    cfs_b->slack_started = false;
}



//4980 lines
static void init_cfs_rq_runtime(struct cfs_rq *cfs_rq)
{
    cfs_rq->runtime_enabled = 0;
    INIT_LIST_HEAD(&cfs_rq->throttled_list);
}

void start_cfs_bandwidth(struct cfs_bandwidth *cfs_b)
{
    lockdep_assert_held(&cfs_b->lock);

    if (cfs_b->period_active)
        return;

    cfs_b->period_active = 1;
    hrtimer_forward_now(&cfs_b->period_timer, cfs_b->period);
    //hrtimer_start_expires(&cfs_b->period_timer, HRTIMER_MODE_ABS_PINNED);
}

static void destroy_cfs_bandwidth(struct cfs_bandwidth *cfs_b)
{
    /* init_cfs_bandwidth() was not called */
    if (!cfs_b->throttled_cfs_rq.next)
        return;

    hrtimer_cancel(&cfs_b->period_timer);
    hrtimer_cancel(&cfs_b->slack_timer);
}
//5008 lines





//5065 lines
#else /* CONFIG_CFS_BANDWIDTH */

static inline bool cfs_bandwidth_used(void)
{
        return false;
}

static void account_cfs_rq_runtime(struct cfs_rq *cfs_rq, u64 delta_exec) {}
static bool check_cfs_rq_runtime(struct cfs_rq *cfs_rq) { return false; }
static void check_enqueue_throttle(struct cfs_rq *cfs_rq) {}
static inline void sync_throttle(struct task_group *tg, int cpu) {}
static __always_inline void return_cfs_rq_runtime(struct cfs_rq *cfs_rq) {}

static inline int cfs_rq_throttled(struct cfs_rq *cfs_rq)
{
        return 0;
}

static inline int throttled_hierarchy(struct cfs_rq *cfs_rq)
{
        return 0;
}

static inline int throttled_lb_pair(struct task_group *tg,
                                    int src_cpu, int dest_cpu)
{
        return 0;
}

void init_cfs_bandwidth(struct cfs_bandwidth *cfs_b) {}

#ifdef CONFIG_FAIR_GROUP_SCHED
static void init_cfs_rq_runtime(struct cfs_rq *cfs_rq) {}
#endif

static inline struct cfs_bandwidth *tg_cfs_bandwidth(struct task_group *tg)
{
        return NULL;
}
static inline void destroy_cfs_bandwidth(struct cfs_bandwidth *cfs_b) {}
static inline void update_runtime_enabled(struct rq *rq) {}
static inline void unthrottle_offline_cfs_rqs(struct rq *rq) {}

#endif /* CONFIG_CFS_BANDWIDTH */
//5110
/**************************************************
 * CFS operations on tasks:
 */

#ifdef CONFIG_SCHED_HRTICK
static void hrtick_start_fair(struct rq *rq, struct task_struct *p)
{
    struct sched_entity *se = &p->se;
    struct cfs_rq *cfs_rq = cfs_rq_of(se);

    SCHED_WARN_ON(task_rq(p) != rq);

    if (rq->cfs.h_nr_running > 1) {
        u64 slice = sched_slice(cfs_rq, se);
        u64 ran = se->sum_exec_runtime - se->prev_sum_exec_runtime;
        s64 delta = slice - ran;

        if (delta < 0) {
            if (rq->curr == p)
                resched_curr(rq);
            return;
        }
        hrtick_start(rq, delta);
    }
}

/*
 * called from enqueue/dequeue and updates the hrtick when the
 * current task is from our class and nr_running is low enough
 * to matter.
 */
static void hrtick_update(struct rq *rq)
{
    struct task_struct *curr = rq->curr;

    if (!hrtick_enabled(rq) || curr->sched_class != &fair_sched_class)
        return;

    if (cfs_rq_of(&curr->se)->nr_running < sched_nr_latency)
        hrtick_start_fair(rq, curr);
}
#else /* !CONFIG_SCHED_HRTICK */
static inline void
hrtick_start_fair(struct rq *rq, struct task_struct *p)
{
}

static inline void hrtick_update(struct rq *rq)
{
}
#endif

#ifdef CONFIG_SMP
static inline unsigned long cpu_util(int cpu);

static inline bool cpu_overutilized(int cpu)
{
    //return !fits_capacity(cpu_util(cpu), capacity_of(cpu));
}

static inline void update_overutilized_status(struct rq *rq)
{
    if (!READ_ONCE(rq->rd->overutilized) && cpu_overutilized(rq->cpu)) {
        WRITE_ONCE(rq->rd->overutilized, SG_OVERUTILIZED);
        //trace_sched_overutilized_tp(rq->rd, SG_OVERUTILIZED);
    }
}
#else
static inline void update_overutilized_status(struct rq *rq) { }
#endif
//5181
/*
 * The enqueue_task method is called before nr_running is
 * increased. Here we update the fair scheduling stats and
 * then put the task into the rbtree:
 */
static void
enqueue_task_fair(struct rq *rq, struct task_struct *p, int flags)
{
    struct cfs_rq *cfs_rq;
    struct sched_entity *se = &p->se;
    int idle_h_nr_running = task_has_idle_policy(p);

    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);

    /*
     * The code below (indirectly) updates schedutil which looks at
     * the cfs_rq utilization to select a frequency.
     * Let's add the task's estimated utilization to the cfs_rq's
     * estimated utilization, before we update schedutil.
     */
    util_est_enqueue(&rq->cfs, p);

    /*
     * If in_iowait is set, the code below may not trigger any cpufreq
     * utilization updates, so do it here explicitly with the IOWAIT flag
     * passed.
     */
    if (p->in_iowait)
        cpufreq_update_util(rq, SCHED_CPUFREQ_IOWAIT);

    for_each_sched_entity(se) {
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
        pr_info_view_on(stack_depth, "%30s : %d\n", se->on_rq);
        if (se->on_rq)
            break;
        cfs_rq = cfs_rq_of(se);
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
        WARN_ON (!cfs_rq);
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq_of(cfs_rq));
        if (!rq_of(cfs_rq)) {
            pr_err("NULL of rq_of(cfs_rq)\n");
            break;
        }
        enqueue_entity(cfs_rq, se, flags);

        /*
         * end evaluation on encountering a throttled cfs_rq
         *
         * note: in the case of encountering a throttled cfs_rq we will
         * post the final h_nr_running increment below.
         */
        if (cfs_rq_throttled(cfs_rq))
            break;
        cfs_rq->h_nr_running++;
        cfs_rq->idle_h_nr_running += idle_h_nr_running;

        flags = ENQUEUE_WAKEUP;
    }

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);

    for_each_sched_entity(se) {
        cfs_rq = cfs_rq_of(se);
        cfs_rq->h_nr_running++;
        cfs_rq->idle_h_nr_running += idle_h_nr_running;

        if (cfs_rq_throttled(cfs_rq))
            break;

        update_load_avg(cfs_rq, se, UPDATE_TG);
        update_cfs_group(se);
    }

    if (!se) {
        add_nr_running(rq, 1);
        /*
         * Since new tasks are assigned an initial util_avg equal to
         * half of the spare capacity of their CPU, tiny tasks have the
         * ability to cross the overutilized threshold, which will
         * result in the load balancer ruining all the task placement
         * done by EAS. As a way to mitigate that effect, do not account
         * for the first enqueue operation of new tasks during the
         * overutilized flag detection.
         *
         * A better way of solving this problem would be to wait for
         * the PELT signals of tasks to converge before taking them
         * into account, but that is not straightforward to implement,
         * and the following generally works well enough in practice.
         */
        if (flags & ENQUEUE_WAKEUP) {
            pr_info_view_on(stack_depth, "%30s : 0x%X\n", flags);
            //update_overutilized_status(rq);	//error
        }
    }

    if (cfs_bandwidth_used()) {
        /*
         * When bandwidth control is enabled; the cfs_rq_throttled()
         * breaks in the above iteration can result in incomplete
         * leaf list maintenance, resulting in triggering the assertion
         * below.
         */
        for_each_sched_entity(se) {
            cfs_rq = cfs_rq_of(se);

            if (list_add_leaf_cfs_rq(cfs_rq))
                break;
        }
    }

    assert_list_leaf_cfs_rq(rq);

    hrtick_update(rq);

    pr_fn_end_on(stack_depth);
}
//5282
static void set_next_buddy(struct sched_entity *se);

/*
 * The dequeue_task method is called before nr_running is
 * decreased. We remove the task from the rbtree and
 * update the fair scheduling stats:
 */
static void dequeue_task_fair(struct rq *rq, struct task_struct *p, int flags)
{
        struct cfs_rq *cfs_rq;
        struct sched_entity *se = &p->se;
        int task_sleep = flags & DEQUEUE_SLEEP;
        int idle_h_nr_running = task_has_idle_policy(p);

        pr_fn_start_on(stack_depth);

        for_each_sched_entity(se) {
                cfs_rq = cfs_rq_of(se);
                pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
                pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
                dequeue_entity(cfs_rq, se, flags);

                /*
                 * end evaluation on encountering a throttled cfs_rq
                 *
                 * note: in the case of encountering a throttled cfs_rq we will
                 * post the final h_nr_running decrement below.
                */
                if (cfs_rq_throttled(cfs_rq))
                        break;
                cfs_rq->h_nr_running--;
                cfs_rq->idle_h_nr_running -= idle_h_nr_running;

                /* Don't dequeue parent if it has other entities besides us */
                if (cfs_rq->load.weight) {
                        /* Avoid re-evaluating load for this entity: */
                        se = parent_entity(se);
                        /*
                         * Bias pick_next to pick a task from this cfs_rq, as
                         * p is sleeping when it is within its sched_slice.
                         */
                        if (task_sleep && se && !throttled_hierarchy(cfs_rq))
                                set_next_buddy(se);
                        break;
                }
                flags |= DEQUEUE_SLEEP;
        }

        for_each_sched_entity(se) {
                cfs_rq = cfs_rq_of(se);
                cfs_rq->h_nr_running--;
                cfs_rq->idle_h_nr_running -= idle_h_nr_running;

                if (cfs_rq_throttled(cfs_rq))
                        break;

                update_load_avg(cfs_rq, se, UPDATE_TG);
                update_cfs_group(se);
        }

        if (!se)
                sub_nr_running(rq, 1);

        util_est_dequeue(&rq->cfs, p, task_sleep);
        hrtick_update(rq);

        pr_fn_end_on(stack_depth);
}
//5345
#ifdef CONFIG_SMP

/* Working cpumask for: load_balance, load_balance_newidle. */
DEFINE_PER_CPU(cpumask_var_t, load_balance_mask);
DEFINE_PER_CPU(cpumask_var_t, select_idle_mask);

#ifdef CONFIG_NO_HZ_COMMON

static struct {
    cpumask_var_t idle_cpus_mask;
    atomic_t nr_cpus;
    int has_blocked;		/* Idle CPUS has blocked load */
    unsigned long next_balance;     /* in jiffy units */
    unsigned long next_blocked;	/* Next update of blocked load in jiffies */
} nohz ____cacheline_aligned;

#endif /* CONFIG_NO_HZ_COMMON */

/* CPU only has SCHED_IDLE tasks enqueued */
static int sched_idle_cpu(int cpu)
{
    struct rq *rq = cpu_rq(cpu);

    return unlikely(rq->nr_running == rq->cfs.idle_h_nr_running &&
            rq->nr_running);
}

static unsigned long cpu_runnable_load(struct rq *rq)
{
    return cfs_rq_runnable_load_avg(&rq->cfs);
}

static unsigned long capacity_of(int cpu)
{
    return cpu_rq(cpu)->cpu_capacity;
}

static unsigned long cpu_avg_load_per_task(int cpu)
{
    struct rq *rq = cpu_rq(cpu);
    unsigned long nr_running = READ_ONCE(rq->cfs.h_nr_running);
    unsigned long load_avg = cpu_runnable_load(rq);

    if (nr_running)
        return load_avg / nr_running;

    return 0;
}
//5394 lines






//6568 lines
static void task_dead_fair(struct task_struct *p)
{
    remove_entity_load_avg(&p->se);
}

static int
balance_fair(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
    if (rq->nr_running)
        return 1;

    //return newidle_balance(rq, rf) != 0;
    return 0;
}
#endif /* CONFIG_SMP */
//6583
static unsigned long wakeup_gran(struct sched_entity *se)
{
    unsigned long gran = sysctl_sched_wakeup_granularity;

    /*
     * Since its curr running now, convert the gran from real-time
     * to virtual-time in his units.
     *
     * By using 'se' instead of 'curr' we penalize light tasks, so
     * they get preempted easier. That is, if 'se' < 'curr' then
     * the resulting gran will be larger, therefore penalizing the
     * lighter, if otoh 'se' > 'curr' then the resulting gran will
     * be smaller, again penalizing the lighter task.
     *
     * This is especially important for buddies when the leftmost
     * task is higher priority than the buddy.
     */
    return calc_delta_fair(gran, se);
}
//6603
/*
 * Should 'se' preempt 'curr'.
 *
 *             |s1
 *        |s2
 *   |s3
 *         g
 *      |<--->|c
 *
 *  w(c, s1) = -1
 *  w(c, s2) =  0
 *  w(c, s3) =  1
 *
 */
static int
wakeup_preempt_entity(struct sched_entity *curr, struct sched_entity *se)
{
    s64 gran, vdiff = curr->vruntime - se->vruntime;

    if (vdiff <= 0)
        return -1;

    gran = wakeup_gran(se);
    if (vdiff > gran)
        return 1;

    return 0;
}

static void set_last_buddy(struct sched_entity *se)
{
    if (entity_is_task(se) && unlikely(task_has_idle_policy(task_of(se))))
        return;

    for_each_sched_entity(se) {
        if (SCHED_WARN_ON(!se->on_rq))
            return;
        cfs_rq_of(se)->last = se;
    }
}

static void set_next_buddy(struct sched_entity *se)
{
    if (entity_is_task(se) && unlikely(task_has_idle_policy(task_of(se))))
        return;

    for_each_sched_entity(se) {
        if (SCHED_WARN_ON(!se->on_rq))
            return;
        cfs_rq_of(se)->next = se;
    }
}

static void set_skip_buddy(struct sched_entity *se)
{
    for_each_sched_entity(se)
        cfs_rq_of(se)->skip = se;
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_wakeup(struct rq *rq, struct task_struct *p, int wake_flags)
{
    struct task_struct *curr = rq->curr;
    struct sched_entity *se = &curr->se, *pse = &p->se;
    struct cfs_rq *cfs_rq = task_cfs_rq(curr);
    int scale = cfs_rq->nr_running >= sched_nr_latency;
    int next_buddy_marked = 0;

    if (unlikely(se == pse))
        return;

    /*
     * This is possible from callers such as attach_tasks(), in which we
     * unconditionally check_prempt_curr() after an enqueue (which may have
     * lead to a throttle).  This both saves work and prevents false
     * next-buddy nomination below.
     */
    if (unlikely(throttled_hierarchy(cfs_rq_of(pse))))
        return;

    if (sched_feat(NEXT_BUDDY) && scale && !(wake_flags & WF_FORK)) {
        set_next_buddy(pse);
        next_buddy_marked = 1;
    }

    /*
     * We can come here with TIF_NEED_RESCHED already set from new task
     * wake up path.
     *
     * Note: this also catches the edge-case of curr being in a throttled
     * group (e.g. via set_curr_task), since update_curr() (in the
     * enqueue of curr) will have resulted in resched being set.  This
     * prevents us from potentially nominating it as a false LAST_BUDDY
     * below.
     */
    if (test_tsk_need_resched(curr))
        return;

    /* Idle tasks are by definition preempted by non-idle tasks. */
    if (unlikely(task_has_idle_policy(curr)) &&
        likely(!task_has_idle_policy(p)))
        goto preempt;

    /*
     * Batch and idle tasks do not preempt non-idle tasks (their preemption
     * is driven by the tick):
     */
    if (unlikely(p->policy != SCHED_NORMAL) || !sched_feat(WAKEUP_PREEMPTION))
        return;

    find_matching_se(&se, &pse);
    update_curr(cfs_rq_of(se));
    BUG_ON(!pse);
    if (wakeup_preempt_entity(se, pse) == 1) {
        /*
         * Bias pick_next to pick the sched entity that is
         * triggering this preemption.
         */
        if (!next_buddy_marked)
            set_next_buddy(pse);
        goto preempt;
    }

    return;

preempt:
    resched_curr(rq);
    /*
     * Only set the backward buddy when the current task is still
     * on the rq. This can happen when a wakeup gets interleaved
     * with schedule on the ->pre_schedule() or idle_balance()
     * point, either of which can * drop the rq lock.
     *
     * Also, during early boot the idle thread is in the fair class,
     * for obvious reasons its a bad idea to schedule back to it.
     */
    if (unlikely(!se->on_rq || curr == rq->idle))
        return;

    if (sched_feat(LAST_BUDDY) && scale && entity_is_task(se))
        set_last_buddy(se);
}
//6748
static struct task_struct *
pick_next_task_fair(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
    struct cfs_rq *cfs_rq = &rq->cfs;
    struct sched_entity *se;
    struct task_struct *p;
    int new_tasks;

again:
    if (!sched_fair_runnable(rq))
        goto idle;

#ifdef CONFIG_FAIR_GROUP_SCHED
    if (!prev || prev->sched_class != &fair_sched_class)
        goto simple;

    /*
     * Because of the set_next_buddy() in dequeue_task_fair() it is rather
     * likely that a next task is from the same cgroup as the current.
     *
     * Therefore attempt to avoid putting and setting the entire cgroup
     * hierarchy, only change the part that actually changes.
     */

    do {
        struct sched_entity *curr = cfs_rq->curr;

        /*
         * Since we got here without doing put_prev_entity() we also
         * have to consider cfs_rq->curr. If it is still a runnable
         * entity, update_curr() will update its vruntime, otherwise
         * forget we've ever seen it.
         */
        if (curr) {
            if (curr->on_rq)
                update_curr(cfs_rq);
            else
                curr = NULL;

            /*
             * This call to check_cfs_rq_runtime() will do the
             * throttle and dequeue its entity in the parent(s).
             * Therefore the nr_running test will indeed
             * be correct.
             */
            if (unlikely(check_cfs_rq_runtime(cfs_rq))) {
                cfs_rq = &rq->cfs;

                if (!cfs_rq->nr_running)
                    goto idle;

                goto simple;
            }
        }

        se = pick_next_entity(cfs_rq, curr);
        cfs_rq = group_cfs_rq(se);
    } while (cfs_rq);

    p = task_of(se);

    /*
     * Since we haven't yet done put_prev_entity and if the selected task
     * is a different task than we started out with, try and touch the
     * least amount of cfs_rqs.
     */
    if (prev != p) {
        struct sched_entity *pse = &prev->se;

        while (!(cfs_rq = is_same_group(se, pse))) {
            int se_depth = se->depth;
            int pse_depth = pse->depth;

            if (se_depth <= pse_depth) {
                put_prev_entity(cfs_rq_of(pse), pse);
                pse = parent_entity(pse);
            }
            if (se_depth >= pse_depth) {
                set_next_entity(cfs_rq_of(se), se);
                se = parent_entity(se);
            }
        }

        put_prev_entity(cfs_rq, pse);
        set_next_entity(cfs_rq, se);
    }

    goto done;
simple:
#endif
    if (prev)
        put_prev_task(rq, prev);

    do {
        se = pick_next_entity(cfs_rq, NULL);
        set_next_entity(cfs_rq, se);
        cfs_rq = group_cfs_rq(se);
    } while (cfs_rq);

    p = task_of(se);

done: __maybe_unused;
#ifdef CONFIG_SMP
    /*
     * Move the next running task to the front of
     * the list, so our cfs_tasks list becomes MRU
     * one.
     */
    list_move(&p->se.group_node, &rq->cfs_tasks);
#endif

    if (hrtick_enabled(rq))
        hrtick_start_fair(rq, p);

    update_misfit_status(p, rq);

    return p;

idle:
    if (!rf)
        return NULL;

    //new_tasks = newidle_balance(rq, rf);

    /*
     * Because newidle_balance() releases (and re-acquires) rq->lock, it is
     * possible for any higher priority task to appear. In that case we
     * must re-start the pick_next_entity() loop.
     */
    if (new_tasks < 0)
        return RETRY_TASK;

    if (new_tasks > 0)
        goto again;

    /*
     * rq is about to be idle, check if we need to update the
     * lost_idle_time of clock_pelt
     */
    update_idle_rq_clock_pelt(rq);

    return NULL;
}

/*
 * Account for a descheduled task:
 */
static void put_prev_task_fair(struct rq *rq, struct task_struct *prev)
{
    struct sched_entity *se = &prev->se;
    struct cfs_rq *cfs_rq;

    for_each_sched_entity(se) {
        cfs_rq = cfs_rq_of(se);
        put_prev_entity(cfs_rq, se);
    }
}

/*
 * sched_yield() is very simple
 *
 * The magic of dealing with the ->skip buddy is in pick_next_entity.
 */
static void yield_task_fair(struct rq *rq)
{
    struct task_struct *curr = rq->curr;
    struct cfs_rq *cfs_rq = task_cfs_rq(curr);
    struct sched_entity *se = &curr->se;

    /*
     * Are we the only task in the tree?
     */
    if (unlikely(rq->nr_running == 1))
        return;

    clear_buddies(cfs_rq, se);

    if (curr->policy != SCHED_BATCH) {
        update_rq_clock(rq);
        /*
         * Update run-time statistics of the 'current'.
         */
        update_curr(cfs_rq);
        /*
         * Tell update_rq_clock() that we've just updated,
         * so we don't do microscopic update in schedule()
         * and double the fastpath cost.
         */
        rq_clock_skip_update(rq);
    }

    set_skip_buddy(se);
}

static bool yield_to_task_fair(struct rq *rq, struct task_struct *p, bool preempt)
{
    struct sched_entity *se = &p->se;

    /* throttled hierarchies are not runnable */
    if (!se->on_rq || throttled_hierarchy(cfs_rq_of(se)))
        return false;

    /* Tell the scheduler that we'd really like pse to run next. */
    set_next_buddy(se);

    yield_task_fair(rq);

    return true;
}
//6958 lines









//7478 lines
#ifdef CONFIG_NO_HZ_COMMON
static inline bool cfs_rq_has_blocked(struct cfs_rq *cfs_rq)
{
    if (cfs_rq->avg.load_avg)
        return true;

    if (cfs_rq->avg.util_avg)
        return true;

    return false;
}

static inline bool others_have_blocked(struct rq *rq)
{
    if (READ_ONCE(rq->avg_rt.util_avg))
        return true;

    if (READ_ONCE(rq->avg_dl.util_avg))
        return true;

#ifdef CONFIG_HAVE_SCHED_AVG_IRQ
    if (READ_ONCE(rq->avg_irq.util_avg))
        return true;
#endif

    return false;
}

static inline void update_blocked_load_status(struct rq *rq, bool has_blocked)
{
    rq->last_blocked_load_update_tick = jiffies;

    if (!has_blocked)
        rq->has_blocked_load = 0;
}
#else
static inline bool cfs_rq_has_blocked(struct cfs_rq *cfs_rq) { return false; }
static inline bool others_have_blocked(struct rq *rq) { return false; }
static inline void update_blocked_load_status(struct rq *rq, bool has_blocked) {}
#endif
//7519
#ifdef CONFIG_FAIR_GROUP_SCHED

static inline bool cfs_rq_is_decayed(struct cfs_rq *cfs_rq)
{
    if (cfs_rq->load.weight)
        return false;

    if (cfs_rq->avg.load_sum)
        return false;

    if (cfs_rq->avg.util_sum)
        return false;

    if (cfs_rq->avg.runnable_load_sum)
        return false;

    return true;
}

static void update_blocked_averages(int cpu)
{
    struct rq *rq = cpu_rq(cpu);
    struct cfs_rq *cfs_rq, *pos;
    const struct sched_class *curr_class;
    struct rq_flags rf;
    bool done = true;

    rq_lock_irqsave(rq, &rf);
    update_rq_clock(rq);

    /*
     * update_cfs_rq_load_avg() can call cpufreq_update_util(). Make sure
     * that RT, DL and IRQ signals have been updated before updating CFS.
     */
    curr_class = rq->curr->sched_class;
    update_rt_rq_load_avg(rq_clock_pelt(rq), rq, curr_class == &rt_sched_class);
    update_dl_rq_load_avg(rq_clock_pelt(rq), rq, curr_class == &dl_sched_class);
    update_irq_load_avg(rq, 0);

    /* Don't need periodic decay once load/util_avg are null */
    if (others_have_blocked(rq))
        done = false;

    /*
     * Iterates the task_group tree in a bottom up fashion, see
     * list_add_leaf_cfs_rq() for details.
     */
    for_each_leaf_cfs_rq_safe(rq, cfs_rq, pos) {
        struct sched_entity *se;

        if (update_cfs_rq_load_avg(cfs_rq_clock_pelt(cfs_rq), cfs_rq))
            update_tg_load_avg(cfs_rq, 0);

        /* Propagate pending load changes to the parent, if any: */
        se = cfs_rq->tg->se[cpu];
        if (se && !skip_blocked_update(se))
            update_load_avg(cfs_rq_of(se), se, 0);

        /*
         * There can be a lot of idle CPU cgroups.  Don't let fully
         * decayed cfs_rqs linger on the list.
         */
        if (cfs_rq_is_decayed(cfs_rq))
            list_del_leaf_cfs_rq(cfs_rq);

        /* Don't need periodic decay once load/util_avg are null */
        if (cfs_rq_has_blocked(cfs_rq))
            done = false;
    }

    update_blocked_load_status(rq, !done);
    rq_unlock_irqrestore(rq, &rf);
}

/*
 * Compute the hierarchical load factor for cfs_rq and all its ascendants.
 * This needs to be done in a top-down fashion because the load of a child
 * group is a fraction of its parents load.
 */
static void update_cfs_rq_h_load(struct cfs_rq *cfs_rq)
{
    struct rq *rq = rq_of(cfs_rq);
    struct sched_entity *se = cfs_rq->tg->se[cpu_of(rq)];
    unsigned long now = jiffies;
    unsigned long load;

    if (cfs_rq->last_h_load_update == now)
        return;

    WRITE_ONCE(cfs_rq->h_load_next, NULL);
    for_each_sched_entity(se) {
        cfs_rq = cfs_rq_of(se);
        WRITE_ONCE(cfs_rq->h_load_next, se);
        if (cfs_rq->last_h_load_update == now)
            break;
    }

    if (!se) {
        cfs_rq->h_load = cfs_rq_load_avg(cfs_rq);
        cfs_rq->last_h_load_update = now;
    }

    while ((se = READ_ONCE(cfs_rq->h_load_next)) != NULL) {
        load = cfs_rq->h_load;
        load = div64_ul(load * se->avg.load_avg,
            cfs_rq_load_avg(cfs_rq) + 1);
        cfs_rq = group_cfs_rq(se);
        cfs_rq->h_load = load;
        cfs_rq->last_h_load_update = now;
    }
}

static unsigned long task_h_load(struct task_struct *p)
{
    struct cfs_rq *cfs_rq = task_cfs_rq(p);

    update_cfs_rq_h_load(cfs_rq);
    return div64_ul(p->se.avg.load_avg * cfs_rq->h_load,
            cfs_rq_load_avg(cfs_rq) + 1);
}
#else
static inline void update_blocked_averages(int cpu)
{
    struct rq *rq = cpu_rq(cpu);
    struct cfs_rq *cfs_rq = &rq->cfs;
    const struct sched_class *curr_class;
    struct rq_flags rf;

    rq_lock_irqsave(rq, &rf);
    update_rq_clock(rq);

    /*
     * update_cfs_rq_load_avg() can call cpufreq_update_util(). Make sure
     * that RT, DL and IRQ signals have been updated before updating CFS.
     */
    curr_class = rq->curr->sched_class;
    update_rt_rq_load_avg(rq_clock_pelt(rq), rq, curr_class == &rt_sched_class);
    update_dl_rq_load_avg(rq_clock_pelt(rq), rq, curr_class == &dl_sched_class);
    update_irq_load_avg(rq, 0);

    update_cfs_rq_load_avg(cfs_rq_clock_pelt(cfs_rq), cfs_rq);

    update_blocked_load_status(rq, cfs_rq_has_blocked(cfs_rq) || others_have_blocked(rq));
    rq_unlock_irqrestore(rq, &rf);
}

static unsigned long task_h_load(struct task_struct *p)
{
    return p->se.avg.load_avg;
}
#endif

/********** Helpers for find_busiest_group ************************/




//9874 lines
/*
 * run_rebalance_domains is triggered when needed from the scheduler tick.
 * Also triggered for nohz idle balancing (with nohz_balancing_kick set).
 */
static __latent_entropy void run_rebalance_domains(struct softirq_action *h)
{
    struct rq *this_rq = this_rq();
    enum cpu_idle_type idle = this_rq->idle_balance ?
                        CPU_IDLE : CPU_NOT_IDLE;

    /*
     * If this CPU has a pending nohz_balance_kick, then do the
     * balancing on behalf of the other idle CPUs whose ticks are
     * stopped. Do nohz_idle_balance *before* rebalance_domains to
     * give the idle CPUs a chance to load balance. Else we may
     * load balance only within the local sched_domain hierarchy
     * and abort nohz_idle_balance altogether if we pull some load.
     */
    //if (nohz_idle_balance(this_rq, idle))
    //    return;

    /* normal load balance */
    update_blocked_averages(this_rq->cpu);
    //rebalance_domains(this_rq, idle);
}







//9932 lines
/*
 * scheduler tick hitting a task of our scheduling class.
 *
 * NOTE: This function can be called remotely by the tick offload that
 * goes along full dynticks. Therefore no local assumption can be made
 * and everything must be accessed through the @rq and @curr passed in
 * parameters.
 */
static void task_tick_fair(struct rq *rq, struct task_struct *curr, int queued)
{
    pr_fn_start_on(stack_depth);

    struct cfs_rq *cfs_rq;
    struct sched_entity *se = &curr->se;

    pr_info_view_on(stack_depth, "%20s : %u\n", rq->nr_running);
    pr_info_view_on(stack_depth, "%20s : %llu\n", rq->clock);
    pr_info_view_on(stack_depth, "%20s : %llu\n", rq->clock_task);
    pr_info_view_on(stack_depth, "%20s : %llu\n", rq->clock_pelt);

    for_each_sched_entity(se) {
        cfs_rq = cfs_rq_of(se);
        pr_info_view_on(stack_depth, "%30s : %p\n", se);
        pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->nr_running);
        pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->h_nr_running);
        entity_tick(cfs_rq, se, queued);
    }
#if 0
    if (static_branch_unlikely(&sched_numa_balancing))
        task_tick_numa(rq, curr);
#endif //0
    update_misfit_status(curr, rq);
    //update_overutilized_status(task_rq(curr));

    pr_fn_end_on(stack_depth);
}
//9957
/*
 * called on fork with the child task as argument from the parent's context
 *  - child not yet on the tasklist
 *  - preemption disabled
 */
static void task_fork_fair(struct task_struct *p)
{
    struct cfs_rq *cfs_rq;
    struct sched_entity *se = &p->se, *curr;
    struct rq *rq = this_rq();
    struct rq_flags rf;

    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    rq_lock(rq, &rf);
    //update_rq_clock(rq);	//error

    //cfs_rq = task_cfs_rq(current);
    cfs_rq = task_cfs_rq(p);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);

    curr = cfs_rq->curr;
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->curr);
    if (curr) {
        update_curr(cfs_rq);
        se->vruntime = curr->vruntime;
    }

    place_entity(cfs_rq, se, 1);

    if (sysctl_sched_child_runs_first && curr && entity_before(curr, se)) {
        /*
         * Upon rescheduling, sched_class::put_prev_task() will place
         * 'current' within the tree based on its new key value.
         */
        swap(curr->vruntime, se->vruntime);
        resched_curr(rq);
    }

    se->vruntime -= cfs_rq->min_vruntime;
    rq_unlock(rq, &rf);

    pr_fn_end_on(stack_depth);
}

/*
 * Priority of the task has changed. Check to see if we preempt
 * the current task.
 */
static void
prio_changed_fair(struct rq *rq, struct task_struct *p, int oldprio)
{
    if (!task_on_rq_queued(p))
        return;

    /*
     * Reschedule if we are currently running on this runqueue and
     * our priority decreased, or if we are not currently running on
     * this runqueue and our priority is higher than the current's
     */
    if (rq->curr == p) {
        if (p->prio > oldprio)
            resched_curr(rq);
    } else
        check_preempt_curr(rq, p, 0);
}
//10015 lines






//10043 lines
#ifdef CONFIG_FAIR_GROUP_SCHED
/*
 * Propagate the changes of the sched_entity across the tg tree to make it
 * visible to the root
 */
static void propagate_entity_cfs_rq(struct sched_entity *se)
{
    struct cfs_rq *cfs_rq;

    /* Start to propagate at parent */
    se = se->parent;

    for_each_sched_entity(se) {
        cfs_rq = cfs_rq_of(se);

        if (cfs_rq_throttled(cfs_rq))
            break;

        update_load_avg(cfs_rq, se, UPDATE_TG);
    }
}
#else
static void propagate_entity_cfs_rq(struct sched_entity *se) { }
#endif
//10068 lines





//10079 lines
static void attach_entity_cfs_rq(struct sched_entity *se)
{
    struct cfs_rq *cfs_rq = cfs_rq_of(se);

    pr_fn_start_on(stack_depth);

#ifdef CONFIG_FAIR_GROUP_SCHED
    /*
     * Since the real-depth could have been changed (only FAIR
     * class maintain depth value), reset depth properly.
     */
    se->depth = se->parent ? se->parent->depth + 1 : 0;
#endif

    /* Synchronize entity with its cfs_rq */
    update_load_avg(cfs_rq, se, sched_feat(ATTACH_AGE_LOAD) ? 0 : SKIP_AGE_LOAD);
    attach_entity_load_avg(cfs_rq, se, 0);
    update_tg_load_avg(cfs_rq, false);
    propagate_entity_cfs_rq(se);

    pr_fn_end_on(stack_depth);
}





//10148 lines
/* Account for a task changing its policy or group.
 *
 * This routine is mostly called to set cfs_rq->curr field when a task
 * migrates between groups/classes.
 */
static void set_next_task_fair(struct rq *rq, struct task_struct *p)
{
    struct sched_entity *se = &p->se;

#ifdef CONFIG_SMP
    if (task_on_rq_queued(p)) {
        /*
         * Move the next running task to the front of the list, so our
         * cfs_tasks list becomes MRU one.
         */
        list_move(&se->group_node, &rq->cfs_tasks);
    }
#endif

    for_each_sched_entity(se) {
        struct cfs_rq *cfs_rq = cfs_rq_of(se);

        set_next_entity(cfs_rq, se);
        /* ensure bandwidth has been allocated on our new cfs_rq */
        account_cfs_rq_runtime(cfs_rq, 0);
    }
}
//10176
void init_cfs_rq(struct cfs_rq *cfs_rq)
{
    cfs_rq->tasks_timeline = RB_ROOT_CACHED;
    cfs_rq->min_vruntime = (u64)(-(1LL << 20));
#ifndef CONFIG_64BIT
    cfs_rq->min_vruntime_copy = cfs_rq->min_vruntime;
#endif
#ifdef CONFIG_SMP
    raw_spin_lock_init(&cfs_rq->removed.lock);
#endif
}







//10222 lines
void free_fair_sched_group(struct task_group *tg)
{
    int i;

    destroy_cfs_bandwidth(tg_cfs_bandwidth(tg));

    for_each_possible_cpu(i) {
        if (tg->cfs_rq)
            kfree(tg->cfs_rq[i]);
        if (tg->se)
            kfree(tg->se[i]);
    }

    kfree(tg->cfs_rq);
    kfree(tg->se);
}

int alloc_fair_sched_group(struct task_group *tg, struct task_group *parent)
{
    struct sched_entity *se;
    struct cfs_rq *cfs_rq;
    int i;

    pr_fn_start_on(stack_depth);

    tg->cfs_rq = kcalloc(nr_cpu_ids, sizeof(cfs_rq), GFP_KERNEL);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)tg->cfs_rq);
    if (!tg->cfs_rq)
        goto err;
    tg->se = kcalloc(nr_cpu_ids, sizeof(se), GFP_KERNEL);
    if (!tg->se)
        goto err;

    tg->shares = NICE_0_LOAD;	//32bits: 1<<10, 64bits: 1<<20

    init_cfs_bandwidth(tg_cfs_bandwidth(tg));

    for_each_possible_cpu(i) {
        cfs_rq = kzalloc_node(sizeof(struct cfs_rq),
                      GFP_KERNEL, cpu_to_node(i));
        if (!cfs_rq)
            goto err;

        se = kzalloc_node(sizeof(struct sched_entity),
                  GFP_KERNEL, cpu_to_node(i));
        if (!se)
            goto err_free_rq;

        init_cfs_rq(cfs_rq);
        init_tg_cfs_entry(tg, cfs_rq, se, i, parent->se[i]);
        init_entity_runnable_average(se);
    }

    pr_fn_end_on(stack_depth);

    return 1;

err_free_rq:
    kfree(cfs_rq);
err:
    return 0;
}
//10280
void online_fair_sched_group(struct task_group *tg)
{
    struct sched_entity *se;
    struct rq_flags rf;
    struct rq *rq;
    int i;

    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%10s : %p\n", (void*)tg);

    for_each_possible_cpu(i) {
        pr_info_view_on(stack_depth, "%10s : %d\n", i);
        rq = cpu_rq(i);
        se = tg->se[i];
        pr_info_view_on(stack_depth, "%10s : %p\n", (void*)rq);
        pr_info_view_on(stack_depth, "%10s : %p\n", (void*)se);
        rq_lock_irq(rq, &rf);
        //update_rq_clock(rq);
        attach_entity_cfs_rq(se);
        //sync_throttle(tg, i);
        rq_unlock_irq(rq, &rf);
    }

    pr_fn_end_on(stack_depth);
}

void unregister_fair_sched_group(struct task_group *tg)
{
    unsigned long flags;
    struct rq *rq;
    int cpu;

    for_each_possible_cpu(cpu) {
        if (tg->se[cpu])
            remove_entity_load_avg(tg->se[cpu]);

        /*
         * Only empty task groups can be destroyed; so we can speculatively
         * check on_list without danger of it being re-added.
         */
        if (!tg->cfs_rq[cpu]->on_list)
            continue;

        rq = cpu_rq(cpu);

        raw_spin_lock_irqsave(&rq->lock, flags);
        list_del_leaf_cfs_rq(tg->cfs_rq[cpu]);
        raw_spin_unlock_irqrestore(&rq->lock, flags);
    }
}

void init_tg_cfs_entry(struct task_group *tg, struct cfs_rq *cfs_rq,
            struct sched_entity *se, int cpu,
            struct sched_entity *parent)
{
    struct rq *rq = cpu_rq(cpu);

    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%20s : %d\n", cpu);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)tg);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->curr);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)parent);

    cfs_rq->tg = tg;
    cfs_rq->rq = rq;
    init_cfs_rq_runtime(cfs_rq);

    tg->cfs_rq[cpu] = cfs_rq;	//rq->fcs
    tg->se[cpu] = se;

    /* se could be NULL for root_task_group */
    if (!se)
        return;

    if (!parent) {
        se->cfs_rq = &rq->cfs;
        se->depth = 0;
    } else {
        se->cfs_rq = parent->my_q;
        se->depth = parent->depth + 1;
    }

    se->my_q = cfs_rq;
    /* guarantee group entities always have weight */
    update_load_set(&se->load, NICE_0_LOAD);
    se->parent = parent;

    pr_fn_end_on(stack_depth);
}
//10354 lines






//10423 lines
/*
 * All the scheduling class methods:
 */
const struct sched_class fair_sched_class = {
    .next			= &idle_sched_class,
    .enqueue_task	= enqueue_task_fair,
    .dequeue_task	= dequeue_task_fair,
    .yield_task		= yield_task_fair,
    .yield_to_task	= yield_to_task_fair,

    //.check_preempt_curr	= check_preempt_wakeup,

    .pick_next_task	= pick_next_task_fair,
    .put_prev_task	= put_prev_task_fair,
    .set_next_task  = set_next_task_fair,

#ifdef CONFIG_SMP
    .balance			= balance_fair,
    //.select_task_rq	= select_task_rq_fair,
    //.migrate_task_rq	= migrate_task_rq_fair,

    //.rq_online		= rq_online_fair,
    //.rq_offline		= rq_offline_fair,

    .task_dead			= task_dead_fair,
    .set_cpus_allowed	= set_cpus_allowed_common,
#endif

    .task_tick			= task_tick_fair,
    .task_fork			= task_fork_fair,

    .prio_changed		= prio_changed_fair,
    //.switched_from	= switched_from_fair,
    //.switched_to		= switched_to_fair,

    //.get_rr_interval	= get_rr_interval_fair,

    .update_curr		= update_curr_fair,

#ifdef CONFIG_FAIR_GROUP_SCHED
    //.task_change_group	= task_change_group_fair,
#endif

#ifdef CONFIG_UCLAMP_TASK
    .uclamp_enabled	= 1,
#endif
};
//10471 lines




//10507 lines
__init void init_sched_fair_class(void)
{
#ifdef CONFIG_SMP
    open_softirq(SCHED_SOFTIRQ, run_rebalance_domains);

#ifdef CONFIG_NO_HZ_COMMON
    nohz.next_balance = jiffies;
    nohz.next_blocked = jiffies;
    zalloc_cpumask_var(&nohz.idle_cpus_mask, GFP_NOWAIT);
#endif
#endif /* SMP */

}


