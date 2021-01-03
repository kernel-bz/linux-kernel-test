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

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);

    int cpu = cpu_of(rq);

    pr_info_view_on(stack_depth, "%30s : %d\n", cpu);
    pr_info_view_on(stack_depth, "%30s : %d\n", cfs_rq->on_list);

    if (cfs_rq->on_list)
        return rq->tmp_alone_branch == &rq->leaf_cfs_rq_list;

    cfs_rq->on_list = 1;
    pr_info_view_on(stack_depth, "%30s : %d\n", cfs_rq->on_list);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->tg);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->tmp_alone_branch);

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

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->tmp_alone_branch);

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

    if ((u64)curr > 0x10000000) {
        pr_err("cfs_rq->curr pointer errror!\n");
        return;
    }

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

    pr_info_view_on(stack_depth, "%30s : %llu\n", cfs_rq->min_vruntime);

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
    u32 cnt = 0;

    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->cfs_rq->rq);
    pr_info_view_on(stack_depth, "%30s : %d\n", se->cfs_rq->rq->cpu);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->tg);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->rq);
    pr_info_view_on(stack_depth, "%30s : %d\n", cfs_rq->rq->cpu);

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
        pr_info_view_on(stack_depth, "%30s : %u\n", cnt++);
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
    pr_info_view_on(stack_depth, "%30s : %d\n", leftmost);
    pr_fn_end_on(stack_depth);
}

static void __dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&se->run_node);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&cfs_rq->tasks_timeline.rb_leftmost);

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
//619
#ifdef CONFIG_SCHED_DEBUG
struct sched_entity *__pick_last_entity(struct cfs_rq *cfs_rq)
{
    struct rb_node *last = rb_last(&cfs_rq->tasks_timeline.rb_root);

    if (!last)
        return NULL;

    return rb_entry(last, struct sched_entity, run_node);
}

/**************************************************************
 * Scheduling class statistics methods:
 */

int sched_proc_update_handler(struct ctl_table *table, int write,
        void __user *buffer, size_t *lenp,
        loff_t *ppos)
{
    //int ret = proc_dointvec_minmax(table, write, buffer, lenp, ppos);
    int ret = 0;
    unsigned int factor = get_update_sysctl_factor();

    if (ret || !write)
        return ret;

    sched_nr_latency = DIV_ROUND_UP(sysctl_sched_latency,
                    sysctl_sched_min_granularity);

#define WRT_SYSCTL(name) \
    (normalized_sysctl_##name = sysctl_##name / (factor))
    WRT_SYSCTL(sched_min_granularity);
    WRT_SYSCTL(sched_latency);
    WRT_SYSCTL(sched_wakeup_granularity);
#undef WRT_SYSCTL

    return 0;
}
#endif
//658
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

    pr_info_view_on(stack_depth, "%30s : %lu\n", se->load.weight);
    pr_info_view_on(stack_depth, "%30s : %lu\n", se->runnable_weight);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa->load_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa->runnable_load_avg);

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
    pr_fn_start_on(stack_depth);

    struct sched_entity *se = &p->se;
    struct cfs_rq *cfs_rq = cfs_rq_of(se);
    struct sched_avg *sa = &se->avg;
    long cpu_scale = arch_scale_cpu_capacity(cpu_of(rq_of(cfs_rq)));
    long cap = (long)(cpu_scale - cfs_rq->avg.util_avg) / 2;

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&p->se);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %ld\n", cpu_scale);
    pr_info_view_on(stack_depth, "%30s : %ld\n", cap);

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

    pr_info_view_on(stack_depth, "%30s : %llu\n", se->avg.last_update_time);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&se->avg);
    pr_sched_avg_info(sa);

    attach_entity_cfs_rq(se);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&se->avg);
    pr_sched_avg_info(sa);

    pr_fn_end_on(stack_depth);
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
    u64 now = rq_clock_task(rq_of(cfs_rq));
    u64 delta_exec;

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq_of(cfs_rq));
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->curr);
    pr_info_view_on(stack_depth, "%30s : %llu\n", now);

    if (unlikely(!curr))
        return;

    delta_exec = now - curr->exec_start;

    pr_info_view_on(stack_depth, "%30s : %llu\n", curr->exec_start);
    pr_info_view_on(stack_depth, "%30s : %lld\n", delta_exec);

    if (unlikely((s64)delta_exec <= 0))
        return;

    curr->exec_start = now;

    schedstat_set(curr->statistics.exec_max,
              max(delta_exec, curr->statistics.exec_max));

    curr->sum_exec_runtime += delta_exec;
    schedstat_add(cfs_rq->exec_clock, delta_exec);

    curr->vruntime += calc_delta_fair(delta_exec, curr);
    update_min_vruntime(cfs_rq);

    pr_info_view_on(stack_depth, "%30s : %llu\n", curr->sum_exec_runtime);
    pr_info_view_on(stack_depth, "%30s : %llu\n", curr->vruntime);
    pr_info_view_on(stack_depth, "%30s : %llu\n", cfs_rq->min_vruntime);

    if (entity_is_task(curr)) {
        struct task_struct *curtask = task_of(curr);

        //trace_sched_stat_runtime(curtask, delta_exec, curr->vruntime);
        //cgroup_account_cputime(curtask, delta_exec);	//error
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
//1048
/**************************************************
 * Scheduling class queueing methods:
 */

//#ifdef CONFIG_NUMA_BALANCING
/*
 * Approximate time to scan a full NUMA task in ms. The task scan period is
 * calculated based on the tasks virtual memory size and
 * numa_balancing_scan_size.
 */
unsigned int sysctl_numa_balancing_scan_period_min = 1000;
unsigned int sysctl_numa_balancing_scan_period_max = 60000;

/* Portion of address space to scan in MB */
unsigned int sysctl_numa_balancing_scan_size = 256;

/* Scan @scan_size MB every @scan_period after an initial @scan_delay in ms */
unsigned int sysctl_numa_balancing_scan_delay = 1000;

struct numa_group {
    refcount_t refcount;

    spinlock_t lock; /* nr_tasks, tasks */
    int nr_tasks;
    pid_t gid;
    int active_nodes;

    struct rcu_head rcu;
    unsigned long total_faults;
    unsigned long max_faults_cpu;
    /*
     * Faults_cpu is used to decide whether memory should move
     * towards the CPU. As a consequence, these stats are weighted
     * more by CPU use than by memory faults.
     */
    unsigned long *faults_cpu;
    unsigned long faults[0];
};
//1087 lines





//2736 lines
//#else
static void task_tick_numa(struct rq *rq, struct task_struct *curr)
{
}

static inline void account_numa_enqueue(struct rq *rq, struct task_struct *p)
{
}

static inline void account_numa_dequeue(struct rq *rq, struct task_struct *p)
{
}

static inline void update_scan_period(struct task_struct *p, int new_cpu)
{
}

//#endif /* CONFIG_NUMA_BALANCING */
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
        list_add(&se->group_node, &rq->cfs_tasks);	//error
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
    pr_fn_start_on(stack_depth);

    cfs_rq->runnable_weight += se->runnable_weight;

    cfs_rq->avg.runnable_load_avg += se->avg.runnable_load_avg;
    cfs_rq->avg.runnable_load_sum += se_runnable(se) * se->avg.runnable_load_sum;

    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->runnable_weight);
    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->avg.runnable_load_avg);
    pr_info_view_on(stack_depth, "%30s : %llu\n", cfs_rq->avg.runnable_load_sum);

    pr_fn_end_on(stack_depth);
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
    pr_fn_start_on(stack_depth);

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

    pr_fn_end_on(stack_depth);
}

void reweight_task(struct task_struct *p, int prio)
{
    pr_fn_start_on(stack_depth);

    struct sched_entity *se = &p->se;
    struct cfs_rq *cfs_rq = cfs_rq_of(se);
    struct load_weight *load = &se->load;
    unsigned long weight = scale_load(sched_prio_to_weight[prio]);

    reweight_entity(cfs_rq, se, weight, weight);
    load->inv_weight = sched_prio_to_wmult[prio];

    pr_fn_end_on(stack_depth);
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
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)gcfs_rq);

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

    pr_info_view_on(stack_depth, "%20s : %ld\n", shares);
    pr_info_view_on(stack_depth, "%20s : %ld\n", runnable);

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

    pr_info_view_on(stack_depth, "%30s : %ld\n", delta);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->tg);

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

    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->tg_load_avg_contrib);

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

    pr_info_view_on(stack_depth, "%30s : %llu\n", se->avg.last_update_time);

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
    pr_fn_start_on(stack_depth);

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

    pr_fn_end_on(stack_depth);
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
    pr_fn_start_on(stack_depth);

    cfs_rq->propagate = 1;
    cfs_rq->prop_runnable_sum += runnable_sum;

    pr_info_view_on(stack_depth, "%30s : %ld\n", runnable_sum);
    pr_info_view_on(stack_depth, "%30s : %ld\n", cfs_rq->prop_runnable_sum);
    pr_info_view_on(stack_depth, "%30s : %ld\n", cfs_rq->propagate);

    pr_fn_end_on(stack_depth);
}

/* Update task and its cfs_rq load average */
static inline int propagate_entity_load_avg(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    struct cfs_rq *cfs_rq, *gcfs_rq;

    pr_info_view_on(stack_depth, "%20s : %p\n", se->my_q);

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

    pr_info_view_on(stack_depth, "%20s : %d\n", cfs_rq->removed.nr);

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

        pr_cfs_rq_removed_info(cfs_rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)&cfs_rq->avg);
        pr_sched_avg_info(sa);
    }

    decayed |= __update_load_avg_cfs_rq(now, cfs_rq);

    pr_info_view_on(stack_depth, "%20s : %d\n", decayed);

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
    pr_fn_start_on(stack_depth);

    u32 divider = LOAD_AVG_MAX - 1024 + cfs_rq->avg.period_contrib;

    pr_info_view_on(stack_depth, "%20s : %u\n", divider);
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

    pr_fn_end_on(stack_depth);
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

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%30s : 0x%X\n", flags);
    pr_info_view_on(stack_depth, "%30s : %llu\n", se->avg.last_update_time);

    u64 now = cfs_rq_clock_pelt(cfs_rq);
    int decayed;

    pr_info_view_on(stack_depth, "%30s : %llu\n", now);

    /*
     * Track task load average for carrying it to new CPU after migrated, and
     * track group sched_entity load average for task_h_load calc in migration
     */
    if (se->avg.last_update_time && !(flags & SKIP_AGE_LOAD))
        __update_load_avg_se(now, cfs_rq, se);

    decayed  = update_cfs_rq_load_avg(now, cfs_rq);
    decayed |= propagate_entity_load_avg(se);

    pr_info_view_on(stack_depth, "%30s : %d\n", decayed);
    pr_info_view_on(stack_depth, "%30s : %llu\n", se->avg.last_update_time);

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
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->tg);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->cfs_rq->rq);
    pr_info_view_on(stack_depth, "%30s : %d\n", se->cfs_rq->rq->cpu);
    pr_info_view_on(stack_depth, "%30s : %d\n", renorm);
    pr_info_view_on(stack_depth, "%30s : %d\n", curr);

    /*
     * If we're the current task, we must renormalise before calling
     * update_curr().
     */
    if (renorm && curr)
        se->vruntime += cfs_rq->min_vruntime;

    pr_info_view_on(stack_depth, "%30s : %llu\n", cfs_rq->min_vruntime);
    pr_info_view_on(stack_depth, "%30s : %llu\n", se->vruntime);

    update_curr(cfs_rq);

    /*
     * Otherwise, renormalise after, such that we're placed at the current
     * moment in time, instead of some random moment in the past. Being
     * placed in the past could significantly boost this task to the
     * fairness detriment of existing tasks.
     */
    if (renorm && !curr)
        se->vruntime += cfs_rq->min_vruntime;

    pr_info_view_on(stack_depth, "%30s : %llu\n", cfs_rq->min_vruntime);
    pr_info_view_on(stack_depth, "%30s : %llu\n", se->vruntime);

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

    pr_info_view_on(stack_depth, "%30s : %u\n", se->on_rq);
    pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->nr_running);

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

    pr_info_view_on(stack_depth, "%30s : %u\n", se->on_rq);
    pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->nr_running);

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
    pr_info_view_on(stack_depth, "%30s : %llu\n", se->sum_exec_runtime);

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

    pr_info_view_on(stack_depth, "%30s : %llu\n", se->prev_sum_exec_runtime);

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
    pr_fn_start_on(stack_depth);

    struct sched_entity *left = __pick_first_entity(cfs_rq);
    struct sched_entity *se;

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)left);

    /*
     * If curr is set we have to see if its left of the leftmost entity
     * still in the tree, provided there was anything in the tree at all.
     */
    if (!left || (curr && entity_before(curr, left)))
        left = curr;

    se = left; /* ideally we run the leftmost entity */

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->skip);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->last);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->next);

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

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);

    pr_fn_end_on(stack_depth);

    return se;
}

static bool check_cfs_rq_runtime(struct cfs_rq *cfs_rq);

static void put_prev_entity(struct cfs_rq *cfs_rq, struct sched_entity *prev)
{
    pr_fn_start_on(stack_depth);

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

    pr_fn_end_on(stack_depth);
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
//4439
/*
 * Ensure that neither of the group entities corresponding to src_cpu or
 * dest_cpu are members of a throttled hierarchy when performing group
 * load-balance operations.
 */
static inline int throttled_lb_pair(struct task_group *tg,
                    int src_cpu, int dest_cpu)
{
    struct cfs_rq *src_cfs_rq, *dest_cfs_rq;

    src_cfs_rq = tg->cfs_rq[src_cpu];
    dest_cfs_rq = tg->cfs_rq[dest_cpu];

    return throttled_hierarchy(src_cfs_rq) ||
           throttled_hierarchy(dest_cfs_rq);
}

static int tg_unthrottle_up(struct task_group *tg, void *data)
{
    struct rq *rq = data;
    struct cfs_rq *cfs_rq = tg->cfs_rq[cpu_of(rq)];

    cfs_rq->throttle_count--;
    if (!cfs_rq->throttle_count) {
        cfs_rq->throttled_clock_task_time += rq_clock_task(rq) -
                         cfs_rq->throttled_clock_task;

        /* Add cfs_rq with already running entity in the list */
        if (cfs_rq->nr_running >= 1)
            list_add_leaf_cfs_rq(cfs_rq);
    }

    return 0;
}

static int tg_throttle_down(struct task_group *tg, void *data)
{
    struct rq *rq = data;
    struct cfs_rq *cfs_rq = tg->cfs_rq[cpu_of(rq)];

    /* group is entering throttled state, stop time */
    if (!cfs_rq->throttle_count) {
        cfs_rq->throttled_clock_task = rq_clock_task(rq);
        list_del_leaf_cfs_rq(cfs_rq);
    }
    cfs_rq->throttle_count++;

    return 0;
}

static void throttle_cfs_rq(struct cfs_rq *cfs_rq)
{
    struct rq *rq = rq_of(cfs_rq);
    struct cfs_bandwidth *cfs_b = tg_cfs_bandwidth(cfs_rq->tg);
    struct sched_entity *se;
    long task_delta, idle_task_delta, dequeue = 1;
    bool empty;

    se = cfs_rq->tg->se[cpu_of(rq_of(cfs_rq))];

    /* freeze hierarchy runnable averages while throttled */
    rcu_read_lock();
    walk_tg_tree_from(cfs_rq->tg, tg_throttle_down, tg_nop, (void *)rq);
    rcu_read_unlock();

    task_delta = cfs_rq->h_nr_running;
    idle_task_delta = cfs_rq->idle_h_nr_running;
    for_each_sched_entity(se) {
        struct cfs_rq *qcfs_rq = cfs_rq_of(se);
        /* throttled entity or throttle-on-deactivate */
        if (!se->on_rq)
            break;

        if (dequeue)
            dequeue_entity(qcfs_rq, se, DEQUEUE_SLEEP);
        qcfs_rq->h_nr_running -= task_delta;
        qcfs_rq->idle_h_nr_running -= idle_task_delta;

        if (qcfs_rq->load.weight)
            dequeue = 0;
    }

    if (!se)
        sub_nr_running(rq, task_delta);

    cfs_rq->throttled = 1;
    cfs_rq->throttled_clock = rq_clock(rq);
    raw_spin_lock(&cfs_b->lock);
    empty = list_empty(&cfs_b->throttled_cfs_rq);

    /*
     * Add to the _head_ of the list, so that an already-started
     * distribute_cfs_runtime will not see us. If disribute_cfs_runtime is
     * not running add to the tail so that later runqueues don't get starved.
     */
    if (cfs_b->distribute_running)
        list_add_rcu(&cfs_rq->throttled_list, &cfs_b->throttled_cfs_rq);
    else
        list_add_tail_rcu(&cfs_rq->throttled_list, &cfs_b->throttled_cfs_rq);

    /*
     * If we're the first throttled task, make sure the bandwidth
     * timer is running.
     */
    if (empty)
        start_cfs_bandwidth(cfs_b);

    raw_spin_unlock(&cfs_b->lock);
}

void unthrottle_cfs_rq(struct cfs_rq *cfs_rq)
{
    struct rq *rq = rq_of(cfs_rq);
    struct cfs_bandwidth *cfs_b = tg_cfs_bandwidth(cfs_rq->tg);
    struct sched_entity *se;
    int enqueue = 1;
    long task_delta, idle_task_delta;

    se = cfs_rq->tg->se[cpu_of(rq)];

    cfs_rq->throttled = 0;

    update_rq_clock(rq);

    raw_spin_lock(&cfs_b->lock);
    cfs_b->throttled_time += rq_clock(rq) - cfs_rq->throttled_clock;
    list_del_rcu(&cfs_rq->throttled_list);
    raw_spin_unlock(&cfs_b->lock);

    /* update hierarchical throttle state */
    walk_tg_tree_from(cfs_rq->tg, tg_nop, tg_unthrottle_up, (void *)rq);

    if (!cfs_rq->load.weight)
        return;

    task_delta = cfs_rq->h_nr_running;
    idle_task_delta = cfs_rq->idle_h_nr_running;
    for_each_sched_entity(se) {
        if (se->on_rq)
            enqueue = 0;

        cfs_rq = cfs_rq_of(se);
        if (enqueue)
            enqueue_entity(cfs_rq, se, ENQUEUE_WAKEUP);
        cfs_rq->h_nr_running += task_delta;
        cfs_rq->idle_h_nr_running += idle_task_delta;

        if (cfs_rq_throttled(cfs_rq))
            break;
    }

    assert_list_leaf_cfs_rq(rq);

    if (!se)
        add_nr_running(rq, task_delta);

    /* Determine whether we need to wake up potentially idle CPU: */
    if (rq->curr == rq->idle && rq->cfs.nr_running)
        resched_curr(rq);
}

static u64 distribute_cfs_runtime(struct cfs_bandwidth *cfs_b, u64 remaining)
{
    struct cfs_rq *cfs_rq;
    u64 runtime;
    u64 starting_runtime = remaining;

    rcu_read_lock();
    list_for_each_entry_rcu(cfs_rq, &cfs_b->throttled_cfs_rq,
                throttled_list) {
        struct rq *rq = rq_of(cfs_rq);
        struct rq_flags rf;

        rq_lock_irqsave(rq, &rf);
        if (!cfs_rq_throttled(cfs_rq))
            goto next;

        /* By the above check, this should never be true */
        SCHED_WARN_ON(cfs_rq->runtime_remaining > 0);

        runtime = -cfs_rq->runtime_remaining + 1;
        if (runtime > remaining)
            runtime = remaining;
        remaining -= runtime;

        cfs_rq->runtime_remaining += runtime;

        /* we check whether we're throttled above */
        if (cfs_rq->runtime_remaining > 0)
            unthrottle_cfs_rq(cfs_rq);

next:
        rq_unlock_irqrestore(rq, &rf);

        if (!remaining)
            break;
    }
    rcu_read_unlock();

    return starting_runtime - remaining;
}
//4641 lines







//4712 lines
/* a cfs_rq won't donate quota below this amount */
static const u64 min_cfs_rq_runtime = 1 * NSEC_PER_MSEC;
/* minimum remaining period time to redistribute slack quota */
static const u64 min_bandwidth_expiration = 2 * NSEC_PER_MSEC;
/* how long we wait to gather additional slack before distributing */
static const u64 cfs_bandwidth_slack_period = 5 * NSEC_PER_MSEC;

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
//4796
/*
 * This is done with a timer (instead of inline with bandwidth return) since
 * it's necessary to juggle rq->locks to unthrottle their respective cfs_rqs.
 */
static void do_sched_cfs_slack_timer(struct cfs_bandwidth *cfs_b)
{
    u64 runtime = 0, slice = sched_cfs_bandwidth_slice();
    unsigned long flags;

    /* confirm we're still not at a refresh boundary */
    raw_spin_lock_irqsave(&cfs_b->lock, flags);
    cfs_b->slack_started = false;
    if (cfs_b->distribute_running) {
        raw_spin_unlock_irqrestore(&cfs_b->lock, flags);
        return;
    }

    if (runtime_refresh_within(cfs_b, min_bandwidth_expiration)) {
        raw_spin_unlock_irqrestore(&cfs_b->lock, flags);
        return;
    }

    if (cfs_b->quota != RUNTIME_INF && cfs_b->runtime > slice)
        runtime = cfs_b->runtime;

    if (runtime)
        cfs_b->distribute_running = 1;

    raw_spin_unlock_irqrestore(&cfs_b->lock, flags);

    if (!runtime)
        return;

    runtime = distribute_cfs_runtime(cfs_b, runtime);

    raw_spin_lock_irqsave(&cfs_b->lock, flags);
    lsub_positive(&cfs_b->runtime, runtime);
    cfs_b->distribute_running = 0;
    raw_spin_unlock_irqrestore(&cfs_b->lock, flags);
}

/*
 * When a group wakes up we want to make sure that its quota is not already
 * expired/exceeded, otherwise it may be allowed to steal additional ticks of
 * runtime as update_curr() throttling can not not trigger until it's on-rq.
 */
static void check_enqueue_throttle(struct cfs_rq *cfs_rq)
{
    if (!cfs_bandwidth_used())
        return;

    /* an active group must be handled by the update_curr()->put() path */
    if (!cfs_rq->runtime_enabled || cfs_rq->curr)
        return;

    /* ensure the group is not already throttled */
    if (cfs_rq_throttled(cfs_rq))
        return;

    /* update runtime allocation */
    account_cfs_rq_runtime(cfs_rq, 0);
    if (cfs_rq->runtime_remaining <= 0)
        throttle_cfs_rq(cfs_rq);
}
//4861
static void sync_throttle(struct task_group *tg, int cpu)
{
    pr_fn_start_on(stack_depth);

        struct cfs_rq *pcfs_rq, *cfs_rq;

        if (!cfs_bandwidth_used())
                return;

        if (!tg->parent)
                return;

        cfs_rq = tg->cfs_rq[cpu];
        pcfs_rq = tg->parent->cfs_rq[cpu];

        cfs_rq->throttle_count = pcfs_rq->throttle_count;
        cfs_rq->throttled_clock_task = rq_clock_task(cpu_rq(cpu));

    pr_fn_end_on(stack_depth);
}

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
    pr_fn_start_on(stack_depth);

    raw_spin_lock_init(&cfs_b->lock);
    cfs_b->runtime = 0;
    cfs_b->quota = RUNTIME_INF;
    cfs_b->period = ns_to_ktime(default_cfs_period());

    INIT_LIST_HEAD(&cfs_b->throttled_cfs_rq);
    hrtimer_init(&cfs_b->period_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS_PINNED);
    //cfs_b->period_timer.function = sched_cfs_period_timer;
    hrtimer_init(&cfs_b->slack_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    //cfs_b->slack_timer.function = sched_cfs_slack_timer;
    cfs_b->distribute_running = 0;
    cfs_b->slack_started = false;

    pr_info_view_on(stack_depth, "%20s: %llu\n", cfs_b->runtime);
    pr_info_view_on(stack_depth, "%20s: %llu\n", cfs_b->quota);
    pr_info_view_on(stack_depth, "%20s: %lld\n", cfs_b->period);
    pr_fn_end_on(stack_depth);
}

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
    hrtimer_start_expires(&cfs_b->period_timer, HRTIMER_MODE_ABS_PINNED);
}

static void destroy_cfs_bandwidth(struct cfs_bandwidth *cfs_b)
{
    /* init_cfs_bandwidth() was not called */
    if (!cfs_b->throttled_cfs_rq.next)
        return;

    hrtimer_cancel(&cfs_b->period_timer);
    hrtimer_cancel(&cfs_b->slack_timer);
}
//5008
/*
 * Both these CPU hotplug callbacks race against unregister_fair_sched_group()
 *
 * The race is harmless, since modifying bandwidth settings of unhooked group
 * bits doesn't do much.
 */

/* cpu online calback */
static void __maybe_unused update_runtime_enabled(struct rq *rq)
{
    struct task_group *tg;

    lockdep_assert_held(&rq->lock);

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        struct cfs_bandwidth *cfs_b = &tg->cfs_bandwidth;
        struct cfs_rq *cfs_rq = tg->cfs_rq[cpu_of(rq)];

        raw_spin_lock(&cfs_b->lock);
        cfs_rq->runtime_enabled = cfs_b->quota != RUNTIME_INF;
        raw_spin_unlock(&cfs_b->lock);
    }
    rcu_read_unlock();
}

/* cpu offline callback */
static void __maybe_unused unthrottle_offline_cfs_rqs(struct rq *rq)
{
    struct task_group *tg;

    lockdep_assert_held(&rq->lock);

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        struct cfs_rq *cfs_rq = tg->cfs_rq[cpu_of(rq)];

        if (!cfs_rq->runtime_enabled)
            continue;

        /*
         * clock_task is not advancing so we just need to make sure
         * there's some valid quota amount
         */
        cfs_rq->runtime_remaining = 1;
        /*
         * Offline rq is schedulable till CPU is completely disabled
         * in take_cpu_down(), so we prevent new cfs throttling here.
         */
        cfs_rq->runtime_enabled = 0;

        if (cfs_rq_throttled(cfs_rq))
            unthrottle_cfs_rq(cfs_rq);
    }
    rcu_read_unlock();
}
//5065
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
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)p);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)&p->se);

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

    int count=0;
    for_each_sched_entity(se) {
        pr_info_view_on(stack_depth, "%20s : %d\n", count++);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
        pr_info_view_on(stack_depth, "%20s : %d\n", se->on_rq);
        if (se->on_rq) {
            pr_info_on(stack_depth, "se->on_rq break\n");
            break;
        }
        cfs_rq = cfs_rq_of(se);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
        WARN_ON (!cfs_rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq_of(cfs_rq));
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

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);

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
            pr_info_view_on(stack_depth, "%20s : 0x%X\n", flags);
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
        pr_fn_start_on(stack_depth);

        struct cfs_rq *cfs_rq;
        struct sched_entity *se = &p->se;
        int task_sleep = flags & DEQUEUE_SLEEP;
        int idle_h_nr_running = task_has_idle_policy(p);

        int count = 0;
        for_each_sched_entity(se) {
                cfs_rq = cfs_rq_of(se);
                pr_info_view_on(stack_depth, "%20s : %d\n", count++);
                pr_info_view_on(stack_depth, "%20s : %d\n", rq->cpu);
                pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
                pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
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
//5394
static void record_wakee(struct task_struct *p)
{
    /*
     * Only decay a single time; tasks that have less then 1 wakeup per
     * jiffy will not have built up many flips.
     */
    if (time_after(jiffies, current->wakee_flip_decay_ts + HZ)) {
        current->wakee_flips >>= 1;
        current->wakee_flip_decay_ts = jiffies;
    }

    if (current->last_wakee != p) {
        current->last_wakee = p;
        current->wakee_flips++;
    }
}

/*
 * Detect M:N waker/wakee relationships via a switching-frequency heuristic.
 *
 * A waker of many should wake a different task than the one last awakened
 * at a frequency roughly N times higher than one of its wakees.
 *
 * In order to determine whether we should let the load spread vs consolidating
 * to shared cache, we look for a minimum 'flip' frequency of llc_size in one
 * partner, and a factor of lls_size higher frequency in the other.
 *
 * With both conditions met, we can be relatively sure that the relationship is
 * non-monogamous, with partner count exceeding socket size.
 *
 * Waker/wakee being client/server, worker/dispatcher, interrupt source or
 * whatever is irrelevant, spread criteria is apparent partner count exceeds
 * socket size.
 */
static int wake_wide(struct task_struct *p)
{
    unsigned int master = current->wakee_flips;
    unsigned int slave = p->wakee_flips;
    int factor = this_cpu_read(sd_llc_size);

    if (master < slave)
        swap(master, slave);
    if (slave < factor || master < slave * factor)
        return 0;
    return 1;
}

/*
 * The purpose of wake_affine() is to quickly determine on which CPU we can run
 * soonest. For the purpose of speed we only consider the waking and previous
 * CPU.
 *
 * wake_affine_idle() - only considers 'now', it check if the waking CPU is
 *			cache-affine and is (or	will be) idle.
 *
 * wake_affine_weight() - considers the weight to reflect the average
 *			  scheduling latency of the CPUs. This seems to work
 *			  for the overloaded case.
 */
static int
wake_affine_idle(int this_cpu, int prev_cpu, int sync)
{
    /*
     * If this_cpu is idle, it implies the wakeup is from interrupt
     * context. Only allow the move if cache is shared. Otherwise an
     * interrupt intensive workload could force all tasks onto one
     * node depending on the IO topology or IRQ affinity settings.
     *
     * If the prev_cpu is idle and cache affine then avoid a migration.
     * There is no guarantee that the cache hot data from an interrupt
     * is more important than cache hot data on the prev_cpu and from
     * a cpufreq perspective, it's better to have higher utilisation
     * on one CPU.
     */
    if (available_idle_cpu(this_cpu) && cpus_share_cache(this_cpu, prev_cpu))
        return available_idle_cpu(prev_cpu) ? prev_cpu : this_cpu;

    if (sync && cpu_rq(this_cpu)->nr_running == 1)
        return this_cpu;

    return nr_cpumask_bits;
}

static int
wake_affine_weight(struct sched_domain *sd, struct task_struct *p,
           int this_cpu, int prev_cpu, int sync)
{
    s64 this_eff_load, prev_eff_load;
    unsigned long task_load;

    this_eff_load = cpu_runnable_load(cpu_rq(this_cpu));

    if (sync) {
        unsigned long current_load = task_h_load(current);

        if (current_load > this_eff_load)
            return this_cpu;

        this_eff_load -= current_load;
    }

    task_load = task_h_load(p);

    this_eff_load += task_load;
    if (sched_feat(WA_BIAS))
        this_eff_load *= 100;
    this_eff_load *= capacity_of(prev_cpu);

    prev_eff_load = cpu_runnable_load(cpu_rq(prev_cpu));
    prev_eff_load -= task_load;
    if (sched_feat(WA_BIAS))
        prev_eff_load *= 100 + (sd->imbalance_pct - 100) / 2;
    prev_eff_load *= capacity_of(this_cpu);

    /*
     * If sync, adjust the weight of prev_eff_load such that if
     * prev_eff == this_eff that select_idle_sibling() will consider
     * stacking the wakee on top of the waker if no other CPU is
     * idle.
     */
    if (sync)
        prev_eff_load += 1;

    return this_eff_load < prev_eff_load ? this_cpu : nr_cpumask_bits;
}

static int wake_affine(struct sched_domain *sd, struct task_struct *p,
               int this_cpu, int prev_cpu, int sync)
{
    int target = nr_cpumask_bits;

    if (sched_feat(WA_IDLE))
        target = wake_affine_idle(this_cpu, prev_cpu, sync);

    if (sched_feat(WA_WEIGHT) && target == nr_cpumask_bits)
        target = wake_affine_weight(sd, p, this_cpu, prev_cpu, sync);

    schedstat_inc(p->se.statistics.nr_wakeups_affine_attempts);
    if (target == nr_cpumask_bits)
        return prev_cpu;

    schedstat_inc(sd->ttwu_move_affine);
    schedstat_inc(p->se.statistics.nr_wakeups_affine);
    return target;
}

static unsigned long cpu_util_without(int cpu, struct task_struct *p);

static unsigned long capacity_spare_without(int cpu, struct task_struct *p)
{
    return max_t(long, capacity_of(cpu) - cpu_util_without(cpu, p), 0);
}
//5547
/*
 * find_idlest_group finds and returns the least busy CPU group within the
 * domain.
 *
 * Assumes p is allowed on at least one CPU in sd.
 */
static struct sched_group *
find_idlest_group(struct sched_domain *sd, struct task_struct *p,
          int this_cpu, int sd_flag)
{
    struct sched_group *idlest = NULL, *group = sd->groups;
    struct sched_group *most_spare_sg = NULL;
    unsigned long min_runnable_load = ULONG_MAX;
    unsigned long this_runnable_load = ULONG_MAX;
    unsigned long min_avg_load = ULONG_MAX, this_avg_load = ULONG_MAX;
    unsigned long most_spare = 0, this_spare = 0;
    int imbalance_scale = 100 + (sd->imbalance_pct-100)/2;
    unsigned long imbalance = scale_load_down(NICE_0_LOAD) *
                (sd->imbalance_pct-100) / 100;

    do {
        unsigned long load, avg_load, runnable_load;
        unsigned long spare_cap, max_spare_cap;
        int local_group;
        int i;

        /* Skip over this group if it has no CPUs allowed */
        if (!cpumask_intersects(sched_group_span(group),
                    p->cpus_ptr))
            continue;

        local_group = cpumask_test_cpu(this_cpu,
                           sched_group_span(group));

        /*
         * Tally up the load of all CPUs in the group and find
         * the group containing the CPU with most spare capacity.
         */
        avg_load = 0;
        runnable_load = 0;
        max_spare_cap = 0;

        for_each_cpu(i, sched_group_span(group)) {
            load = cpu_runnable_load(cpu_rq(i));
            runnable_load += load;

            avg_load += cfs_rq_load_avg(&cpu_rq(i)->cfs);

            spare_cap = capacity_spare_without(i, p);

            if (spare_cap > max_spare_cap)
                max_spare_cap = spare_cap;
        }

        /* Adjust by relative CPU capacity of the group */
        avg_load = (avg_load * SCHED_CAPACITY_SCALE) /
                    group->sgc->capacity;
        runnable_load = (runnable_load * SCHED_CAPACITY_SCALE) /
                    group->sgc->capacity;

        if (local_group) {
            this_runnable_load = runnable_load;
            this_avg_load = avg_load;
            this_spare = max_spare_cap;
        } else {
            if (min_runnable_load > (runnable_load + imbalance)) {
                /*
                 * The runnable load is significantly smaller
                 * so we can pick this new CPU:
                 */
                min_runnable_load = runnable_load;
                min_avg_load = avg_load;
                idlest = group;
            } else if ((runnable_load < (min_runnable_load + imbalance)) &&
                   (100*min_avg_load > imbalance_scale*avg_load)) {
                /*
                 * The runnable loads are close so take the
                 * blocked load into account through avg_load:
                 */
                min_avg_load = avg_load;
                idlest = group;
            }

            if (most_spare < max_spare_cap) {
                most_spare = max_spare_cap;
                most_spare_sg = group;
            }
        }
    } while (group = group->next, group != sd->groups);

    /*
     * The cross-over point between using spare capacity or least load
     * is too conservative for high utilization tasks on partially
     * utilized systems if we require spare_capacity > task_util(p),
     * so we allow for some task stuffing by using
     * spare_capacity > task_util(p)/2.
     *
     * Spare capacity can't be used for fork because the utilization has
     * not been set yet, we must first select a rq to compute the initial
     * utilization.
     */
    if (sd_flag & SD_BALANCE_FORK)
        goto skip_spare;

    if (this_spare > task_util(p) / 2 &&
        imbalance_scale*this_spare > 100*most_spare)
        return NULL;

    if (most_spare > task_util(p) / 2)
        return most_spare_sg;

skip_spare:
    if (!idlest)
        return NULL;

    /*
     * When comparing groups across NUMA domains, it's possible for the
     * local domain to be very lightly loaded relative to the remote
     * domains but "imbalance" skews the comparison making remote CPUs
     * look much more favourable. When considering cross-domain, add
     * imbalance to the runnable load on the remote node and consider
     * staying local.
     */
    if ((sd->flags & SD_NUMA) &&
        min_runnable_load + imbalance >= this_runnable_load)
        return NULL;

    if (min_runnable_load > (this_runnable_load + imbalance))
        return NULL;

    if ((this_runnable_load < (min_runnable_load + imbalance)) &&
         (100*this_avg_load < imbalance_scale*min_avg_load))
        return NULL;

    return idlest;
}

/*
 * find_idlest_group_cpu - find the idlest CPU among the CPUs in the group.
 */
static int
find_idlest_group_cpu(struct sched_group *group, struct task_struct *p, int this_cpu)
{
    unsigned long load, min_load = ULONG_MAX;
    unsigned int min_exit_latency = UINT_MAX;
    u64 latest_idle_timestamp = 0;
    int least_loaded_cpu = this_cpu;
    int shallowest_idle_cpu = -1, si_cpu = -1;
    int i;

    /* Check if we have any choice: */
    if (group->group_weight == 1)
        return cpumask_first(sched_group_span(group));

    /* Traverse only the allowed CPUs */
    for_each_cpu_and(i, sched_group_span(group), p->cpus_ptr) {
        if (available_idle_cpu(i)) {
            struct rq *rq = cpu_rq(i);
            struct cpuidle_state *idle = idle_get_state(rq);
            if (idle && idle->exit_latency < min_exit_latency) {
                /*
                 * We give priority to a CPU whose idle state
                 * has the smallest exit latency irrespective
                 * of any idle timestamp.
                 */
                min_exit_latency = idle->exit_latency;
                latest_idle_timestamp = rq->idle_stamp;
                shallowest_idle_cpu = i;
            } else if ((!idle || idle->exit_latency == min_exit_latency) &&
                   rq->idle_stamp > latest_idle_timestamp) {
                /*
                 * If equal or no active idle state, then
                 * the most recently idled CPU might have
                 * a warmer cache.
                 */
                latest_idle_timestamp = rq->idle_stamp;
                shallowest_idle_cpu = i;
            }
        } else if (shallowest_idle_cpu == -1 && si_cpu == -1) {
            if (sched_idle_cpu(i)) {
                si_cpu = i;
                continue;
            }

            load = cpu_runnable_load(cpu_rq(i));
            if (load < min_load) {
                min_load = load;
                least_loaded_cpu = i;
            }
        }
    }

    if (shallowest_idle_cpu != -1)
        return shallowest_idle_cpu;
    if (si_cpu != -1)
        return si_cpu;
    return least_loaded_cpu;
}

static inline int find_idlest_cpu(struct sched_domain *sd, struct task_struct *p,
                  int cpu, int prev_cpu, int sd_flag)
{
    int new_cpu = cpu;

    if (!cpumask_intersects(sched_domain_span(sd), p->cpus_ptr))
        return prev_cpu;

    /*
     * We need task's util for capacity_spare_without, sync it up to
     * prev_cpu's last_update_time.
     */
    if (!(sd_flag & SD_BALANCE_FORK))
        sync_entity_load_avg(&p->se);

    while (sd) {
        struct sched_group *group;
        struct sched_domain *tmp;
        int weight;

        if (!(sd->flags & sd_flag)) {
            sd = sd->child;
            continue;
        }

        group = find_idlest_group(sd, p, cpu, sd_flag);
        if (!group) {
            sd = sd->child;
            continue;
        }

        new_cpu = find_idlest_group_cpu(group, p, cpu);
        if (new_cpu == cpu) {
            /* Now try balancing at a lower domain level of 'cpu': */
            sd = sd->child;
            continue;
        }

        /* Now try balancing at a lower domain level of 'new_cpu': */
        cpu = new_cpu;
        weight = sd->span_weight;
        sd = NULL;
        for_each_domain(cpu, tmp) {
            if (weight <= tmp->span_weight)
                break;
            if (tmp->flags & sd_flag)
                sd = tmp;
        }
    }

    return new_cpu;
}

#ifdef CONFIG_SCHED_SMT
DEFINE_STATIC_KEY_FALSE(sched_smt_present);
EXPORT_SYMBOL_GPL(sched_smt_present);

static inline void set_idle_cores(int cpu, int val)
{
    struct sched_domain_shared *sds;

    sds = rcu_dereference(per_cpu(sd_llc_shared, cpu));
    if (sds)
        WRITE_ONCE(sds->has_idle_cores, val);
}

static inline bool test_idle_cores(int cpu, bool def)
{
    struct sched_domain_shared *sds;

    sds = rcu_dereference(per_cpu(sd_llc_shared, cpu));
    if (sds)
        return READ_ONCE(sds->has_idle_cores);

    return def;
}

/*
 * Scans the local SMT mask to see if the entire core is idle, and records this
 * information in sd_llc_shared->has_idle_cores.
 *
 * Since SMT siblings share all cache levels, inspecting this limited remote
 * state should be fairly cheap.
 */
void __update_idle_core(struct rq *rq)
{
    int core = cpu_of(rq);
    int cpu;

    rcu_read_lock();
    if (test_idle_cores(core, true))
        goto unlock;

    for_each_cpu(cpu, cpu_smt_mask(core)) {
        if (cpu == core)
            continue;

        if (!available_idle_cpu(cpu))
            goto unlock;
    }

    set_idle_cores(core, 1);
unlock:
    rcu_read_unlock();
}
//5852
/*
 * Scan the entire LLC domain for idle cores; this dynamically switches off if
 * there are no idle cores left in the system; tracked through
 * sd_llc->shared->has_idle_cores and enabled through update_idle_core() above.
 */
static int select_idle_core(struct task_struct *p, struct sched_domain *sd, int target)
{
    struct cpumask *cpus = this_cpu_cpumask_var_ptr(select_idle_mask);
    int core, cpu;

    if (!static_branch_likely(&sched_smt_present))
        return -1;

    if (!test_idle_cores(target, false))
        return -1;

    cpumask_and(cpus, sched_domain_span(sd), p->cpus_ptr);

    for_each_cpu_wrap(core, cpus, target) {
        bool idle = true;

        for_each_cpu(cpu, cpu_smt_mask(core)) {
            __cpumask_clear_cpu(cpu, cpus);
            if (!available_idle_cpu(cpu))
                idle = false;
        }

        if (idle)
            return core;
    }

    /*
     * Failed to find an idle core; stop looking for one.
     */
    set_idle_cores(target, 0);

    return -1;
}

/*
 * Scan the local SMT mask for idle CPUs.
 */
static int select_idle_smt(struct task_struct *p, int target)
{
    int cpu, si_cpu = -1;

    if (!static_branch_likely(&sched_smt_present))
        return -1;

    for_each_cpu(cpu, cpu_smt_mask(target)) {
        if (!cpumask_test_cpu(cpu, p->cpus_ptr))
            continue;
        if (available_idle_cpu(cpu))
            return cpu;
        if (si_cpu == -1 && sched_idle_cpu(cpu))
            si_cpu = cpu;
    }

    return si_cpu;
}

#else /* CONFIG_SCHED_SMT */

static inline int select_idle_core(struct task_struct *p, struct sched_domain *sd, int target)
{
    return -1;
}

static inline int select_idle_smt(struct task_struct *p, int target)
{
    return -1;
}

#endif /* CONFIG_SCHED_SMT */

/*
 * Scan the LLC domain for idle CPUs; this is dynamically regulated by
 * comparing the average scan cost (tracked in sd->avg_scan_cost) against the
 * average idle time for this rq (as found in rq->avg_idle).
 */
static int select_idle_cpu(struct task_struct *p, struct sched_domain *sd, int target)
{
    struct sched_domain *this_sd;
    u64 avg_cost, avg_idle;
    u64 time, cost;
    s64 delta;
    int this = smp_processor_id();
    int cpu, nr = INT_MAX, si_cpu = -1;

    this_sd = rcu_dereference(*this_cpu_ptr(&sd_llc));
    if (!this_sd)
        return -1;

    /*
     * Due to large variance we need a large fuzz factor; hackbench in
     * particularly is sensitive here.
     */
    avg_idle = this_rq()->avg_idle / 512;
    avg_cost = this_sd->avg_scan_cost + 1;

    if (sched_feat(SIS_AVG_CPU) && avg_idle < avg_cost)
        return -1;

    if (sched_feat(SIS_PROP)) {
        u64 span_avg = sd->span_weight * avg_idle;
        if (span_avg > 4*avg_cost)
            nr = div_u64(span_avg, avg_cost);
        else
            nr = 4;
    }

    time = cpu_clock(this);

    for_each_cpu_wrap(cpu, sched_domain_span(sd), target) {
        if (!--nr)
            return si_cpu;
        if (!cpumask_test_cpu(cpu, p->cpus_ptr))
            continue;
        if (available_idle_cpu(cpu))
            break;
        if (si_cpu == -1 && sched_idle_cpu(cpu))
            si_cpu = cpu;
    }

    time = cpu_clock(this) - time;
    cost = this_sd->avg_scan_cost;
    delta = (s64)(time - cost) / 8;
    this_sd->avg_scan_cost += delta;

    return cpu;
}

/*
 * Try and locate an idle core/thread in the LLC cache domain.
 */
static int select_idle_sibling(struct task_struct *p, int prev, int target)
{
    struct sched_domain *sd;
    int i, recent_used_cpu;

    if (available_idle_cpu(target) || sched_idle_cpu(target))
        return target;

    /*
     * If the previous CPU is cache affine and idle, don't be stupid:
     */
    if (prev != target && cpus_share_cache(prev, target) &&
        (available_idle_cpu(prev) || sched_idle_cpu(prev)))
        return prev;

    /* Check a recently used CPU as a potential idle candidate: */
    recent_used_cpu = p->recent_used_cpu;
    if (recent_used_cpu != prev &&
        recent_used_cpu != target &&
        cpus_share_cache(recent_used_cpu, target) &&
        (available_idle_cpu(recent_used_cpu) || sched_idle_cpu(recent_used_cpu)) &&
        cpumask_test_cpu(p->recent_used_cpu, p->cpus_ptr)) {
        /*
         * Replace recent_used_cpu with prev as it is a potential
         * candidate for the next wake:
         */
        p->recent_used_cpu = prev;
        return recent_used_cpu;
    }

    sd = rcu_dereference(per_cpu(sd_llc, target));
    if (!sd)
        return target;

    i = select_idle_core(p, sd, target);
    if ((unsigned)i < nr_cpumask_bits)
        return i;

    i = select_idle_cpu(p, sd, target);
    if ((unsigned)i < nr_cpumask_bits)
        return i;

    i = select_idle_smt(p, target);
    if ((unsigned)i < nr_cpumask_bits)
        return i;

    return target;
}

/**
 * Amount of capacity of a CPU that is (estimated to be) used by CFS tasks
 * @cpu: the CPU to get the utilization of
 *
 * The unit of the return value must be the one of capacity so we can compare
 * the utilization with the capacity of the CPU that is available for CFS task
 * (ie cpu_capacity).
 *
 * cfs_rq.avg.util_avg is the sum of running time of runnable tasks plus the
 * recent utilization of currently non-runnable tasks on a CPU. It represents
 * the amount of utilization of a CPU in the range [0..capacity_orig] where
 * capacity_orig is the cpu_capacity available at the highest frequency
 * (arch_scale_freq_capacity()).
 * The utilization of a CPU converges towards a sum equal to or less than the
 * current capacity (capacity_curr <= capacity_orig) of the CPU because it is
 * the running time on this CPU scaled by capacity_curr.
 *
 * The estimated utilization of a CPU is defined to be the maximum between its
 * cfs_rq.avg.util_avg and the sum of the estimated utilization of the tasks
 * currently RUNNABLE on that CPU.
 * This allows to properly represent the expected utilization of a CPU which
 * has just got a big task running since a long sleep period. At the same time
 * however it preserves the benefits of the "blocked utilization" in
 * describing the potential for other tasks waking up on the same CPU.
 *
 * Nevertheless, cfs_rq.avg.util_avg can be higher than capacity_curr or even
 * higher than capacity_orig because of unfortunate rounding in
 * cfs.avg.util_avg or just after migrating tasks and new task wakeups until
 * the average stabilizes with the new running time. We need to check that the
 * utilization stays within the range of [0..capacity_orig] and cap it if
 * necessary. Without utilization capping, a group could be seen as overloaded
 * (CPU0 utilization at 121% + CPU1 utilization at 80%) whereas CPU1 has 20% of
 * available capacity. We allow utilization to overshoot capacity_curr (but not
 * capacity_orig) as it useful for predicting the capacity required after task
 * migrations (scheduler-driven DVFS).
 *
 * Return: the (estimated) utilization for the specified CPU
 */
static inline unsigned long cpu_util(int cpu)
{
    struct cfs_rq *cfs_rq;
    unsigned int util;

    cfs_rq = &cpu_rq(cpu)->cfs;
    util = READ_ONCE(cfs_rq->avg.util_avg);

    if (sched_feat(UTIL_EST))
        util = max(util, READ_ONCE(cfs_rq->avg.util_est.enqueued));

    return min_t(unsigned long, util, capacity_orig_of(cpu));
}

/*
 * cpu_util_without: compute cpu utilization without any contributions from *p
 * @cpu: the CPU which utilization is requested
 * @p: the task which utilization should be discounted
 *
 * The utilization of a CPU is defined by the utilization of tasks currently
 * enqueued on that CPU as well as tasks which are currently sleeping after an
 * execution on that CPU.
 *
 * This method returns the utilization of the specified CPU by discounting the
 * utilization of the specified task, whenever the task is currently
 * contributing to the CPU utilization.
 */
static unsigned long cpu_util_without(int cpu, struct task_struct *p)
{
    struct cfs_rq *cfs_rq;
    unsigned int util;

    /* Task has no contribution or is new */
    if (cpu != task_cpu(p) || !READ_ONCE(p->se.avg.last_update_time))
        return cpu_util(cpu);

    cfs_rq = &cpu_rq(cpu)->cfs;
    util = READ_ONCE(cfs_rq->avg.util_avg);

    /* Discount task's util from CPU's util */
    lsub_positive(&util, task_util(p));

    /*
     * Covered cases:
     *
     * a) if *p is the only task sleeping on this CPU, then:
     *      cpu_util (== task_util) > util_est (== 0)
     *    and thus we return:
     *      cpu_util_without = (cpu_util - task_util) = 0
     *
     * b) if other tasks are SLEEPING on this CPU, which is now exiting
     *    IDLE, then:
     *      cpu_util >= task_util
     *      cpu_util > util_est (== 0)
     *    and thus we discount *p's blocked utilization to return:
     *      cpu_util_without = (cpu_util - task_util) >= 0
     *
     * c) if other tasks are RUNNABLE on that CPU and
     *      util_est > cpu_util
     *    then we use util_est since it returns a more restrictive
     *    estimation of the spare capacity on that CPU, by just
     *    considering the expected utilization of tasks already
     *    runnable on that CPU.
     *
     * Cases a) and b) are covered by the above code, while case c) is
     * covered by the following code when estimated utilization is
     * enabled.
     */
    if (sched_feat(UTIL_EST)) {
        unsigned int estimated =
            READ_ONCE(cfs_rq->avg.util_est.enqueued);

        /*
         * Despite the following checks we still have a small window
         * for a possible race, when an execl's select_task_rq_fair()
         * races with LB's detach_task():
         *
         *   detach_task()
         *     p->on_rq = TASK_ON_RQ_MIGRATING;
         *     ---------------------------------- A
         *     deactivate_task()                   \
         *       dequeue_task()                     + RaceTime
         *         util_est_dequeue()              /
         *     ---------------------------------- B
         *
         * The additional check on "current == p" it's required to
         * properly fix the execl regression and it helps in further
         * reducing the chances for the above race.
         */
        if (unlikely(task_on_rq_queued(p) || current == p))
            lsub_positive(&estimated, _task_util_est(p));

        util = max(util, estimated);
    }

    /*
     * Utilization (estimated) can exceed the CPU capacity, thus let's
     * clamp to the maximum CPU capacity to ensure consistency with
     * the cpu_util call.
     */
    return min_t(unsigned long, util, capacity_orig_of(cpu));
}
//6177
/*
 * Disable WAKE_AFFINE in the case where task @p doesn't fit in the
 * capacity of either the waking CPU @cpu or the previous CPU @prev_cpu.
 *
 * In that case WAKE_AFFINE doesn't make sense and we'll let
 * BALANCE_WAKE sort things out.
 */
static int wake_cap(struct task_struct *p, int cpu, int prev_cpu)
{
    long min_cap, max_cap;

    if (!static_branch_unlikely(&sched_asym_cpucapacity))
        return 0;

    min_cap = min(capacity_orig_of(prev_cpu), capacity_orig_of(cpu));
    max_cap = cpu_rq(cpu)->rd->max_cpu_capacity;

    /* Minimum capacity is close to max, no need to abort wake_affine */
    if (max_cap - min_cap < max_cap >> 3)
        return 0;

    /* Bring task utilization in sync with prev_cpu */
    sync_entity_load_avg(&p->se);

    return !task_fits_capacity(p, min_cap);
}
//6204 lines



//6333 lines
static int find_energy_efficient_cpu(struct task_struct *p, int prev_cpu)
{
    return -1;
}



//6429 lines
/*
 * select_task_rq_fair: Select target runqueue for the waking task in domains
 * that have the 'sd_flag' flag set. In practice, this is SD_BALANCE_WAKE,
 * SD_BALANCE_FORK, or SD_BALANCE_EXEC.
 *
 * Balances load by selecting the idlest CPU in the idlest group, or under
 * certain conditions an idle sibling CPU if the domain has SD_WAKE_AFFINE set.
 *
 * Returns the target CPU number.
 *
 * preempt must be disabled.
 */
static int
select_task_rq_fair(struct task_struct *p, int prev_cpu, int sd_flag, int wake_flags)
{
    struct sched_domain *tmp, *sd = NULL;
    int cpu = smp_processor_id();
    int new_cpu = prev_cpu;
    int want_affine = 0;
    int sync = (wake_flags & WF_SYNC) && !(current->flags & PF_EXITING);

    if (sd_flag & SD_BALANCE_WAKE) {
        record_wakee(p);

        if (sched_energy_enabled()) {
            new_cpu = find_energy_efficient_cpu(p, prev_cpu);
            if (new_cpu >= 0)
                return new_cpu;
            new_cpu = prev_cpu;
        }

        want_affine = !wake_wide(p) && !wake_cap(p, cpu, prev_cpu) &&
                  cpumask_test_cpu(cpu, p->cpus_ptr);
    }

    rcu_read_lock();
    for_each_domain(cpu, tmp) {
        if (!(tmp->flags & SD_LOAD_BALANCE))
            break;

        /*
         * If both 'cpu' and 'prev_cpu' are part of this domain,
         * cpu is a valid SD_WAKE_AFFINE target.
         */
        if (want_affine && (tmp->flags & SD_WAKE_AFFINE) &&
            cpumask_test_cpu(prev_cpu, sched_domain_span(tmp))) {
            if (cpu != prev_cpu)
                new_cpu = wake_affine(tmp, p, cpu, prev_cpu, sync);

            sd = NULL; /* Prefer wake_affine over balance flags */
            break;
        }

        if (tmp->flags & sd_flag)
            sd = tmp;
        else if (!want_affine)
            break;
    }

    if (unlikely(sd)) {
        /* Slow path */
        new_cpu = find_idlest_cpu(sd, p, cpu, prev_cpu, sd_flag);
    } else if (sd_flag & SD_BALANCE_WAKE) { /* XXX always ? */
        /* Fast path */

        new_cpu = select_idle_sibling(p, prev_cpu, new_cpu);

        if (want_affine)
            current->recent_used_cpu = cpu;
    }
    rcu_read_unlock();

    return new_cpu;
}

static void detach_entity_cfs_rq(struct sched_entity *se);

/*
 * Called immediately before a task is migrated to a new CPU; task_cpu(p) and
 * cfs_rq_of(p) references at time of call are still valid and identify the
 * previous CPU. The caller guarantees p->pi_lock or task_rq(p)->lock is held.
 */
static void migrate_task_rq_fair(struct task_struct *p, int new_cpu)
{
    /*
     * As blocked tasks retain absolute vruntime the migration needs to
     * deal with this by subtracting the old and adding the new
     * min_vruntime -- the latter is done by enqueue_entity() when placing
     * the task on the new runqueue.
     */
    if (p->state == TASK_WAKING) {
        struct sched_entity *se = &p->se;
        struct cfs_rq *cfs_rq = cfs_rq_of(se);
        u64 min_vruntime;

#ifndef CONFIG_64BIT
        u64 min_vruntime_copy;

        do {
            min_vruntime_copy = cfs_rq->min_vruntime_copy;
            smp_rmb();
            min_vruntime = cfs_rq->min_vruntime;
        } while (min_vruntime != min_vruntime_copy);
#else
        min_vruntime = cfs_rq->min_vruntime;
#endif

        se->vruntime -= min_vruntime;
    }

    if (p->on_rq == TASK_ON_RQ_MIGRATING) {
        /*
         * In case of TASK_ON_RQ_MIGRATING we in fact hold the 'old'
         * rq->lock and can modify state directly.
         */
        lockdep_assert_held(&task_rq(p)->lock);
        detach_entity_cfs_rq(&p->se);

    } else {
        /*
         * We are supposed to update the task to "current" time, then
         * its up to date and ready to go to new CPU/cfs_rq. But we
         * have difficulty in getting what current time is, so simply
         * throw away the out-of-date time. This will result in the
         * wakee task is less decayed, but giving the wakee more load
         * sounds not bad.
         */
        remove_entity_load_avg(&p->se);
    }

    /* Tell new CPU we are migrated */
    p->se.avg.last_update_time = 0;

    /* We have migrated, no longer consider this task hot */
    p->se.exec_start = 0;

    update_scan_period(p, new_cpu);
}
//6568
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
    pr_fn_start_on(stack_depth);

    s64 gran, vdiff = curr->vruntime - se->vruntime;

    pr_info_view_on(stack_depth, "%20s : %llu\n", curr->vruntime);
    pr_info_view_on(stack_depth, "%20s : %llu\n", se->vruntime);
    pr_info_view_on(stack_depth, "%20s : %lld\n", vdiff);

    if (vdiff <= 0)
        return -1;

    gran = wakeup_gran(se);

    pr_info_view_on(stack_depth, "%20s : %lld\n", gran);
    pr_fn_end_on(stack_depth);

    if (vdiff > gran)
        return 1;

    return 0;
}

static void set_last_buddy(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    if (entity_is_task(se) && unlikely(task_has_idle_policy(task_of(se))))
        return;

    for_each_sched_entity(se) {
        if (SCHED_WARN_ON(!se->on_rq))
            return;
        cfs_rq_of(se)->last = se;
    }

    pr_fn_end_on(stack_depth);
}

static void set_next_buddy(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    if (entity_is_task(se) && unlikely(task_has_idle_policy(task_of(se))))
        return;

    for_each_sched_entity(se) {
        if (SCHED_WARN_ON(!se->on_rq))
            return;
        cfs_rq_of(se)->next = se;
    }

    pr_fn_end_on(stack_depth);
}

static void set_skip_buddy(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    for_each_sched_entity(se)
        cfs_rq_of(se)->skip = se;

    pr_fn_end_on(stack_depth);
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_wakeup(struct rq *rq, struct task_struct *p, int wake_flags)
{
    pr_fn_start_on(stack_depth);

    struct task_struct *curr = rq->curr;
    struct sched_entity *se = &curr->se, *pse = &p->se;
    struct cfs_rq *cfs_rq = task_cfs_rq(curr);
    int scale = cfs_rq->nr_running >= sched_nr_latency;
    int next_buddy_marked = 0;

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)pse);

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

    pr_info_view_on(stack_depth, "%20s : %d\n", scale);

    if (sched_feat(NEXT_BUDDY) && scale && !(wake_flags & WF_FORK)) {
        set_next_buddy(pse);
        next_buddy_marked = 1;
    }

    pr_info_view_on(stack_depth, "%20s : %d\n", next_buddy_marked);

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
    int need_resched = test_tsk_need_resched(curr);
    pr_info_view_on(stack_depth, "%20s : %d\n", need_resched);
    if (need_resched)
        return;

    /* Idle tasks are by definition preempted by non-idle tasks. */
    if (unlikely(task_has_idle_policy(curr)) &&
        likely(!task_has_idle_policy(p)))
        goto preempt;

    /*
     * Batch and idle tasks do not preempt non-idle tasks (their preemption
     * is driven by the tick):
     */
    //if (unlikely(p->policy != SCHED_NORMAL) || !sched_feat(WAKEUP_PREEMPTION))
    if (unlikely(p->policy != SCHED_NORMAL))
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

    pr_fn_end_on(stack_depth);

    return;

preempt:
    pr_out_on(stack_depth, "preempt:\n)");
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
    pr_fn_start_on(stack_depth);

    struct cfs_rq *cfs_rq = &rq->cfs;
    struct sched_entity *se;
    struct task_struct *p;
    int new_tasks;

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)&rq->cfs);

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

        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)curr);

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

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)prev);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)p);

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

            pr_info_view_on(stack_depth, "%20s : %d\n", se_depth);
            pr_info_view_on(stack_depth, "%20s : %d\n", pse_depth);

            if (se_depth <= pse_depth) {
                put_prev_entity(cfs_rq_of(pse), pse);
                pse = parent_entity(pse);
            }
            if (se_depth >= pse_depth) {
                set_next_entity(cfs_rq_of(se), se);
                se = parent_entity(se);
            }
        }

        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)pse);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);

        put_prev_entity(cfs_rq, pse);
        set_next_entity(cfs_rq, se);
    }

    goto done;
simple:
#endif

    pr_out_on(stack_depth, "simple:\n");
    if (prev)
        put_prev_task(rq, prev);

    do {
        se = pick_next_entity(cfs_rq, NULL);
        set_next_entity(cfs_rq, se);
        cfs_rq = group_cfs_rq(se);
    } while (cfs_rq);

    p = task_of(se);

done: __maybe_unused;

    pr_out_on(stack_depth, "done:\n");

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

    pr_fn_end_on(stack_depth);

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
    pr_fn_start_on(stack_depth);

    struct sched_entity *se = &prev->se;
    struct cfs_rq *cfs_rq;

    for_each_sched_entity(se) {
        cfs_rq = cfs_rq_of(se);
        put_prev_entity(cfs_rq, se);
    }

    pr_fn_end_on(stack_depth);
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




//7077 lines
static unsigned long __read_mostly max_load_balance_interval = HZ/10;

enum fbq_type { regular, remote, all };

enum group_type {
    group_other = 0,
    group_misfit_task,
    group_imbalanced,
    group_overloaded,
};

#define LBF_ALL_PINNED	0x01
#define LBF_NEED_BREAK	0x02
#define LBF_DST_PINNED  0x04
#define LBF_SOME_PINNED	0x08
#define LBF_NOHZ_STATS	0x10
#define LBF_NOHZ_AGAIN	0x20
//7095
struct lb_env {
    struct sched_domain	*sd;

    struct rq		*src_rq;
    int				src_cpu;

    int				dst_cpu;
    struct rq		*dst_rq;

    struct cpumask		*dst_grpmask;
    int					new_dst_cpu;
    enum cpu_idle_type	idle;
    long				imbalance;
    /* The set of CPUs under consideration for load-balancing */
    struct cpumask		*cpus;

    unsigned int		flags;

    unsigned int		loop;
    unsigned int		loop_break;
    unsigned int		loop_max;

    enum fbq_type		fbq_type;
    enum group_type		src_grp_type;
    struct list_head	tasks;
};
//7122
/*
 * Is this task likely cache-hot:
 */
static int task_hot(struct task_struct *p, struct lb_env *env)
{
    s64 delta;

    lockdep_assert_held(&env->src_rq->lock);

    if (p->sched_class != &fair_sched_class)
        return 0;

    if (unlikely(task_has_idle_policy(p)))
        return 0;

    /*
     * Buddy candidates are cache hot:
     */
    if (sched_feat(CACHE_HOT_BUDDY) && env->dst_rq->nr_running &&
            (&p->se == cfs_rq_of(&p->se)->next ||
             &p->se == cfs_rq_of(&p->se)->last))
        return 1;

    if (sysctl_sched_migration_cost == -1)
        return 1;
    if (sysctl_sched_migration_cost == 0)
        return 0;

    delta = rq_clock_task(env->src_rq) - p->se.exec_start;

    return delta < (s64)sysctl_sched_migration_cost;
}

#ifdef CONFIG_NUMA_BALANCING
/*
 * Returns 1, if task migration degrades locality
 * Returns 0, if task migration improves locality i.e migration preferred.
 * Returns -1, if task migration is not affected by locality.
 */
static int migrate_degrades_locality(struct task_struct *p, struct lb_env *env)
{
    struct numa_group *numa_group = rcu_dereference(p->numa_group);
    unsigned long src_weight, dst_weight;
    int src_nid, dst_nid, dist;

    if (!static_branch_likely(&sched_numa_balancing))
        return -1;

    if (!p->numa_faults || !(env->sd->flags & SD_NUMA))
        return -1;

    src_nid = cpu_to_node(env->src_cpu);
    dst_nid = cpu_to_node(env->dst_cpu);

    if (src_nid == dst_nid)
        return -1;

    /* Migrating away from the preferred node is always bad. */
    if (src_nid == p->numa_preferred_nid) {
        if (env->src_rq->nr_running > env->src_rq->nr_preferred_running)
            return 1;
        else
            return -1;
    }

    /* Encourage migration to the preferred node. */
    if (dst_nid == p->numa_preferred_nid)
        return 0;

    /* Leaving a core idle is often worse than degrading locality. */
    if (env->idle == CPU_IDLE)
        return -1;

    dist = node_distance(src_nid, dst_nid);
    if (numa_group) {
        src_weight = group_weight(p, src_nid, dist);
        dst_weight = group_weight(p, dst_nid, dist);
    } else {
        src_weight = task_weight(p, src_nid, dist);
        dst_weight = task_weight(p, dst_nid, dist);
    }

    return dst_weight < src_weight;
}

#else
static inline int migrate_degrades_locality(struct task_struct *p,
                         struct lb_env *env)
{
    return -1;
}
#endif

/*
 * can_migrate_task - may task p from runqueue rq be migrated to this_cpu?
 */
static
int can_migrate_task(struct task_struct *p, struct lb_env *env)
{
    pr_fn_start_on(stack_depth);

    int tsk_cache_hot;

    lockdep_assert_held(&env->src_rq->lock);

    /*
     * We do not migrate tasks that are:
     * 1) throttled_lb_pair, or
     * 2) cannot be migrated to this CPU due to cpus_ptr, or
     * 3) running (obviously), or
     * 4) are cache-hot on their current CPU.
     */
    if (throttled_lb_pair(task_group(p), env->src_cpu, env->dst_cpu))
        return 0;

    if (!cpumask_test_cpu(env->dst_cpu, p->cpus_ptr)) {
        int cpu;

        schedstat_inc(p->se.statistics.nr_failed_migrations_affine);

        env->flags |= LBF_SOME_PINNED;

        /*
         * Remember if this task can be migrated to any other CPU in
         * our sched_group. We may want to revisit it if we couldn't
         * meet load balance goals by pulling other tasks on src_cpu.
         *
         * Avoid computing new_dst_cpu for NEWLY_IDLE or if we have
         * already computed one in current iteration.
         */
        if (env->idle == CPU_NEWLY_IDLE || (env->flags & LBF_DST_PINNED))
            return 0;

        /* Prevent to re-select dst_cpu via env's CPUs: */
        for_each_cpu_and(cpu, env->dst_grpmask, env->cpus) {
            if (cpumask_test_cpu(cpu, p->cpus_ptr)) {
                env->flags |= LBF_DST_PINNED;
                env->new_dst_cpu = cpu;
                break;
            }
        }

        return 0;
    }

    /* Record that we found atleast one task that could run on dst_cpu */
    env->flags &= ~LBF_ALL_PINNED;

    if (task_running(env->src_rq, p)) {
        schedstat_inc(p->se.statistics.nr_failed_migrations_running);
        return 0;
    }

    /*
     * Aggressive migration if:
     * 1) destination numa is preferred
     * 2) task is cache cold, or
     * 3) too many balance attempts have failed.
     */
    tsk_cache_hot = migrate_degrades_locality(p, env);
    if (tsk_cache_hot == -1)
        tsk_cache_hot = task_hot(p, env);

    if (tsk_cache_hot <= 0 ||
        env->sd->nr_balance_failed > env->sd->cache_nice_tries) {
        if (tsk_cache_hot == 1) {
            schedstat_inc(env->sd->lb_hot_gained[env->idle]);
            schedstat_inc(p->se.statistics.nr_forced_migrations);
        }
        return 1;
    }

    pr_fn_end_on(stack_depth);

    schedstat_inc(p->se.statistics.nr_failed_migrations_hot);
    return 0;
}

/*
 * detach_task() -- detach the task for the migration specified in env
 */
static void detach_task(struct task_struct *p, struct lb_env *env)
{
    lockdep_assert_held(&env->src_rq->lock);

    deactivate_task(env->src_rq, p, DEQUEUE_NOCLOCK);
    set_task_cpu(p, env->dst_cpu);
}

/*
 * detach_one_task() -- tries to dequeue exactly one task from env->src_rq, as
 * part of active balancing operations within "domain".
 *
 * Returns a task if successful and NULL otherwise.
 */
static struct task_struct *detach_one_task(struct lb_env *env)
{
    struct task_struct *p;

    lockdep_assert_held(&env->src_rq->lock);

    list_for_each_entry_reverse(p,
            &env->src_rq->cfs_tasks, se.group_node) {
        if (!can_migrate_task(p, env))
            continue;

        detach_task(p, env);

        /*
         * Right now, this is only the second place where
         * lb_gained[env->idle] is updated (other is detach_tasks)
         * so we can safely collect stats here rather than
         * inside detach_tasks().
         */
        schedstat_inc(env->sd->lb_gained[env->idle]);
        return p;
    }
    return NULL;
}
//7338
static const unsigned int sched_nr_migrate_break = 32;
/*
 * detach_tasks() -- tries to detach up to imbalance runnable load from
 * busiest_rq, as part of a balancing operation within domain "sd".
 *
 * Returns number of detached tasks if successful and 0 otherwise.
 */
static int detach_tasks(struct lb_env *env)
{
    pr_fn_start_on(stack_depth);

    struct list_head *tasks = &env->src_rq->cfs_tasks;
    struct task_struct *p;
    unsigned long load;
    int detached = 0;

    lockdep_assert_held(&env->src_rq->lock);

    if (env->imbalance <= 0)
        return 0;

    pr_info_view_on(stack_depth, "%30s : %u\n", env->loop_max);

    while (!list_empty(tasks)) {
        /*
         * We don't want to steal all, otherwise we may be treated likewise,
         * which could at worst lead to a livelock crash.
         */
        if (env->idle != CPU_NOT_IDLE && env->src_rq->nr_running <= 1)
            break;

        p = list_last_entry(tasks, struct task_struct, se.group_node);

        env->loop++;
        pr_info_view_on(stack_depth, "%30s : %u\n", env->loop);

        /* We've more or less seen every task there is, call it quits */
        if (env->loop > env->loop_max)
            break;

        /* take a breather every nr_migrate tasks */
        if (env->loop > env->loop_break) {
            env->loop_break += sched_nr_migrate_break;
            env->flags |= LBF_NEED_BREAK;
            break;
        }

        if (!can_migrate_task(p, env))
            goto next;

        load = task_h_load(p);

        pr_info_view_on(stack_depth, "%30s : task: %lu\n", load);
        pr_info_view_on(stack_depth, "%30s : task: %lu\n", env->imbalance);

        if (sched_feat(LB_MIN) && load < 16 && !env->sd->nr_balance_failed)
            goto next;

        if ((load / 2) > env->imbalance)
            goto next;

        detach_task(p, env);
        list_add(&p->se.group_node, &env->tasks);

        detached++;
        env->imbalance -= load;

#ifdef CONFIG_PREEMPTION
        /*
         * NEWIDLE balancing is a source of latency, so preemptible
         * kernels will stop after the first task is detached to minimize
         * the critical section.
         */
        if (env->idle == CPU_NEWLY_IDLE)
            break;
#endif

        /*
         * We only want to steal up to the prescribed amount of
         * runnable load.
         */
        if (env->imbalance <= 0)
            break;

        continue;
next:
        list_move(&p->se.group_node, tasks);
    }

    /*
     * Right now, this is one of only two places we collect this stat
     * so we can safely collect detach_one_task() stats here rather
     * than inside detach_one_task().
     */
    schedstat_add(env->sd->lb_gained[env->idle], detached);

    pr_info_view_on(stack_depth, "%30s : %d\n", detached);
    pr_fn_end_on(stack_depth);

    return detached;
}

/*
 * attach_task() -- attach the task detached by detach_task() to its new rq.
 */
static void attach_task(struct rq *rq, struct task_struct *p)
{
    lockdep_assert_held(&rq->lock);

    BUG_ON(task_rq(p) != rq);
    activate_task(rq, p, ENQUEUE_NOCLOCK);
    check_preempt_curr(rq, p, 0);
}

/*
 * attach_one_task() -- attaches the task returned from detach_one_task() to
 * its new rq.
 */
static void attach_one_task(struct rq *rq, struct task_struct *p)
{
    struct rq_flags rf;

    rq_lock(rq, &rf);
    update_rq_clock(rq);
    attach_task(rq, p);
    rq_unlock(rq, &rf);
}

/*
 * attach_tasks() -- attaches all tasks detached by detach_tasks() to their
 * new rq.
 */
static void attach_tasks(struct lb_env *env)
{
    pr_fn_start_on(stack_depth);

    struct list_head *tasks = &env->tasks;
    struct task_struct *p;
    struct rq_flags rf;

    rq_lock(env->dst_rq, &rf);
    update_rq_clock(env->dst_rq);

    while (!list_empty(tasks)) {
        p = list_first_entry(tasks, struct task_struct, se.group_node);
        list_del_init(&p->se.group_node);

        attach_task(env->dst_rq, p);
    }

    rq_unlock(env->dst_rq, &rf);

    pr_fn_end_on(stack_depth);
}
//7478
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
    pr_fn_start_on(stack_depth);

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

    pr_fn_end_on(stack_depth);
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
//7673
/*
 * sg_lb_stats - stats of a sched_group required for load_balancing
 */
struct sg_lb_stats {
    unsigned long avg_load; /*Avg load across the CPUs of the group */
    unsigned long group_load; /* Total load over the CPUs of the group */
    unsigned long load_per_task;
    unsigned long group_capacity;
    unsigned long group_util; /* Total utilization of the group */
    unsigned int sum_nr_running; /* Nr tasks running in the group */
    unsigned int idle_cpus;
    unsigned int group_weight;
    enum group_type group_type;
    int group_no_capacity;
    unsigned long group_misfit_task_load; /* A CPU has a task too big for its capacity */
#ifdef CONFIG_NUMA_BALANCING
    unsigned int nr_numa_running;
    unsigned int nr_preferred_running;
#endif
};

/*
 * sd_lb_stats - Structure to store the statistics of a sched_domain
 *		 during load balancing.
 */
struct sd_lb_stats {
    struct sched_group *busiest;	/* Busiest group in this sd */
    struct sched_group *local;	/* Local group in this sd */
    unsigned long total_running;
    unsigned long total_load;	/* Total load of all groups in sd */
    unsigned long total_capacity;	/* Total capacity of all groups in sd */
    unsigned long avg_load;	/* Average load across all groups in sd */

    struct sg_lb_stats busiest_stat;/* Statistics of the busiest group */
    struct sg_lb_stats local_stat;	/* Statistics of the local group */
};
//7710
static inline void init_sd_lb_stats(struct sd_lb_stats *sds)
{
    /*
     * Skimp on the clearing to avoid duplicate work. We can avoid clearing
     * local_stat because update_sg_lb_stats() does a full clear/assignment.
     * We must however clear busiest_stat::avg_load because
     * update_sd_pick_busiest() reads this before assignment.
     */
    *sds = (struct sd_lb_stats){
        .busiest = NULL,
        .local = NULL,
        .total_running = 0UL,
        .total_load = 0UL,
        .total_capacity = 0UL,
        .busiest_stat = {
            .avg_load = 0UL,
            .sum_nr_running = 0,
            .group_type = group_other,
        },
    };
}
//7732
static unsigned long scale_rt_capacity(struct sched_domain *sd, int cpu)
{
    struct rq *rq = cpu_rq(cpu);
    unsigned long max = arch_scale_cpu_capacity(cpu);
    unsigned long used, free;
    unsigned long irq;

    irq = cpu_util_irq(rq);

    if (unlikely(irq >= max))
        return 1;

    used = READ_ONCE(rq->avg_rt.util_avg);
    used += READ_ONCE(rq->avg_dl.util_avg);

    if (unlikely(used >= max))
        return 1;

    free = max - used;

    return scale_irq_capacity(free, irq, max);
}

static void update_cpu_capacity(struct sched_domain *sd, int cpu)
{
    pr_fn_start_on(stack_depth);

    unsigned long capacity = scale_rt_capacity(sd, cpu);
    struct sched_group *sdg = sd->groups;

    pr_info_view_on(stack_depth, "%20s : %d\n", cpu);

    cpu_rq(cpu)->cpu_capacity_orig = arch_scale_cpu_capacity(cpu);

    if (!capacity)
        capacity = 1;

    cpu_rq(cpu)->cpu_capacity = capacity;
    sdg->sgc->capacity = capacity;
    sdg->sgc->min_capacity = capacity;
    sdg->sgc->max_capacity = capacity;

    pr_info_view_on(stack_depth, "%20s : %lu\n", capacity);
    pr_fn_end_on(stack_depth);
}

void update_group_capacity(struct sched_domain *sd, int cpu)
{
    pr_fn_start_on(stack_depth);

    struct sched_domain *child = sd->child;
    struct sched_group *group, *sdg = sd->groups;
    unsigned long capacity, min_capacity, max_capacity;
    unsigned long interval;

    interval = msecs_to_jiffies(sd->balance_interval);
    interval = clamp(interval, 1UL, max_load_balance_interval);
    sdg->sgc->next_update = jiffies + interval;

    pr_info_view_on(stack_depth, "%30s : 0x%X\n", sd->groups->cpumask[0]);

    if (!child) {
        update_cpu_capacity(sd, cpu);
        return;
    }

    capacity = 0;
    min_capacity = ULONG_MAX;
    max_capacity = 0;

    if (child->flags & SD_OVERLAP) {
        /*
         * SD_OVERLAP domains cannot assume that child groups
         * span the current group.
         */

        for_each_cpu(cpu, sched_group_span(sdg)) {
            struct sched_group_capacity *sgc;
            struct rq *rq = cpu_rq(cpu);

            pr_info_view_on(stack_depth, "%30s : %d\n", cpu);

            /*
             * build_sched_domains() -> init_sched_groups_capacity()
             * gets here before we've attached the domains to the
             * runqueues.
             *
             * Use capacity_of(), which is set irrespective of domains
             * in update_cpu_capacity().
             *
             * This avoids capacity from being 0 and
             * causing divide-by-zero issues on boot.
             */
            if (unlikely(!rq->sd)) {
                capacity += capacity_of(cpu);
            } else {
                sgc = rq->sd->groups->sgc;
                capacity += sgc->capacity;
            }

            min_capacity = min(capacity, min_capacity);
            max_capacity = max(capacity, max_capacity);
        }
    } else  {
        /*
         * !SD_OVERLAP domains can assume that child groups
         * span the current group.
         */

        group = child->groups;
        pr_info_view_on(stack_depth, "%30s : 0x%X\n", child->groups->cpumask[0]);

        do {
            struct sched_group_capacity *sgc = group->sgc;

            capacity += sgc->capacity;
            min_capacity = min(sgc->min_capacity, min_capacity);
            max_capacity = max(sgc->max_capacity, max_capacity);
            group = group->next;
        } while (group != child->groups);
    }

    sdg->sgc->capacity = capacity;
    sdg->sgc->min_capacity = min_capacity;
    sdg->sgc->max_capacity = max_capacity;

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)sdg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sdg->sgc->capacity);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sdg->sgc->min_capacity);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sdg->sgc->max_capacity);

    pr_fn_end_on(stack_depth);
}
//7844
/*
 * Check whether the capacity of the rq has been noticeably reduced by side
 * activity. The imbalance_pct is used for the threshold.
 * Return true is the capacity is reduced
 */
static inline int
check_cpu_capacity(struct rq *rq, struct sched_domain *sd)
{
    return ((rq->cpu_capacity * sd->imbalance_pct) <
                (rq->cpu_capacity_orig * 100));
}
//7856
/*
 * Check whether a rq has a misfit task and if it looks like we can actually
 * help that task: we can migrate the task to a CPU of higher capacity, or
 * the task's current CPU is heavily pressured.
 */
static inline int check_misfit_status(struct rq *rq, struct sched_domain *sd)
{
    return rq->misfit_task_load &&
        (rq->cpu_capacity_orig < rq->rd->max_cpu_capacity ||
         check_cpu_capacity(rq, sd));
}

/*
 * Group imbalance indicates (and tries to solve) the problem where balancing
 * groups is inadequate due to ->cpus_ptr constraints.
 *
 * Imagine a situation of two groups of 4 CPUs each and 4 tasks each with a
 * cpumask covering 1 CPU of the first group and 3 CPUs of the second group.
 * Something like:
 *
 *	{ 0 1 2 3 } { 4 5 6 7 }
 *	        *     * * *
 *
 * If we were to balance group-wise we'd place two tasks in the first group and
 * two tasks in the second group. Clearly this is undesired as it will overload
 * cpu 3 and leave one of the CPUs in the second group unused.
 *
 * The current solution to this issue is detecting the skew in the first group
 * by noticing the lower domain failed to reach balance and had difficulty
 * moving tasks due to affinity constraints.
 *
 * When this is so detected; this group becomes a candidate for busiest; see
 * update_sd_pick_busiest(). And calculate_imbalance() and
 * find_busiest_group() avoid some of the usual balance conditions to allow it
 * to create an effective group imbalance.
 *
 * This is a somewhat tricky proposition since the next run might not find the
 * group imbalance and decide the groups need to be balanced again. A most
 * subtle and fragile situation.
 */

static inline int sg_imbalanced(struct sched_group *group)
{
    return group->sgc->imbalance;
}

/*
 * group_has_capacity returns true if the group has spare capacity that could
 * be used by some tasks.
 * We consider that a group has spare capacity if the  * number of task is
 * smaller than the number of CPUs or if the utilization is lower than the
 * available capacity for CFS tasks.
 * For the latter, we use a threshold to stabilize the state, to take into
 * account the variance of the tasks' load and to return true if the available
 * capacity in meaningful for the load balancer.
 * As an example, an available capacity of 1% can appear but it doesn't make
 * any benefit for the load balance.
 */
static inline bool
group_has_capacity(struct lb_env *env, struct sg_lb_stats *sgs)
{
    if (sgs->sum_nr_running < sgs->group_weight)
        return true;

    if ((sgs->group_capacity * 100) >
            (sgs->group_util * env->sd->imbalance_pct))
        return true;

    return false;
}

/*
 *  group_is_overloaded returns true if the group has more tasks than it can
 *  handle.
 *  group_is_overloaded is not equals to !group_has_capacity because a group
 *  with the exact right number of tasks, has no more spare capacity but is not
 *  overloaded so both group_has_capacity and group_is_overloaded return
 *  false.
 */
static inline bool
group_is_overloaded(struct lb_env *env, struct sg_lb_stats *sgs)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%30s : %u\n", sgs->sum_nr_running);
    pr_info_view_on(stack_depth, "%30s : %u\n", sgs->group_weight);

    if (sgs->sum_nr_running <= sgs->group_weight)
        return false;

    pr_info_view_on(stack_depth, "%30s : %lu\n", sgs->group_capacity);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sgs->group_util);
    pr_info_view_on(stack_depth, "%30s : %u\n", env->sd->imbalance_pct);

    pr_fn_end_on(stack_depth);

    if ((sgs->group_capacity * 100) <
            (sgs->group_util * env->sd->imbalance_pct))
        return true;

    return false;
}

/*
 * group_smaller_min_cpu_capacity: Returns true if sched_group sg has smaller
 * per-CPU capacity than sched_group ref.
 */
static inline bool
group_smaller_min_cpu_capacity(struct sched_group *sg, struct sched_group *ref)
{
    return fits_capacity(sg->sgc->min_capacity, ref->sgc->min_capacity);
}

/*
 * group_smaller_max_cpu_capacity: Returns true if sched_group sg has smaller
 * per-CPU capacity_orig than sched_group ref.
 */
static inline bool
group_smaller_max_cpu_capacity(struct sched_group *sg, struct sched_group *ref)
{
    return fits_capacity(sg->sgc->max_capacity, ref->sgc->max_capacity);
}

static inline enum
group_type group_classify(struct sched_group *group,
              struct sg_lb_stats *sgs)
{
    if (sgs->group_no_capacity)
        return group_overloaded;

    if (sg_imbalanced(group))
        return group_imbalanced;

    if (sgs->group_misfit_task_load)
        return group_misfit_task;

    return group_other;
}

static bool update_nohz_stats(struct rq *rq, bool force)
{
#ifdef CONFIG_NO_HZ_COMMON
    unsigned int cpu = rq->cpu;

    if (!rq->has_blocked_load)
        return false;

    if (!cpumask_test_cpu(cpu, nohz.idle_cpus_mask))
        return false;

    if (!force && !time_after(jiffies, rq->last_blocked_load_update_tick))
        return true;

    update_blocked_averages(cpu);

    return rq->has_blocked_load;
#else
    return false;
#endif
}

/**
 * update_sg_lb_stats - Update sched_group's statistics for load balancing.
 * @env: The load balancing environment.
 * @group: sched_group whose statistics are to be updated.
 * @sgs: variable to hold the statistics for this group.
 * @sg_status: Holds flag indicating the status of the sched_group
 */
static inline void update_sg_lb_stats(struct lb_env *env,
                      struct sched_group *group,
                      struct sg_lb_stats *sgs,
                      int *sg_status)
{
    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%20s : 0x%X\n", group->cpumask[0]);

    int i, nr_running;

    memset(sgs, 0, sizeof(*sgs));

    for_each_cpu_and(i, sched_group_span(group), env->cpus) {
        struct rq *rq = cpu_rq(i);

        pr_info_view_on(stack_depth, "%20s : cpu: %d\n", i);

        if ((env->flags & LBF_NOHZ_STATS) && update_nohz_stats(rq, false))
            env->flags |= LBF_NOHZ_AGAIN;

        sgs->group_load += cpu_runnable_load(rq);
        sgs->group_util += cpu_util(i);
        sgs->sum_nr_running += rq->cfs.h_nr_running;

        nr_running = rq->nr_running;
        if (nr_running > 1)
            *sg_status |= SG_OVERLOAD;

        if (cpu_overutilized(i))
            *sg_status |= SG_OVERUTILIZED;

#ifdef CONFIG_NUMA_BALANCING
        sgs->nr_numa_running += rq->nr_numa_running;
        sgs->nr_preferred_running += rq->nr_preferred_running;
#endif
        /*
         * No need to call idle_cpu() if nr_running is not 0
         */
        if (!nr_running && idle_cpu(i))
            sgs->idle_cpus++;

        if (env->sd->flags & SD_ASYM_CPUCAPACITY &&
            sgs->group_misfit_task_load < rq->misfit_task_load) {
            sgs->group_misfit_task_load = rq->misfit_task_load;
            *sg_status |= SG_OVERLOAD;
        }
    }

    /* Adjust by relative CPU capacity of the group */
    sgs->group_capacity = group->sgc->capacity;
    sgs->avg_load = (sgs->group_load*SCHED_CAPACITY_SCALE) / sgs->group_capacity;

    if (sgs->sum_nr_running)
        sgs->load_per_task = sgs->group_load / sgs->sum_nr_running;

    sgs->group_weight = group->group_weight;

    sgs->group_no_capacity = group_is_overloaded(env, sgs);
    sgs->group_type = group_classify(group, sgs);

    pr_info_view_on(stack_depth, "%30s : %lu\n", sgs->avg_load);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sgs->group_load);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sgs->load_per_task);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sgs->group_capacity);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sgs->group_util);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sgs->group_misfit_task_load);
    pr_info_view_on(stack_depth, "%30s : %u\n", sgs->sum_nr_running);
    pr_info_view_on(stack_depth, "%30s : %u\n", sgs->idle_cpus);
    pr_info_view_on(stack_depth, "%30s : %u\n", sgs->group_weight);
    pr_info_view_on(stack_depth, "%30s : %u\n", sgs->group_no_capacity);
    pr_info_view_on(stack_depth, "%30s : %d\n", sgs->group_type);

    pr_fn_end_on(stack_depth);
}

/**
 * update_sd_pick_busiest - return 1 on busiest group
 * @env: The load balancing environment.
 * @sds: sched_domain statistics
 * @sg: sched_group candidate to be checked for being the busiest
 * @sgs: sched_group statistics
 *
 * Determine if @sg is a busier group than the previously selected
 * busiest group.
 *
 * Return: %true if @sg is a busier group than the previously selected
 * busiest group. %false otherwise.
 */
static bool update_sd_pick_busiest(struct lb_env *env,
                   struct sd_lb_stats *sds,
                   struct sched_group *sg,
                   struct sg_lb_stats *sgs)
{
    pr_fn_start_on(stack_depth);

    struct sg_lb_stats *busiest = &sds->busiest_stat;

    pr_info_view_on(stack_depth, "%30s : %d\n", sgs->group_type);
    pr_info_view_on(stack_depth, "%30s : %d\n", busiest->group_type);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sgs->avg_load);
    pr_info_view_on(stack_depth, "%30s : %lu\n", busiest->avg_load);

    /*
     * Don't try to pull misfit tasks we can't help.
     * We can use max_capacity here as reduction in capacity on some
     * CPUs in the group should either be possible to resolve
     * internally or be covered by avg_load imbalance (eventually).
     */
    if (sgs->group_type == group_misfit_task &&
        (!group_smaller_max_cpu_capacity(sg, sds->local) ||
         !group_has_capacity(env, &sds->local_stat)))
        return false;

    if (sgs->group_type > busiest->group_type)
        return true;

    if (sgs->group_type < busiest->group_type)
        return false;

    if (sgs->avg_load <= busiest->avg_load)
        return false;

    if (!(env->sd->flags & SD_ASYM_CPUCAPACITY))
        goto asym_packing;

    /*
     * Candidate sg has no more than one task per CPU and
     * has higher per-CPU capacity. Migrating tasks to less
     * capable CPUs may harm throughput. Maximize throughput,
     * power/energy consequences are not considered.
     */
    if (sgs->sum_nr_running <= sgs->group_weight &&
        group_smaller_min_cpu_capacity(sds->local, sg))
        return false;

    /*
     * If we have more than one misfit sg go with the biggest misfit.
     */
    if (sgs->group_type == group_misfit_task &&
        sgs->group_misfit_task_load < busiest->group_misfit_task_load)
        return false;

asym_packing:
    pr_out_on(stack_depth, "asym_packing:\n");

    /* This is the busiest node in its class. */
    if (!(env->sd->flags & SD_ASYM_PACKING))
        return true;

    /* No ASYM_PACKING if target CPU is already busy */
    if (env->idle == CPU_NOT_IDLE)
        return true;
    /*
     * ASYM_PACKING needs to move all the work to the highest
     * prority CPUs in the group, therefore mark all groups
     * of lower priority than ourself as busy.
     */
    if (sgs->sum_nr_running &&
        sched_asym_prefer(env->dst_cpu, sg->asym_prefer_cpu)) {
        if (!sds->busiest)
            return true;

        /* Prefer to move from lowest priority CPU's work */
        if (sched_asym_prefer(sds->busiest->asym_prefer_cpu,
                      sg->asym_prefer_cpu))
            return true;
    }

    pr_fn_end_on(stack_depth);
    return false;
}

#ifdef CONFIG_NUMA_BALANCING
static inline enum fbq_type fbq_classify_group(struct sg_lb_stats *sgs)
{
    if (sgs->sum_nr_running > sgs->nr_numa_running)
        return regular;
    if (sgs->sum_nr_running > sgs->nr_preferred_running)
        return remote;
    return all;
}

static inline enum fbq_type fbq_classify_rq(struct rq *rq)
{
    if (rq->nr_running > rq->nr_numa_running)
        return regular;
    if (rq->nr_running > rq->nr_preferred_running)
        return remote;
    return all;
}
#else
static inline enum fbq_type fbq_classify_group(struct sg_lb_stats *sgs)
{
    return all;
}

static inline enum fbq_type fbq_classify_rq(struct rq *rq)
{
    return regular;
}
#endif /* CONFIG_NUMA_BALANCING */
//8186
/**
 * update_sd_lb_stats - Update sched_domain's statistics for load balancing.
 * @env: The load balancing environment.
 * @sds: variable to hold the statistics for this sched_domain.
 */
static inline void update_sd_lb_stats(struct lb_env *env, struct sd_lb_stats *sds)
{
    pr_fn_start_on(stack_depth);

    struct sched_domain *child = env->sd->child;
    struct sched_group *sg = env->sd->groups;
    struct sg_lb_stats *local = &sds->local_stat;
    struct sg_lb_stats tmp_sgs;
    bool prefer_sibling = child && child->flags & SD_PREFER_SIBLING;
    int sg_status = 0;

#ifdef CONFIG_NO_HZ_COMMON
    if (env->idle == CPU_NEWLY_IDLE && READ_ONCE(nohz.has_blocked))
        env->flags |= LBF_NOHZ_STATS;
#endif

    do {
        struct sg_lb_stats *sgs = &tmp_sgs;
        int local_group;

        pr_info_view_on(stack_depth, "%20s : 0x%X\n", sg->cpumask[0]);
        local_group = cpumask_test_cpu(env->dst_cpu, sched_group_span(sg));
        pr_info_view_on(stack_depth, "%20s : %d\n", local_group);

        if (local_group) {
            sds->local = sg;
            sgs = local;

            if (env->idle != CPU_NEWLY_IDLE ||
                time_after_eq(jiffies, sg->sgc->next_update))
                update_group_capacity(env->sd, env->dst_cpu);
        }

        update_sg_lb_stats(env, sg, sgs, &sg_status);

        if (local_group)
            goto next_group;

        /*
         * In case the child domain prefers tasks go to siblings
         * first, lower the sg capacity so that we'll try
         * and move all the excess tasks away. We lower the capacity
         * of a group only if the local group has the capacity to fit
         * these excess tasks. The extra check prevents the case where
         * you always pull from the heaviest group when it is already
         * under-utilized (possible with a large weight task outweighs
         * the tasks on the system).
         */
        if (prefer_sibling && sds->local &&
            group_has_capacity(env, local) &&
            (sgs->sum_nr_running > local->sum_nr_running + 1)) {
            sgs->group_no_capacity = 1;
            sgs->group_type = group_classify(sg, sgs);
            pr_info_view_on(stack_depth, "%20s : %d\n", sgs->group_type);
        }

        if (update_sd_pick_busiest(env, sds, sg, sgs)) {
            sds->busiest = sg;
            sds->busiest_stat = *sgs;
            pr_info_view_on(stack_depth, "%20s : %p\n", (void*)sds->busiest);
        }

next_group:
        pr_out_on(stack_depth, "next_group:\n");

        /* Now, start updating sd_lb_stats */
        sds->total_running += sgs->sum_nr_running;
        sds->total_load += sgs->group_load;
        sds->total_capacity += sgs->group_capacity;

        sg = sg->next;
    } while (sg != env->sd->groups);

#ifdef CONFIG_NO_HZ_COMMON
    if ((env->flags & LBF_NOHZ_AGAIN) &&
        cpumask_subset(nohz.idle_cpus_mask, sched_domain_span(env->sd))) {

        WRITE_ONCE(nohz.next_blocked,
               jiffies + msecs_to_jiffies(LOAD_AVG_PERIOD));
    }
#endif

    if (env->sd->flags & SD_NUMA)
        env->fbq_type = fbq_classify_group(&sds->busiest_stat);

    if (!env->sd->parent) {
        struct root_domain *rd = env->dst_rq->rd;

        /* update overload indicator if we are at root domain */
        WRITE_ONCE(rd->overload, sg_status & SG_OVERLOAD);

        /* Update over-utilization (tipping point, U >= 0) indicator */
        WRITE_ONCE(rd->overutilized, sg_status & SG_OVERUTILIZED);
        //trace_sched_overutilized_tp(rd, sg_status & SG_OVERUTILIZED);
    } else if (sg_status & SG_OVERUTILIZED) {
        struct root_domain *rd = env->dst_rq->rd;

        WRITE_ONCE(rd->overutilized, SG_OVERUTILIZED);
        //trace_sched_overutilized_tp(rd, SG_OVERUTILIZED);
    }

    pr_info_view_on(stack_depth, "%30s : %s\n", env->sd->name);
    pr_info_view_on(stack_depth, "%30s : 0x%X\n", env->sd->span[0]);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sds->total_running);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sds->total_load);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sds->total_capacity);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sds->avg_load);

    pr_fn_end_on(stack_depth);
}

/**
 * check_asym_packing - Check to see if the group is packed into the
 *			sched domain.
 *
 * This is primarily intended to used at the sibling level.  Some
 * cores like POWER7 prefer to use lower numbered SMT threads.  In the
 * case of POWER7, it can move to lower SMT modes only when higher
 * threads are idle.  When in lower SMT modes, the threads will
 * perform better since they share less core resources.  Hence when we
 * have idle threads, we want them to be the higher ones.
 *
 * This packing function is run on idle threads.  It checks to see if
 * the busiest CPU in this domain (core in the P7 case) has a higher
 * CPU number than the packing function is being run on.  Here we are
 * assuming lower CPU number will be equivalent to lower a SMT thread
 * number.
 *
 * Return: 1 when packing is required and a task should be moved to
 * this CPU.  The amount of the imbalance is returned in env->imbalance.
 *
 * @env: The load balancing environment.
 * @sds: Statistics of the sched_domain which is to be packed
 */
static int check_asym_packing(struct lb_env *env, struct sd_lb_stats *sds)
{
    int busiest_cpu;

    if (!(env->sd->flags & SD_ASYM_PACKING))
        return 0;

    if (env->idle == CPU_NOT_IDLE)
        return 0;

    if (!sds->busiest)
        return 0;

    busiest_cpu = sds->busiest->asym_prefer_cpu;
    if (sched_asym_prefer(busiest_cpu, env->dst_cpu))
        return 0;

    env->imbalance = sds->busiest_stat.group_load;

    return 1;
}

/**
 * fix_small_imbalance - Calculate the minor imbalance that exists
 *			amongst the groups of a sched_domain, during
 *			load balancing.
 * @env: The load balancing environment.
 * @sds: Statistics of the sched_domain whose imbalance is to be calculated.
 */
static inline
void fix_small_imbalance(struct lb_env *env, struct sd_lb_stats *sds)
{
    pr_fn_start_on(stack_depth);

    unsigned long tmp, capa_now = 0, capa_move = 0;
    unsigned int imbn = 2;
    unsigned long scaled_busy_load_per_task;
    struct sg_lb_stats *local, *busiest;

    local = &sds->local_stat;
    busiest = &sds->busiest_stat;

    if (!local->sum_nr_running)
        local->load_per_task = cpu_avg_load_per_task(env->dst_cpu);
    else if (busiest->load_per_task > local->load_per_task)
        imbn = 1;

    scaled_busy_load_per_task =
        (busiest->load_per_task * SCHED_CAPACITY_SCALE) /
        busiest->group_capacity;

    if (busiest->avg_load + scaled_busy_load_per_task >=
        local->avg_load + (scaled_busy_load_per_task * imbn)) {
        env->imbalance = busiest->load_per_task;
        return;
    }

    /*
     * OK, we don't have enough imbalance to justify moving tasks,
     * however we may be able to increase total CPU capacity used by
     * moving them.
     */

    capa_now += busiest->group_capacity *
            min(busiest->load_per_task, busiest->avg_load);
    capa_now += local->group_capacity *
            min(local->load_per_task, local->avg_load);
    capa_now /= SCHED_CAPACITY_SCALE;

    /* Amount of load we'd subtract */
    if (busiest->avg_load > scaled_busy_load_per_task) {
        capa_move += busiest->group_capacity *
                min(busiest->load_per_task,
                busiest->avg_load - scaled_busy_load_per_task);
    }

    /* Amount of load we'd add */
    if (busiest->avg_load * busiest->group_capacity <
        busiest->load_per_task * SCHED_CAPACITY_SCALE) {
        tmp = (busiest->avg_load * busiest->group_capacity) /
              local->group_capacity;
    } else {
        tmp = (busiest->load_per_task * SCHED_CAPACITY_SCALE) /
              local->group_capacity;
    }
    capa_move += local->group_capacity *
            min(local->load_per_task, local->avg_load + tmp);
    capa_move /= SCHED_CAPACITY_SCALE;

    /* Move if we gain throughput */
    if (capa_move > capa_now)
        env->imbalance = busiest->load_per_task;

    pr_info_view_on(stack_depth, "%30s : %lu\n", capa_now);
    pr_info_view_on(stack_depth, "%30s : %lu\n", capa_move);
    pr_info_view_on(stack_depth, "%30s : %lu\n", busiest->load_per_task);
    pr_info_view_on(stack_depth, "%30s : %ld\n", env->imbalance);

    pr_fn_end_on(stack_depth);
}

/**
 * calculate_imbalance - Calculate the amount of imbalance present within the
 *			 groups of a given sched_domain during load balance.
 * @env: load balance environment
 * @sds: statistics of the sched_domain whose imbalance is to be calculated.
 */
static inline void calculate_imbalance(struct lb_env *env, struct sd_lb_stats *sds)
{
    pr_fn_start_on(stack_depth);

    unsigned long max_pull, load_above_capacity = ~0UL;
    struct sg_lb_stats *local, *busiest;

    local = &sds->local_stat;
    busiest = &sds->busiest_stat;

    pr_info_view_on(stack_depth, "%30s : %d\n", local->group_type);
    pr_info_view_on(stack_depth, "%30s : %d\n", busiest->group_type);

    if (busiest->group_type == group_imbalanced) {
        /*
         * In the group_imb case we cannot rely on group-wide averages
         * to ensure CPU-load equilibrium, look at wider averages. XXX
         */
        busiest->load_per_task =
            min(busiest->load_per_task, sds->avg_load);
    }

    /*
     * Avg load of busiest sg can be less and avg load of local sg can
     * be greater than avg load across all sgs of sd because avg load
     * factors in sg capacity and sgs with smaller group_type are
     * skipped when updating the busiest sg:
     */
    if (busiest->group_type != group_misfit_task &&
        (busiest->avg_load <= sds->avg_load ||
         local->avg_load >= sds->avg_load)) {
        env->imbalance = 0;
        return fix_small_imbalance(env, sds);
    }

    /*
     * If there aren't any idle CPUs, avoid creating some.
     */
    if (busiest->group_type == group_overloaded &&
        local->group_type   == group_overloaded) {
        load_above_capacity = busiest->sum_nr_running * SCHED_CAPACITY_SCALE;
        if (load_above_capacity > busiest->group_capacity) {
            load_above_capacity -= busiest->group_capacity;
            load_above_capacity *= scale_load_down(NICE_0_LOAD);
            load_above_capacity /= busiest->group_capacity;
        } else
            load_above_capacity = ~0UL;
    }

    /*
     * We're trying to get all the CPUs to the average_load, so we don't
     * want to push ourselves above the average load, nor do we wish to
     * reduce the max loaded CPU below the average load. At the same time,
     * we also don't want to reduce the group load below the group
     * capacity. Thus we look for the minimum possible imbalance.
     */
    max_pull = min(busiest->avg_load - sds->avg_load, load_above_capacity);

    /* How much load to actually move to equalise the imbalance */
    env->imbalance = min(
        max_pull * busiest->group_capacity,
        (sds->avg_load - local->avg_load) * local->group_capacity
    ) / SCHED_CAPACITY_SCALE;

    /* Boost imbalance to allow misfit task to be balanced. */
    if (busiest->group_type == group_misfit_task) {
        env->imbalance = max_t(long, env->imbalance,
                       busiest->group_misfit_task_load);
    }

    pr_info_view_on(stack_depth, "%30s : %ld\n", env->imbalance);
    pr_info_view_on(stack_depth, "%30s : %lu\n", busiest->load_per_task);
    pr_fn_end_on(stack_depth);

    /*
     * if *imbalance is less than the average load per runnable task
     * there is no guarantee that any tasks will be moved so we'll have
     * a think about bumping its value to force at least one task to be
     * moved
     */
    if (env->imbalance < busiest->load_per_task)
        return fix_small_imbalance(env, sds);
}
//8480
/******* find_busiest_group() helpers end here *********************/

/**
 * find_busiest_group - Returns the busiest group within the sched_domain
 * if there is an imbalance.
 *
 * Also calculates the amount of runnable load which should be moved
 * to restore balance.
 *
 * @env: The load balancing environment.
 *
 * Return:	- The busiest group if imbalance exists.
 */
static struct sched_group *find_busiest_group(struct lb_env *env)
{
    pr_fn_start_on(stack_depth);

    struct sg_lb_stats *local, *busiest;
    struct sd_lb_stats sds;

    init_sd_lb_stats(&sds);

    /*
     * Compute the various statistics relavent for load balancing at
     * this level.
     */
    update_sd_lb_stats(env, &sds);

    if (sched_energy_enabled()) {
        struct root_domain *rd = env->dst_rq->rd;

        if (rcu_dereference(rd->pd) && !READ_ONCE(rd->overutilized))
            goto out_balanced;
    }

    local = &sds.local_stat;
    busiest = &sds.busiest_stat;

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)sds.busiest);
    pr_info_view_on(stack_depth, "%30s : %u\n", busiest->sum_nr_running);

    /* ASYM feature bypasses nice load balance check */
    if (check_asym_packing(env, &sds))
        return sds.busiest;

    /* There is no busy sibling group to pull tasks from */
    if (!sds.busiest || busiest->sum_nr_running == 0)
        goto out_balanced;

    /* XXX broken for overlapping NUMA groups */
    sds.avg_load = (SCHED_CAPACITY_SCALE * sds.total_load)
                        / sds.total_capacity;

    pr_info_view_on(stack_depth, "%30s : %lu\n", sds.total_load);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sds.total_capacity);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sds.avg_load);
    pr_info_view_on(stack_depth, "%30s : %lu\n", local->avg_load);
    pr_info_view_on(stack_depth, "%30s : %lu\n", busiest->avg_load);
    pr_info_view_on(stack_depth, "%30s : %d\n", busiest->group_type);

    /*
     * If the busiest group is imbalanced the below checks don't
     * work because they assume all things are equal, which typically
     * isn't true due to cpus_ptr constraints and the like.
     */
    if (busiest->group_type == group_imbalanced)
        goto force_balance;

    /*
     * When dst_cpu is idle, prevent SMP nice and/or asymmetric group
     * capacities from resulting in underutilization due to avg_load.
     */
    if (env->idle != CPU_NOT_IDLE && group_has_capacity(env, local) &&
        busiest->group_no_capacity)
        goto force_balance;

    /* Misfit tasks should be dealt with regardless of the avg load */
    if (busiest->group_type == group_misfit_task)
        goto force_balance;

    /*
     * If the local group is busier than the selected busiest group
     * don't try and pull any tasks.
     */
    if (local->avg_load >= busiest->avg_load)
        goto out_balanced;

    /*
     * Don't pull any tasks if this group is already above the domain
     * average load.
     */
    if (local->avg_load >= sds.avg_load)
        goto out_balanced;

    if (env->idle == CPU_IDLE) {
        /*
         * This CPU is idle. If the busiest group is not overloaded
         * and there is no imbalance between this and busiest group
         * wrt idle CPUs, it is balanced. The imbalance becomes
         * significant if the diff is greater than 1 otherwise we
         * might end up to just move the imbalance on another group
         */
        if ((busiest->group_type != group_overloaded) &&
                (local->idle_cpus <= (busiest->idle_cpus + 1)))
            goto out_balanced;
    } else {
        /*
         * In the CPU_NEWLY_IDLE, CPU_NOT_IDLE cases, use
         * imbalance_pct to be conservative.
         */
        if (100 * busiest->avg_load <=
                env->sd->imbalance_pct * local->avg_load)
            goto out_balanced;
    }

force_balance:
    pr_out_on(stack_depth, "force_balance:\n");

    /* Looks like there is an imbalance. Compute it */
    env->src_grp_type = busiest->group_type;
    calculate_imbalance(env, &sds);

    pr_info_view_on(stack_depth, "%30s : %ld\n", env->imbalance);
    pr_info_view_on(stack_depth, "%30s : %d\n", env->src_grp_type);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)sds.busiest);
    pr_fn_end_on(stack_depth);

    return env->imbalance ? sds.busiest : NULL;

out_balanced:
    pr_out_on(stack_depth, "out_balanced:\n");

    env->imbalance = 0;

    pr_info_view_on(stack_depth, "%30s : %ld\n", env->imbalance);
    pr_fn_end_on(stack_depth);

    return NULL;
}

/*
 * find_busiest_queue - find the busiest runqueue among the CPUs in the group.
 */
static struct rq *find_busiest_queue(struct lb_env *env,
                     struct sched_group *group)
{
    pr_fn_start_on(stack_depth);

    struct rq *busiest = NULL, *rq;
    unsigned long busiest_load = 0, busiest_capacity = 1;
    int i;

    pr_info_view_on(stack_depth, "%30s : 0x%X\n", group->cpumask[0]);

    for_each_cpu_and(i, sched_group_span(group), env->cpus) {
        unsigned long capacity, load;
        enum fbq_type rt;

        rq = cpu_rq(i);

        pr_info_view_on(stack_depth, "%30s : cpu: %d\n", i);
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
        pr_info_view_on(stack_depth, "%30s : %d\n", rq->nr_running);

        rt = fbq_classify_rq(rq);

        pr_info_view_on(stack_depth, "%30s : fbq_type: %d\n", rt);
        pr_info_view_on(stack_depth, "%30s : %d\n", env->fbq_type);

        /*
         * We classify groups/runqueues into three groups:
         *  - regular: there are !numa tasks
         *  - remote:  there are numa tasks that run on the 'wrong' node
         *  - all:     there is no distinction
         *
         * In order to avoid migrating ideally placed numa tasks,
         * ignore those when there's better options.
         *
         * If we ignore the actual busiest queue to migrate another
         * task, the next balance pass can still reduce the busiest
         * queue by moving tasks around inside the node.
         *
         * If we cannot move enough load due to this classification
         * the next pass will adjust the group classification and
         * allow migration of more tasks.
         *
         * Both cases only affect the total convergence complexity.
         */
        if (rt > env->fbq_type)
            continue;

        pr_info_view_on(stack_depth, "%30s : %d\n", env->src_grp_type);
        pr_info_view_on(stack_depth, "%30s : %lu\n", rq->misfit_task_load);
        pr_info_view_on(stack_depth, "%30s : %lu\n", busiest_load);

        /*
         * For ASYM_CPUCAPACITY domains with misfit tasks we simply
         * seek the "biggest" misfit task.
         */
        if (env->src_grp_type == group_misfit_task) {
            if (rq->misfit_task_load > busiest_load) {
                busiest_load = rq->misfit_task_load;
                busiest = rq;
            }
            continue;
        }

        capacity = capacity_of(i);
        pr_info_view_on(stack_depth, "%30s : cpu: %lu\n", capacity);

        /*
         * For ASYM_CPUCAPACITY domains, don't pick a CPU that could
         * eventually lead to active_balancing high->low capacity.
         * Higher per-CPU capacity is considered better than balancing
         * average load.
         */
        if (env->sd->flags & SD_ASYM_CPUCAPACITY &&
            capacity_of(env->dst_cpu) < capacity &&
            rq->nr_running == 1)
            continue;

        load = cpu_runnable_load(rq);
        pr_info_view_on(stack_depth, "%30s : rq.runnable_load_avg: %lu\n", load);
        pr_info_view_on(stack_depth, "%30s : %d\n", rq->nr_running);
        pr_info_view_on(stack_depth, "%30s : %ld\n", env->imbalance);

        /*
         * When comparing with imbalance, use cpu_runnable_load()
         * which is not scaled with the CPU capacity.
         */

        if (rq->nr_running == 1 && load > env->imbalance &&
            !check_cpu_capacity(rq, env->sd))
            continue;

        pr_info_view_on(stack_depth, "%30s : %lu\n", load);
        pr_info_view_on(stack_depth, "%30s : %lu\n", busiest_capacity);
        pr_info_view_on(stack_depth, "%30s : %lu\n", busiest_load);
        pr_info_view_on(stack_depth, "%30s : %lu\n", capacity);
        /*
         * For the load comparisons with the other CPU's, consider
         * the cpu_runnable_load() scaled with the CPU capacity, so
         * that the load can be moved away from the CPU that is
         * potentially running at a lower capacity.
         *
         * Thus we're looking for max(load_i / capacity_i), crosswise
         * multiplication to rid ourselves of the division works out
         * to: load_i * capacity_j > load_j * capacity_i;  where j is
         * our previous maximum.
         */
        if (load * busiest_capacity > busiest_load * capacity) {
            busiest_load = load;
            busiest_capacity = capacity;
            busiest = rq;
            pr_info_view_on(stack_depth, "%30s : new rq: %p\n", (void*)busiest);
        }
    }

    pr_info_view_on(stack_depth, "%30s : %lu\n", busiest_load);
    pr_info_view_on(stack_depth, "%30s : %lu\n", busiest_capacity);
    pr_info_view_on(stack_depth, "%30s : rq: %p\n", (void*)busiest);

    pr_fn_end_on(stack_depth);

    return busiest;
}
//8691
/*
 * Max backoff if we encounter pinned tasks. Pretty arbitrary value, but
 * so long as it is large enough.
 */
#define MAX_PINNED_INTERVAL	512

static inline bool
asym_active_balance(struct lb_env *env)
{
    /*
     * ASYM_PACKING needs to force migrate tasks from busy but
     * lower priority CPUs in order to pack all tasks in the
     * highest priority CPUs.
     */
    return env->idle != CPU_NOT_IDLE && (env->sd->flags & SD_ASYM_PACKING) &&
           sched_asym_prefer(env->dst_cpu, env->src_cpu);
}

static inline bool
voluntary_active_balance(struct lb_env *env)
{
    struct sched_domain *sd = env->sd;

    if (asym_active_balance(env))
        return 1;

    /*
     * The dst_cpu is idle and the src_cpu CPU has only 1 CFS task.
     * It's worth migrating the task if the src_cpu's capacity is reduced
     * because of other sched_class or IRQs if more capacity stays
     * available on dst_cpu.
     */
    if ((env->idle != CPU_NOT_IDLE) &&
        (env->src_rq->cfs.h_nr_running == 1)) {
        if ((check_cpu_capacity(env->src_rq, sd)) &&
            (capacity_of(env->src_cpu)*sd->imbalance_pct < capacity_of(env->dst_cpu)*100))
            return 1;
    }

    if (env->src_grp_type == group_misfit_task)
        return 1;

    return 0;
}

static int need_active_balance(struct lb_env *env)
{
    struct sched_domain *sd = env->sd;

    if (voluntary_active_balance(env))
        return 1;

    return unlikely(sd->nr_balance_failed > sd->cache_nice_tries+2);
}

static int active_load_balance_cpu_stop(void *data);

static int should_we_balance(struct lb_env *env)
{
    pr_fn_start_on(stack_depth);

    struct sched_group *sg = env->sd->groups;
    int cpu, balance_cpu = -1;

    /*
     * Ensure the balancing environment is consistent; can happen
     * when the softirq triggers 'during' hotplug.
     */
    if (!cpumask_test_cpu(env->dst_cpu, env->cpus))
        return 0;

    /*
     * In the newly idle case, we will allow all the CPUs
     * to do the newly idle load balance.
     */
    if (env->idle == CPU_NEWLY_IDLE)
        return 1;

    /* Try to find first idle CPU */
    for_each_cpu_and(cpu, group_balance_mask(sg), env->cpus) {
        if (!idle_cpu(cpu))
            continue;

        balance_cpu = cpu;
        break;
    }

    pr_info_view_on(stack_depth, "%30s : %d\n", balance_cpu);

    if (balance_cpu == -1)
        balance_cpu = group_balance_cpu(sg);	//first

    pr_info_view_on(stack_depth, "%30s : %d\n", balance_cpu);
    pr_info_view_on(stack_depth, "%30s : %d\n", env->dst_cpu);

    pr_fn_end_on(stack_depth);

    /*
     * First idle CPU or the first CPU(busiest) in this sched group
     * is eligible for doing load balancing at this and above domains.
     */
    return balance_cpu == env->dst_cpu;
}
//8786
/*
 * Check this_cpu to ensure it is balanced within domain. Attempt to move
 * tasks if there is an imbalance.
 */
static int load_balance(int this_cpu, struct rq *this_rq,
            struct sched_domain *sd, enum cpu_idle_type idle,
            int *continue_balancing)
{
    pr_fn_start_on(stack_depth);

    int ld_moved, cur_ld_moved, active_balance = 0;
    struct sched_domain *sd_parent = sd->parent;
    struct sched_group *group;
    struct rq *busiest;
    struct rq_flags rf;
    struct cpumask *cpus = this_cpu_cpumask_var_ptr(load_balance_mask);

    pr_info_view_on(stack_depth, "%20s : 0x%X\n", cpus->bits[0]);

    struct lb_env env = {
        .sd			= sd,
        .dst_cpu	= this_cpu,
        .dst_rq		= this_rq,
        .dst_grpmask    = sched_group_span(sd->groups),
        .idle		= idle,
        .loop_break	= sched_nr_migrate_break,
        .cpus		= cpus,
        .fbq_type	= all,
        .tasks		= LIST_HEAD_INIT(env.tasks),
    };

    cpumask_and(cpus, sched_domain_span(sd), cpu_active_mask);

    pr_info_view_on(stack_depth, "%20s : 0x%X\n", sd->span[0]);
    pr_info_view_on(stack_depth, "%20s : 0x%X\n", cpus->bits[0]);

    schedstat_inc(sd->lb_count[idle]);

redo:
    pr_out_on(stack_depth, "redo:\n");

    if (!should_we_balance(&env)) {
        *continue_balancing = 0;
        goto out_balanced;
    }

    group = find_busiest_group(&env);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)group);

    if (!group) {
        schedstat_inc(sd->lb_nobusyg[idle]);
        goto out_balanced;
    }

    busiest = find_busiest_queue(&env, group);
    pr_info_view_on(stack_depth, "%20s : rq: %p\n", (void*)busiest);

    if (!busiest) {
        schedstat_inc(sd->lb_nobusyq[idle]);
        goto out_balanced;
    }

    pr_info_view_on(stack_depth, "%20s : %d\n", env.dst_cpu);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)env.dst_rq);
    BUG_ON(busiest == env.dst_rq);

    schedstat_add(sd->lb_imbalance[idle], env.imbalance);

    env.src_cpu = busiest->cpu;
    env.src_rq = busiest;

    pr_info_view_on(stack_depth, "%20s : %d\n", env.src_cpu);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)env.src_rq);
    pr_info_view_on(stack_depth, "%20s : %d\n", busiest->nr_running);

    ld_moved = 0;
    if (busiest->nr_running > 1) {
        /*
         * Attempt to move tasks. If find_busiest_group has found
         * an imbalance but busiest->nr_running <= 1, the group is
         * still unbalanced. ld_moved simply stays zero, so it is
         * correctly treated as an imbalance.
         */
        env.flags |= LBF_ALL_PINNED;
        env.loop_max  = min(sysctl_sched_nr_migrate, busiest->nr_running);

more_balance:
        pr_out_on(stack_depth, "more_balance:\n");

        rq_lock_irqsave(busiest, &rf);
        update_rq_clock(busiest);

        /*
         * cur_ld_moved - load moved in current iteration
         * ld_moved     - cumulative load moved across iterations
         */
        cur_ld_moved = detach_tasks(&env);

        pr_info_view_on(stack_depth, "%20s : %d\n", cur_ld_moved);

        /*
         * We've detached some tasks from busiest_rq. Every
         * task is masked "TASK_ON_RQ_MIGRATING", so we can safely
         * unlock busiest->lock, and we are able to be sure
         * that nobody can manipulate the tasks in parallel.
         * See task_rq_lock() family for the details.
         */

        rq_unlock(busiest, &rf);

        if (cur_ld_moved) {
            attach_tasks(&env);
            ld_moved += cur_ld_moved;
        }

        local_irq_restore(rf.flags);

        if (env.flags & LBF_NEED_BREAK) {
            env.flags &= ~LBF_NEED_BREAK;
            goto more_balance;
        }

        /*
         * Revisit (affine) tasks on src_cpu that couldn't be moved to
         * us and move them to an alternate dst_cpu in our sched_group
         * where they can run. The upper limit on how many times we
         * iterate on same src_cpu is dependent on number of CPUs in our
         * sched_group.
         *
         * This changes load balance semantics a bit on who can move
         * load to a given_cpu. In addition to the given_cpu itself
         * (or a ilb_cpu acting on its behalf where given_cpu is
         * nohz-idle), we now have balance_cpu in a position to move
         * load to given_cpu. In rare situations, this may cause
         * conflicts (balance_cpu and given_cpu/ilb_cpu deciding
         * _independently_ and at _same_ time to move some load to
         * given_cpu) causing exceess load to be moved to given_cpu.
         * This however should not happen so much in practice and
         * moreover subsequent load balance cycles should correct the
         * excess load moved.
         */
        if ((env.flags & LBF_DST_PINNED) && env.imbalance > 0) {

            /* Prevent to re-select dst_cpu via env's CPUs */
            __cpumask_clear_cpu(env.dst_cpu, env.cpus);

            env.dst_rq	 = cpu_rq(env.new_dst_cpu);
            env.dst_cpu	 = env.new_dst_cpu;
            env.flags	&= ~LBF_DST_PINNED;
            env.loop	 = 0;
            env.loop_break	 = sched_nr_migrate_break;

            /*
             * Go back to "more_balance" rather than "redo" since we
             * need to continue with same src_cpu.
             */
            goto more_balance;
        }

        /*
         * We failed to reach balance because of affinity.
         */
        if (sd_parent) {
            int *group_imbalance = &sd_parent->groups->sgc->imbalance;

            if ((env.flags & LBF_SOME_PINNED) && env.imbalance > 0)
                *group_imbalance = 1;
        }

        /* All tasks on this runqueue were pinned by CPU affinity */
        if (unlikely(env.flags & LBF_ALL_PINNED)) {
            __cpumask_clear_cpu(cpu_of(busiest), cpus);
            /*
             * Attempting to continue load balancing at the current
             * sched_domain level only makes sense if there are
             * active CPUs remaining as possible busiest CPUs to
             * pull load from which are not contained within the
             * destination group that is receiving any migrated
             * load.
             */
            if (!cpumask_subset(cpus, env.dst_grpmask)) {
                env.loop = 0;
                env.loop_break = sched_nr_migrate_break;
                goto redo;
            }
            goto out_all_pinned;
        }
    }

    pr_info_view_on(stack_depth, "%20s : %d\n", ld_moved);

    if (!ld_moved) {
        schedstat_inc(sd->lb_failed[idle]);
        /*
         * Increment the failure counter only on periodic balance.
         * We do not want newidle balance, which can be very
         * frequent, pollute the failure counter causing
         * excessive cache_hot migrations and active balances.
         */
        if (idle != CPU_NEWLY_IDLE)
            sd->nr_balance_failed++;

        if (need_active_balance(&env)) {
            unsigned long flags;

            raw_spin_lock_irqsave(&busiest->lock, flags);

            /*
             * Don't kick the active_load_balance_cpu_stop,
             * if the curr task on busiest CPU can't be
             * moved to this_cpu:
             */
            if (!cpumask_test_cpu(this_cpu, busiest->curr->cpus_ptr)) {
                raw_spin_unlock_irqrestore(&busiest->lock,
                                flags);
                env.flags |= LBF_ALL_PINNED;
                goto out_one_pinned;
            }

            /*
             * ->active_balance synchronizes accesses to
             * ->active_balance_work.  Once set, it's cleared
             * only after active load balance is finished.
             */
            if (!busiest->active_balance) {
                busiest->active_balance = 1;
                busiest->push_cpu = this_cpu;
                active_balance = 1;
            }
            raw_spin_unlock_irqrestore(&busiest->lock, flags);
#if 0
            if (active_balance) {
                stop_one_cpu_nowait(cpu_of(busiest),
                    active_load_balance_cpu_stop, busiest,
                    &busiest->active_balance_work);
            }
#endif //0
            /* We've kicked active balancing, force task migration. */
            sd->nr_balance_failed = sd->cache_nice_tries+1;
        }
    } else
        sd->nr_balance_failed = 0;

    if (likely(!active_balance) || voluntary_active_balance(&env)) {
        /* We were unbalanced, so reset the balancing interval */
        sd->balance_interval = sd->min_interval;
    } else {
        /*
         * If we've begun active balancing, start to back off. This
         * case may not be covered by the all_pinned logic if there
         * is only 1 task on the busy runqueue (because we don't call
         * detach_tasks).
         */
        if (sd->balance_interval < sd->max_interval)
            sd->balance_interval *= 2;
    }

    goto out;

out_balanced:
    pr_out_on(stack_depth, "out_balanced:\n");
    /*
     * We reach balance although we may have faced some affinity
     * constraints. Clear the imbalance flag only if other tasks got
     * a chance to move and fix the imbalance.
     */
    if (sd_parent && !(env.flags & LBF_ALL_PINNED)) {
        int *group_imbalance = &sd_parent->groups->sgc->imbalance;

        if (*group_imbalance)
            *group_imbalance = 0;
    }

out_all_pinned:
    pr_out_on(stack_depth, "out_all_pinned:\n");
    /*
     * We reach balance because all tasks are pinned at this level so
     * we can't migrate them. Let the imbalance flag set so parent level
     * can try to migrate them.
     */
    schedstat_inc(sd->lb_balanced[idle]);

    sd->nr_balance_failed = 0;

out_one_pinned:
    pr_out_on(stack_depth, "out_one_pinned:\n");
    ld_moved = 0;

    /*
     * newidle_balance() disregards balance intervals, so we could
     * repeatedly reach this code, which would lead to balance_interval
     * skyrocketting in a short amount of time. Skip the balance_interval
     * increase logic to avoid that.
     */
    if (env.idle == CPU_NEWLY_IDLE)
        goto out;

    /* tune up the balancing interval */
    if ((env.flags & LBF_ALL_PINNED &&
         sd->balance_interval < MAX_PINNED_INTERVAL) ||
        sd->balance_interval < sd->max_interval)
        sd->balance_interval *= 2;
out:
    pr_out_on(stack_depth, "out:\n");
    pr_info_view_on(stack_depth, "%20s : %d\n", ld_moved);
    pr_fn_end_on(stack_depth);

    return ld_moved;
}
//9064
static inline unsigned long
get_sd_balance_interval(struct sched_domain *sd, int cpu_busy)
{
    pr_fn_start_on(stack_depth);

    unsigned long interval = sd->balance_interval;

    pr_info_view_on(stack_depth, "%30s : %lu\n", sd->balance_interval);
    pr_info_view_on(stack_depth, "%30s : %u\n", sd->busy_factor);
    pr_info_view_on(stack_depth, "%30s : %d\n", cpu_busy);

    if (cpu_busy)
        interval *= sd->busy_factor;

    pr_info_view_on(stack_depth, "%30s : ms: %lu\n", interval);

    /* scale ms to jiffies */
    interval = msecs_to_jiffies(interval);
    interval = clamp(interval, 1UL, max_load_balance_interval);

    pr_info_view_on(stack_depth, "%30s : jiffies: %lu\n", interval);

    pr_fn_end_on(stack_depth);

    return interval;
}

static inline void
update_next_balance(struct sched_domain *sd, unsigned long *next_balance)
{
    unsigned long interval, next;

    /* used by idle balance, so cpu_busy = 0 */
    interval = get_sd_balance_interval(sd, 0);
    next = sd->last_balance + interval;

    if (time_after(*next_balance, next))
        *next_balance = next;
}
//9092
/*
 * active_load_balance_cpu_stop is run by the CPU stopper. It pushes
 * running tasks off the busiest CPU onto idle CPUs. It requires at
 * least 1 task to be running on each physical CPU where possible, and
 * avoids physical / logical imbalances.
 */
static int active_load_balance_cpu_stop(void *data)
{
    struct rq *busiest_rq = data;
    int busiest_cpu = cpu_of(busiest_rq);
    int target_cpu = busiest_rq->push_cpu;
    struct rq *target_rq = cpu_rq(target_cpu);
    struct sched_domain *sd;
    struct task_struct *p = NULL;
    struct rq_flags rf;

    rq_lock_irq(busiest_rq, &rf);
    /*
     * Between queueing the stop-work and running it is a hole in which
     * CPUs can become inactive. We should not move tasks from or to
     * inactive CPUs.
     */
    if (!cpu_active(busiest_cpu) || !cpu_active(target_cpu))
        goto out_unlock;

    /* Make sure the requested CPU hasn't gone down in the meantime: */
    if (unlikely(busiest_cpu != smp_processor_id() ||
             !busiest_rq->active_balance))
        goto out_unlock;

    /* Is there any task to move? */
    if (busiest_rq->nr_running <= 1)
        goto out_unlock;

    /*
     * This condition is "impossible", if it occurs
     * we need to fix it. Originally reported by
     * Bjorn Helgaas on a 128-CPU setup.
     */
    BUG_ON(busiest_rq == target_rq);

    /* Search for an sd spanning us and the target CPU. */
    rcu_read_lock();
    for_each_domain(target_cpu, sd) {
        if ((sd->flags & SD_LOAD_BALANCE) &&
            cpumask_test_cpu(busiest_cpu, sched_domain_span(sd)))
                break;
    }

    if (likely(sd)) {
        struct lb_env env = {
            .sd		= sd,
            .dst_cpu	= target_cpu,
            .dst_rq		= target_rq,
            .src_cpu	= busiest_rq->cpu,
            .src_rq		= busiest_rq,
            .idle		= CPU_IDLE,
            /*
             * can_migrate_task() doesn't need to compute new_dst_cpu
             * for active balancing. Since we have CPU_IDLE, but no
             * @dst_grpmask we need to make that test go away with lying
             * about DST_PINNED.
             */
            .flags		= LBF_DST_PINNED,
        };

        schedstat_inc(sd->alb_count);
        update_rq_clock(busiest_rq);

        p = detach_one_task(&env);
        if (p) {
            schedstat_inc(sd->alb_pushed);
            /* Active balancing done, reset the failure counter. */
            sd->nr_balance_failed = 0;
        } else {
            schedstat_inc(sd->alb_failed);
        }
    }
    rcu_read_unlock();
out_unlock:
    busiest_rq->active_balance = 0;
    rq_unlock(busiest_rq, &rf);

    if (p)
        attach_one_task(target_rq, p);

    local_irq_enable();

    return 0;
}
//9183
static DEFINE_SPINLOCK(balancing);

/*
 * Scale the max load_balance interval with the number of CPUs in the system.
 * This trades load-balance latency on larger machines for less cross talk.
 */
void update_max_interval(void)
{
    max_load_balance_interval = HZ*num_online_cpus()/10;
}
//9194
/*
 * It checks each scheduling domain to see if it is due to be balanced,
 * and initiates a balancing operation if so.
 *
 * Balancing parameters are set up in init_sched_domains.
 */
static void rebalance_domains(struct rq *rq, enum cpu_idle_type idle)
{
    pr_fn_start_on(stack_depth);

    int continue_balancing = 1;
    int cpu = rq->cpu;
    unsigned long interval;
    struct sched_domain *sd;
    /* Earliest time when we have to do rebalance again */
    unsigned long next_balance = jiffies + 60*HZ;
    int update_next_balance = 0;
    int need_serialize, need_decay = 0;
    u64 max_cost = 0;

    pr_info_view_on(stack_depth, "%10s : %d\n", cpu);
    pr_info_view_on(stack_depth, "%10s : %d\n", idle);
    pr_info_view_on(stack_depth, "%10s : %p\n", (void*)rq);

    rcu_read_lock();
    for_each_domain(cpu, sd) {
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)sd);
        pr_info_view_on(stack_depth, "%30s : %s\n", sd->name);
        pr_out_on(stack_depth, "-------------------------------------------\n");
        pr_info_view_on(stack_depth, "%30s : %lu\n", jiffies);
        pr_info_view_on(stack_depth, "%30s : %lu\n", next_balance);
        pr_info_view_on(stack_depth, "%30s : %llu\n", sd->max_newidle_lb_cost);
        pr_info_view_on(stack_depth, "%30s : %lu\n", sd->next_decay_max_lb_cost);
        /*
         * Decay the newidle max times here because this is a regular
         * visit to all the domains. Decay ~1% per second.
         */
        if (time_after(jiffies, sd->next_decay_max_lb_cost)) {
            sd->max_newidle_lb_cost =
                (sd->max_newidle_lb_cost * 253) / 256;
            sd->next_decay_max_lb_cost = jiffies + HZ;
            need_decay = 1;
        }
        max_cost += sd->max_newidle_lb_cost;

        if (!(sd->flags & SD_LOAD_BALANCE))
            continue;

        pr_info_view_on(stack_depth, "%30s : %d\n", continue_balancing);

        /*
         * Stop the load balance at this level. There is another
         * CPU in our sched group which is doing load balancing more
         * actively.
         */
        if (!continue_balancing) {
            if (need_decay)
                continue;
            break;
        }

        interval = get_sd_balance_interval(sd, idle != CPU_IDLE);

        pr_info_view_on(stack_depth, "%30s : %lu\n", jiffies);
        pr_info_view_on(stack_depth, "%30s : %lu\n", sd->last_balance);
        pr_info_view_on(stack_depth, "%30s : %lu\n", interval);
        pr_info_view_on(stack_depth, "%30s : %d\n", need_decay);
        pr_info_view_on(stack_depth, "%30s : %lu\n", sd->next_decay_max_lb_cost);
        pr_info_view_on(stack_depth, "%30s : %llu\n", sd->max_newidle_lb_cost);
        pr_info_view_on(stack_depth, "%30s : %llu\n", max_cost);

        need_serialize = sd->flags & SD_SERIALIZE;
        pr_info_view_on(stack_depth, "%30s : %d\n", need_serialize);

        if (need_serialize) {
            //if (!spin_trylock(&balancing))
            //if (!spin_lock(&balancing))
                //goto out;
        }

        if (time_after_eq(jiffies, sd->last_balance + interval)) {
            if (load_balance(cpu, rq, sd, idle, &continue_balancing)) {
                /*
                 * The LBF_DST_PINNED logic could have changed
                 * env->dst_cpu, so we can't know our idle
                 * state even if we migrated tasks. Update it.
                 */
                idle = idle_cpu(cpu) ? CPU_IDLE : CPU_NOT_IDLE;
            }
            sd->last_balance = jiffies;
            interval = get_sd_balance_interval(sd, idle != CPU_IDLE);
        }
        if (need_serialize)
            spin_unlock(&balancing);
out:
        if (time_after(next_balance, sd->last_balance + interval)) {
            next_balance = sd->last_balance + interval;
            update_next_balance = 1;
        }

        pr_info_view_on(stack_depth, "%30s : %lu\n", sd->last_balance);
        pr_info_view_on(stack_depth, "%30s : %lu\n", next_balance);
        pr_info_view_on(stack_depth, "%30s : %d\n", update_next_balance);
        pr_info_view_on(stack_depth, "%30s : %d\n", continue_balancing);
        jiffies += 3;
    }
    if (need_decay) {
        /*
         * Ensure the rq-wide value also decays but keep it at a
         * reasonable floor to avoid funnies with rq->avg_idle.
         */
        rq->max_idle_balance_cost =
            max((u64)sysctl_sched_migration_cost, max_cost);
    }
    rcu_read_unlock();

    /*
     * next_balance will be updated only when there is a need.
     * When the cpu is attached to null domain for ex, it will not be
     * updated.
     */
    if (likely(update_next_balance)) {
        rq->next_balance = next_balance;

#ifdef CONFIG_NO_HZ_COMMON
        /*
         * If this CPU has been elected to perform the nohz idle
         * balance. Other idle CPUs have already rebalanced with
         * nohz_idle_balance() and nohz.next_balance has been
         * updated accordingly. This CPU is now running the idle load
         * balance for itself and we need to update the
         * nohz.next_balance accordingly.
         */
        if ((idle == CPU_IDLE) && time_after(nohz.next_balance, rq->next_balance))
            nohz.next_balance = rq->next_balance;
#endif
    }

    pr_info_view_on(stack_depth, "%30s : %lu\n", rq->next_balance);

    pr_fn_end_on(stack_depth);
}

static inline int on_null_domain(struct rq *rq)
{
    return unlikely(!rcu_dereference_sched(rq->sd));
}
//9306 lines



//9745 lines
//#else /* !CONFIG_NO_HZ_COMMON */
static inline void nohz_balancer_kick(struct rq *rq) { }

static inline bool nohz_idle_balance(struct rq *this_rq, enum cpu_idle_type idle)
{
    return false;
}

static inline void nohz_newidle_balance(struct rq *this_rq) { }
//#endif /* CONFIG_NO_HZ_COMMON */
//9756 lines




//9874 lines
/*
 * run_rebalance_domains is triggered when needed from the scheduler tick.
 * Also triggered for nohz idle balancing (with nohz_balancing_kick set).
 */
static __latent_entropy void run_rebalance_domains(struct softirq_action *h)
{
    pr_fn_start_on(stack_depth);

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
    //CONFIG_NO_HZ_COMMON
    //if (nohz_idle_balance(this_rq, idle))
    //    return;

    /* normal load balance */
    update_blocked_averages(this_rq->cpu);
    //rebalance_domains(this_rq, idle);
    rebalance_domains(this_rq, CPU_IDLE);

    pr_fn_end_on(stack_depth);
}

/*
 * Trigger the SCHED_SOFTIRQ if it is time to do periodic load balancing.
 */
void trigger_load_balance(struct rq *rq)
{
    /* Don't need to rebalance while attached to NULL domain */
    if (unlikely(on_null_domain(rq)))
        return;

    //if (time_after_eq(jiffies, rq->next_balance))
    //	raise_softirq(SCHED_SOFTIRQ);

    nohz_balancer_kick(rq);
}
//9915
static void rq_online_fair(struct rq *rq)
{
    update_sysctl();

    update_runtime_enabled(rq);
}

static void rq_offline_fair(struct rq *rq)
{
    update_sysctl();

    /* Ensure any throttled groups are reachable by pick_next_task */
    unthrottle_offline_cfs_rqs(rq);
}

//#endif /* CONFIG_SMP */

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
    update_rq_clock(rq);

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
//10015
static inline bool vruntime_normalized(struct task_struct *p)
{
    struct sched_entity *se = &p->se;

    /*
     * In both the TASK_ON_RQ_QUEUED and TASK_ON_RQ_MIGRATING cases,
     * the dequeue_entity(.flags=0) will already have normalized the
     * vruntime.
     */
    if (p->on_rq)
        return true;

    /*
     * When !on_rq, vruntime of the task has usually NOT been normalized.
     * But there are some cases where it has already been normalized:
     *
     * - A forked child which is waiting for being woken up by
     *   wake_up_new_task().
     * - A task which has been woken up by try_to_wake_up() and
     *   waiting for actually being woken up by sched_ttwu_pending().
     */
    if (!se->sum_exec_runtime ||
        (p->state == TASK_WAKING && p->sched_remote_wakeup))
        return true;

    return false;
}
//10043
#ifdef CONFIG_FAIR_GROUP_SCHED
/*
 * Propagate the changes of the sched_entity across the tg tree to make it
 * visible to the root
 */
static void propagate_entity_cfs_rq(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    struct cfs_rq *cfs_rq;

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->parent);

    /* Start to propagate at parent */
    se = se->parent;

    for_each_sched_entity(se) {
        cfs_rq = cfs_rq_of(se);

        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->tg);

        if (cfs_rq_throttled(cfs_rq))
            break;

        update_load_avg(cfs_rq, se, UPDATE_TG);
    }

    pr_fn_end_on(stack_depth);
}
#else
static void propagate_entity_cfs_rq(struct sched_entity *se) { }
#endif
//10068
static void detach_entity_cfs_rq(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    struct cfs_rq *cfs_rq = cfs_rq_of(se);

    /* Catch up with the cfs_rq and remove our load when we leave */
    update_load_avg(cfs_rq, se, 0);
    detach_entity_load_avg(cfs_rq, se);
    update_tg_load_avg(cfs_rq, false);
    propagate_entity_cfs_rq(se);

    pr_fn_end_on(stack_depth);
}

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

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%30s : %d\n", se->depth);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->my_q);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->cfs_rq);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&se->avg);
    pr_sched_avg_info(&se->avg);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&cfs_rq->avg);
    pr_sched_avg_info(&cfs_rq->avg);

    /* Synchronize entity with its cfs_rq */
    update_load_avg(cfs_rq, se, sched_feat(ATTACH_AGE_LOAD) ? 0 : SKIP_AGE_LOAD);
    attach_entity_load_avg(cfs_rq, se, 0);
    update_tg_load_avg(cfs_rq, false);
    propagate_entity_cfs_rq(se);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se->cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&se->avg);
    pr_sched_avg_info(&se->avg);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&cfs_rq->avg);
    pr_sched_avg_info(&cfs_rq->avg);
    pr_fn_end_on(stack_depth);
}

static void detach_task_cfs_rq(struct task_struct *p)
{
    pr_fn_start_on(stack_depth);

    struct sched_entity *se = &p->se;
    struct cfs_rq *cfs_rq = cfs_rq_of(se);

    if (!vruntime_normalized(p)) {
        /*
         * Fix up our vruntime so that the current sleep doesn't
         * cause 'unlimited' sleep bonus.
         */
        place_entity(cfs_rq, se, 0);
        se->vruntime -= cfs_rq->min_vruntime;
    }

    detach_entity_cfs_rq(se);

    pr_fn_end_on(stack_depth);
}

static void attach_task_cfs_rq(struct task_struct *p)
{
    pr_fn_start_on(stack_depth);

    struct sched_entity *se = &p->se;
    struct cfs_rq *cfs_rq = cfs_rq_of(se);

    attach_entity_cfs_rq(se);

    if (!vruntime_normalized(p))
        se->vruntime += cfs_rq->min_vruntime;

    pr_fn_end_on(stack_depth);
}

static void switched_from_fair(struct rq *rq, struct task_struct *p)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)p);

    detach_task_cfs_rq(p);

    pr_fn_end_on(stack_depth);
}

static void switched_to_fair(struct rq *rq, struct task_struct *p)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)p);

    attach_task_cfs_rq(p);

    if (task_on_rq_queued(p)) {
        /*
         * We were most likely switched from sched_rt, so
         * kick off the schedule if running, otherwise just see
         * if we can still preempt the current task.
         */
        if (rq->curr == p)
            resched_curr(rq);
        else
            check_preempt_curr(rq, p, 0);
    }

    pr_fn_end_on(stack_depth);
}
//10148
/* Account for a task changing its policy or group.
 *
 * This routine is mostly called to set cfs_rq->curr field when a task
 * migrates between groups/classes.
 */
static void set_next_task_fair(struct rq *rq, struct task_struct *p)
{
    pr_fn_start_on(stack_depth);

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

    pr_fn_end_on(stack_depth);
}
//10176
void init_cfs_rq(struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    cfs_rq->tasks_timeline = RB_ROOT_CACHED;
    cfs_rq->min_vruntime = (u64)(-(1LL << 20));
#ifndef CONFIG_64BIT
    cfs_rq->min_vruntime_copy = cfs_rq->min_vruntime;
#endif
#ifdef CONFIG_SMP
    raw_spin_lock_init(&cfs_rq->removed.lock);
#endif

    pr_info_view_on(stack_depth, "%30s : %llu\n", cfs_rq->min_vruntime);
    pr_fn_end_on(stack_depth);
}
//10188
#ifdef CONFIG_FAIR_GROUP_SCHED
static void task_set_group_fair(struct task_struct *p)
{
    struct sched_entity *se = &p->se;

    set_task_rq(p, task_cpu(p));
    se->depth = se->parent ? se->parent->depth + 1 : 0;
}

static void task_move_group_fair(struct task_struct *p)
{
    detach_task_cfs_rq(p);
    set_task_rq(p, task_cpu(p));

#ifdef CONFIG_SMP
    /* Tell se's cfs_rq has been changed -- migrated */
    p->se.avg.last_update_time = 0;
#endif
    attach_task_cfs_rq(p);
}

static void task_change_group_fair(struct task_struct *p, int type)
{
    switch (type) {
    case TASK_SET_GROUP:
        task_set_group_fair(p);
        break;

    case TASK_MOVE_GROUP:
        task_move_group_fair(p);
        break;
    }
}
//10222
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

    for_each_possible_cpu(i) {
        pr_info_view_on(stack_depth, "%20s : %d\n", i);
        rq = cpu_rq(i);
        se = tg->se[i];
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)tg);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cpu_rq(i));
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)tg->se[i]);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->cfs_rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->cfs_rq->tg);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->cfs_rq->rq);

        rq_lock_irq(rq, &rf);
        update_rq_clock(rq);
        attach_entity_cfs_rq(se);
        sync_throttle(tg, i);
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
    pr_info_view_on(stack_depth, "%30s : %d\n", cpu);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)tg);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)parent);

    cfs_rq->tg = tg;
    cfs_rq->rq = rq;
    init_cfs_rq_runtime(cfs_rq);

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->tg);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->rq);

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
//10354
static DEFINE_MUTEX(shares_mutex);

int sched_group_set_shares(struct task_group *tg, unsigned long shares)
{
    int i;

    /*
     * We can't change the weight of the root cgroup.
     */
    if (!tg->se[0])
        return -EINVAL;

    shares = clamp(shares, scale_load(MIN_SHARES), scale_load(MAX_SHARES));

    mutex_lock(&shares_mutex);
    if (tg->shares == shares)
        goto done;

    tg->shares = shares;
    for_each_possible_cpu(i) {
        struct rq *rq = cpu_rq(i);
        struct sched_entity *se = tg->se[i];
        struct rq_flags rf;

        /* Propagate contribution to hierarchy */
        rq_lock_irqsave(rq, &rf);
        update_rq_clock(rq);
        for_each_sched_entity(se) {
            update_load_avg(cfs_rq_of(se), se, UPDATE_TG);
            update_cfs_group(se);
        }
        rq_unlock_irqrestore(rq, &rf);
    }

done:
    mutex_unlock(&shares_mutex);
    return 0;
}
#else /* CONFIG_FAIR_GROUP_SCHED */

void free_fair_sched_group(struct task_group *tg) { }

int alloc_fair_sched_group(struct task_group *tg, struct task_group *parent)
{
    return 1;
}

void online_fair_sched_group(struct task_group *tg) { }

void unregister_fair_sched_group(struct task_group *tg) { }

#endif /* CONFIG_FAIR_GROUP_SCHED */


static unsigned int get_rr_interval_fair(struct rq *rq, struct task_struct *task)
{
    struct sched_entity *se = &task->se;
    unsigned int rr_interval = 0;

    /*
     * Time slice is 0 for SCHED_OTHER tasks that are on an otherwise
     * idle runqueue:
     */
    if (rq->cfs.load.weight)
        rr_interval = NS_TO_JIFFIES(sched_slice(cfs_rq_of(se), se));

    return rr_interval;
}
//10423
/*
 * All the scheduling class methods:
 */
const struct sched_class fair_sched_class = {
    .next				= &idle_sched_class,
    .enqueue_task		= enqueue_task_fair,
    .dequeue_task		= dequeue_task_fair,
    .yield_task			= yield_task_fair,
    .yield_to_task		= yield_to_task_fair,

    .check_preempt_curr	= check_preempt_wakeup,

    .pick_next_task		= pick_next_task_fair,
    .put_prev_task		= put_prev_task_fair,
    .set_next_task  	= set_next_task_fair,

#ifdef CONFIG_SMP
    .balance			= balance_fair,
    .select_task_rq		= select_task_rq_fair,
    .migrate_task_rq	= migrate_task_rq_fair,

    .rq_online			= rq_online_fair,
    .rq_offline			= rq_offline_fair,

    .task_dead			= task_dead_fair,
    .set_cpus_allowed	= set_cpus_allowed_common,
#endif

    .task_tick			= task_tick_fair,
    .task_fork			= task_fork_fair,

    .prio_changed		= prio_changed_fair,
    .switched_from		= switched_from_fair,
    .switched_to		= switched_to_fair,

    .get_rr_interval	= get_rr_interval_fair,

    .update_curr		= update_curr_fair,

#ifdef CONFIG_FAIR_GROUP_SCHED
    .task_change_group	= task_change_group_fair,
#endif

#ifdef CONFIG_UCLAMP_TASK
    .uclamp_enabled	= 1,
#endif
};

//10472
#ifdef CONFIG_SCHED_DEBUG
void print_cfs_stats(struct seq_file *m, int cpu)
{
    struct cfs_rq *cfs_rq, *pos;

    rcu_read_lock();
    for_each_leaf_cfs_rq_safe(cpu_rq(cpu), cfs_rq, pos)
        print_cfs_rq(m, cpu, cfs_rq);
    rcu_read_unlock();
}

#ifdef CONFIG_NUMA_BALANCING
void show_numa_stats(struct task_struct *p, struct seq_file *m)
{
    int node;
    unsigned long tsf = 0, tpf = 0, gsf = 0, gpf = 0;
    struct numa_group *ng;

    rcu_read_lock();
    ng = rcu_dereference(p->numa_group);
    for_each_online_node(node) {
        if (p->numa_faults) {
            tsf = p->numa_faults[task_faults_idx(NUMA_MEM, node, 0)];
            tpf = p->numa_faults[task_faults_idx(NUMA_MEM, node, 1)];
        }
        if (ng) {
            gsf = ng->faults[task_faults_idx(NUMA_MEM, node, 0)],
            gpf = ng->faults[task_faults_idx(NUMA_MEM, node, 1)];
        }
        print_numa_stats(m, node, tsf, tpf, gsf, gpf);
    }
    rcu_read_unlock();
}
#endif /* CONFIG_NUMA_BALANCING */
#endif /* CONFIG_SCHED_DEBUG */

//10507
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
//10521 lines




//----------------- User Defined Functions --------------------------
/*
 * sched load balance flow
 * run_rebalnce_domains(): CPU_IDLE
 * idle_balance(): CPU_NEWLY_IDLE
 *
 * select_task_rq_*():
 * wake_up_new_task(): SD_BALANCE_FORK
 * sched_exec(): SD_BALANCE_EXEC
 * try_to_wake_up(): SD_BALANCE_WAKE
 */
void sched_fair_run_rebalance(void)
{
    jiffies += 17;	//interval 7tick
    run_rebalance_domains(NULL);
}
EXPORT_SYMBOL(sched_fair_run_rebalance);

