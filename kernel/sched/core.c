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
#include <linux/radix-tree-user.h>
#include <linux/sched/clock.h>
#include <linux/sched/debug.h>
#include <linux/export.h>

#include <asm-generic/preempt.h>
//#include <asm/switch_to.h>
#include <asm-generic/switch_to.h>
//#include <asm/tlb.h>

//#include "../workqueue_internal.h"
//#include "../smpboot.h"

#include "pelt.h"

#include <linux/uaccess.h>
#include <linux/cpu.h>

#define CREATE_TRACE_POINTS
//#include <trace/events/sched.h>

//DEFINE_PER_CPU_SHARED_ALIGNED(struct rq, runqueues);
struct rq runqueues[NR_CPUS];

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


#ifdef CONFIG_SCHED_CORE

DEFINE_STATIC_KEY_FALSE(__sched_core_enabled);

/* kernel prio, less is more */
static inline int __task_prio(struct task_struct *p)
{
    if (p->sched_class == &stop_sched_class) /* trumps deadline */
        return -2;

    if (rt_prio(p->prio)) /* includes deadline */
        return p->prio; /* [-1, 99] */

    if (p->sched_class == &idle_sched_class)
        return MAX_RT_PRIO + NICE_WIDTH; /* 140 */

    return MAX_RT_PRIO + MAX_NICE; /* 120, squash fair */
}

/*
 * l(a,b)
 * le(a,b) := !l(b,a)
 * g(a,b)  := l(b,a)
 * ge(a,b) := !l(a,b)
 */

/* real prio, less is less */
static inline bool prio_less(struct task_struct *a, struct task_struct *b, bool in_fi)
{

    int pa = __task_prio(a), pb = __task_prio(b);

    if (-pa < -pb)
        return true;

    if (-pb < -pa)
        return false;

    if (pa == -1) /* dl_prio() doesn't work because of stop_class above */
        return !dl_time_before(a->dl.deadline, b->dl.deadline);

    if (pa == MAX_RT_PRIO + MAX_NICE)       /* fair */
        return cfs_prio_less(a, b, in_fi);

    return false;
}

static inline bool __sched_core_less(struct task_struct *a, struct task_struct *b)
{
    if (a->core_cookie < b->core_cookie)
        return true;

    if (a->core_cookie > b->core_cookie)
        return false;

    /* flip prio, so high prio is leftmost */
    if (prio_less(b, a, !!task_rq(a)->core->core_forceidle_count))
        return true;

    return false;
}

#define __node_2_sc(node) rb_entry((node), struct task_struct, core_node)

static inline bool rb_sched_core_less(struct rb_node *a, const struct rb_node *b)
{
    return __sched_core_less(__node_2_sc(a), __node_2_sc(b));
}

static inline int rb_sched_core_cmp(const void *key, const struct rb_node *node)
{
    const struct task_struct *p = __node_2_sc(node);
    unsigned long cookie = (unsigned long)key;

    if (cookie < p->core_cookie)
        return -1;

    if (cookie > p->core_cookie)
        return 1;

    return 0;
}

void sched_core_enqueue(struct rq *rq, struct task_struct *p)
{
    rq->core->core_task_seq++;

    if (!p->core_cookie)
        return;

    rb_add(&p->core_node, &rq->core_tree, rb_sched_core_less);
}

void sched_core_dequeue(struct rq *rq, struct task_struct *p, int flags)
{
    rq->core->core_task_seq++;

    if (sched_core_enqueued(p)) {
        rb_erase(&p->core_node, &rq->core_tree);
        RB_CLEAR_NODE(&p->core_node);
    }

    /*
         * Migrating the last task off the cpu, with the cpu in forced idle
         * state. Reschedule to create an accounting edge for forced idle,
         * and re-examine whether the core is still in forced idle state.
         */
    if (!(flags & DEQUEUE_SAVE) && rq->nr_running == 1 &&
            rq->core->core_forceidle_count && rq->curr == rq->idle)
        resched_curr(rq);
}

/*
 * Find left-most (aka, highest priority) task matching @cookie.
 */
static struct task_struct *sched_core_find(struct rq *rq, unsigned long cookie)
{
    struct rb_node *node;

    node = rb_find_first((void *)cookie, &rq->core_tree, rb_sched_core_cmp);
    /*
         * The idle task always matches any cookie!
         */
    if (!node)
        return idle_sched_class.pick_task(rq);

    return __node_2_sc(node);
}

static struct task_struct *sched_core_next(struct task_struct *p, unsigned long cookie)
{
    struct rb_node *node = &p->core_node;

    node = rb_next(node);
    if (!node)
        return NULL;

    p = container_of(node, struct task_struct, core_node);
    if (p->core_cookie != cookie)
        return NULL;

    return p;
}

/*
 * Magic required such that:
 *
 *      raw_spin_rq_lock(rq);
 *      ...
 *      raw_spin_rq_unlock(rq);
 *
 * ends up locking and unlocking the _same_ lock, and all CPUs
 * always agree on what rq has what lock.
 *
 * XXX entirely possible to selectively enable cores, don't bother for now.
 */

static DEFINE_MUTEX(sched_core_mutex);
static atomic_t sched_core_count;
static struct cpumask sched_core_mask;

static void sched_core_lock(int cpu, unsigned long *flags)
{
    const struct cpumask *smt_mask = cpu_smt_mask(cpu);
    int t, i = 0;

    local_irq_save(*flags);
    for_each_cpu(t, smt_mask)
            raw_spin_lock_nested(&cpu_rq(t)->__lock, i++);
}

static void sched_core_unlock(int cpu, unsigned long *flags)
{
    const struct cpumask *smt_mask = cpu_smt_mask(cpu);
    int t;

    for_each_cpu(t, smt_mask)
            raw_spin_unlock(&cpu_rq(t)->__lock);
    local_irq_restore(*flags);
}

static void __sched_core_flip(bool enabled)
{
    unsigned long flags;
    int cpu, t;

    pr_fn_start_on(stack_depth);

    cpus_read_lock();

    /*
     * Toggle the online cores, one by one.
     */
    cpumask_copy(&sched_core_mask, cpu_online_mask);
    pr_view_on(stack_depth, "%20s : %X\n", cpu_online_mask->bits[0]);
    pr_view_on(stack_depth, "%20s : %X\n", sched_core_mask.bits[0]);

    for_each_cpu(cpu, &sched_core_mask) {
        pr_view_on(stack_depth, "%30s : %d\n", cpu);
        const struct cpumask *smt_mask = cpu_smt_mask(cpu);

        pr_view_on(stack_depth, "%30s : %X\n", smt_mask->bits[0]);

        sched_core_lock(cpu, &flags);

        for_each_cpu(t, smt_mask)
                cpu_rq(t)->core_enabled = enabled;

        cpu_rq(cpu)->core->core_forceidle_start = 0;

        sched_core_unlock(cpu, &flags);

        cpumask_andnot(&sched_core_mask, &sched_core_mask, smt_mask);
        pr_view_on(stack_depth, "%30s : %X\n", sched_core_mask.bits[0]);
    }

    /*
     * Toggle the offline CPUs.
     */
    cpumask_copy(&sched_core_mask, cpu_possible_mask);
    cpumask_andnot(&sched_core_mask, &sched_core_mask, cpu_online_mask);

    for_each_cpu(cpu, &sched_core_mask)
            cpu_rq(cpu)->core_enabled = enabled;

    cpus_read_unlock();

    pr_fn_end_on(stack_depth);
}

static void sched_core_assert_empty(void)
{
    int cpu;

    for_each_possible_cpu(cpu)
            WARN_ON_ONCE(!RB_EMPTY_ROOT(&cpu_rq(cpu)->core_tree));
}

static void __sched_core_enable(void)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%40s : %d\n", __sched_core_enabled.key.enabled.counter);

    static_branch_enable(&__sched_core_enabled);

    pr_view_on(stack_depth, "%40s : %d\n", __sched_core_enabled.key.enabled.counter);
    /*
     * Ensure all previous instances of raw_spin_rq_*lock() have finished
     * and future ones will observe !sched_core_disabled().
     */
    synchronize_rcu();
    __sched_core_flip(true);
    sched_core_assert_empty();

    pr_fn_end_on(stack_depth);
}

static void __sched_core_disable(void)
{
    sched_core_assert_empty();
    __sched_core_flip(false);
    static_branch_disable(&__sched_core_enabled);
}

void sched_core_get(void)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%30s : %d\n", sched_core_count.counter);

    //if (atomic_inc_not_zero(&sched_core_count))
    //        return;
    if (sched_core_count.counter > 0) {
        atomic_inc(&sched_core_count);
        pr_view_on(stack_depth, "%30s : %d\n", sched_core_count.counter);
        return;
    }

    mutex_lock(&sched_core_mutex);
    if (!atomic_read(&sched_core_count))
        __sched_core_enable();

    smp_mb__before_atomic();
    atomic_inc(&sched_core_count);
    mutex_unlock(&sched_core_mutex);

    pr_view_on(stack_depth, "%30s : %d\n", sched_core_count.counter);
    pr_fn_end_on(stack_depth);
}

static void __sched_core_put(struct work_struct *work)
{
    /*
        if (atomic_dec_and_mutex_lock(&sched_core_count, &sched_core_mutex)) {
            __sched_core_disable();
            mutex_unlock(&sched_core_mutex);
        }
    */
    atomic_dec(&sched_core_count);
    if (sched_core_count.counter == 0)
        __sched_core_disable();
}

void sched_core_put(void)
{
    static DECLARE_WORK(_work, __sched_core_put);

    /*
         * "There can be only one"
         *
         * Either this is the last one, or we don't actually need to do any
         * 'work'. If it is the last *again*, we rely on
         * WORK_STRUCT_PENDING_BIT.
         */
    //if (!atomic_add_unless(&sched_core_count, -1, 1))
    //        schedule_work(&_work);
    atomic_add(-1, &sched_core_count);
    if (sched_core_count.counter == 1)
        schedule_work(&_work);
}

#else /* !CONFIG_SCHED_CORE */

static inline void sched_core_enqueue(struct rq *rq, struct task_struct *p) { }
static inline void
sched_core_dequeue(struct rq *rq, struct task_struct *p, int flags) { }

#endif /* CONFIG_SCHED_CORE */




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
//74
/*
 * __task_rq_lock - lock the rq @p resides on.
 */
struct rq *__task_rq_lock(struct task_struct *p, struct rq_flags *rf)
    __acquires(rq->lock)
{
    struct rq *rq;

    lockdep_assert_held(&p->pi_lock);

    for (;;) {
        rq = task_rq(p);
        raw_spin_lock(&rq->lock);
        if (likely(rq == task_rq(p) && !task_on_rq_migrating(p))) {
            rq_pin_lock(rq, rf);
            return rq;
        }
        raw_spin_unlock(&rq->lock);

        while (unlikely(task_on_rq_migrating(p)))
            cpu_relax();
    }
}

/*
 * task_rq_lock - lock p->pi_lock and lock the rq @p resides on.
 */
struct rq *task_rq_lock(struct task_struct *p, struct rq_flags *rf)
    __acquires(p->pi_lock)
    __acquires(rq->lock)
{
    struct rq *rq;

    for (;;) {
        raw_spin_lock_irqsave(&p->pi_lock, rf->flags);
        rq = task_rq(p);
        raw_spin_lock(&rq->lock);
        /*
         *	move_queued_task()		task_rq_lock()
         *
         *	ACQUIRE (rq->lock)
         *	[S] ->on_rq = MIGRATING		[L] rq = task_rq()
         *	WMB (__set_task_cpu())		ACQUIRE (rq->lock);
         *	[S] ->cpu = new_cpu		[L] task_rq()
         *					[L] ->on_rq
         *	RELEASE (rq->lock)
         *
         * If we observe the old CPU in task_rq_lock(), the acquire of
         * the old rq->lock will fully serialize against the stores.
         *
         * If we observe the new CPU in task_rq_lock(), the address
         * dependency headed by '[L] rq = task_rq()' and the acquire
         * will pair with the WMB to ensure we then also see migrating.
         */
        if (likely(rq == task_rq(p) && !task_on_rq_migrating(p))) {
            rq_pin_lock(rq, rf);
            return rq;
        }
        raw_spin_unlock(&rq->lock);
        raw_spin_unlock_irqrestore(&p->pi_lock, rf->flags);

        while (unlikely(task_on_rq_migrating(p)))
            cpu_relax();
    }
}

/*
 * RQ-clock updating methods:
 */

static void update_rq_clock_task(struct rq *rq, s64 delta)
{
    pr_fn_start_on(stack_depth);

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

    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock_task);

#ifdef CONFIG_HAVE_SCHED_AVG_IRQ
    if ((irq_delta + steal) && sched_feat(NONTASK_CAPACITY))
        update_irq_load_avg(rq, irq_delta + steal);
#endif
    update_rq_clock_pelt(rq, delta);

    pr_fn_end_on(stack_depth);
}

void update_rq_clock(struct rq *rq)
{
    s64 delta;

    pr_fn_start_on(stack_depth);

    lockdep_assert_held(&rq->lock);

    if (rq->clock_update_flags & RQCF_ACT_SKIP)
        return;

#ifdef CONFIG_SCHED_DEBUG
    if (sched_feat(WARN_DOUBLE_CLOCK))
        SCHED_WARN_ON(rq->clock_update_flags & RQCF_UPDATED);
    rq->clock_update_flags |= RQCF_UPDATED;
#endif

    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock);
    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock_task);
    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock_pelt);

    delta = sched_clock_cpu(cpu_of(rq)) - rq->clock;

    pr_view_on(stack_depth, "%20s : %lld\n", delta);

    if (delta < 0)
        return;
    rq->clock += delta;
    update_rq_clock_task(rq, delta);

    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock);
    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock_task);
    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock_pelt);
    pr_fn_end_on(stack_depth);
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
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %p\n", rq->curr);
    if (!rq->curr)
        return;

    struct task_struct *curr = rq->curr;
    int cpu;

    lockdep_assert_held(&rq->lock);

    if (test_tsk_need_resched(curr))
        return;

    cpu = cpu_of(rq);

    pr_view_on(stack_depth, "%20s : %d\n", cpu);
    pr_view_on(stack_depth, "%20s : %d\n", smp_processor_id());

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
#endif

    pr_fn_end_on(stack_depth);
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







//701 lines
#if defined(CONFIG_RT_GROUP_SCHED) || (defined(CONFIG_FAIR_GROUP_SCHED) && \
            (defined(CONFIG_SMP) || defined(CONFIG_CFS_BANDWIDTH)))
/*
 * Iterate task_group tree rooted at *from, calling @down when first entering a
 * node and @up when leaving it for the final time.
 *
 * Caller must hold rcu_lock or sufficient equivalent.
 */
int walk_tg_tree_from(struct task_group *from,
                 tg_visitor down, tg_visitor up, void *data)
{
    struct task_group *parent, *child;
    int ret;

    pr_fn_start_on(stack_depth);

    parent = from;

down:
    pr_view_on(stack_depth, "%10s : down: %p\n", parent);

    ret = (*down)(parent, data);
    if (ret)
        goto out;
    list_for_each_entry_rcu(child, &parent->children, siblings) {
        parent = child;
        goto down;

up:
        continue;
    }
    ret = (*up)(parent, data);
    if (ret || parent == from)
        goto out;

    child = parent;
    parent = parent->parent;
    pr_view_on(stack_depth, "%10s : up: %p\n", parent);
    if (parent)
        goto up;
out:

    pr_view_on(stack_depth, "%10s : %d\n", ret);
    pr_fn_end_on(stack_depth);
    return ret;
}

int tg_nop(struct task_group *tg, void *data)
{
    return 0;
}
#endif
//746
static void set_load_weight(struct task_struct *p, bool update_load)
{
    pr_fn_start_on(stack_depth);

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

    pr_view_on(stack_depth, "%30s : %d\n", prio);
    pr_view_on(stack_depth, "%30s : %p\n", (void*)&p->se.load);
    pr_view_on(stack_depth, "%30s : %lu\n", load->weight);
    pr_view_on(stack_depth, "%30s : %u\n", load->inv_weight);
    pr_view_on(stack_depth, "%30s : %lu\n", p->se.runnable_weight);

    pr_fn_end_on(stack_depth);
}
//774
#ifdef CONFIG_UCLAMP_TASK
/*
 * Serializes updates of utilization clamp values
 *
 * The (slow-path) user-space triggers utilization clamp value updates which
 * can require updates on (fast-path) scheduler's data structures used to
 * support enqueue/dequeue operations.
 * While the per-CPU rq lock protects fast-path update operations, user-space
 * requests are serialized using a mutex to reduce the risk of conflicting
 * updates or API abuses.
 */
static DEFINE_MUTEX(uclamp_mutex);

/* Max allowed minimum utilization */
unsigned int sysctl_sched_uclamp_util_min = SCHED_CAPACITY_SCALE;

/* Max allowed maximum utilization */
unsigned int sysctl_sched_uclamp_util_max = SCHED_CAPACITY_SCALE;

/* All clamps are required to be less or equal than these values */
static struct uclamp_se uclamp_default[UCLAMP_CNT];

/* Integer rounded range for each bucket */
#define UCLAMP_BUCKET_DELTA DIV_ROUND_CLOSEST(SCHED_CAPACITY_SCALE, UCLAMP_BUCKETS)

#define for_each_clamp_id(clamp_id) \
    for ((clamp_id) = 0; (clamp_id) < UCLAMP_CNT; (clamp_id)++)

static inline unsigned int uclamp_bucket_id(unsigned int clamp_value)
{
    return clamp_value / UCLAMP_BUCKET_DELTA;
}

static inline unsigned int uclamp_bucket_base_value(unsigned int clamp_value)
{
    return UCLAMP_BUCKET_DELTA * uclamp_bucket_id(clamp_value);
}

static inline enum uclamp_id uclamp_none(enum uclamp_id clamp_id)
{
    if (clamp_id == UCLAMP_MIN)
        return 0;
    return SCHED_CAPACITY_SCALE;
}

static inline void uclamp_se_set(struct uclamp_se *uc_se,
                 unsigned int value, bool user_defined)
{
    uc_se->value = value;
    uc_se->bucket_id = uclamp_bucket_id(value);
    uc_se->user_defined = user_defined;
}

static inline unsigned int
uclamp_idle_value(struct rq *rq, enum uclamp_id clamp_id,
          unsigned int clamp_value)
{
    /*
     * Avoid blocked utilization pushing up the frequency when we go
     * idle (which drops the max-clamp) by retaining the last known
     * max-clamp.
     */
    if (clamp_id == UCLAMP_MAX) {
        rq->uclamp_flags |= UCLAMP_FLAG_IDLE;
        return clamp_value;
    }

    return uclamp_none(UCLAMP_MIN);
}

static inline void uclamp_idle_reset(struct rq *rq, enum uclamp_id clamp_id,
                     unsigned int clamp_value)
{
    /* Reset max-clamp retention only on idle exit */
    if (!(rq->uclamp_flags & UCLAMP_FLAG_IDLE))
        return;

    WRITE_ONCE(rq->uclamp[clamp_id].value, clamp_value);
}

static inline
enum uclamp_id uclamp_rq_max_value(struct rq *rq, enum uclamp_id clamp_id,
                   unsigned int clamp_value)
{
    struct uclamp_bucket *bucket = rq->uclamp[clamp_id].bucket;
    int bucket_id = UCLAMP_BUCKETS - 1;

    /*
     * Since both min and max clamps are max aggregated, find the
     * top most bucket with tasks in.
     */
    for ( ; bucket_id >= 0; bucket_id--) {
        if (!bucket[bucket_id].tasks)
            continue;
        return bucket[bucket_id].value;
    }

    /* No tasks -- default clamp values */
    return uclamp_idle_value(rq, clamp_id, clamp_value);
}

static inline struct uclamp_se
uclamp_tg_restrict(struct task_struct *p, enum uclamp_id clamp_id)
{
    struct uclamp_se uc_req = p->uclamp_req[clamp_id];
#ifdef CONFIG_UCLAMP_TASK_GROUP
    struct uclamp_se uc_max;

    /*
     * Tasks in autogroups or root task group will be
     * restricted by system defaults.
     */
    if (task_group_is_autogroup(task_group(p)))
        return uc_req;
    if (task_group(p) == &root_task_group)
        return uc_req;

    uc_max = task_group(p)->uclamp[clamp_id];
    if (uc_req.value > uc_max.value || !uc_req.user_defined)
        return uc_max;
#endif

    return uc_req;
}

/*
 * The effective clamp bucket index of a task depends on, by increasing
 * priority:
 * - the task specific clamp value, when explicitly requested from userspace
 * - the task group effective clamp value, for tasks not either in the root
 *   group or in an autogroup
 * - the system default clamp value, defined by the sysadmin
 */
static inline struct uclamp_se
uclamp_eff_get(struct task_struct *p, enum uclamp_id clamp_id)
{
    struct uclamp_se uc_req = uclamp_tg_restrict(p, clamp_id);
    struct uclamp_se uc_max = uclamp_default[clamp_id];

    /* System default restrictions always apply */
    if (unlikely(uc_req.value > uc_max.value))
        return uc_max;

    return uc_req;
}

enum uclamp_id uclamp_eff_value(struct task_struct *p, enum uclamp_id clamp_id)
{
    struct uclamp_se uc_eff;

    /* Task currently refcounted: use back-annotated (effective) value */
    if (p->uclamp[clamp_id].active)
        return p->uclamp[clamp_id].value;

    uc_eff = uclamp_eff_get(p, clamp_id);

    return uc_eff.value;
}

/*
 * When a task is enqueued on a rq, the clamp bucket currently defined by the
 * task's uclamp::bucket_id is refcounted on that rq. This also immediately
 * updates the rq's clamp value if required.
 *
 * Tasks can have a task-specific value requested from user-space, track
 * within each bucket the maximum value for tasks refcounted in it.
 * This "local max aggregation" allows to track the exact "requested" value
 * for each bucket when all its RUNNABLE tasks require the same clamp.
 */
static inline void uclamp_rq_inc_id(struct rq *rq, struct task_struct *p,
                    enum uclamp_id clamp_id)
{
    struct uclamp_rq *uc_rq = &rq->uclamp[clamp_id];
    struct uclamp_se *uc_se = &p->uclamp[clamp_id];
    struct uclamp_bucket *bucket;

    lockdep_assert_held(&rq->lock);

    /* Update task effective clamp */
    p->uclamp[clamp_id] = uclamp_eff_get(p, clamp_id);

    bucket = &uc_rq->bucket[uc_se->bucket_id];
    bucket->tasks++;
    uc_se->active = true;

    uclamp_idle_reset(rq, clamp_id, uc_se->value);

    /*
     * Local max aggregation: rq buckets always track the max
     * "requested" clamp value of its RUNNABLE tasks.
     */
    if (bucket->tasks == 1 || uc_se->value > bucket->value)
        bucket->value = uc_se->value;

    if (uc_se->value > READ_ONCE(uc_rq->value))
        WRITE_ONCE(uc_rq->value, uc_se->value);
}

/*
 * When a task is dequeued from a rq, the clamp bucket refcounted by the task
 * is released. If this is the last task reference counting the rq's max
 * active clamp value, then the rq's clamp value is updated.
 *
 * Both refcounted tasks and rq's cached clamp values are expected to be
 * always valid. If it's detected they are not, as defensive programming,
 * enforce the expected state and warn.
 */
static inline void uclamp_rq_dec_id(struct rq *rq, struct task_struct *p,
                    enum uclamp_id clamp_id)
{
    struct uclamp_rq *uc_rq = &rq->uclamp[clamp_id];
    struct uclamp_se *uc_se = &p->uclamp[clamp_id];
    struct uclamp_bucket *bucket;
    unsigned int bkt_clamp;
    unsigned int rq_clamp;

    lockdep_assert_held(&rq->lock);

    bucket = &uc_rq->bucket[uc_se->bucket_id];
    SCHED_WARN_ON(!bucket->tasks);
    if (likely(bucket->tasks))
        bucket->tasks--;
    uc_se->active = false;

    /*
     * Keep "local max aggregation" simple and accept to (possibly)
     * overboost some RUNNABLE tasks in the same bucket.
     * The rq clamp bucket value is reset to its base value whenever
     * there are no more RUNNABLE tasks refcounting it.
     */
    if (likely(bucket->tasks))
        return;

    rq_clamp = READ_ONCE(uc_rq->value);
    /*
     * Defensive programming: this should never happen. If it happens,
     * e.g. due to future modification, warn and fixup the expected value.
     */
    SCHED_WARN_ON(bucket->value > rq_clamp);
    if (bucket->value >= rq_clamp) {
        bkt_clamp = uclamp_rq_max_value(rq, clamp_id, uc_se->value);
        WRITE_ONCE(uc_rq->value, bkt_clamp);
    }
}

static inline void uclamp_rq_inc(struct rq *rq, struct task_struct *p)
{
    enum uclamp_id clamp_id;

    if (unlikely(!p->sched_class->uclamp_enabled))
        return;

    for_each_clamp_id(clamp_id)
        uclamp_rq_inc_id(rq, p, clamp_id);

    /* Reset clamp idle holding when there is one RUNNABLE task */
    if (rq->uclamp_flags & UCLAMP_FLAG_IDLE)
        rq->uclamp_flags &= ~UCLAMP_FLAG_IDLE;
}

static inline void uclamp_rq_dec(struct rq *rq, struct task_struct *p)
{
    enum uclamp_id clamp_id;

    if (unlikely(!p->sched_class->uclamp_enabled))
        return;

    for_each_clamp_id(clamp_id)
        uclamp_rq_dec_id(rq, p, clamp_id);
}

static inline void
uclamp_update_active(struct task_struct *p, enum uclamp_id clamp_id)
{
    struct rq_flags rf;
    struct rq *rq;

    /*
     * Lock the task and the rq where the task is (or was) queued.
     *
     * We might lock the (previous) rq of a !RUNNABLE task, but that's the
     * price to pay to safely serialize util_{min,max} updates with
     * enqueues, dequeues and migration operations.
     * This is the same locking schema used by __set_cpus_allowed_ptr().
     */
    rq = task_rq_lock(p, &rf);

    /*
     * Setting the clamp bucket is serialized by task_rq_lock().
     * If the task is not yet RUNNABLE and its task_struct is not
     * affecting a valid clamp bucket, the next time it's enqueued,
     * it will already see the updated clamp bucket value.
     */
    if (p->uclamp[clamp_id].active) {
        uclamp_rq_dec_id(rq, p, clamp_id);
        uclamp_rq_inc_id(rq, p, clamp_id);
    }

    task_rq_unlock(rq, p, &rf);
}

#ifdef CONFIG_UCLAMP_TASK_GROUP
static inline void
uclamp_update_active_tasks(struct cgroup_subsys_state *css,
               unsigned int clamps)
{
    enum uclamp_id clamp_id;
    struct css_task_iter it;
    struct task_struct *p;

    css_task_iter_start(css, 0, &it);
    while ((p = css_task_iter_next(&it))) {
        for_each_clamp_id(clamp_id) {
            if ((0x1 << clamp_id) & clamps)
                uclamp_update_active(p, clamp_id);
        }
    }
    css_task_iter_end(&it);
}

static void cpu_util_update_eff(struct cgroup_subsys_state *css);
static void uclamp_update_root_tg(void)
{
    struct task_group *tg = &root_task_group;

    uclamp_se_set(&tg->uclamp_req[UCLAMP_MIN],
              sysctl_sched_uclamp_util_min, false);
    uclamp_se_set(&tg->uclamp_req[UCLAMP_MAX],
              sysctl_sched_uclamp_util_max, false);

    rcu_read_lock();
    cpu_util_update_eff(&root_task_group.css);
    rcu_read_unlock();
}
#else
static void uclamp_update_root_tg(void) { }
#endif

int sysctl_sched_uclamp_handler(struct ctl_table *table, int write,
                void __user *buffer, size_t *lenp,
                loff_t *ppos)
{
    bool update_root_tg = false;
    int old_min, old_max;
    int result;

    mutex_lock(&uclamp_mutex);
    old_min = sysctl_sched_uclamp_util_min;
    old_max = sysctl_sched_uclamp_util_max;

    result = proc_dointvec(table, write, buffer, lenp, ppos);
    if (result)
        goto undo;
    if (!write)
        goto done;

    if (sysctl_sched_uclamp_util_min > sysctl_sched_uclamp_util_max ||
        sysctl_sched_uclamp_util_max > SCHED_CAPACITY_SCALE) {
        result = -EINVAL;
        goto undo;
    }

    if (old_min != sysctl_sched_uclamp_util_min) {
        uclamp_se_set(&uclamp_default[UCLAMP_MIN],
                  sysctl_sched_uclamp_util_min, false);
        update_root_tg = true;
    }
    if (old_max != sysctl_sched_uclamp_util_max) {
        uclamp_se_set(&uclamp_default[UCLAMP_MAX],
                  sysctl_sched_uclamp_util_max, false);
        update_root_tg = true;
    }

    if (update_root_tg)
        uclamp_update_root_tg();

    /*
     * We update all RUNNABLE tasks only when task groups are in use.
     * Otherwise, keep it simple and do just a lazy update at each next
     * task enqueue time.
     */

    goto done;

undo:
    sysctl_sched_uclamp_util_min = old_min;
    sysctl_sched_uclamp_util_max = old_max;
done:
    mutex_unlock(&uclamp_mutex);

    return result;
}

static int uclamp_validate(struct task_struct *p,
               const struct sched_attr *attr)
{
    unsigned int lower_bound = p->uclamp_req[UCLAMP_MIN].value;
    unsigned int upper_bound = p->uclamp_req[UCLAMP_MAX].value;

    if (attr->sched_flags & SCHED_FLAG_UTIL_CLAMP_MIN)
        lower_bound = attr->sched_util_min;
    if (attr->sched_flags & SCHED_FLAG_UTIL_CLAMP_MAX)
        upper_bound = attr->sched_util_max;

    if (lower_bound > upper_bound)
        return -EINVAL;
    if (upper_bound > SCHED_CAPACITY_SCALE)
        return -EINVAL;

    return 0;
}

static void __setscheduler_uclamp(struct task_struct *p,
                  const struct sched_attr *attr)
{
    enum uclamp_id clamp_id;

    /*
     * On scheduling class change, reset to default clamps for tasks
     * without a task-specific value.
     */
    for_each_clamp_id(clamp_id) {
        struct uclamp_se *uc_se = &p->uclamp_req[clamp_id];
        unsigned int clamp_value = uclamp_none(clamp_id);

        /* Keep using defined clamps across class changes */
        if (uc_se->user_defined)
            continue;

        /* By default, RT tasks always get 100% boost */
        if (unlikely(rt_task(p) && clamp_id == UCLAMP_MIN))
            clamp_value = uclamp_none(UCLAMP_MAX);

        uclamp_se_set(uc_se, clamp_value, false);
    }

    if (likely(!(attr->sched_flags & SCHED_FLAG_UTIL_CLAMP)))
        return;

    if (attr->sched_flags & SCHED_FLAG_UTIL_CLAMP_MIN) {
        uclamp_se_set(&p->uclamp_req[UCLAMP_MIN],
                  attr->sched_util_min, true);
    }

    if (attr->sched_flags & SCHED_FLAG_UTIL_CLAMP_MAX) {
        uclamp_se_set(&p->uclamp_req[UCLAMP_MAX],
                  attr->sched_util_max, true);
    }
}

static void uclamp_fork(struct task_struct *p)
{
    enum uclamp_id clamp_id;

    for_each_clamp_id(clamp_id)
        p->uclamp[clamp_id].active = false;

    if (likely(!p->sched_reset_on_fork))
        return;

    for_each_clamp_id(clamp_id) {
        unsigned int clamp_value = uclamp_none(clamp_id);

        /* By default, RT tasks always get 100% boost */
        if (unlikely(rt_task(p) && clamp_id == UCLAMP_MIN))
            clamp_value = uclamp_none(UCLAMP_MAX);

        uclamp_se_set(&p->uclamp_req[clamp_id], clamp_value, false);
    }
}

static void __init init_uclamp(void)
{
    struct uclamp_se uc_max = {};
    enum uclamp_id clamp_id;
    int cpu;

    mutex_init(&uclamp_mutex);

    for_each_possible_cpu(cpu) {
        memset(&cpu_rq(cpu)->uclamp, 0, sizeof(struct uclamp_rq));
        cpu_rq(cpu)->uclamp_flags = 0;
    }

    for_each_clamp_id(clamp_id) {
        uclamp_se_set(&init_task.uclamp_req[clamp_id],
                  uclamp_none(clamp_id), false);
    }

    /* System defaults allow max clamp values for both indexes */
    uclamp_se_set(&uc_max, uclamp_none(UCLAMP_MAX), false);
    for_each_clamp_id(clamp_id) {
        uclamp_default[clamp_id] = uc_max;
#ifdef CONFIG_UCLAMP_TASK_GROUP
        root_task_group.uclamp_req[clamp_id] = uc_max;
        root_task_group.uclamp[clamp_id] = uc_max;
#endif
    }
}

#else /* CONFIG_UCLAMP_TASK */
static inline void uclamp_rq_inc(struct rq *rq, struct task_struct *p) { }
static inline void uclamp_rq_dec(struct rq *rq, struct task_struct *p) { }
static inline int uclamp_validate(struct task_struct *p,
                  const struct sched_attr *attr)
{
    return -EOPNOTSUPP;
}
static void __setscheduler_uclamp(struct task_struct *p,
                  const struct sched_attr *attr) { }
static inline void uclamp_fork(struct task_struct *p) { }
static inline void init_uclamp(void) { }
#endif /* CONFIG_UCLAMP_TASK */
//1288
static inline void enqueue_task(struct rq *rq, struct task_struct *p, int flags)
{
    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%10s : 0x%X\n", flags);

    if (!(flags & ENQUEUE_NOCLOCK))
        update_rq_clock(rq);

    if (!(flags & ENQUEUE_RESTORE)) {
        sched_info_queued(rq, p);
        psi_enqueue(p, flags & ENQUEUE_WAKEUP);
    }

    uclamp_rq_inc(rq, p);
    p->sched_class->enqueue_task(rq, p, flags);

    pr_fn_end_on(stack_depth);
}
//1302
static inline void dequeue_task(struct rq *rq, struct task_struct *p, int flags)
{
    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)p);
    pr_view_on(stack_depth, "%20s : 0x%X\n", flags);

    if (!(flags & DEQUEUE_NOCLOCK))
        update_rq_clock(rq);

    if (!(flags & DEQUEUE_SAVE)) {
        sched_info_dequeued(rq, p);
        psi_dequeue(p, flags & DEQUEUE_SLEEP);
    }

    uclamp_rq_dec(rq, p);
    p->sched_class->dequeue_task(rq, p, flags);

    pr_fn_end_on(stack_depth);
}

void activate_task(struct rq *rq, struct task_struct *p, int flags)
{
    pr_fn_start_on(stack_depth);

    if (task_contributes_to_load(p))
        rq->nr_uninterruptible--;

    pr_view_on(stack_depth, "%20s : %d\n", task_cpu(p));
    pr_view_on(stack_depth, "%20s : %d\n", cpu_of(rq));

    enqueue_task(rq, p, flags);

    p->on_rq = TASK_ON_RQ_QUEUED;

    pr_fn_end_on(stack_depth);
}

void deactivate_task(struct rq *rq, struct task_struct *p, int flags)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %d\n",  p->on_rq);
    pr_view_on(stack_depth, "%20s : 0x%X\n", flags);

    p->on_rq = (flags & DEQUEUE_SLEEP) ? 0 : TASK_ON_RQ_MIGRATING;

    pr_view_on(stack_depth, "%20s : %d\n",  p->on_rq);

    if (task_contributes_to_load(p))
        rq->nr_uninterruptible++;

    dequeue_task(rq, p, flags);

    pr_fn_end_on(stack_depth);
}
//1336
/*
 * __normal_prio - return the priority that is based on the static prio
 */
static inline int __normal_prio(struct task_struct *p)
{
    return p->static_prio;
}

/*
 * Calculate the expected normal priority: i.e. priority
 * without taking RT-inheritance into account. Might be
 * boosted by interactivity modifiers. Changes upon fork,
 * setprio syscalls, and whenever the interactivity
 * estimator recalculates.
 */
static inline int normal_prio(struct task_struct *p)
{
    int prio;

    if (task_has_dl_policy(p))
        prio = MAX_DL_PRIO-1;
    else if (task_has_rt_policy(p))
        prio = MAX_RT_PRIO-1 - p->rt_priority;
    else
        prio = __normal_prio(p);
    return prio;
}

/*
 * Calculate the current priority, i.e. the priority
 * taken into account by the scheduler. This value might
 * be boosted by RT tasks, or might be boosted by
 * interactivity modifiers. Will be RT if the task got
 * RT-boosted. If not then it returns p->normal_prio.
 */
static int effective_prio(struct task_struct *p)
{
    p->normal_prio = normal_prio(p);
    /*
     * If we are RT tasks or we were boosted to RT priority,
     * keep the priority unchanged. Otherwise, update priority
     * to the normal priority:
     */
    if (!rt_prio(p->prio))
        return p->normal_prio;
    return p->prio;
}

/**
 * task_curr - is this task currently executing on a CPU?
 * @p: the task in question.
 *
 * Return: 1 if the task is currently executing. 0 otherwise.
 */
inline int task_curr(const struct task_struct *p)
{
    return cpu_curr(task_cpu(p)) == p;
}

/*
 * switched_from, switched_to and prio_changed must _NOT_ drop rq->lock,
 * use the balance_callback list if you want balancing.
 *
 * this means any call to check_class_changed() must be followed by a call to
 * balance_callback().
 */
static inline void check_class_changed(struct rq *rq, struct task_struct *p,
                       const struct sched_class *prev_class,
                       int oldprio)
{
    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)p);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)prev_class);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)p->sched_class);
    pr_view_on(stack_depth, "%20s : %d\n", oldprio);
    pr_view_on(stack_depth, "%20s : %d\n", p->prio);

    if (prev_class != p->sched_class) {
        if (prev_class->switched_from)
            prev_class->switched_from(rq, p);

        p->sched_class->switched_to(rq, p);
    } else if (oldprio != p->prio || dl_task(p))
        p->sched_class->prio_changed(rq, p, oldprio);

    pr_fn_end_on(stack_depth);
}

void check_preempt_curr(struct rq *rq, struct task_struct *p, int flags)
{
    pr_fn_start_on(stack_depth);

    const struct sched_class *class;

    pr_view_on(stack_depth, "%20s : %p\n", rq->curr);
    if (!rq->curr) {
        pr_err("rq->curr is NULL.\n");
        return;
    }

    if (p->sched_class == rq->curr->sched_class) {
        rq->curr->sched_class->check_preempt_curr(rq, p, flags);
    } else {
        for_each_class(class) {
            if (class == rq->curr->sched_class)
                break;
            if (class == p->sched_class) {
                resched_curr(rq);
                break;
            }
        }
    }

    /*
     * A queue event has occurred, and we're going to schedule.  In
     * this case, we can save a useless back to back clock update.
     */
    if (task_on_rq_queued(rq->curr) && test_tsk_need_resched(rq->curr))
        rq_clock_skip_update(rq);

    pr_fn_end_on(stack_depth);
}
//1440
#ifdef CONFIG_SMP

static inline bool is_per_cpu_kthread(struct task_struct *p)
{
    if (!(p->flags & PF_KTHREAD))
        return false;

    if (p->nr_cpus_allowed != 1)
        return false;

    return true;
}
//1453
/*
 * Per-CPU kthreads are allowed to run on !active && online CPUs, see
 * __set_cpus_allowed_ptr() and select_fallback_rq().
 */
static inline bool is_cpu_allowed(struct task_struct *p, int cpu)
{
    if (!cpumask_test_cpu(cpu, p->cpus_ptr))
        return false;

    if (is_per_cpu_kthread(p))
        return cpu_online(cpu);

    return cpu_active(cpu);
}
//1468
/*
 * This is how migration works:
 *
 * 1) we invoke migration_cpu_stop() on the target CPU using
 *    stop_one_cpu().
 * 2) stopper starts to run (implicitly forcing the migrated thread
 *    off the CPU)
 * 3) it checks whether the migrated task is still in the wrong runqueue.
 * 4) if it's in the wrong runqueue then the migration thread removes
 *    it and puts it into the right queue.
 * 5) stopper completes and stop_one_cpu() returns and the migration
 *    is done.
 */

/*
 * move_queued_task - move a queued task to new rq.
 *
 * Returns (locked) new rq. Old rq's lock is released.
 */
static struct rq *move_queued_task(struct rq *rq, struct rq_flags *rf,
                   struct task_struct *p, int new_cpu)
{
    lockdep_assert_held(&rq->lock);

    WRITE_ONCE(p->on_rq, TASK_ON_RQ_MIGRATING);
    dequeue_task(rq, p, DEQUEUE_NOCLOCK);
    set_task_cpu(p, new_cpu);
    rq_unlock(rq, rf);

    rq = cpu_rq(new_cpu);

    rq_lock(rq, rf);
    BUG_ON(task_cpu(p) != new_cpu);
    enqueue_task(rq, p, 0);
    p->on_rq = TASK_ON_RQ_QUEUED;
    check_preempt_curr(rq, p, 0);

    return rq;
}

struct migration_arg {
    struct task_struct *task;
    int dest_cpu;
};

/*
 * Move (not current) task off this CPU, onto the destination CPU. We're doing
 * this because either it can't run here any more (set_cpus_allowed()
 * away from this CPU, or CPU going down), or because we're
 * attempting to rebalance this task on exec (sched_exec).
 *
 * So we race with normal scheduler movements, but that's OK, as long
 * as the task is no longer on this CPU.
 */
static struct rq *__migrate_task(struct rq *rq, struct rq_flags *rf,
                 struct task_struct *p, int dest_cpu)
{
    /* Affinity changed (again). */
    if (!is_cpu_allowed(p, dest_cpu))
        return rq;

    update_rq_clock(rq);
    rq = move_queued_task(rq, rf, p, dest_cpu);

    return rq;
}

/*
 * migration_cpu_stop - this will be executed by a highprio stopper thread
 * and performs thread migration by bumping thread off CPU then
 * 'pushing' onto another runqueue.
 */
static int migration_cpu_stop(void *data)
{
    struct migration_arg *arg = data;
    struct task_struct *p = arg->task;
    struct rq *rq = this_rq();
    struct rq_flags rf;

    /*
     * The original target CPU might have gone down and we might
     * be on another CPU but it doesn't matter.
     */
    local_irq_disable();
    /*
     * We need to explicitly wake pending tasks before running
     * __migrate_task() such that we will not miss enforcing cpus_ptr
     * during wakeups, see set_cpus_allowed_ptr()'s TASK_WAKING test.
     */
    sched_ttwu_pending();

    raw_spin_lock(&p->pi_lock);
    rq_lock(rq, &rf);
    /*
     * If task_rq(p) != rq, it cannot be migrated here, because we're
     * holding rq->lock, if p->on_rq == 0 it cannot get enqueued because
     * we're holding p->pi_lock.
     */
    if (task_rq(p) == rq) {
        if (task_on_rq_queued(p))
            rq = __migrate_task(rq, &rf, p, arg->dest_cpu);
        else
            p->wake_cpu = arg->dest_cpu;
    }
    rq_unlock(rq, &rf);
    raw_spin_unlock(&p->pi_lock);

    local_irq_enable();
    return 0;
}
//1579
/*
 * sched_class::set_cpus_allowed must do the below, but is not required to
 * actually call this function.
 */
void set_cpus_allowed_common(struct task_struct *p, const struct cpumask *new_mask)
{
    cpumask_copy(&p->cpus_mask, new_mask);
    p->nr_cpus_allowed = cpumask_weight(new_mask);
}

void do_set_cpus_allowed(struct task_struct *p, const struct cpumask *new_mask)
{
    struct rq *rq = task_rq(p);
    bool queued, running;

    lockdep_assert_held(&p->pi_lock);

    queued = task_on_rq_queued(p);
    running = task_current(rq, p);

    if (queued) {
        /*
         * Because __kthread_bind() calls this on blocked tasks without
         * holding rq->lock.
         */
        lockdep_assert_held(&rq->lock);
        dequeue_task(rq, p, DEQUEUE_SAVE | DEQUEUE_NOCLOCK);
    }
    if (running)
        put_prev_task(rq, p);

    p->sched_class->set_cpus_allowed(p, new_mask);

    if (queued)
        enqueue_task(rq, p, ENQUEUE_RESTORE | ENQUEUE_NOCLOCK);
    if (running)
        set_next_task(rq, p);
}

/*
 * Change a given task's CPU affinity. Migrate the thread to a
 * proper CPU and schedule it away if the CPU it's executing on
 * is removed from the allowed bitmask.
 *
 * NOTE: the caller must have a valid reference to the task, the
 * task must not exit() & deallocate itself prematurely. The
 * call is not atomic; no spinlocks may be held.
 */
static int __set_cpus_allowed_ptr(struct task_struct *p,
                  const struct cpumask *new_mask, bool check)
{
    const struct cpumask *cpu_valid_mask = cpu_active_mask;
    unsigned int dest_cpu;
    struct rq_flags rf;
    struct rq *rq;
    int ret = 0;

    rq = task_rq_lock(p, &rf);
    update_rq_clock(rq);

    if (p->flags & PF_KTHREAD) {
        /*
         * Kernel threads are allowed on online && !active CPUs
         */
        cpu_valid_mask = cpu_online_mask;
    }

    /*
     * Must re-check here, to close a race against __kthread_bind(),
     * sched_setaffinity() is not guaranteed to observe the flag.
     */
    if (check && (p->flags & PF_NO_SETAFFINITY)) {
        ret = -EINVAL;
        goto out;
    }

    if (cpumask_equal(p->cpus_ptr, new_mask))
        goto out;

    dest_cpu = cpumask_any_and(cpu_valid_mask, new_mask);
    if (dest_cpu >= nr_cpu_ids) {
        ret = -EINVAL;
        goto out;
    }

    do_set_cpus_allowed(p, new_mask);

    if (p->flags & PF_KTHREAD) {
        /*
         * For kernel threads that do indeed end up on online &&
         * !active we want to ensure they are strict per-CPU threads.
         */
        WARN_ON(cpumask_intersects(new_mask, cpu_online_mask) &&
            !cpumask_intersects(new_mask, cpu_active_mask) &&
            p->nr_cpus_allowed != 1);
    }

    /* Can the task run on the task's current CPU? If so, we're done */
    if (cpumask_test_cpu(task_cpu(p), new_mask))
        goto out;

    if (task_running(rq, p) || p->state == TASK_WAKING) {
        struct migration_arg arg = { p, dest_cpu };
        /* Need help from migration thread: drop lock and wait. */
        task_rq_unlock(rq, p, &rf);
        //stop_one_cpu(cpu_of(rq), migration_cpu_stop, &arg);
        return 0;
    } else if (task_on_rq_queued(p)) {
        /*
         * OK, since we're going to drop the lock immediately
         * afterwards anyway.
         */
        rq = move_queued_task(rq, &rf, p, dest_cpu);
    }
out:
    task_rq_unlock(rq, p, &rf);

    return ret;
}

int set_cpus_allowed_ptr(struct task_struct *p, const struct cpumask *new_mask)
{
    return __set_cpus_allowed_ptr(p, new_mask, false);
}
EXPORT_SYMBOL_GPL(set_cpus_allowed_ptr);
//1705
void set_task_cpu(struct task_struct *p, unsigned int new_cpu)
{
    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %u\n", new_cpu);

#ifdef CONFIG_SCHED_DEBUG
    /*
     * We should never call set_task_cpu() on a blocked task,
     * ttwu() will sort out the placement.
     */
    WARN_ON_ONCE(p->state != TASK_RUNNING && p->state != TASK_WAKING &&
            !p->on_rq);

    /*
     * Migrating fair class task must have p->on_rq = TASK_ON_RQ_MIGRATING,
     * because schedstat_wait_{start,end} rebase migrating task's wait_start
     * time relying on p->on_rq.
     */
    WARN_ON_ONCE(p->state == TASK_RUNNING &&
             p->sched_class == &fair_sched_class &&
             (p->on_rq && !task_on_rq_migrating(p)));

#ifdef CONFIG_LOCKDEP
    /*
     * The caller should hold either p->pi_lock or rq->lock, when changing
     * a task's CPU. ->pi_lock for waking tasks, rq->lock for runnable tasks.
     *
     * sched_move_task() holds both and thus holding either pins the cgroup,
     * see task_group().
     *
     * Furthermore, all task_rq users should acquire both locks, see
     * task_rq_lock().
     */
    WARN_ON_ONCE(debug_locks && !(lockdep_is_held(&p->pi_lock) ||
                      lockdep_is_held(&task_rq(p)->lock)));
#endif
    /*
     * Clearly, migrating tasks to offline CPUs is a fairly daft thing.
     */
    WARN_ON_ONCE(!cpu_online(new_cpu));
#endif

    //trace_sched_migrate_task(p, new_cpu);

    if (task_cpu(p) != new_cpu) {
        if (p->sched_class->migrate_task_rq)
            p->sched_class->migrate_task_rq(p, new_cpu);
        p->se.nr_migrations++;
        rseq_migrate(p);
        //perf_event_task_migrate(p);
    }

    __set_task_cpu(p, new_cpu);

    pr_fn_end_on(stack_depth);
}
//1757 lines






//2007 lines
/*
 * ->cpus_ptr is protected by both rq->lock and p->pi_lock
 *
 * A few notes on cpu_active vs cpu_online:
 *
 *  - cpu_active must be a subset of cpu_online
 *
 *  - on CPU-up we allow per-CPU kthreads on the online && !active CPU,
 *    see __set_cpus_allowed_ptr(). At this point the newly online
 *    CPU isn't yet part of the sched domains, and balancing will not
 *    see it.
 *
 *  - on CPU-down we clear cpu_active() to mask the sched domains and
 *    avoid the load balancer to place new tasks on the to be removed
 *    CPU. Existing tasks will remain running there and will be taken
 *    off.
 *
 * This means that fallback selection must not select !active CPUs.
 * And can assume that any active CPU must be online. Conversely
 * select_task_rq() below may allow selection of !active CPUs in order
 * to satisfy the above rules.
 */
static int select_fallback_rq(int cpu, struct task_struct *p)
{
    int nid = cpu_to_node(cpu);
    const struct cpumask *nodemask = NULL;
    enum { cpuset, possible, fail } state = cpuset;
    int dest_cpu;

    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%10s : %d\n", cpu);
    pr_view_on(stack_depth, "%10s : %d\n", nid);

    /*
     * If the node that the CPU is on has been offlined, cpu_to_node()
     * will return -1. There is no CPU on the node, and we should
     * select the CPU on the other node.
     */
    if (nid != -1) {
        nodemask = cpumask_of_node(nid);
        pr_view_on(stack_depth, "%20s : 0x%X\n", nodemask->bits[0]);
        pr_view_on(stack_depth, "%20s : 0x%X\n", p->cpus_ptr->bits[0]);

        /* Look for allowed, online CPU in same node. */
        for_each_cpu(dest_cpu, nodemask) {
            if (cpu == dest_cpu) continue;	//Modified for test by JJJ.

            if (!cpu_active(dest_cpu))
                continue;
            if (cpumask_test_cpu(dest_cpu, p->cpus_ptr))
                return dest_cpu;
        }
    }

    for (;;) {
        /* Any allowed, online CPU? */
        for_each_cpu(dest_cpu, p->cpus_ptr) {
            if (!is_cpu_allowed(p, dest_cpu))
                continue;

            goto out;
        }

        /* No more Mr. Nice Guy. */
        switch (state) {
        case cpuset:
            if (IS_ENABLED(CONFIG_CPUSETS)) {
                cpuset_cpus_allowed_fallback(p);
                state = possible;
                break;
            }
            /* Fall-through */
        case possible:
            do_set_cpus_allowed(p, cpu_possible_mask);
            state = fail;
            break;

        case fail:
            BUG();
            break;
        }
    }

out:
    if (state != cpuset) {
        /*
         * Don't tell them about moving exiting tasks or
         * kernel threads (both mm NULL), since they never
         * leave kernel.
         */
        //if (p->mm && printk_ratelimit()) {
        if (p->mm) {
            //printk_deferred("process %d (%s) no longer affine to cpu%d\n",
            printk("process %d (%s) no longer affine to cpu%d\n",
                    task_pid_nr(p), p->comm, cpu);
        }
    }

    pr_fn_end_on(stack_depth);

    return dest_cpu;
}
//2098
/*
 * The caller (fork, wakeup) owns p->pi_lock, ->cpus_ptr is stable.
 */
static inline
int select_task_rq(struct task_struct *p, int cpu, int sd_flags, int wake_flags)
{
    pr_fn_start_on(stack_depth);

    lockdep_assert_held(&p->pi_lock);

    if (p->nr_cpus_allowed > 1)
        cpu = p->sched_class->select_task_rq(p, cpu, sd_flags, wake_flags);
    else
        cpu = cpumask_any(p->cpus_ptr);

    /*
     * In order not to call set_task_cpu() on a blocking task we need
     * to rely on ttwu() to place the task on a valid ->cpus_ptr
     * CPU.
     *
     * Since this is common to all placement strategies, this lives here.
     *
     * [ this allows ->select_task() to simply return task_cpu(p) and
     *   not worry about this generic constraint ]
     */
    if (unlikely(!is_cpu_allowed(p, cpu)))
        cpu = select_fallback_rq(task_cpu(p), p);

    pr_view_on(stack_depth, "%20s : %d(selected)\n", cpu);

    pr_fn_end_on(stack_depth);
    return cpu;
}

static void update_avg(u64 *avg, u64 sample)
{
    s64 diff = sample - *avg;
    *avg += diff >> 3;
}

void sched_set_stop_task(int cpu, struct task_struct *stop)
{
    struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
    struct task_struct *old_stop = cpu_rq(cpu)->stop;

    if (stop) {
        /*
         * Make it appear like a SCHED_FIFO task, its something
         * userspace knows about and won't get confused about.
         *
         * Also, it will make PI more or less work without too
         * much confusion -- but then, stop work should not
         * rely on PI working anyway.
         */
        sched_setscheduler_nocheck(stop, SCHED_FIFO, &param);

        stop->sched_class = &stop_sched_class;
    }

    cpu_rq(cpu)->stop = stop;

    if (old_stop) {
        /*
         * Reset it back to a normal scheduling class so that
         * it can die in pieces.
         */
        old_stop->sched_class = &rt_sched_class;
    }
}


//2163 lines
#else

static inline int __set_cpus_allowed_ptr(struct task_struct *p,
                     const struct cpumask *new_mask, bool check)
{
    return set_cpus_allowed_ptr(p, new_mask);
}

#endif /* CONFIG_SMP */

static void
ttwu_stat(struct task_struct *p, int cpu, int wake_flags)
{
    struct rq *rq;

    if (!schedstat_enabled())
        return;

    rq = this_rq();

#ifdef CONFIG_SMP
    if (cpu == rq->cpu) {
        __schedstat_inc(rq->ttwu_local);
        __schedstat_inc(p->se.statistics.nr_wakeups_local);
    } else {
        struct sched_domain *sd;

        __schedstat_inc(p->se.statistics.nr_wakeups_remote);
        rcu_read_lock();
        for_each_domain(rq->cpu, sd) {
            if (cpumask_test_cpu(cpu, sched_domain_span(sd))) {
                __schedstat_inc(sd->ttwu_wake_remote);
                break;
            }
        }
        rcu_read_unlock();
    }

    if (wake_flags & WF_MIGRATED)
        __schedstat_inc(p->se.statistics.nr_wakeups_migrate);
#endif /* CONFIG_SMP */

    __schedstat_inc(rq->ttwu_count);
    __schedstat_inc(p->se.statistics.nr_wakeups);

    if (wake_flags & WF_SYNC)
        __schedstat_inc(p->se.statistics.nr_wakeups_sync);
}

/*
 * Mark the task runnable and perform wakeup-preemption.
 */
static void ttwu_do_wakeup(struct rq *rq, struct task_struct *p, int wake_flags,
               struct rq_flags *rf)
{
    pr_fn_start_on(stack_depth);

    check_preempt_curr(rq, p, wake_flags);
    p->state = TASK_RUNNING;
    //trace_sched_wakeup(p);

#ifdef CONFIG_SMP
    if (p->sched_class->task_woken) {
        /*
         * Our task @p is fully woken up and running; so its safe to
         * drop the rq->lock, hereafter rq is only used for statistics.
         */
        rq_unpin_lock(rq, rf);
        p->sched_class->task_woken(rq, p);
        rq_repin_lock(rq, rf);
    }

    if (rq->idle_stamp) {
        u64 delta = rq_clock(rq) - rq->idle_stamp;
        u64 max = 2*rq->max_idle_balance_cost;

        update_avg(&rq->avg_idle, delta);

        if (rq->avg_idle > max)
            rq->avg_idle = max;

        rq->idle_stamp = 0;
    }
#endif

    pr_fn_end_on(stack_depth);
}
//2247
static void
ttwu_do_activate(struct rq *rq, struct task_struct *p, int wake_flags,
         struct rq_flags *rf)
{
    int en_flags = ENQUEUE_WAKEUP | ENQUEUE_NOCLOCK;
    pr_fn_start_on(stack_depth);

    lockdep_assert_held(&rq->lock);

#ifdef CONFIG_SMP
    if (p->sched_contributes_to_load)
        rq->nr_uninterruptible--;

    if (wake_flags & WF_MIGRATED)
        en_flags |= ENQUEUE_MIGRATED;
#endif

    activate_task(rq, p, en_flags);
    ttwu_do_wakeup(rq, p, wake_flags, rf);

    pr_fn_end_on(stack_depth);
}
//2267
/*
 * Called in case the task @p isn't fully descheduled from its runqueue,
 * in this case we must do a remote wakeup. Its a 'light' wakeup though,
 * since all we need to do is flip p->state to TASK_RUNNING, since
 * the task is still ->on_rq.
 */
static int ttwu_remote(struct task_struct *p, int wake_flags)
{
    pr_fn_start_on(stack_depth);

    struct rq_flags rf;
    struct rq *rq;
    int ret = 0;

    rq = __task_rq_lock(p, &rf);
    if (task_on_rq_queued(p)) {
        /* check_preempt_curr() may use rq clock */
        update_rq_clock(rq);
        ttwu_do_wakeup(rq, p, wake_flags, &rf);
        ret = 1;
    }
    __task_rq_unlock(rq, &rf);

    pr_fn_end_on(stack_depth);

    return ret;
}
//2291
#ifdef CONFIG_SMP

void sched_ttwu_pending(void)
{
    struct rq *rq = this_rq();
    struct llist_node *llist = llist_del_all(&rq->wake_list);
    struct task_struct *p, *t;
    struct rq_flags rf;

    if (!llist)
        return;

    rq_lock_irqsave(rq, &rf);
    update_rq_clock(rq);

    llist_for_each_entry_safe(p, t, llist, wake_entry)
        ttwu_do_activate(rq, p, p->sched_remote_wakeup ? WF_MIGRATED : 0, &rf);

    rq_unlock_irqrestore(rq, &rf);
}
//2311 lines



//2349 lines
static void ttwu_queue_remote(struct task_struct *p, int cpu, int wake_flags)
{
    struct rq *rq = cpu_rq(cpu);

    p->sched_remote_wakeup = !!(wake_flags & WF_MIGRATED);
#if 0
    if (llist_add(&p->wake_entry, &cpu_rq(cpu)->wake_list)) {
        if (!set_nr_if_polling(rq->idle))
            smp_send_reschedule(cpu);
        else
            trace_sched_wake_idle_without_ipi(cpu);
    }
#endif
}
//2363 lines



//2387 lines
bool cpus_share_cache(int this_cpu, int that_cpu)
{
    return per_cpu(sd_llc_id, this_cpu) == per_cpu(sd_llc_id, that_cpu);
}
#endif /* CONFIG_SMP */

static void ttwu_queue(struct task_struct *p, int cpu, int wake_flags)
{
    struct rq *rq = cpu_rq(cpu);
    struct rq_flags rf;
    pr_fn_start_on(stack_depth);

#if defined(CONFIG_SMP)
    if (sched_feat(TTWU_QUEUE) && !cpus_share_cache(smp_processor_id(), cpu)) {
        sched_clock_cpu(cpu); /* Sync clocks across CPUs */
        ttwu_queue_remote(p, cpu, wake_flags);
        return;
    }
#endif

    rq_lock(rq, &rf);
    update_rq_clock(rq);
    ttwu_do_activate(rq, p, wake_flags, &rf);
    rq_unlock(rq, &rf);

    pr_fn_end_on(stack_depth);
}

/*
 * Notes on Program-Order guarantees on SMP systems.
 *
 *  MIGRATION
 *
 * The basic program-order guarantee on SMP systems is that when a task [t]
 * migrates, all its activity on its old CPU [c0] happens-before any subsequent
 * execution on its new CPU [c1].
 *
 * For migration (of runnable tasks) this is provided by the following means:
 *
 *  A) UNLOCK of the rq(c0)->lock scheduling out task t
 *  B) migration for t is required to synchronize *both* rq(c0)->lock and
 *     rq(c1)->lock (if not at the same time, then in that order).
 *  C) LOCK of the rq(c1)->lock scheduling in task
 *
 * Release/acquire chaining guarantees that B happens after A and C after B.
 * Note: the CPU doing B need not be c0 or c1
 *
 * Example:
 *
 *   CPU0            CPU1            CPU2
 *
 *   LOCK rq(0)->lock
 *   sched-out X
 *   sched-in Y
 *   UNLOCK rq(0)->lock
 *
 *                                   LOCK rq(0)->lock // orders against CPU0
 *                                   dequeue X
 *                                   UNLOCK rq(0)->lock
 *
 *                                   LOCK rq(1)->lock
 *                                   enqueue X
 *                                   UNLOCK rq(1)->lock
 *
 *                   LOCK rq(1)->lock // orders against CPU2
 *                   sched-out Z
 *                   sched-in X
 *                   UNLOCK rq(1)->lock
 *
 *
 *  BLOCKING -- aka. SLEEP + WAKEUP
 *
 * For blocking we (obviously) need to provide the same guarantee as for
 * migration. However the means are completely different as there is no lock
 * chain to provide order. Instead we do:
 *
 *   1) smp_store_release(X->on_cpu, 0)
 *   2) smp_cond_load_acquire(!X->on_cpu)
 *
 * Example:
 *
 *   CPU0 (schedule)  CPU1 (try_to_wake_up) CPU2 (schedule)
 *
 *   LOCK rq(0)->lock LOCK X->pi_lock
 *   dequeue X
 *   sched-out X
 *   smp_store_release(X->on_cpu, 0);
 *
 *                    smp_cond_load_acquire(&X->on_cpu, !VAL);
 *                    X->state = WAKING
 *                    set_task_cpu(X,2)
 *
 *                    LOCK rq(2)->lock
 *                    enqueue X
 *                    X->state = RUNNING
 *                    UNLOCK rq(2)->lock
 *
 *                                          LOCK rq(2)->lock // orders against CPU1
 *                                          sched-out Z
 *                                          sched-in X
 *                                          UNLOCK rq(2)->lock
 *
 *                    UNLOCK X->pi_lock
 *   UNLOCK rq(0)->lock
 *
 *
 * However, for wakeups there is a second guarantee we must provide, namely we
 * must ensure that CONDITION=1 done by the caller can not be reordered with
 * accesses to the task state; see try_to_wake_up() and set_current_state().
 */

/**
 * try_to_wake_up - wake up a thread
 * @p: the thread to be awakened
 * @state: the mask of task states that can be woken
 * @wake_flags: wake modifier flags (WF_*)
 *
 * If (@state & @p->state) @p->state = TASK_RUNNING.
 *
 * If the task was not queued/runnable, also place it back on a runqueue.
 *
 * Atomic against schedule() which would dequeue a task, also see
 * set_current_state().
 *
 * This function executes a full memory barrier before accessing the task
 * state; see set_current_state().
 *
 * Return: %true if @p->state changes (an actual wakeup was done),
 *	   %false otherwise.
 */
static int
try_to_wake_up(struct task_struct *p, unsigned int state, int wake_flags)
{
    pr_fn_start_on(stack_depth);

    unsigned long flags;
    int cpu, success = 0;

    pr_view_on(stack_depth, "%20s : %p\n", p);
    pr_view_on(stack_depth, "%20s : %p\n", current);

    preempt_disable();
    if (p == current) {
        /*
         * We're waking current, this means 'p->on_rq' and 'task_cpu(p)
         * == smp_processor_id()'. Together this means we can special
         * case the whole 'p->on_rq && ttwu_remote()' case below
         * without taking any locks.
         *
         * In particular:
         *  - we rely on Program-Order guarantees for all the ordering,
         *  - we're serialized against set_special_state() by virtue of
         *    it disabling IRQs (this allows not taking ->pi_lock).
         */
        if (!(p->state & state))
            goto out;

        success = 1;
        cpu = task_cpu(p);
        //trace_sched_waking(p);
        p->state = TASK_RUNNING;
        //trace_sched_wakeup(p);
        goto out;
    }

    /*
     * If we are going to wake up a thread waiting for CONDITION we
     * need to ensure that CONDITION=1 done by the caller can not be
     * reordered with p->state check below. This pairs with mb() in
     * set_current_state() the waiting thread does.
     */
    raw_spin_lock_irqsave(&p->pi_lock, flags);
    //smp_mb__after_spinlock();

    pr_view_on(stack_depth, "%20s : 0x%X\n", p->state);
    p->state = TASK_INTERRUPTIBLE;		//as test
    //p->on_rq = TASK_ON_RQ_MIGRATING;	//as test
    p->on_rq = 0;	//as test
    pr_view_on(stack_depth, "%20s : 0x%X\n", state);

    if (!(p->state & state))
        goto unlock;

    //trace_sched_waking(p);

    /* We're going to change ->state: */
    success = 1;
    cpu = task_cpu(p);

    /*
     * Ensure we load p->on_rq _after_ p->state, otherwise it would
     * be possible to, falsely, observe p->on_rq == 0 and get stuck
     * in smp_cond_load_acquire() below.
     *
     * sched_ttwu_pending()			try_to_wake_up()
     *   STORE p->on_rq = 1			  LOAD p->state
     *   UNLOCK rq->lock
     *
     * __schedule() (switch to task 'p')
     *   LOCK rq->lock			  smp_rmb();
     *   smp_mb__after_spinlock();
     *   UNLOCK rq->lock
     *
     * [task p]
     *   STORE p->state = UNINTERRUPTIBLE	  LOAD p->on_rq
     *
     * Pairs with the LOCK+smp_mb__after_spinlock() on rq->lock in
     * __schedule().  See the comment for smp_mb__after_spinlock().
     */
    smp_rmb();
    if (p->on_rq && ttwu_remote(p, wake_flags))
        goto unlock;

#ifdef CONFIG_SMP
    /*
     * Ensure we load p->on_cpu _after_ p->on_rq, otherwise it would be
     * possible to, falsely, observe p->on_cpu == 0.
     *
     * One must be running (->on_cpu == 1) in order to remove oneself
     * from the runqueue.
     *
     * __schedule() (switch to task 'p')	try_to_wake_up()
     *   STORE p->on_cpu = 1		  LOAD p->on_rq
     *   UNLOCK rq->lock
     *
     * __schedule() (put 'p' to sleep)
     *   LOCK rq->lock			  smp_rmb();
     *   smp_mb__after_spinlock();
     *   STORE p->on_rq = 0			  LOAD p->on_cpu
     *
     * Pairs with the LOCK+smp_mb__after_spinlock() on rq->lock in
     * __schedule().  See the comment for smp_mb__after_spinlock().
     */
    smp_rmb();

    /*
     * If the owning (remote) CPU is still in the middle of schedule() with
     * this task as prev, wait until its done referencing the task.
     *
     * Pairs with the smp_store_release() in finish_task().
     *
     * This ensures that tasks getting woken will be fully ordered against
     * their previous state and preserve Program Order.
     */
    smp_cond_load_acquire(&p->on_cpu, !VAL);

    p->sched_contributes_to_load = !!task_contributes_to_load(p);
    p->state = TASK_WAKING;

    if (p->in_iowait) {
        //delayacct_blkio_end(p);
        atomic_dec(&task_rq(p)->nr_iowait);
    }

    pr_view_on(stack_depth, "%20s : %d\n", task_cpu(p));
    pr_view_on(stack_depth, "%20s : %d\n", p->wake_cpu);

    cpu = select_task_rq(p, p->wake_cpu, SD_BALANCE_WAKE, wake_flags);

    pr_view_on(stack_depth, "%20s : %d\n", task_cpu(p));
    pr_view_on(stack_depth, "%20s : %d(selected)\n", cpu);

    if (task_cpu(p) != cpu) {
        wake_flags |= WF_MIGRATED;
        psi_ttwu_dequeue(p);
        set_task_cpu(p, cpu);
    }

#else /* CONFIG_SMP */

    if (p->in_iowait) {
        //delayacct_blkio_end(p);
        atomic_dec(&task_rq(p)->nr_iowait);
    }

#endif /* CONFIG_SMP */

    ttwu_queue(p, cpu, wake_flags);

unlock:
    pr_info_on(stack_depth, "unlock:\n");
    raw_spin_unlock_irqrestore(&p->pi_lock, flags);

out:
    pr_info_on(stack_depth, "out:\n");

    if (success)
        ttwu_stat(p, cpu, wake_flags);
    preempt_enable();

    pr_fn_end_on(stack_depth);

    return success;
}

/**
 * wake_up_process - Wake up a specific process
 * @p: The process to be woken up.
 *
 * Attempt to wake up the nominated process and move it to the set of runnable
 * processes.
 *
 * Return: 1 if the process was woken up, 0 if it was already running.
 *
 * This function executes a full memory barrier before accessing the task state.
 */
int wake_up_process(struct task_struct *p)
{
    return try_to_wake_up(p, TASK_NORMAL, 0);
}
EXPORT_SYMBOL(wake_up_process);

int wake_up_state(struct task_struct *p, unsigned int state)
{
    return try_to_wake_up(p, state, 0);
}
//2675
/*
 * Perform scheduler related setup for a newly forked process p.
 * p is forked by current.
 *
 * __sched_fork() is basic setup used by init_idle() too:
 */
static void __sched_fork(unsigned long clone_flags, struct task_struct *p)
{
    pr_fn_start_on(stack_depth);

    p->on_rq					= 0;
    p->se.on_rq					= 0;
    p->se.exec_start			= 0;
    p->se.sum_exec_runtime		= 0;
    p->se.prev_sum_exec_runtime	= 0;
    p->se.nr_migrations			= 0;
    p->se.vruntime				= 0;
    INIT_LIST_HEAD(&p->se.group_node);

#ifdef CONFIG_FAIR_GROUP_SCHED
    p->se.cfs_rq			= NULL;
#endif

#ifdef CONFIG_SCHEDSTATS
    /* Even if schedstat is disabled, there should not be garbage */
    memset(&p->se.statistics, 0, sizeof(p->se.statistics));
#endif

    RB_CLEAR_NODE(&p->dl.rb_node);
    init_dl_task_timer(&p->dl);
    init_dl_inactive_task_timer(&p->dl);
    __dl_clear_params(p);

    INIT_LIST_HEAD(&p->rt.run_list);
    p->rt.timeout		= 0;
    p->rt.time_slice	= sched_rr_timeslice;
    p->rt.on_rq			= 0;
    p->rt.on_list		= 0;

#ifdef CONFIG_PREEMPT_NOTIFIERS
    INIT_HLIST_HEAD(&p->preempt_notifiers);
#endif

#ifdef CONFIG_COMPACTION
    p->capture_control = NULL;
#endif
    init_numa_balancing(clone_flags, p);

    pr_fn_end_on(stack_depth);
}
//2723 lines




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
//2835
/*
 * fork()/clone()-time setup:
 */
int sched_fork(unsigned long clone_flags, struct task_struct *p)
{
    unsigned long flags;

    pr_fn_start_on(stack_depth);

    __sched_fork(clone_flags, p);
    /*
     * We mark the process as NEW here. This guarantees that
     * nobody will actually run it, and a signal or other external
     * event cannot wake it up and insert it on the runqueue either.
     */
    p->state = TASK_NEW;

    /*
     * Make sure we do not leak PI boosting priority to the child.
     */
    //p->prio = current->normal_prio;	//current error
    p->prio = current_task->normal_prio;

    uclamp_fork(p);

    pr_view_on(stack_depth, "%30s : %u\n", p->sched_reset_on_fork);

    /*
     * Revert to default priority/policy on fork if requested.
     */
    if (unlikely(p->sched_reset_on_fork)) {
        if (task_has_dl_policy(p) || task_has_rt_policy(p)) {
            p->policy = SCHED_NORMAL;
            p->static_prio = NICE_TO_PRIO(0);
            p->rt_priority = 0;
        } else if (PRIO_TO_NICE(p->static_prio) < 0)
            p->static_prio = NICE_TO_PRIO(0);

        p->prio = p->normal_prio = __normal_prio(p);
        set_load_weight(p, false);

        /*
         * We don't need the reset flag anymore after the fork. It has
         * fulfilled its duty:
         */
        p->sched_reset_on_fork = 0;
    }

    pr_view_on(stack_depth, "%30s : %u\n", p->policy);
    pr_view_on(stack_depth, "%30s : %d\n", p->prio);
    pr_view_on(stack_depth, "%30s : %d\n", p->normal_prio);
    pr_view_on(stack_depth, "%30s : %d\n", p->static_prio);
    pr_view_on(stack_depth, "%30s : %d\n", p->rt_priority);

    if (dl_prio(p->prio))
        return -EAGAIN;
    else if (rt_prio(p->prio))
        p->sched_class = &rt_sched_class;
    else
        p->sched_class = &fair_sched_class;

    init_entity_runnable_average(&p->se);

    /*
     * The child is not yet in the pid-hash so no cgroup attach races,
     * and the cgroup is pinned to this child due to cgroup_fork()
     * is ran before sched_fork().
     *
     * Silence PROVE_RCU.
     */
    raw_spin_lock_irqsave(&p->pi_lock, flags);
    /*
     * We're setting the CPU for the first time, we don't migrate,
     * so use __set_task_cpu().
     */
    __set_task_cpu(p, smp_processor_id());	//smp_user_cpu == p->cpu

    pr_view_on(stack_depth, "%30s : %p\n", p->sched_class->task_fork);
    pr_view_on(stack_depth, "%30s : %p\n", p->se.cfs_rq);
    pr_view_on(stack_depth, "%30s : %p\n", p->se.cfs_rq->curr);

    if (p->sched_class->task_fork)
        p->sched_class->task_fork(p);
    raw_spin_unlock_irqrestore(&p->pi_lock, flags);

#ifdef CONFIG_SCHED_INFO
    if (likely(sched_info_on()))
        memset(&p->sched_info, 0, sizeof(p->sched_info));
#endif
#if defined(CONFIG_SMP)
    p->on_cpu = 0;
#endif
    //init_task_preempt_count(p);
#ifdef CONFIG_SMP
    plist_node_init(&p->pushable_tasks, MAX_PRIO);
    RB_CLEAR_NODE(&p->pushable_dl_tasks);
#endif

    pr_fn_end_on(stack_depth);

    return 0;
}
//2919
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

/*
 * wake_up_new_task - wake up a newly created task for the first time.
 *
 * This function will do some initial scheduler statistics housekeeping
 * that must be done for every newly created context, then puts the task
 * on the runqueue and wakes it.
 */
void wake_up_new_task(struct task_struct *p)
{
    struct rq_flags rf;
    struct rq *rq;

    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%10s : %p\n", (void*)p);
    pr_view_on(stack_depth, "%10s : %d\n", p->cpu);

    raw_spin_lock_irqsave(&p->pi_lock, rf.flags);
    p->state = TASK_RUNNING;
#ifdef CONFIG_SMP
    /*
     * Fork balancing, do it here and not earlier because:
     *  - cpus_ptr can change in the fork path
     *  - any previously selected CPU might disappear through hotplug
     *
     * Use __set_task_cpu() to avoid calling sched_class::migrate_task_rq,
     * as we're not fully set-up yet.
     */
    p->recent_used_cpu = task_cpu(p);
    __set_task_cpu(p, select_task_rq(p, task_cpu(p), SD_BALANCE_FORK, 0));
#endif
    rq = __task_rq_lock(p, &rf);
    rq->cpu = p->cpu;

    pr_view_on(stack_depth, "%20s : %d\n", p->cpu);
    pr_view_on(stack_depth, "%20s : %d\n", rq->cpu);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_view_on(stack_depth, "%20s : %d\n", cpu_of(rq));

    update_rq_clock(rq);
    post_init_entity_util_avg(p);

    //pr_sched_tg_info_all();

    activate_task(rq, p, ENQUEUE_NOCLOCK);

    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    if (!rq->curr)
        rq->curr = p;
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);

    //trace_sched_wakeup_new(p);
    check_preempt_curr(rq, p, WF_FORK);
#ifdef CONFIG_SMP
    pr_view_on(stack_depth, "%30s : %p\n", p->sched_class->task_woken);
    if (p->sched_class->task_woken) {
        /*
         * Nothing relies on rq->lock after this, so its fine to
         * drop it.
         */
        rq_unpin_lock(rq, &rf);
        p->sched_class->task_woken(rq, p);
        rq_repin_lock(rq, &rf);
    }
#endif
    task_rq_unlock(rq, p, &rf);

    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    pr_fn_end_on(stack_depth);
}
//2982 lines





//3069 lines
static inline void prepare_task(struct task_struct *next)
{
#ifdef CONFIG_SMP
    /*
     * Claim the task as running, we do this before switching to it
     * such that any running task will have this set.
     */
    next->on_cpu = 1;
#endif
}

static inline void finish_task(struct task_struct *prev)
{
#ifdef CONFIG_SMP
    /*
     * After ->on_cpu is cleared, the task can be moved to a different CPU.
     * We must ensure this doesn't happen until the switch is completely
     * finished.
     *
     * In particular, the load of prev->state in finish_task_switch() must
     * happen before this.
     *
     * Pairs with the smp_cond_load_acquire() in try_to_wake_up().
     */
    smp_store_release(&prev->on_cpu, 0);
#endif
}

static inline void
prepare_lock_switch(struct rq *rq, struct task_struct *next, struct rq_flags *rf)
{
    /*
     * Since the runqueue lock will be released by the next
     * task (which is an invalid locking op but in the case
     * of the scheduler it's an obvious special-case), so we
     * do an early lockdep release here:
     */
    rq_unpin_lock(rq, rf);
    //spin_release(&rq->lock.dep_map, 1, _THIS_IP_);
#ifdef CONFIG_DEBUG_SPINLOCK
    /* this is a valid case when another task releases the spinlock */
    rq->lock.owner = next;
#endif
}

static inline void finish_lock_switch(struct rq *rq)
{
    /*
     * If we are tracking spinlock dependencies then we have to
     * fix up the runqueue lock - which gets 'carried over' from
     * prev into current:
     */
    //spin_acquire(&rq->lock.dep_map, 0, 0, _THIS_IP_);
    raw_spin_unlock_irq(&rq->lock);
}

/*
 * NOP if the arch has not defined these:
 */

#ifndef prepare_arch_switch
# define prepare_arch_switch(next)	do { } while (0)
#endif

#ifndef finish_arch_post_lock_switch
# define finish_arch_post_lock_switch()	do { } while (0)
#endif

/**
 * prepare_task_switch - prepare to switch tasks
 * @rq: the runqueue preparing to switch
 * @prev: the current task that is being switched out
 * @next: the task we are going to switch to.
 *
 * This is called with the rq lock held and interrupts off. It must
 * be paired with a subsequent finish_task_switch after the context
 * switch.
 *
 * prepare_task_switch sets up locking and calls architecture specific
 * hooks.
 */
static inline void
prepare_task_switch(struct rq *rq, struct task_struct *prev,
            struct task_struct *next)
{
    //kcov_prepare_switch(prev);
    sched_info_switch(rq, prev, next);
    //perf_event_task_sched_out(prev, next);
    rseq_preempt(prev);
    //fire_sched_out_preempt_notifiers(prev, next);
    prepare_task(next);
    prepare_arch_switch(next);
}

/**
 * finish_task_switch - clean up after a task-switch
 * @prev: the thread we just switched away from.
 *
 * finish_task_switch must be called after the context switch, paired
 * with a prepare_task_switch call before the context switch.
 * finish_task_switch will reconcile locking set up by prepare_task_switch,
 * and do any other architecture-specific cleanup actions.
 *
 * Note that we may have delayed dropping an mm in context_switch(). If
 * so, we finish that here outside of the runqueue lock. (Doing it
 * with the lock held can cause deadlocks; see schedule() for
 * details.)
 *
 * The context switch have flipped the stack from under us and restored the
 * local variables which were saved when this task called schedule() in the
 * past. prev == current is still correct but we need to recalculate this_rq
 * because prev may have moved to another CPU.
 */
static struct rq *finish_task_switch(struct task_struct *prev)
    __releases(rq->lock)
{
    struct rq *rq = this_rq();
    struct mm_struct *mm = rq->prev_mm;
    long prev_state;

    /*
     * The previous task will have left us with a preempt_count of 2
     * because it left us after:
     *
     *	schedule()
     *	  preempt_disable();			// 1
     *	  __schedule()
     *	    raw_spin_lock_irq(&rq->lock)	// 2
     *
     * Also, see FORK_PREEMPT_COUNT.
     */
    if (WARN_ONCE(preempt_count() != 2*PREEMPT_DISABLE_OFFSET,
              "corrupted preempt_count: %s/%d/0x%x\n",
              current_task->comm, current_task->pid, preempt_count()))
        preempt_count_set(FORK_PREEMPT_COUNT);

    rq->prev_mm = NULL;

    /*
     * A task struct has one reference for the use as "current".
     * If a task dies, then it sets TASK_DEAD in tsk->state and calls
     * schedule one last time. The schedule call will never return, and
     * the scheduled task must drop that reference.
     *
     * We must observe prev->state before clearing prev->on_cpu (in
     * finish_task), otherwise a concurrent wakeup can get prev
     * running on another CPU and we could rave with its RUNNING -> DEAD
     * transition, resulting in a double drop.
     */
    prev_state = prev->state;
    //vtime_task_switch(prev);
    //perf_event_task_sched_in(prev, current);
    finish_task(prev);
    finish_lock_switch(rq);
    finish_arch_post_lock_switch();
    //kcov_finish_switch(current);

    //fire_sched_in_preempt_notifiers(current);
    /*
     * When switching through a kernel thread, the loop in
     * membarrier_{private,global}_expedited() may have observed that
     * kernel thread and not issued an IPI. It is therefore possible to
     * schedule between user->kernel->user threads without passing though
     * switch_mm(). Membarrier requires a barrier after storing to
     * rq->curr, before returning to userspace, so provide them here:
     *
     * - a full memory barrier for {PRIVATE,GLOBAL}_EXPEDITED, implicitly
     *   provided by mmdrop(),
     * - a sync_core for SYNC_CORE.
     */
    //if (mm) {
    //    membarrier_mm_sync_core_before_usermode(mm);
    //    mmdrop(mm);
    //}
    if (unlikely(prev_state == TASK_DEAD)) {
        if (prev->sched_class->task_dead)
            prev->sched_class->task_dead(prev);

        /*
         * Remove function-return probe instances associated with this
         * task and put them back on the free list.
         */
        //kprobe_flush_task(prev);

        /* Task is done with its stack. */
        //put_task_stack(prev);

        //put_task_struct_rcu_user(prev);
    }

    //tick_nohz_task_switch();
    return rq;
}
//3263
#ifdef CONFIG_SMP

/* rq->lock is NOT held, but preemption is disabled */
static void __balance_callback(struct rq *rq)
{
    pr_fn_start_on(stack_depth);

    struct callback_head *head, *next;
    void (*func)(struct rq *rq);
    unsigned long flags;
    unsigned int debug_cnt = 0;

    pr_view_on(stack_depth, "%10s : %p\n", rq);
    pr_view_on(stack_depth, "%10s : %d\n", rq->cpu);

    raw_spin_lock_irqsave(&rq->lock, flags);
    head = rq->balance_callback;
    rq->balance_callback = NULL;
    while (head) {
        pr_view_on(stack_depth, "%20s : %u\n", debug_cnt++);
        pr_view_on(stack_depth, "%20s : %p\n", head);
        func = (void (*)(struct rq *))head->func;
        next = head->next;
        head->next = NULL;
        head = next;

        pr_view_on(stack_depth, "%20s : %p\n", func);
        func(rq);
    }
    raw_spin_unlock_irqrestore(&rq->lock, flags);

    pr_fn_end_on(stack_depth);
}

static inline void balance_callback(struct rq *rq)
{
    pr_fn_start(stack_depth);

    pr_view_on(stack_depth, "%20s : %p\n", rq->balance_callback);

    if (unlikely(rq->balance_callback))
        __balance_callback(rq);

    pr_fn_end_on(stack_depth);
}

#else

static inline void balance_callback(struct rq *rq)
{
}

#endif
//3300 lines






//3328 lines
/*
 * context_switch - switch to the new MM and the new thread's register state.
 */
static __always_inline struct rq *
context_switch(struct rq *rq, struct task_struct *prev,
           struct task_struct *next, struct rq_flags *rf)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)prev);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)next);

    prepare_task_switch(rq, prev, next);

    /*
     * For paravirt, this is coupled with an exit in switch_to to
     * combine the page table reload and the switch backend into
     * one hypercall.
     */
    //arch_start_context_switch(prev);

    /*
     * kernel -> kernel   lazy + transfer active
     *   user -> kernel   lazy + mmgrab() active
     *
     * kernel ->   user   switch + mmdrop() active
     *   user ->   user   switch
     */
    if (!next->mm) {                                // to kernel
        //enter_lazy_tlb(prev->active_mm, next);

        next->active_mm = prev->active_mm;
        //if (prev->mm)                           // from user
            //mmgrab(prev->active_mm);
        //else
            prev->active_mm = NULL;
    } else {                                        // to user
        membarrier_switch_mm(rq, prev->active_mm, next->mm);
        /*
         * sys_membarrier() requires an smp_mb() between setting
         * rq->curr / membarrier_switch_mm() and returning to userspace.
         *
         * The below provides this either through switch_mm(), or in
         * case 'prev->active_mm == next->mm' through
         * finish_task_switch()'s mmdrop().
         */
        //switch_mm_irqs_off(prev->active_mm, next->mm, next);

        if (!prev->mm) {                        // from kernel
            /* will mmdrop() in finish_task_switch(). */
            rq->prev_mm = prev->active_mm;
            prev->active_mm = NULL;
        }
    }

    rq->clock_update_flags &= ~(RQCF_ACT_SKIP|RQCF_REQ_SKIP);

    prepare_lock_switch(rq, next, rf);

    /* Here we just switch the register state and the stack. */
    switch_to(prev, next, prev);

    rq->curr = next;	//Modified for switch to next;

    barrier();

    pr_view_on(stack_depth, "%20s : %p\n", (void*)next);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)prev);
    pr_view_on(stack_depth, "%20s : %p\n", current_task);
    pr_view_on(stack_depth, "%20s : %p\n", rq->curr);

    pr_fn_end_on(stack_depth);

    return finish_task_switch(prev);
}

/*
 * nr_running and nr_context_switches:
 *
 * externally visible scheduler statistics: current number of runnable
 * threads, total number of context switches performed since bootup.
 */
unsigned long nr_running(void)
{
    unsigned long i, sum = 0;

    for_each_online_cpu(i)
        sum += cpu_rq(i)->nr_running;

    return sum;
}

/*
 * Check if only the current task is running on the CPU.
 *
 * Caution: this function does not check that the caller has disabled
 * preemption, thus the result might have a time-of-check-to-time-of-use
 * race.  The caller is responsible to use it correctly, for example:
 *
 * - from a non-preemptible section (of course)
 *
 * - from a thread that is bound to a single CPU
 *
 * - in a loop with very short iterations (e.g. a polling loop)
 */
bool single_task_running(void)
{
    //return raw_rq()->nr_running == 1;
}
//EXPORT_SYMBOL(single_task_running);

unsigned long long nr_context_switches(void)
{
    int i;
    unsigned long long sum = 0;

    for_each_possible_cpu(i)
        sum += cpu_rq(i)->nr_switches;

    return sum;
}

/*
 * Consumers of these two interfaces, like for example the cpuidle menu
 * governor, are using nonsensical data. Preferring shallow idle state selection
 * for a CPU that has IO-wait which might not even end up running the task when
 * it does become runnable.
 */

unsigned long nr_iowait_cpu(int cpu)
{
    return atomic_read(&cpu_rq(cpu)->nr_iowait);
}

/*
 * IO-wait accounting, and how its mostly bollocks (on SMP).
 *
 * The idea behind IO-wait account is to account the idle time that we could
 * have spend running if it were not for IO. That is, if we were to improve the
 * storage performance, we'd have a proportional reduction in IO-wait time.
 *
 * This all works nicely on UP, where, when a task blocks on IO, we account
 * idle time as IO-wait, because if the storage were faster, it could've been
 * running and we'd not be idle.
 *
 * This has been extended to SMP, by doing the same for each CPU. This however
 * is broken.
 *
 * Imagine for instance the case where two tasks block on one CPU, only the one
 * CPU will have IO-wait accounted, while the other has regular idle. Even
 * though, if the storage were faster, both could've ran at the same time,
 * utilising both CPUs.
 *
 * This means, that when looking globally, the current IO-wait accounting on
 * SMP is a lower bound, by reason of under accounting.
 *
 * Worse, since the numbers are provided per CPU, they are sometimes
 * interpreted per CPU, and that is nonsensical. A blocked task isn't strictly
 * associated with any one particular CPU, it can wake to another CPU than it
 * blocked on. This means the per CPU IO-wait number is meaningless.
 *
 * Task CPU affinities can make all that even more 'interesting'.
 */

unsigned long nr_iowait(void)
{
    unsigned long i, sum = 0;

    for_each_possible_cpu(i)
        sum += nr_iowait_cpu(i);

    return sum;
}
//3487
#ifdef CONFIG_SMP

/*
 * sched_exec - execve() is a valuable balancing opportunity, because at
 * this point the task has the smallest effective memory and cache footprint.
 */
void sched_exec(void)
{
    struct task_struct *p = current;
    unsigned long flags;
    int dest_cpu;

    raw_spin_lock_irqsave(&p->pi_lock, flags);
    dest_cpu = p->sched_class->select_task_rq(p, task_cpu(p), SD_BALANCE_EXEC, 0);
    if (dest_cpu == smp_processor_id())
        goto unlock;

    if (likely(cpu_active(dest_cpu))) {
        struct migration_arg arg = { p, dest_cpu };

        raw_spin_unlock_irqrestore(&p->pi_lock, flags);
        //stop_one_cpu(task_cpu(p), migration_cpu_stop, &arg);
        return;
    }
unlock:
    raw_spin_unlock_irqrestore(&p->pi_lock, flags);
}

#endif
//3517 lines



//3584 lines
/*
 * This function gets called by the timer code, with HZ frequency.
 * We call it with interrupts disabled.
 */
void scheduler_tick(void)
{
    pr_fn_start_on(stack_depth);

    int cpu = smp_processor_id();
    struct rq *rq = cpu_rq(cpu);
    struct task_struct *curr = rq->curr;
    struct rq_flags rf;

    pr_view_on(stack_depth, "%20s : %d\n", cpu);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)cpu_rq(cpu));
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);

    sched_clock_tick();

    rq_lock(rq, &rf);

    update_rq_clock(rq);
    curr->sched_class->task_tick(rq, curr, 0);
    calc_global_load_tick(rq);
    psi_task_tick(rq);

    rq_unlock(rq, &rf);

    //perf_event_task_tick();

#ifdef CONFIG_SMP
    //rq->idle_balance = idle_cpu(cpu);
    trigger_load_balance(rq);
#endif

    pr_fn_end_on(stack_depth);
}
//3614 lines


static void sched_tick_start(int cpu)
{
        int os;
        struct tick_work *twork;

        if (housekeeping_cpu(cpu, HK_FLAG_TICK))
                return;

#if 0
        WARN_ON_ONCE(!tick_work_cpu);

        twork = per_cpu_ptr(tick_work_cpu, cpu);
        os = atomic_xchg(&twork->state, TICK_SCHED_REMOTE_RUNNING);
        WARN_ON_ONCE(os == TICK_SCHED_REMOTE_RUNNING);
        if (os == TICK_SCHED_REMOTE_OFFLINE) {
                twork->cpu = cpu;
                INIT_DELAYED_WORK(&twork->work, sched_tick_remote);
                queue_delayed_work(system_unbound_wq, &twork->work, HZ);
        }
#endif
}

#ifdef CONFIG_HOTPLUG_CPU
static void sched_tick_stop(int cpu)
{
        struct tick_work *twork;
        int os;

        if (housekeeping_cpu(cpu, HK_FLAG_TICK))
                return;
#if 0
        WARN_ON_ONCE(!tick_work_cpu);

        twork = per_cpu_ptr(tick_work_cpu, cpu);
        /* There cannot be competing actions, but don't rely on stop-machine. */
        os = atomic_xchg(&twork->state, TICK_SCHED_REMOTE_OFFLINING);
        WARN_ON_ONCE(os != TICK_SCHED_REMOTE_RUNNING);
        /* Don't cancel, as this would mess up the state machine. */
#endif
}
#endif /* CONFIG_HOTPLUG_CPU */




//3830 lines
static inline unsigned long get_preempt_disable_ip(struct task_struct *p)
{
#ifdef CONFIG_DEBUG_PREEMPT
    return p->preempt_disable_ip;
#else
    return 0;
#endif
}

/*
 * Print scheduling while atomic bug:
 */
static noinline void __schedule_bug(struct task_struct *prev)
{
    /* Save this before calling printk(), since that will clobber it */
    unsigned long preempt_disable_ip = get_preempt_disable_ip(current_task);

    //if (oops_in_progress)
        //return;

    printk(KERN_ERR "BUG: scheduling while atomic: %s/%d/0x%08x\n",
        prev->comm, prev->pid, preempt_count());

    //debug_show_held_locks(prev);
    //print_modules();
    //if (irqs_disabled())
        //print_irqtrace_events(prev);
    if (IS_ENABLED(CONFIG_DEBUG_PREEMPT)
        && in_atomic_preempt_off()) {
        pr_err("Preemption disabled at:");
        //print_ip_sym(preempt_disable_ip);
        pr_cont("\n");
    }
    //if (panic_on_warn)
        panic("scheduling while atomic\n");

    dump_stack();
    //add_taint(TAINT_WARN, LOCKDEP_STILL_OK);
}

/*
 * Various schedule()-time debugging checks and statistics:
 */
static inline void schedule_debug(struct task_struct *prev, bool preempt)
{
#ifdef CONFIG_SCHED_STACK_END_CHECK
    if (task_stack_end_corrupted(prev))
        panic("corrupted stack end detected inside scheduler\n");
#endif

#ifdef CONFIG_DEBUG_ATOMIC_SLEEP
    if (!preempt && prev->state && prev->non_block_count) {
        printk(KERN_ERR "BUG: scheduling in a non-blocking section: %s/%d/%i\n",
            prev->comm, prev->pid, prev->non_block_count);
        dump_stack();
        add_taint(TAINT_WARN, LOCKDEP_STILL_OK);
    }
#endif

    if (unlikely(in_atomic_preempt_off())) {
        __schedule_bug(prev);
        preempt_count_set(PREEMPT_DISABLED);
    }
    //rcu_sleep_check();

    profile_hit(SCHED_PROFILING, __builtin_return_address(0));

    schedstat_inc(this_rq()->sched_count);
}

//kernel version >= v5.19
static void put_prev_task_balance(struct rq *rq, struct task_struct *prev,
                                  struct rq_flags *rf)
{
#ifdef CONFIG_SMP
    const struct sched_class *class;
    /*
         * We must do the balancing pass before put_prev_task(), such
         * that when we release the rq->lock the task is in the same
         * state as before we took rq->lock.
         *
         * We can terminate the balance pass as soon as we know there is
         * a runnable task of @class priority or higher.
         */
    for_class_range(class, prev->sched_class, &idle_sched_class) {
        if (class->balance(rq, prev, rf))
            break;
    }
#endif

    put_prev_task(rq, prev);
}

/*
 * Pick up the highest-prio task:
 */
static inline struct task_struct *
        __pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
    const struct sched_class *class;
    struct task_struct *p;

    pr_fn_start_on(stack_depth);

    /*
     * Optimization: we know that if all tasks are in the fair class we can
     * call that function directly, but only if the @prev task wasn't of a
     * higher scheduling class, because otherwise those lose the
     * opportunity to pull in more work from other CPUs.
     */
    if (likely((prev->sched_class == &idle_sched_class ||
                prev->sched_class == &fair_sched_class) &&
               rq->nr_running == rq->cfs.h_nr_running)) {
#if 0
    if (likely(!sched_class_above(prev->sched_class, &fair_sched_class) &&
               rq->nr_running == rq->cfs.h_nr_running)) {
#endif
        p = pick_next_task_fair(rq, prev, rf);
        pr_view_on(stack_depth, "%10s : %p\n", p);

        if (unlikely(p == RETRY_TASK))
            goto restart;

        /* Assume the next prioritized class is idle_sched_class */
        if (!p) {
            put_prev_task(rq, prev);
            p = pick_next_task_idle(rq);
        }

        return p;
    }

restart:
    put_prev_task_balance(rq, prev, rf);

    for_each_class(class) {
        p = class->pick_next_task(rq);
        if (p)
            return p;
    }

    BUG(); /* The idle class should always have a runnable task. */
}

//kernel version >= v5.19
#ifdef CONFIG_SCHED_CORE
static inline bool is_task_rq_idle(struct task_struct *t)
{
        return (task_rq(t)->idle == t);
}

static inline bool cookie_equals(struct task_struct *a, unsigned long cookie)
{
        return is_task_rq_idle(a) || (a->core_cookie == cookie);
}

static inline bool cookie_match(struct task_struct *a, struct task_struct *b)
{
        if (is_task_rq_idle(a) || is_task_rq_idle(b))
                return true;

        return a->core_cookie == b->core_cookie;
}

static inline struct task_struct *pick_task(struct rq *rq)
{
        const struct sched_class *class;
        struct task_struct *p;

        for_each_class(class) {
                p = class->pick_task(rq);
                if (p)
                        return p;
        }

        BUG(); /* The idle class should always have a runnable task. */
        return current; //for test
}

extern void task_vruntime_update(struct rq *rq, struct task_struct *p, bool in_fi);

static void queue_core_balance(struct rq *rq);

static struct task_struct *
        pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
    struct task_struct *next, *p, *max = NULL;
    const struct cpumask *smt_mask;
    bool fi_before = false;
    bool core_clock_updated = (rq == rq->core);
    unsigned long cookie;
    int i, cpu, occ = 0;
    struct rq *rq_i;
    bool need_sync;

    pr_fn_start_on(stack_depth);

    if (!sched_core_enabled(rq))
        return __pick_next_task(rq, prev, rf);

    cpu = cpu_of(rq);

    /* Stopper task is switching into idle, no need core-wide selection. */
    if (cpu_is_offline(cpu)) {
        /*
         * Reset core_pick so that we don't enter the fastpath when
         * coming online. core_pick would already be migrated to
         * another cpu during offline.
         */
        rq->core_pick = NULL;
        return __pick_next_task(rq, prev, rf);
    }

    pr_view_on(stack_depth, "%30s : %u\n", rq->core->core_pick_seq);
    pr_view_on(stack_depth, "%30s : %u\n", rq->core->core_task_seq);
    pr_view_on(stack_depth, "%30s : %u\n", rq->core->core_sched_seq);
    pr_view_on(stack_depth, "%30s : %p\n", rq->core_pick);
    /*
     * If there were no {en,de}queues since we picked (IOW, the task
     * pointers are all still valid), and we haven't scheduled the last
     * pick yet, do so now.
     *
     * rq->core_pick can be NULL if no selection was made for a CPU because
     * it was either offline or went offline during a sibling's core-wide
     * selection. In this case, do a core-wide selection.
     */
    if (rq->core->core_pick_seq == rq->core->core_task_seq &&
            rq->core->core_pick_seq != rq->core_sched_seq &&
            rq->core_pick) {
        WRITE_ONCE(rq->core_sched_seq, rq->core->core_pick_seq);

        next = rq->core_pick;
        if (next != prev) {
            put_prev_task(rq, prev);
            set_next_task(rq, next);
        }

        rq->core_pick = NULL;
        goto out;
    }

    put_prev_task_balance(rq, prev, rf);

    smt_mask = cpu_smt_mask(cpu);
    need_sync = !!rq->core->core_cookie;

    /* reset state */
    rq->core->core_cookie = 0UL;
    if (rq->core->core_forceidle_count) {
        if (!core_clock_updated) {
            update_rq_clock(rq->core);
            core_clock_updated = true;
        }
        sched_core_account_forceidle(rq);
        /* reset after accounting force idle */
        rq->core->core_forceidle_start = 0;
        rq->core->core_forceidle_count = 0;
        rq->core->core_forceidle_occupation = 0;
        need_sync = true;
        fi_before = true;
    }

    /*
     * core->core_task_seq, core->core_pick_seq, rq->core_sched_seq
     *
     * @task_seq guards the task state ({en,de}queues)
     * @pick_seq is the @task_seq we did a selection on
     * @sched_seq is the @pick_seq we scheduled
     *
     * However, preemptions can cause multiple picks on the same task set.
     * 'Fix' this by also increasing @task_seq for every pick.
     */
    rq->core->core_task_seq++;

    /*
     * Optimize for common case where this CPU has no cookies
     * and there are no cookied tasks running on siblings.
     */
    if (!need_sync) {
        next = pick_task(rq);
        if (!next->core_cookie) {
            rq->core_pick = NULL;
            /*
             * For robustness, update the min_vruntime_fi for
             * unconstrained picks as well.
             */
            WARN_ON_ONCE(fi_before);
            task_vruntime_update(rq, next, false);
            goto out_set_next;
        }
    }

    /*
     * For each thread: do the regular task pick and find the max prio task
     * amongst them.
     *
     * Tie-break prio towards the current CPU
     */
    for_each_cpu_wrap(i, smt_mask, cpu) {
        rq_i = cpu_rq(i);

        /*
         * Current cpu always has its clock updated on entrance to
         * pick_next_task(). If the current cpu is not the core,
         * the core may also have been updated above.
         */
        if (i != cpu && (rq_i != rq->core || !core_clock_updated))
            update_rq_clock(rq_i);

        p = rq_i->core_pick = pick_task(rq_i);
        if (!max || prio_less(max, p, fi_before))
            max = p;
    }

    cookie = rq->core->core_cookie = max->core_cookie;

    /*
     * For each thread: try and find a runnable task that matches @max or
     * force idle.
     */
    for_each_cpu(i, smt_mask) {
        rq_i = cpu_rq(i);
        p = rq_i->core_pick;

        if (!cookie_equals(p, cookie)) {
            p = NULL;
            if (cookie)
                p = sched_core_find(rq_i, cookie);
            if (!p)
                p = idle_sched_class.pick_task(rq_i);
        }

        rq_i->core_pick = p;

        if (p == rq_i->idle) {
            if (rq_i->nr_running) {
                rq->core->core_forceidle_count++;
                if (!fi_before)
                    rq->core->core_forceidle_seq++;
            }
        } else {
            occ++;
        }
    }

    if (schedstat_enabled() && rq->core->core_forceidle_count) {
        rq->core->core_forceidle_start = rq_clock(rq->core);
        rq->core->core_forceidle_occupation = occ;
    }

    rq->core->core_pick_seq = rq->core->core_task_seq;
    next = rq->core_pick;
    rq->core_sched_seq = rq->core->core_pick_seq;

    /* Something should have been selected for current CPU */
    WARN_ON_ONCE(!next);

    /*
     * Reschedule siblings
     *
     * NOTE: L1TF -- at this point we're no longer running the old task and
     * sending an IPI (below) ensures the sibling will no longer be running
     * their task. This ensures there is no inter-sibling overlap between
     * non-matching user state.
     */
    for_each_cpu(i, smt_mask) {
        rq_i = cpu_rq(i);

        /*
         * An online sibling might have gone offline before a task
         * could be picked for it, or it might be offline but later
         * happen to come online, but its too late and nothing was
         * picked for it.  That's Ok - it will pick tasks for itself,
         * so ignore it.
         */
        if (!rq_i->core_pick)
            continue;

        /*
         * Update for new !FI->FI transitions, or if continuing to be in !FI:
         * fi_before     fi      update?
         *  0            0       1
         *  0            1       1
         *  1            0       1
         *  1            1       0
         */
        if (!(fi_before && rq->core->core_forceidle_count))
            task_vruntime_update(rq_i, rq_i->core_pick, !!rq->core->core_forceidle_count);

        rq_i->core_pick->core_occupation = occ;

        if (i == cpu) {
            rq_i->core_pick = NULL;
            continue;
        }

        /* Did we break L1TF mitigation requirements? */
        WARN_ON_ONCE(!cookie_match(next, rq_i->core_pick));

        if (rq_i->curr == rq_i->core_pick) {
            rq_i->core_pick = NULL;
            continue;
        }

        resched_curr(rq_i);
    }

out_set_next:
    set_next_task(rq, next);
out:
    if (rq->core->core_forceidle_count && next == rq->idle)
        queue_core_balance(rq);

    pr_fn_end_on(stack_depth);
    return next;
}

static bool try_steal_cookie(int this, int that)
{
        struct rq *dst = cpu_rq(this), *src = cpu_rq(that);
        struct task_struct *p;
        unsigned long cookie;
        bool success = false;

        local_irq_disable();
        double_rq_lock(dst, src);

        cookie = dst->core->core_cookie;
        if (!cookie)
                goto unlock;

        if (dst->curr != dst->idle)
                goto unlock;

        p = sched_core_find(src, cookie);
        if (p == src->idle)
                goto unlock;

        do {
                if (p == src->core_pick || p == src->curr)
                        goto next;

                if (!is_cpu_allowed(p, this))
                        goto next;

                if (p->core_occupation > dst->idle->core_occupation)
                        goto next;

                deactivate_task(src, p, 0);
                set_task_cpu(p, this);
                activate_task(dst, p, 0);

                resched_curr(dst);

                success = true;
                break;

next:
                p = sched_core_next(p, cookie);
        } while (p);

unlock:
        double_rq_unlock(dst, src);
        local_irq_enable();

        return success;
}

static bool steal_cookie_task(int cpu, struct sched_domain *sd)
{
        int i;

        for_each_cpu_wrap(i, sched_domain_span(sd), cpu) {
                if (i == cpu)
                        continue;

                if (need_resched())
                        break;

                if (try_steal_cookie(cpu, i))
                        return true;
        }

        return false;
}

static void sched_core_balance(struct rq *rq)
{
        struct sched_domain *sd;
        int cpu = cpu_of(rq);

        preempt_disable();
        rcu_read_lock();
        //raw_spin_rq_unlock_irq(rq);
        for_each_domain(cpu, sd) {
                if (need_resched())
                        break;

                if (steal_cookie_task(cpu, sd))
                        break;
        }
        //raw_spin_rq_lock_irq(rq);
        rcu_read_unlock();
        preempt_enable();
}

static DEFINE_PER_CPU(struct callback_head, core_balance_head);

static void queue_core_balance(struct rq *rq)
{
        if (!sched_core_enabled(rq))
                return;

        if (!rq->core->core_cookie)
                return;

        if (!rq->nr_running) /* not forced idle */
                return;

        queue_balance_callback(rq, &per_cpu(core_balance_head, rq->cpu), sched_core_balance);
}

static void sched_core_cpu_starting(unsigned int cpu)
{
        const struct cpumask *smt_mask = cpu_smt_mask(cpu);
        struct rq *rq = cpu_rq(cpu), *core_rq = NULL;
        unsigned long flags;
        int t;

        //sched_core_lock(cpu, &flags);

        WARN_ON_ONCE(rq->core != rq);

        /* if we're the first, we'll be our own leader */
        if (cpumask_weight(smt_mask) == 1)
                goto unlock;

        /* find the leader */
        for_each_cpu(t, smt_mask) {
                if (t == cpu)
                        continue;
                rq = cpu_rq(t);
                if (rq->core == rq) {
                        core_rq = rq;
                        break;
                }
        }

        if (WARN_ON_ONCE(!core_rq)) /* whoopsie */
                goto unlock;

        /* install and validate core_rq */
        for_each_cpu(t, smt_mask) {
                rq = cpu_rq(t);

                if (t == cpu)
                        rq->core = core_rq;

                WARN_ON_ONCE(rq->core != core_rq);
        }

unlock:
        //sched_core_unlock(cpu, &flags);
        return;
}



#else /* !CONFIG_SCHED_CORE */

static inline void sched_core_cpu_starting(unsigned int cpu) {}
static inline void sched_core_cpu_deactivate(unsigned int cpu) {}
static inline void sched_core_cpu_dying(unsigned int cpu) {}

static struct task_struct *
pick_next_task(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
        return __pick_next_task(rq, prev, rf);
}

#endif /* CONFIG_SCHED_CORE */



/*
 * __schedule() is the main scheduler function.
 *
 * The main means of driving the scheduler and thus entering this function are:
 *
 *   1. Explicit blocking: mutex, semaphore, waitqueue, etc.
 *
 *   2. TIF_NEED_RESCHED flag is checked on interrupt and userspace return
 *      paths. For example, see arch/x86/entry_64.S.
 *
 *      To drive preemption between tasks, the scheduler sets the flag in timer
 *      interrupt handler scheduler_tick().
 *
 *   3. Wakeups don't really cause entry into schedule(). They add a
 *      task to the run-queue and that's it.
 *
 *      Now, if the new task added to the run-queue preempts the current
 *      task, then the wakeup sets TIF_NEED_RESCHED and schedule() gets
 *      called on the nearest possible occasion:
 *
 *       - If the kernel is preemptible (CONFIG_PREEMPTION=y):
 *
 *         - in syscall or exception context, at the next outmost
 *           preempt_enable(). (this might be as soon as the wake_up()'s
 *           spin_unlock()!)
 *
 *         - in IRQ context, return from interrupt-handler to
 *           preemptible context
 *
 *       - If the kernel is not preemptible (CONFIG_PREEMPTION is not set)
 *         then at the next:
 *
 *          - cond_resched() call
 *          - explicit schedule() call
 *          - return from syscall or exception to user-space
 *          - return from interrupt-handler to user-space
 *
 * WARNING: must be called with preemption disabled!
 */
static void __sched notrace __schedule(bool preempt)
{
    pr_fn_start_on(stack_depth);

    struct task_struct *prev, *next;
    unsigned long *switch_count;
    struct rq_flags rf;
    struct rq *rq;
    int cpu;

    cpu = smp_processor_id();
    rq = cpu_rq(cpu);
    prev = rq->curr;

    pr_view_on(stack_depth, "%20s : %d\n", cpu);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)prev);
    pr_view_on(stack_depth, "%20s : %ld\n", prev->state);

    schedule_debug(prev, preempt);

    //if (sched_feat(HRTICK))
        //hrtick_clear(rq);

    local_irq_disable();
    //rcu_note_context_switch(preempt);

    /*
     * Make sure that signal_pending_state()->signal_pending() below
     * can't be reordered with __set_current_state(TASK_INTERRUPTIBLE)
     * done by the caller to avoid the race with signal_wake_up().
     *
     * The membarrier system call requires a full memory barrier
     * after coming from user-space, before storing to rq->curr.
     */
    rq_lock(rq, &rf);
    //smp_mb__after_spinlock();

    /* Promote REQ to ACT */
    rq->clock_update_flags <<= 1;
    update_rq_clock(rq);

    switch_count = &prev->nivcsw;
    if (!preempt && prev->state) {
        if (signal_pending_state(prev->state, prev)) {
            prev->state = TASK_RUNNING;
        } else {
            deactivate_task(rq, prev, DEQUEUE_SLEEP | DEQUEUE_NOCLOCK);

            if (prev->in_iowait) {
                atomic_inc(&rq->nr_iowait);
                //delayacct_blkio_start();
            }
        }
        switch_count = &prev->nvcsw;
    }

    next = pick_next_task(rq, prev, &rf);
    clear_tsk_need_resched(prev);
    clear_preempt_need_resched();

    pr_view_on(stack_depth, "%20s : %p\n", (void*)prev);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)next);

    if (likely(prev != next)) {
        rq->nr_switches++;
        /*
         * RCU users of rcu_dereference(rq->curr) may not see
         * changes to task_struct made by pick_next_task().
         */
        RCU_INIT_POINTER(rq->curr, next);
        /*
         * The membarrier system call requires each architecture
         * to have a full memory barrier after updating
         * rq->curr, before returning to user-space.
         *
         * Here are the schemes providing that barrier on the
         * various architectures:
         * - mm ? switch_mm() : mmdrop() for x86, s390, sparc, PowerPC.
         *   switch_mm() rely on membarrier_arch_switch_mm() on PowerPC.
         * - finish_lock_switch() for weakly-ordered
         *   architectures where spin_unlock is a full barrier,
         * - switch_to() for arm64 (weakly-ordered, spin_unlock
         *   is a RELEASE barrier),
         */
        ++*switch_count;

        //trace_sched_switch(preempt, prev, next);

        /* Also unlocks the rq: */
        rq = context_switch(rq, prev, next, &rf);
    } else {
        rq->clock_update_flags &= ~(RQCF_ACT_SKIP|RQCF_REQ_SKIP);
        rq_unlock_irq(rq, &rf);
    }

    balance_callback(rq);

    current_task = next;
    pr_view_on(stack_depth, "%20s : %p\n", (void*)next);
    pr_view_on(stack_depth, "%20s : %p\n", current_task);

    pr_fn_end_on(stack_depth);
}

void __noreturn do_task_dead(void)
{
    /* Causes final put_task_struct in finish_task_switch(): */
    set_special_state(TASK_DEAD);

    /* Tell freezer to ignore us: */
    //current->flags |= PF_NOFREEZE;
    current_task->flags |= PF_NOFREEZE;

    __schedule(false);
    BUG();

    /* Avoid "noreturn function does return" - but don't continue if BUG() is a NOP: */
    for (;;)
        cpu_relax();
}

static inline void sched_submit_work(struct task_struct *tsk)
{
    if (!tsk->state)
        return;

    /*
     * If a worker went to sleep, notify and ask workqueue whether
     * it wants to wake up a task to maintain concurrency.
     * As this function is called inside the schedule() context,
     * we disable preemption to avoid it calling schedule() again
     * in the possible wakeup of a kworker.
     */
    if (tsk->flags & PF_WQ_WORKER) {
        preempt_disable();
        wq_worker_sleeping(tsk);
        preempt_enable_no_resched();
    }

    if (tsk_is_pi_blocked(tsk))
        return;

    /*
     * If we are going to sleep and we have plugged IO queued,
     * make sure to submit it to avoid deadlocks.
     */
    //if (blk_needs_flush_plug(tsk))
    //    blk_schedule_flush_plug(tsk);
}

static void sched_update_worker(struct task_struct *tsk)
{
    if (tsk->flags & PF_WQ_WORKER)
        wq_worker_running(tsk);
}

asmlinkage __visible void __sched schedule(void)
{
    pr_fn_start_on(stack_depth);

    //struct task_struct *tsk = current;
    struct task_struct *tsk = current_task;

    pr_view_on(stack_depth, "%20s : %p\n", (void*)current_task);
    if (!current_task) return;

    sched_submit_work(tsk);
    do {
        preempt_disable();
        __schedule(false);
        sched_preempt_enable_no_resched();
    } while (need_resched());
    sched_update_worker(tsk);

    pr_view_on(stack_depth, "%30s : %p\n", (void*)current_task);
    pr_view_on(stack_depth, "%30s : %p\n", (void*)&current_task->se);
    pr_view_on(stack_depth, "%30s : %llu\n", current_task->se.sum_exec_runtime);
    pr_view_on(stack_depth, "%30s : %llu\n", current_task->se.prev_sum_exec_runtime);

    pr_view_on(stack_depth, "%35s : %p\n", (void*)tsk);
    pr_view_on(stack_depth, "%35s : %p\n", (void*)&tsk->se);
    pr_view_on(stack_depth, "%35s : %llu\n", tsk->se.sum_exec_runtime);
    pr_view_on(stack_depth, "%35s : %llu\n", tsk->se.prev_sum_exec_runtime);

    pr_fn_end_on(stack_depth);
}
EXPORT_SYMBOL(schedule);

/*
 * synchronize_rcu_tasks() makes sure that no task is stuck in preempted
 * state (have scheduled out non-voluntarily) by making sure that all
 * tasks have either left the run queue or have gone into user space.
 * As idle tasks do not do either, they must not ever be preempted
 * (schedule out non-voluntarily).
 *
 * schedule_idle() is similar to schedule_preempt_disable() except that it
 * never enables preemption because it does not call sched_submit_work().
 */
void __sched schedule_idle(void)
{
    /*
     * As this skips calling sched_submit_work(), which the idle task does
     * regardless because that function is a nop when the task is in a
     * TASK_RUNNING state, make sure this isn't used someplace that the
     * current task can be in any other state. Note, idle is always in the
     * TASK_RUNNING state.
     */
    WARN_ON_ONCE(current->state);
    do {
        __schedule(false);
    } while (need_resched());
}
//4176 lines









//4342 lines
#ifdef CONFIG_RT_MUTEXES

static inline int __rt_effective_prio(struct task_struct *pi_task, int prio)
{
    if (pi_task)
        prio = min(prio, pi_task->prio);

    return prio;
}

static inline int rt_effective_prio(struct task_struct *p, int prio)
{
    struct task_struct *pi_task = rt_mutex_get_top_task(p);

    return __rt_effective_prio(pi_task, prio);
}

/*
 * rt_mutex_setprio - set the current priority of a task
 * @p: task to boost
 * @pi_task: donor task
 *
 * This function changes the 'effective' priority of a task. It does
 * not touch ->normal_prio like __setscheduler().
 *
 * Used by the rt_mutex code to implement priority inheritance
 * logic. Call site only calls if the priority of the task changed.
 */
void rt_mutex_setprio(struct task_struct *p, struct task_struct *pi_task)
{
    int prio, oldprio, queued, running, queue_flag =
        DEQUEUE_SAVE | DEQUEUE_MOVE | DEQUEUE_NOCLOCK;
    const struct sched_class *prev_class;
    struct rq_flags rf;
    struct rq *rq;

    /* XXX used to be waiter->prio, not waiter->task->prio */
    prio = __rt_effective_prio(pi_task, p->normal_prio);

    /*
     * If nothing changed; bail early.
     */
    if (p->pi_top_task == pi_task && prio == p->prio && !dl_prio(prio))
        return;

    rq = __task_rq_lock(p, &rf);
    update_rq_clock(rq);
    /*
     * Set under pi_lock && rq->lock, such that the value can be used under
     * either lock.
     *
     * Note that there is loads of tricky to make this pointer cache work
     * right. rt_mutex_slowunlock()+rt_mutex_postunlock() work together to
     * ensure a task is de-boosted (pi_task is set to NULL) before the
     * task is allowed to run again (and can exit). This ensures the pointer
     * points to a blocked task -- which guaratees the task is present.
     */
    p->pi_top_task = pi_task;

    /*
     * For FIFO/RR we only need to set prio, if that matches we're done.
     */
    if (prio == p->prio && !dl_prio(prio))
        goto out_unlock;

    /*
     * Idle task boosting is a nono in general. There is one
     * exception, when PREEMPT_RT and NOHZ is active:
     *
     * The idle task calls get_next_timer_interrupt() and holds
     * the timer wheel base->lock on the CPU and another CPU wants
     * to access the timer (probably to cancel it). We can safely
     * ignore the boosting request, as the idle CPU runs this code
     * with interrupts disabled and will complete the lock
     * protected section without being interrupted. So there is no
     * real need to boost.
     */
    if (unlikely(p == rq->idle)) {
        WARN_ON(p != rq->curr);
        WARN_ON(p->pi_blocked_on);
        goto out_unlock;
    }

    trace_sched_pi_setprio(p, pi_task);
    oldprio = p->prio;

    if (oldprio == prio)
        queue_flag &= ~DEQUEUE_MOVE;

    prev_class = p->sched_class;
    queued = task_on_rq_queued(p);
    running = task_current(rq, p);
    if (queued)
        dequeue_task(rq, p, queue_flag);
    if (running)
        put_prev_task(rq, p);

    /*
     * Boosting condition are:
     * 1. -rt task is running and holds mutex A
     *      --> -dl task blocks on mutex A
     *
     * 2. -dl task is running and holds mutex A
     *      --> -dl task blocks on mutex A and could preempt the
     *          running task
     */
    if (dl_prio(prio)) {
        if (!dl_prio(p->normal_prio) ||
            (pi_task && dl_entity_preempt(&pi_task->dl, &p->dl))) {
            p->dl.dl_boosted = 1;
            queue_flag |= ENQUEUE_REPLENISH;
        } else
            p->dl.dl_boosted = 0;
        p->sched_class = &dl_sched_class;
    } else if (rt_prio(prio)) {
        if (dl_prio(oldprio))
            p->dl.dl_boosted = 0;
        if (oldprio < prio)
            queue_flag |= ENQUEUE_HEAD;
        p->sched_class = &rt_sched_class;
    } else {
        if (dl_prio(oldprio))
            p->dl.dl_boosted = 0;
        if (rt_prio(oldprio))
            p->rt.timeout = 0;
        p->sched_class = &fair_sched_class;
    }

    p->prio = prio;

    if (queued)
        enqueue_task(rq, p, queue_flag);
    if (running)
        set_next_task(rq, p);

    check_class_changed(rq, p, prev_class, oldprio);
out_unlock:
    /* Avoid rq from going away on us: */
    preempt_disable();
    __task_rq_unlock(rq, &rf);

    balance_callback(rq);
    preempt_enable();
}
#else
static inline int rt_effective_prio(struct task_struct *p, int prio)
{
    return prio;
}
#endif
//4493
void set_user_nice(struct task_struct *p, long nice)
{
    bool queued, running;
    int old_prio, delta;
    struct rq_flags rf;
    struct rq *rq;

    pr_fn_start_on(stack_depth);

    if (task_nice(p) == nice || nice < MIN_NICE || nice > MAX_NICE)
        return;
    /*
     * We have to be careful, if called from sys_setpriority(),
     * the task might be in the middle of scheduling on another CPU.
     */
    rq = task_rq_lock(p, &rf);
    update_rq_clock(rq);

    /*
     * The RT priorities are set via sched_setscheduler(), but we still
     * allow the 'normal' nice value to be set - but as expected
     * it wont have any effect on scheduling until the task is
     * SCHED_DEADLINE, SCHED_FIFO or SCHED_RR:
     */
    if (task_has_dl_policy(p) || task_has_rt_policy(p)) {
        p->static_prio = NICE_TO_PRIO(nice);
        goto out_unlock;
    }
    queued = task_on_rq_queued(p);
    running = task_current(rq, p);
    if (queued)
        dequeue_task(rq, p, DEQUEUE_SAVE | DEQUEUE_NOCLOCK);
    if (running)
        put_prev_task(rq, p);

    p->static_prio = NICE_TO_PRIO(nice);
    set_load_weight(p, true);
    old_prio = p->prio;
    p->prio = effective_prio(p);
    delta = p->prio - old_prio;

    if (queued) {
        enqueue_task(rq, p, ENQUEUE_RESTORE | ENQUEUE_NOCLOCK);
        /*
         * If the task increased its priority or is running and
         * lowered its priority, then reschedule its CPU:
         */
        if (delta < 0 || (delta > 0 && task_running(rq, p)))
            resched_curr(rq);
    }
    if (running)
        set_next_task(rq, p);
out_unlock:
    task_rq_unlock(rq, p, &rf);

    pr_fn_end_on(stack_depth);
}
//EXPORT_SYMBOL(set_user_nice);
//4548
/*
 * can_nice - check if a task can reduce its nice value
 * @p: task
 * @nice: nice value
 */
int can_nice(const struct task_struct *p, const int nice)
{
    /* Convert nice value [19,-20] to rlimit style value [1,40]: */
    int nice_rlim = nice_to_rlimit(nice);

    //return (nice_rlim <= task_rlimit(p, RLIMIT_NICE) ||
        //capable(CAP_SYS_NICE));
    return (nice_rlim <= task_rlimit(p, RLIMIT_NICE));
}
//4562 lines





//4597 lines
/**
 * task_prio - return the priority value of a given task.
 * @p: the task in question.
 *
 * Return: The priority value as seen by users in /proc.
 * RT tasks are offset by -200. Normal tasks are centered
 * around 0, value goes from -16 to +15.
 */
int task_prio(const struct task_struct *p)
{
    return p->prio - MAX_RT_PRIO;
}

/**
 * idle_cpu - is a given CPU idle currently?
 * @cpu: the processor in question.
 *
 * Return: 1 if the CPU is currently idle. 0 otherwise.
 */
int idle_cpu(int cpu)
{
    struct rq *rq = cpu_rq(cpu);

    if (rq->curr != rq->idle)
        return 0;

    if (rq->nr_running)
        return 0;

#ifdef CONFIG_SMP
    if (!llist_empty(&rq->wake_list))
        return 0;
#endif

    return 1;
}

/**
 * available_idle_cpu - is a given CPU idle for enqueuing work.
 * @cpu: the CPU in question.
 *
 * Return: 1 if the CPU is currently idle. 0 otherwise.
 */
int available_idle_cpu(int cpu)
{
    if (!idle_cpu(cpu))
        return 0;

    if (vcpu_is_preempted(cpu))
        return 0;

    return 1;
}

/**
 * idle_task - return the idle task for a given CPU.
 * @cpu: the processor in question.
 *
 * Return: The idle task for the CPU @cpu.
 */
struct task_struct *idle_task(int cpu)
{
    return cpu_rq(cpu)->idle;
}

/**
 * find_process_by_pid - find a process with a matching PID value.
 * @pid: the pid in question.
 *
 * The task of @pid, if found. %NULL otherwise.
 */
static struct task_struct *find_process_by_pid(pid_t pid)
{
    //return pid ? find_task_by_vpid(pid) : current;
    return current_task;
}
//4673
/*
 * sched_setparam() passes in -1 for its policy, to let the functions
 * it calls know not to change it.
 */
#define SETPARAM_POLICY	-1

static void __setscheduler_params(struct task_struct *p,
        const struct sched_attr *attr)
{
    int policy = attr->sched_policy;

    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %d\n", policy);

    if (policy == SETPARAM_POLICY)
        policy = p->policy;

    p->policy = policy;

    if (dl_policy(policy))
        __setparam_dl(p, attr);
    else if (fair_policy(policy))
        p->static_prio = NICE_TO_PRIO(attr->sched_nice);

    /*
     * __sched_setscheduler() ensures attr->sched_priority == 0 when
     * !rt_policy. Always setting this ensures that things like
     * getparam()/getattr() don't report silly values for !rt tasks.
     */
    p->rt_priority = attr->sched_priority;
    p->normal_prio = normal_prio(p);

    set_load_weight(p, true);

    pr_fn_end_on(stack_depth);
}

/* Actually do priority change: must hold pi & rq lock. */
static void __setscheduler(struct rq *rq, struct task_struct *p,
               const struct sched_attr *attr, bool keep_boost)
{
    pr_fn_start_on(stack_depth);

    /*
     * If params can't change scheduling class changes aren't allowed
     * either.
     */
    if (attr->sched_flags & SCHED_FLAG_KEEP_PARAMS)
        return;

    __setscheduler_params(p, attr);

    /*
     * Keep a potential priority boosting if called from
     * sched_setscheduler().
     */
    p->prio = normal_prio(p);
    if (keep_boost)
        p->prio = rt_effective_prio(p, p->prio);

    if (dl_prio(p->prio))
        p->sched_class = &dl_sched_class;
    else if (rt_prio(p->prio))
        p->sched_class = &rt_sched_class;
    else
        p->sched_class = &fair_sched_class;

    pr_fn_end_on(stack_depth);
}

/*
 * Check the target process has a UID that matches the current process's:
 */
static bool check_same_owner(struct task_struct *p)
{
#if 0
    const struct cred *cred = current_cred(), *pcred;
    bool match;

    rcu_read_lock();
    pcred = __task_cred(p);
    match = (uid_eq(cred->euid, pcred->euid) ||
         uid_eq(cred->euid, pcred->uid));
    rcu_read_unlock();
    return match;
#endif //0
    return true;
}

static int __sched_setscheduler(struct task_struct *p,
                const struct sched_attr *attr,
                bool user, bool pi)
{
    pr_fn_start_on(stack_depth);

    int newprio = dl_policy(attr->sched_policy) ? MAX_DL_PRIO - 1 :
              MAX_RT_PRIO - 1 - attr->sched_priority;
    int retval, oldprio, oldpolicy = -1, queued, running;
    int new_effective_prio, policy = attr->sched_policy;
    const struct sched_class *prev_class;
    struct rq_flags rf;
    int reset_on_fork;
    int queue_flags = DEQUEUE_SAVE | DEQUEUE_MOVE | DEQUEUE_NOCLOCK;
    struct rq *rq;

    pr_view_on(stack_depth, "%20s : %d\n", newprio);
    pr_view_on(stack_depth, "%20s : %d\n", policy);

    /* The pi code expects interrupts enabled */
    //BUG_ON(pi && in_interrupt());
recheck:
    /* Double check policy once rq lock held: */
    if (policy < 0) {
        reset_on_fork = p->sched_reset_on_fork;
        policy = oldpolicy = p->policy;
    } else {
        reset_on_fork = !!(attr->sched_flags & SCHED_FLAG_RESET_ON_FORK);

        if (!valid_policy(policy))
            return -EINVAL;
    }

    if (attr->sched_flags & ~(SCHED_FLAG_ALL | SCHED_FLAG_SUGOV))
        return -EINVAL;

    pr_view_on(stack_depth, "%20s : 0x%X\n", attr->sched_flags);
    pr_view_on(stack_depth, "%20s : %u\n", attr->sched_priority);
    pr_view_on(stack_depth, "%20s : %u\n", attr->sched_policy);
    pr_view_on(stack_depth, "%20s : %d\n", attr->sched_nice);

    /*
     * Valid priorities for SCHED_FIFO and SCHED_RR are
     * 1..MAX_USER_RT_PRIO-1, valid priority for SCHED_NORMAL,
     * SCHED_BATCH and SCHED_IDLE is 0.
     */
    if ((p->mm && attr->sched_priority > MAX_USER_RT_PRIO-1) ||
        (!p->mm && attr->sched_priority > MAX_RT_PRIO-1))
        return -EINVAL;
    if ((dl_policy(policy) && !__checkparam_dl(attr)) ||
        (rt_policy(policy) != (attr->sched_priority != 0)))
        return -EINVAL;

    /*
     * Allow unprivileged RT tasks to decrease priority:
     */
    //if (user && !capable(CAP_SYS_NICE)) {
    if (user) {
        if (fair_policy(policy)) {
            if (attr->sched_nice < task_nice(p) &&
                !can_nice(p, attr->sched_nice))
                return -EPERM;
        }

        if (rt_policy(policy)) {
            unsigned long rlim_rtprio =
                    task_rlimit(p, RLIMIT_RTPRIO);

            pr_view_on(stack_depth, "%20s : %lu\n", rlim_rtprio);

            /* Can't set/change the rt policy: */
            if (policy != p->policy && !rlim_rtprio)
                return -EPERM;

            /* Can't increase priority: */
            if (attr->sched_priority > p->rt_priority &&
                attr->sched_priority > rlim_rtprio)
                return -EPERM;
        }

         /*
          * Can't set/change SCHED_DEADLINE policy at all for now
          * (safest behavior); in the future we would like to allow
          * unprivileged DL tasks to increase their relative deadline
          * or reduce their runtime (both ways reducing utilization)
          */
        if (dl_policy(policy))
            return -EPERM;

        /*
         * Treat SCHED_IDLE as nice 20. Only allow a switch to
         * SCHED_NORMAL if the RLIMIT_NICE would normally permit it.
         */
        if (task_has_idle_policy(p) && !idle_policy(policy)) {
            if (!can_nice(p, task_nice(p)))
                return -EPERM;
        }

        /* Can't change other user's priorities: */
        if (!check_same_owner(p))
            return -EPERM;

        /* Normal users shall not reset the sched_reset_on_fork flag: */
        if (p->sched_reset_on_fork && !reset_on_fork)
            return -EPERM;
    }

    if (user) {
        if (attr->sched_flags & SCHED_FLAG_SUGOV)
            return -EINVAL;

        //retval = security_task_setscheduler(p);
        //if (retval)
            //return retval;
    }

    /* Update task specific "requested" clamps */
    if (attr->sched_flags & SCHED_FLAG_UTIL_CLAMP) {
        retval = uclamp_validate(p, attr);
        if (retval)
            return retval;
    }

    //if (pi)
        //cpuset_read_lock();

    /*
     * Make sure no PI-waiters arrive (or leave) while we are
     * changing the priority of the task:
     *
     * To be able to change p->policy safely, the appropriate
     * runqueue lock must be held.
     */
    rq = task_rq_lock(p, &rf);

    pr_view_on(stack_depth, "%20s : %p\n", rq);

    update_rq_clock(rq);

    /*
     * Changing the policy of the stop threads its a very bad idea:
     */
    if (p == rq->stop) {
        retval = -EINVAL;
        goto unlock;
    }

    pr_view_on(stack_depth, "%20s : %d\n", policy);
    pr_view_on(stack_depth, "%20s : %u\n", p->policy);
    /*
     * If not changing anything there's no need to proceed further,
     * but store a possible modification of reset_on_fork.
     */
    if (unlikely(policy == p->policy)) {
        if (fair_policy(policy) && attr->sched_nice != task_nice(p))
            goto change;
        if (rt_policy(policy) && attr->sched_priority != p->rt_priority)
            goto change;
        if (dl_policy(policy) && dl_param_changed(p, attr))
            goto change;
        if (attr->sched_flags & SCHED_FLAG_UTIL_CLAMP)
            goto change;

        p->sched_reset_on_fork = reset_on_fork;
        retval = 0;
        goto unlock;
    }
change:

    if (user) {
#ifdef CONFIG_RT_GROUP_SCHED
        /*
         * Do not allow realtime tasks into groups that have no runtime
         * assigned.
         */
        if (rt_bandwidth_enabled() && rt_policy(policy) &&
                task_group(p)->rt_bandwidth.rt_runtime == 0 &&
                !task_group_is_autogroup(task_group(p))) {
            retval = -EPERM;
            goto unlock;
        }
#endif
#ifdef CONFIG_SMP
        if (dl_bandwidth_enabled() && dl_policy(policy) &&
                !(attr->sched_flags & SCHED_FLAG_SUGOV)) {
            cpumask_t *span = rq->rd->span;

            /*
             * Don't allow tasks with an affinity mask smaller than
             * the entire root_domain to become SCHED_DEADLINE. We
             * will also fail if there's no bandwidth available.
             */
            if (!cpumask_subset(span, p->cpus_ptr) ||
                rq->rd->dl_bw.bw == 0) {
                retval = -EPERM;
                goto unlock;
            }
        }
#endif
    }

    pr_view_on(stack_depth, "%20s : %d\n", oldpolicy);

    /* Re-check policy now with rq lock held: */
    if (unlikely(oldpolicy != -1 && oldpolicy != p->policy)) {
        policy = oldpolicy = -1;
        task_rq_unlock(rq, p, &rf);
        //if (pi)
            //cpuset_read_unlock();
        goto recheck;
    }

    /*
     * If setscheduling to SCHED_DEADLINE (or changing the parameters
     * of a SCHED_DEADLINE task) we need to check if enough bandwidth
     * is available.
     */
    if ((dl_policy(policy) || dl_task(p)) && sched_dl_overflow(p, policy, attr)) {
        retval = -EBUSY;
        goto unlock;
    }

    p->sched_reset_on_fork = reset_on_fork;
    oldprio = p->prio;

    if (pi) {
        /*
         * Take priority boosted tasks into account. If the new
         * effective priority is unchanged, we just store the new
         * normal parameters and do not touch the scheduler class and
         * the runqueue. This will be done when the task deboost
         * itself.
         */
        new_effective_prio = rt_effective_prio(p, newprio);
        if (new_effective_prio == oldprio)
            queue_flags &= ~DEQUEUE_MOVE;
    }

    queued = task_on_rq_queued(p);
    running = task_current(rq, p);

    pr_view_on(stack_depth, "%20s : %d\n", queued);
    pr_view_on(stack_depth, "%20s : %d\n", running);

    if (queued)
        dequeue_task(rq, p, queue_flags);
    if (running)
        put_prev_task(rq, p);

    prev_class = p->sched_class;

    pr_view_on(stack_depth, "%20s : %d\n", pi);
    __setscheduler(rq, p, attr, pi);
    __setscheduler_uclamp(p, attr);

    pr_sched_task_info(p);

    pr_view_on(stack_depth, "%20s : %d\n", queued);
    pr_view_on(stack_depth, "%20s : %d\n", running);
    if (queued) {
        /*
         * We enqueue to tail when the priority of a task is
         * increased (user space view).
         */
        if (oldprio < p->prio)
            queue_flags |= ENQUEUE_HEAD;

        enqueue_task(rq, p, queue_flags);
    }

    if (running)
        set_next_task(rq, p);

    check_class_changed(rq, p, prev_class, oldprio);

    /* Avoid rq from going away on us: */
    preempt_disable();
    task_rq_unlock(rq, p, &rf);

    if (pi) {
        //cpuset_read_unlock();
        rt_mutex_adjust_pi(p);
    }

    /* Run balance callbacks after we've adjusted the PI chain: */
    balance_callback(rq);
    preempt_enable();

    pr_fn_end_on(stack_depth);
    return 0;

unlock:
    task_rq_unlock(rq, p, &rf);
    //if (pi)
        //cpuset_read_unlock();

    pr_fn_end_on(stack_depth);
    return retval;
}

static int _sched_setscheduler(struct task_struct *p, int policy,
                   const struct sched_param *param, bool check)
{
    pr_fn_start_on(stack_depth);

    struct sched_attr attr = {
        .sched_policy   = policy,
        .sched_priority = param->sched_priority,
        .sched_nice	= PRIO_TO_NICE(p->static_prio),
    };

    /* Fixup the legacy SCHED_RESET_ON_FORK hack. */
    if ((policy != SETPARAM_POLICY) && (policy & SCHED_RESET_ON_FORK)) {
        attr.sched_flags |= SCHED_FLAG_RESET_ON_FORK;
        policy &= ~SCHED_RESET_ON_FORK;
        attr.sched_policy = policy;
    }

    pr_view_on(stack_depth, "%20s : %u\n", attr.sched_policy);
    pr_view_on(stack_depth, "%20s : %u\n", attr.sched_priority);
    pr_view_on(stack_depth, "%20s : %d\n", p->static_prio);
    pr_view_on(stack_depth, "%20s : %d\n", attr.sched_nice);
    pr_view_on(stack_depth, "%20s : 0x%llX\n", attr.sched_flags);

    return __sched_setscheduler(p, &attr, check, true);

    pr_fn_end_on(stack_depth);
}
/**
 * sched_setscheduler - change the scheduling policy and/or RT priority of a thread.
 * @p: the task in question.
 * @policy: new policy.
 * @param: structure containing the new RT priority.
 *
 * Return: 0 on success. An error code otherwise.
 *
 * NOTE that the task may be already dead.
 */
int sched_setscheduler_check(struct task_struct *p, int policy,
               const struct sched_param *param)
{
    return _sched_setscheduler(p, policy, param, true);
}
EXPORT_SYMBOL_GPL(sched_setscheduler);

int sched_setattr(struct task_struct *p, const struct sched_attr *attr)
{
    return __sched_setscheduler(p, attr, true, true);
}
EXPORT_SYMBOL_GPL(sched_setattr);

int sched_setattr_nocheck(struct task_struct *p, const struct sched_attr *attr)
{
    return __sched_setscheduler(p, attr, false, true);
}

/**
 * sched_setscheduler_nocheck - change the scheduling policy and/or RT priority of a thread from kernelspace.
 * @p: the task in question.
 * @policy: new policy.
 * @param: structure containing the new RT priority.
 *
 * Just like sched_setscheduler, only don't bother checking if the
 * current context has permission.  For example, this is needed in
 * stop_machine(): we create temporary high priority worker threads,
 * but our caller might not have that capability.
 *
 * Return: 0 on success. An error code otherwise.
 */
int sched_setscheduler_nocheck(struct task_struct *p, int policy,
                   const struct sched_param *param)
{
    return _sched_setscheduler(p, policy, param, false);
}
//EXPORT_SYMBOL_GPL(sched_setscheduler_nocheck);

static int
do_sched_setscheduler(pid_t pid, int policy, struct sched_param __user *param)
{
    struct sched_param lparam;
    struct task_struct *p;
    int retval;

    if (!param || pid < 0)
        return -EINVAL;
    if (copy_from_user(&lparam, param, sizeof(struct sched_param)))
        return -EFAULT;

    rcu_read_lock();
    retval = -ESRCH;
    p = find_process_by_pid(pid);
    if (likely(p))
        get_task_struct(p);
    rcu_read_unlock();

    if (likely(p)) {
        retval = sched_setscheduler(p, policy, &lparam);
        put_task_struct(p);
    }

    return retval;
}

/*
 * Mimics kernel/events/core.c perf_copy_attr().
 */
static int sched_copy_attr(struct sched_attr __user *uattr, struct sched_attr *attr)
{
    u32 size;
    int ret;

    /* Zero the full structure, so that a short copy will be nice: */
    memset(attr, 0, sizeof(*attr));

    ret = get_user(size, &uattr->size);
    if (ret)
        return ret;

    /* ABI compatibility quirk: */
    if (!size)
        size = SCHED_ATTR_SIZE_VER0;
    if (size < SCHED_ATTR_SIZE_VER0 || size > PAGE_SIZE)
        goto err_size;

    ret = copy_struct_from_user(attr, sizeof(*attr), uattr, size);
    if (ret) {
        if (ret == -E2BIG)
            goto err_size;
        return ret;
    }

    if ((attr->sched_flags & SCHED_FLAG_UTIL_CLAMP) &&
        size < SCHED_ATTR_SIZE_VER1)
        return -EINVAL;

    /*
     * XXX: Do we want to be lenient like existing syscalls; or do we want
     * to be strict and return an error on out-of-bounds values?
     */
    attr->sched_nice = clamp(attr->sched_nice, MIN_NICE, MAX_NICE);

    return 0;

err_size:
    put_user(sizeof(*attr), &uattr->size);
    return -E2BIG;
}
//5154 lines




//6008lines
/**
 * init_idle - set up an idle thread for a given CPU
 * @idle: task in question
 * @cpu: CPU the idle task belongs to
 *
 * NOTE: this function does not set the idle thread's NEED_RESCHED
 * flag, to make booting more robust.
 */
void init_idle(struct task_struct *idle, int cpu)
{
    struct rq *rq = cpu_rq(cpu);
    unsigned long flags;

    __sched_fork(0, idle);

    raw_spin_lock_irqsave(&idle->pi_lock, flags);
    raw_spin_lock(&rq->lock);

    idle->state = TASK_RUNNING;
    idle->se.exec_start = sched_clock();
    idle->flags |= PF_IDLE;

    //kasan_unpoison_task_stack(idle);

#ifdef CONFIG_SMP
    /*
         * Its possible that init_idle() gets called multiple times on a task,
         * in that case do_set_cpus_allowed() will not do the right thing.
         *
         * And since this is boot we can forgo the serialization.
         */
    set_cpus_allowed_common(idle, cpumask_of(cpu));
#endif
    /*
         * We're having a chicken and egg problem, even though we are
         * holding rq->lock, the CPU isn't yet set to this CPU so the
         * lockdep check in task_group() will fail.
         *
         * Similar case to sched_fork(). / Alternatively we could
         * use task_rq_lock() here and obtain the other rq->lock.
         *
         * Silence PROVE_RCU
         */
    rcu_read_lock();
    __set_task_cpu(idle, cpu);
    rcu_read_unlock();

    rq->idle = idle;
    rcu_assign_pointer(rq->curr, idle);
    idle->on_rq = TASK_ON_RQ_QUEUED;
#ifdef CONFIG_SMP
    idle->on_cpu = 1;
#endif
    raw_spin_unlock(&rq->lock);
    raw_spin_unlock_irqrestore(&idle->pi_lock, flags);

    /* Set the preempt count _outside_ the spinlocks! */
    init_idle_preempt_count(idle, cpu);

    /*
         * The idle tasks have their own, simple scheduling class:
         */
    idle->sched_class = &idle_sched_class;
    //ftrace_graph_init_idle_task(idle, cpu);
    //vtime_init_idle(idle, cpu);
#ifdef CONFIG_SMP
    sprintf(idle->comm, "%s/%d", INIT_TASK_COMM, cpu);
#endif
}

#ifdef CONFIG_SMP




//6120 lines
bool sched_smp_initialized __read_mostly;



#ifdef CONFIG_HOTPLUG_CPU
/*
 * Ensure that the idle task is using init_mm right before its CPU goes
 * offline.
 */
void idle_task_exit(void)
{
    struct mm_struct *mm = current->active_mm;

    BUG_ON(cpu_online(smp_processor_id()));
#if 0
    if (mm != &init_mm) {
        switch_mm(mm, &init_mm, current);
        current->active_mm = &init_mm;
        finish_arch_post_lock_switch();
    }

    mmdrop(mm);
#endif
}

/*
 * Since this CPU is going 'away' for a while, fold any nr_active delta
 * we might have. Assumes we're called after migrate_tasks() so that the
 * nr_active count is stable. We need to take the teardown thread which
 * is calling this into account, so we hand in adjust = 1 to the load
 * calculation.
 *
 * Also see the comment "Global load-average calculations".
 */
static void calc_load_migrate(struct rq *rq)
{
    long delta = calc_load_fold_active(rq, 1);
    if (delta)
        atomic_long_add(delta, &calc_load_tasks);
}

static struct task_struct *__pick_migrate_task(struct rq *rq)
{
    const struct sched_class *class;
    struct task_struct *next;

    pr_fn_start_on(stack_depth);

    for_each_class(class) {
        next = class->pick_next_task(rq);
        if (next) {
            next->sched_class->put_prev_task(rq, next);
            return next;
        }
    }

    /* The idle class should always have a runnable task */
    BUG();

    pr_fn_end_on(stack_depth);
}

/*
 * Migrate all tasks from the rq, sleeping tasks will be migrated by
 * try_to_wake_up()->select_task_rq().
 *
 * Called with rq->lock held even though we'er in stop_machine() and
 * there's no concurrency possible, we hold the required locks anyway
 * because of lock validation efforts.
 */
static void migrate_tasks(struct rq *dead_rq, struct rq_flags *rf)
{
    struct rq *rq = dead_rq;
    struct task_struct *next, *stop = rq->stop;
    struct rq_flags orf = *rf;
    int dest_cpu;

    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %p\n", rq->stop);
    pr_view_on(stack_depth, "%20s : %p\n", stop);

    /*
         * Fudge the rq selection such that the below task selection loop
         * doesn't get stuck on the currently eligible stop task.
         *
         * We're currently inside stop_machine() and the rq is either stuck
         * in the stop_machine_cpu_stop() loop, or we're executing this code,
         * either way we should never end up calling schedule() until we're
         * done here.
         */
    rq->stop = NULL;
    pr_view_on(stack_depth, "%20s : %p\n", rq->stop);

    /*
         * put_prev_task() and pick_next_task() sched
         * class method both need to have an up-to-date
         * value of rq->clock[_task]
         */
    update_rq_clock(rq);

    for (;;) {
        pr_view_on(stack_depth, "%30s : %u\n", rq->nr_running);
        /*
                 * There's this thread running, bail when that's the only
                 * remaining thread:
                 */
        if (rq->nr_running == 1)
            break;

        next = __pick_migrate_task(rq);

        /*
                 * Rules for changing task_struct::cpus_mask are holding
                 * both pi_lock and rq->lock, such that holding either
                 * stabilizes the mask.
                 *
                 * Drop rq->lock is not quite as disastrous as it usually is
                 * because !cpu_active at this point, which means load-balance
                 * will not interfere. Also, stop-machine.
                 */
        rq_unlock(rq, rf);
        raw_spin_lock(&next->pi_lock);
        rq_relock(rq, rf);

        /*
                 * Since we're inside stop-machine, _nothing_ should have
                 * changed the task, WARN if weird stuff happened, because in
                 * that case the above rq->lock drop is a fail too.
                 */
        if (WARN_ON(task_rq(next) != rq || !task_on_rq_queued(next))) {
            raw_spin_unlock(&next->pi_lock);
            continue;
        }

        /* Find suitable destination for @next, with force if needed. */
        dest_cpu = select_fallback_rq(dead_rq->cpu, next);
        pr_view_on(stack_depth, "%30s : %d\n", dest_cpu);

        rq = __migrate_task(rq, rf, next, dest_cpu);
        if (rq != dead_rq) {
            rq_unlock(rq, rf);
            rq = dead_rq;
            *rf = orf;
            rq_relock(rq, rf);
        }
        raw_spin_unlock(&next->pi_lock);
    }

    rq->stop = stop;

    pr_view_on(stack_depth, "%30s : %p\n", stop);
    pr_view_on(stack_depth, "%30s : %p\n", rq->stop);
    pr_fn_end_on(stack_depth);
}
#endif /* CONFIG_HOTPLUG_CPU */


//6304 lines
void set_rq_online(struct rq *rq)
{
    pr_fn_start_on(stack_depth);

    if (!rq->online) {
        const struct sched_class *class;

        cpumask_set_cpu(rq->cpu, rq->rd->online);
        rq->online = 1;

        for_each_class(class) {
            if (class->rq_online)
                class->rq_online(rq);
        }
    }

    pr_view_on(stack_depth, "%20s : %d\n", rq->online);

    pr_fn_end_on(stack_depth);
}

void set_rq_offline(struct rq *rq)
{
    if (rq->online) {
        const struct sched_class *class;

        for_each_class(class) {
            if (class->rq_offline)
                class->rq_offline(rq);
        }

        cpumask_clear_cpu(rq->cpu, rq->rd->online);
        rq->online = 0;
    }
}

/*
 * used to mark begin/end of suspend/resume:
 */
static int num_cpus_frozen;
//6339
/*
 * Update cpusets according to cpu_active mask.  If cpusets are
 * disabled, cpuset_update_active_cpus() becomes a simple wrapper
 * around partition_sched_domains().
 *
 * If we come here as part of a suspend/resume, don't touch cpusets because we
 * want to restore it back to its original state upon resume anyway.
 */
static void cpuset_cpu_active(void)
{
    if (cpuhp_tasks_frozen) {
        /*
         * num_cpus_frozen tracks how many CPUs are involved in suspend
         * resume sequence. As long as this is not the last online
         * operation in the resume sequence, just build a single sched
         * domain, ignoring cpusets.
         */
        partition_sched_domains(1, NULL, NULL);
        if (--num_cpus_frozen)
            return;
        /*
         * This is the last CPU online operation. So fall through and
         * restore the original sched domains by considering the
         * cpuset configurations.
         */
        //cpuset_force_rebuild();
    }
    //cpuset_update_active_cpus();
}

static int cpuset_cpu_inactive(unsigned int cpu)
{
    if (!cpuhp_tasks_frozen) {
        if (dl_cpu_busy(cpu))
            return -EBUSY;
        //cpuset_update_active_cpus();
    } else {
        num_cpus_frozen++;
        partition_sched_domains(1, NULL, NULL);
    }
    return 0;
}

int sched_cpu_activate(unsigned int cpu)
{
    struct rq *rq = cpu_rq(cpu);
    struct rq_flags rf;

#ifdef CONFIG_SCHED_SMT
    /*
     * When going up, increment the number of cores with SMT present.
     */
    if (cpumask_weight(cpu_smt_mask(cpu)) == 2)
        static_branch_inc_cpuslocked(&sched_smt_present);
#endif
    set_cpu_active(cpu, true);

    if (sched_smp_initialized) {
        sched_domains_numa_masks_set(cpu);
        cpuset_cpu_active();
    }

    /*
     * Put the rq online, if not already. This happens:
     *
     * 1) In the early boot process, because we build the real domains
     *    after all CPUs have been brought up.
     *
     * 2) At runtime, if cpuset_cpu_active() fails to rebuild the
     *    domains.
     */
    rq_lock_irqsave(rq, &rf);
    if (rq->rd) {
        BUG_ON(!cpumask_test_cpu(cpu, rq->rd->span));
        set_rq_online(rq);
    }
    rq_unlock_irqrestore(rq, &rf);

    return 0;
}

int sched_cpu_deactivate(unsigned int cpu)
{
    int ret;

    set_cpu_active(cpu, false);
    /*
     * We've cleared cpu_active_mask, wait for all preempt-disabled and RCU
     * users of this state to go away such that all new such users will
     * observe it.
     *
     * Do sync before park smpboot threads to take care the rcu boost case.
     */
    //synchronize_rcu();

#ifdef CONFIG_SCHED_SMT
    /*
     * When going down, decrement the number of cores with SMT present.
     */
    if (cpumask_weight(cpu_smt_mask(cpu)) == 2)
        static_branch_dec_cpuslocked(&sched_smt_present);
#endif

    if (!sched_smp_initialized)
        return 0;

    ret = cpuset_cpu_inactive(cpu);
    if (ret) {
        set_cpu_active(cpu, true);
        return ret;
    }
    sched_domains_numa_masks_clear(cpu);
    return 0;
}

static void sched_rq_cpu_starting(unsigned int cpu)
{
    struct rq *rq = cpu_rq(cpu);

    rq->calc_load_update = calc_load_update;
    update_max_interval();
}

int sched_cpu_starting(unsigned int cpu)
{
    sched_core_cpu_starting(cpu);	//>=v5.19
    sched_rq_cpu_starting(cpu);
    //sched_tick_start(cpu);
    return 0;
}

#ifdef CONFIG_HOTPLUG_CPU
//kernel/cpu.c
//struct cpuhp_step cpuhp_hp_states[]
int sched_cpu_dying(unsigned int cpu)
{
    struct rq *rq = cpu_rq(cpu);
    struct rq_flags rf;

    pr_fn_start_on(stack_depth);

    /* Handle pending wakeups and then migrate everything off */
    sched_ttwu_pending();
    sched_tick_stop(cpu);

    rq_lock_irqsave(rq, &rf);
    if (rq->rd) {
        BUG_ON(!cpumask_test_cpu(cpu, rq->rd->span));
        set_rq_offline(rq);
    }
    migrate_tasks(rq, &rf);
    BUG_ON(rq->nr_running != 1);
    rq_unlock_irqrestore(rq, &rf);

    calc_load_migrate(rq);
    update_max_interval();
    //nohz_balance_exit_idle(rq);
    //hrtick_clear(rq);

    pr_fn_end_on(stack_depth);
    return 0;
}
#endif

static int __init migration_init(void)
{
    sched_cpu_starting(smp_processor_id());
    return 0;
}
//early_initcall(migration_init);


//6496
void __init sched_init_smp(void)
{
    pr_fn_start_on(stack_depth);

    sched_init_numa();

    /*
     * There's no userspace yet to cause hotplug operations; hence all the
     * CPU masks are stable and all blatant races in the below code cannot
     * happen.
     */
    mutex_lock(&sched_domains_mutex);
    sched_init_domains(cpu_active_mask);
    mutex_unlock(&sched_domains_mutex);

    /* Move init over to a non-isolated CPU */
    //if (set_cpus_allowed_ptr(current, housekeeping_cpumask(HK_FLAG_DOMAIN)) < 0)
    //    BUG();
    sched_init_granularity();

    init_sched_rt_class();
    init_sched_dl_class();

    sched_smp_initialized = true;

    migration_init();

    pr_fn_end_on(stack_depth);
}



#else
void __init sched_init_smp(void)
{
    sched_init_granularity();
}
#endif /* CONFIG_SMP */

#if 0
int in_sched_functions(unsigned long addr)
{
    return in_lock_functions(addr) ||
        (addr >= (unsigned long)__sched_text_start
        && addr < (unsigned long)__sched_text_end);
}
#endif //0

//6541
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

static DECLARE_PER_CPU(cpumask_var_t, load_balance_mask);
static DECLARE_PER_CPU(cpumask_var_t, select_idle_mask);

//init/main.c:
//  start_kernel()
void __init sched_init(void)
{
        pr_fn_start_on(stack_depth);

        unsigned long ptr = 0;
        int i;

        //wait_bit_init();

#ifdef CONFIG_FAIR_GROUP_SCHED
        ptr += 2 * nr_cpu_ids * sizeof(void **);
#endif
#ifdef CONFIG_RT_GROUP_SCHED
        ptr += 2 * nr_cpu_ids * sizeof(void **);
#endif
        pr_view_on(stack_depth, "%30s : %u\n", nr_cpu_ids);
        pr_view_on(stack_depth, "%30s : %d\n", sizeof(void **));
        pr_view_on(stack_depth, "%30s : %lu\n", ptr);
        pr_view_on(stack_depth, "%30s : %p\n", &root_task_group);
        if (ptr) {
                //ptr = (unsigned long)kzalloc(ptr, GFP_NOWAIT);
                ptr = kzalloc(ptr, GFP_NOWAIT);
                pr_view_on(stack_depth, "%30s : %p\n", ptr);

#ifdef CONFIG_FAIR_GROUP_SCHED
                root_task_group.se = (struct sched_entity **)ptr;
                pr_view_on(stack_depth, "%30s : %p\n", root_task_group.se);
                ptr += nr_cpu_ids * sizeof(void **);

                root_task_group.cfs_rq = (struct cfs_rq **)ptr;
                pr_view_on(stack_depth, "%30s : %p\n", root_task_group.cfs_rq);
                ptr += nr_cpu_ids * sizeof(void **);
#endif /* CONFIG_FAIR_GROUP_SCHED */

#ifdef CONFIG_RT_GROUP_SCHED
                root_task_group.rt_se = (struct sched_rt_entity **)ptr;
                pr_view_on(stack_depth, "%30s : %p\n", root_task_group.rt_se);
                ptr += nr_cpu_ids * sizeof(void **);

                root_task_group.rt_rq = (struct rt_rq **)ptr;
                pr_view_on(stack_depth, "%30s : %p\n", root_task_group.rt_rq);
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

        pr_view_on(stack_depth, "%30s : 0x%X\n", __cpu_possible_mask.bits[0]);
        pr_view_on(stack_depth, "%30s : %p\n", (void*)&runqueues);

        for_each_possible_cpu(i) {
            struct rq *rq;
            rq = cpu_rq(i);

            pr_view_on(stack_depth, "%30s : %d\n", i);
            pr_view_on(stack_depth, "%30s : %p\n", (void*)rq);
            pr_view_on(stack_depth, "%30s : %llu\n", rq->clock);
            pr_view_on(stack_depth, "%30s : %p\n", (void*)&rq->cfs);
            pr_view_on(stack_depth, "%30s : %p\n", (void*)&rq->rt);
            pr_view_on(stack_depth, "%30s : %p\n", (void*)&rq->dl);

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
            pr_view_on(stack_depth, "%30s : %p\n", (void*)&rq->leaf_cfs_rq_list);
            pr_view_on(stack_depth, "%30s : %p\n", (void*)rq->tmp_alone_branch);
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

            rq_attach_root(rq, &def_root_domain);
    #ifdef CONFIG_NO_HZ_COMMON
            rq->last_load_update_tick = jiffies;
            rq->last_blocked_load_update_tick = jiffies;
            atomic_set(&rq->nohz_flags, 0);
    #endif
    #endif /* CONFIG_SMP */
            //hrtick_rq_init(rq);
            atomic_set(&rq->nr_iowait, 0);

#ifdef CONFIG_SCHED_CORE
            rq->core = rq;
            rq->core_pick = NULL;
            rq->core_enabled = 0;
            rq->core_tree = RB_ROOT;
            rq->core_forceidle_count = 0;
            rq->core_forceidle_occupation = 0;
            rq->core_forceidle_start = 0;

            rq->core_cookie = 0UL;
#endif
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
        current_task = &init_task;
        init_idle(current, smp_processor_id());

        calc_load_update = jiffies + LOAD_FREQ;
        pr_view_on(stack_depth, "%30s : %lu\n", jiffies);
        pr_view_on(stack_depth, "%30s : %lu\n", calc_load_update);

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

        pr_fn_end_on(stack_depth);
}
//6720 lines






//6910 lines
#ifdef CONFIG_CGROUP_SCHED
/* task_group_lock serializes the addition/removal of task groups */
static DEFINE_SPINLOCK(task_group_lock);

static inline void alloc_uclamp_sched_group(struct task_group *tg,
                        struct task_group *parent)
{
#ifdef CONFIG_UCLAMP_TASK_GROUP
    enum uclamp_id clamp_id;

    for_each_clamp_id(clamp_id) {
        uclamp_se_set(&tg->uclamp_req[clamp_id],
                  uclamp_none(clamp_id), false);
        tg->uclamp[clamp_id] = parent->uclamp[clamp_id];
    }
#endif
}

static void sched_free_group(struct task_group *tg)
{
    free_fair_sched_group(tg);
    //free_rt_sched_group(tg);
    autogroup_free(tg);
    kmem_cache_free(task_group_cache, tg);
}

/* allocate runqueue etc for a new task group */
struct task_group *sched_create_group(struct task_group *parent)
{
    struct task_group *tg;

    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)parent);

    tg = kmem_cache_alloc(task_group_cache, GFP_KERNEL | __GFP_ZERO);
    if (!tg)
        return ERR_PTR(-ENOMEM);

    pr_view_on(stack_depth, "%20s : %p\n", (void*)tg);
    if (!alloc_fair_sched_group(tg, parent))
        goto err;

    if (!alloc_rt_sched_group(tg, parent))
        goto err;

    alloc_uclamp_sched_group(tg, parent);

    pr_fn_end_on(stack_depth);

    return tg;

err:
    sched_free_group(tg);
    return ERR_PTR(-ENOMEM);
}

void sched_online_group(struct task_group *tg, struct task_group *parent)
{
    unsigned long flags;

    pr_fn_start_on(stack_depth);

    spin_lock_irqsave(&task_group_lock, flags);
    list_add_rcu(&tg->list, &task_groups);

    /* Root should already exist: */
    WARN_ON(!parent);

    tg->parent = parent;
    INIT_LIST_HEAD(&tg->children);
    list_add_rcu(&tg->siblings, &parent->children);
    spin_unlock_irqrestore(&task_group_lock, flags);

    online_fair_sched_group(tg);

    pr_fn_end_on(stack_depth);
}

/* rcu callback to free various structures associated with a task group */
static void sched_free_group_rcu(struct rcu_head *rhp)
{
    /* Now it should be safe to free those cfs_rqs: */
    sched_free_group(container_of(rhp, struct task_group, rcu));
}

void sched_destroy_group(struct task_group *tg)
{
    /* Wait for possible concurrent references to cfs_rqs complete: */
    call_rcu(&tg->rcu, sched_free_group_rcu);
}

void sched_offline_group(struct task_group *tg)
{
    unsigned long flags;

    /* End participation in shares distribution: */
    unregister_fair_sched_group(tg);

    spin_lock_irqsave(&task_group_lock, flags);
    list_del_rcu(&tg->list);
    list_del_rcu(&tg->siblings);
    spin_unlock_irqrestore(&task_group_lock, flags);
}

static void sched_change_group(struct task_struct *tsk, int type)
{
    struct task_group *tg;

    /*
     * All callers are synchronized by task_rq_lock(); we do not use RCU
     * which is pointless here. Thus, we pass "true" to task_css_check()
     * to prevent lockdep warnings.
     */
    tg = container_of(task_css_check(tsk, cpu_cgrp_id, true),
              struct task_group, css);
    tg = autogroup_task_group(tsk, tg);
    tsk->sched_task_group = tg;

#ifdef CONFIG_FAIR_GROUP_SCHED
    if (tsk->sched_class->task_change_group)
        tsk->sched_class->task_change_group(tsk, type);
    else
#endif
        set_task_rq(tsk, task_cpu(tsk));
}

/*
 * Change task's runqueue when it moves between groups.
 *
 * The caller of this function should have put the task in its new group by
 * now. This function just updates tsk->se.cfs_rq and tsk->se.parent to reflect
 * its new group.
 */
void sched_move_task(struct task_struct *tsk)
{
    int queued, running, queue_flags =
        DEQUEUE_SAVE | DEQUEUE_MOVE | DEQUEUE_NOCLOCK;
    struct rq_flags rf;
    struct rq *rq;

    rq = task_rq_lock(tsk, &rf);
    update_rq_clock(rq);

    running = task_current(rq, tsk);
    queued = task_on_rq_queued(tsk);

    if (queued)
        dequeue_task(rq, tsk, queue_flags);
    if (running)
        put_prev_task(rq, tsk);

    sched_change_group(tsk, TASK_MOVE_GROUP);

    if (queued)
        enqueue_task(rq, tsk, queue_flags);
    if (running)
        set_next_task(rq, tsk);

    task_rq_unlock(rq, tsk, &rf);
}
//7061
static inline struct task_group *css_tg(struct cgroup_subsys_state *css)
{
    return css ? container_of(css, struct task_group, css) : NULL;
}

static struct cgroup_subsys_state *
cpu_cgroup_css_alloc(struct cgroup_subsys_state *parent_css)
{
    struct task_group *parent = css_tg(parent_css);
    struct task_group *tg;

    if (!parent) {
        /* This is early initialization for the top cgroup */
        return &root_task_group.css;
    }

    tg = sched_create_group(parent);
    if (IS_ERR(tg))
        return ERR_PTR(-ENOMEM);

    return &tg->css;
}

/* Expose task group only after completing cgroup initialization */
static int cpu_cgroup_css_online(struct cgroup_subsys_state *css)
{
    struct task_group *tg = css_tg(css);
    struct task_group *parent = css_tg(css->parent);

    if (parent)
        sched_online_group(tg, parent);
    return 0;
}

static void cpu_cgroup_css_released(struct cgroup_subsys_state *css)
{
    struct task_group *tg = css_tg(css);

    sched_offline_group(tg);
}

static void cpu_cgroup_css_free(struct cgroup_subsys_state *css)
{
    struct task_group *tg = css_tg(css);

    /*
     * Relies on the RCU grace period between css_released() and this.
     */
    sched_free_group(tg);
}

/*
 * This is called before wake_up_new_task(), therefore we really only
 * have to set its group bits, all the other stuff does not apply.
 */
static void cpu_cgroup_fork(struct task_struct *task)
{
    struct rq_flags rf;
    struct rq *rq;

    rq = task_rq_lock(task, &rf);

    update_rq_clock(rq);
    sched_change_group(task, TASK_SET_GROUP);

    task_rq_unlock(rq, task, &rf);
}

static int cpu_cgroup_can_attach(struct cgroup_taskset *tset)
{
    struct task_struct *task;
    struct cgroup_subsys_state *css;
    int ret = 0;

    cgroup_taskset_for_each(task, css, tset) {
#ifdef CONFIG_RT_GROUP_SCHED
        if (!sched_rt_can_attach(css_tg(css), task))
            return -EINVAL;
#endif
        /*
         * Serialize against wake_up_new_task() such that if its
         * running, we're sure to observe its full state.
         */
        raw_spin_lock_irq(&task->pi_lock);
        /*
         * Avoid calling sched_move_task() before wake_up_new_task()
         * has happened. This would lead to problems with PELT, due to
         * move wanting to detach+attach while we're not attached yet.
         */
        if (task->state == TASK_NEW)
            ret = -EINVAL;
        raw_spin_unlock_irq(&task->pi_lock);

        if (ret)
            break;
    }
    return ret;
}

static void cpu_cgroup_attach(struct cgroup_taskset *tset)
{
    struct task_struct *task;
    struct cgroup_subsys_state *css;

    cgroup_taskset_for_each(task, css, tset)
        sched_move_task(task);
}
//7169 lines



//7341 lines
#ifdef CONFIG_FAIR_GROUP_SCHED
static int cpu_shares_write_u64(struct cgroup_subsys_state *css,
                                struct cftype *cftype, u64 shareval)
{
    if (shareval > scale_load_down(ULONG_MAX))
        shareval = MAX_SHARES;
    return sched_group_set_shares(css_tg(css), scale_load(shareval));
}

static u64 cpu_shares_read_u64(struct cgroup_subsys_state *css,
                               struct cftype *cft)
{
    struct task_group *tg = css_tg(css);

    return (u64) scale_load_down(tg->shares);
}

#ifdef CONFIG_CFS_BANDWIDTH
static DEFINE_MUTEX(cfs_constraints_mutex);

const u64 max_cfs_quota_period = 1 * NSEC_PER_SEC; /* 1s */
static const u64 min_cfs_quota_period = 1 * NSEC_PER_MSEC; /* 1ms */

static int __cfs_schedulable(struct task_group *tg, u64 period, u64 runtime);

//static int tg_set_cfs_bandwidth(struct task_group *tg, u64 period, u64 quota)
int tg_set_cfs_bandwidth(struct task_group *tg, u64 period, u64 quota)
{
    int i, ret = 0, runtime_enabled, runtime_was_enabled;
    struct cfs_bandwidth *cfs_b = &tg->cfs_bandwidth;

    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%10s : %llu\n", period);
    pr_view_on(stack_depth, "%10s : %llu\n", quota);

    if (tg == &root_task_group)
        return -EINVAL;

    /*
     * Ensure we have at some amount of bandwidth every period.  This is
     * to prevent reaching a state of large arrears when throttled via
     * entity_tick() resulting in prolonged exit starvation.
     */
    if (quota < min_cfs_quota_period || period < min_cfs_quota_period)
        return -EINVAL;

    /*
     * Likewise, bound things on the otherside by preventing insane quota
     * periods.  This also allows us to normalize in computing quota
     * feasibility.
     */
    if (period > max_cfs_quota_period)
        return -EINVAL;

    /*
     * Prevent race between setting of cfs_rq->runtime_enabled and
     * unthrottle_offline_cfs_rqs().
     */
    get_online_cpus();
    mutex_lock(&cfs_constraints_mutex);
    ret = __cfs_schedulable(tg, period, quota);

    pr_view_on(stack_depth, "%10s : %d\n", ret);
    if (ret)
        goto out_unlock;

    runtime_enabled = quota != RUNTIME_INF;
    runtime_was_enabled = cfs_b->quota != RUNTIME_INF;
    /*
     * If we need to toggle cfs_bandwidth_used, off->on must occur
     * before making related changes, and on->off must occur afterwards
     */
    if (runtime_enabled && !runtime_was_enabled)
        cfs_bandwidth_usage_inc();
    raw_spin_lock_irq(&cfs_b->lock);
    cfs_b->period = ns_to_ktime(period);
    cfs_b->quota = quota;

    __refill_cfs_bandwidth_runtime(cfs_b);

    pr_view_on(stack_depth, "%20s : %lld\n", cfs_b->period);
    pr_view_on(stack_depth, "%20s : %llu\n", cfs_b->quota);
    pr_view_on(stack_depth, "%20s : %llu\n", cfs_b->runtime);
    pr_view_on(stack_depth, "%20s : %d\n", runtime_enabled);

    /* Restart the period timer (if active) to handle new period expiry: */
    if (runtime_enabled)
        start_cfs_bandwidth(cfs_b);

    raw_spin_unlock_irq(&cfs_b->lock);

    for_each_online_cpu(i) {
        struct cfs_rq *cfs_rq = tg->cfs_rq[i];
        struct rq *rq = cfs_rq->rq;
        struct rq_flags rf;

        rq_lock_irq(rq, &rf);
        cfs_rq->runtime_enabled = runtime_enabled;
        cfs_rq->runtime_remaining = 0;

        pr_view_on(stack_depth, "%20s : cpu : %d\n", i);
        pr_view_on(stack_depth, "%30s : %p\n", cfs_rq);
        pr_view_on(stack_depth, "%30s : %d\n", cfs_rq->throttled);
        pr_view_on(stack_depth, "%30s : %lld\n", cfs_rq->runtime_remaining);

        if (cfs_rq->throttled)
            unthrottle_cfs_rq(cfs_rq);
        rq_unlock_irq(rq, &rf);
    }
    if (runtime_was_enabled && !runtime_enabled)
        cfs_bandwidth_usage_dec();
out_unlock:
    mutex_unlock(&cfs_constraints_mutex);
    put_online_cpus();

    pr_view_on(stack_depth, "%30s : %d\n", ret);
    pr_fn_end_on(stack_depth);
    return ret;
}

static int tg_set_cfs_quota(struct task_group *tg, long cfs_quota_us)
{
    u64 quota, period;

    period = ktime_to_ns(tg->cfs_bandwidth.period);
    if (cfs_quota_us < 0)
        quota = RUNTIME_INF;
    else if ((u64)cfs_quota_us <= U64_MAX / NSEC_PER_USEC)
        quota = (u64)cfs_quota_us * NSEC_PER_USEC;
    else
        return -EINVAL;

    return tg_set_cfs_bandwidth(tg, period, quota);
}

static long tg_get_cfs_quota(struct task_group *tg)
{
    u64 quota_us;

    if (tg->cfs_bandwidth.quota == RUNTIME_INF)
        return -1;

    quota_us = tg->cfs_bandwidth.quota;
    do_div(quota_us, NSEC_PER_USEC);

    return quota_us;
}

static int tg_set_cfs_period(struct task_group *tg, long cfs_period_us)
{
    u64 quota, period;

    if ((u64)cfs_period_us > U64_MAX / NSEC_PER_USEC)
        return -EINVAL;

    period = (u64)cfs_period_us * NSEC_PER_USEC;
    quota = tg->cfs_bandwidth.quota;

    return tg_set_cfs_bandwidth(tg, period, quota);
}

static long tg_get_cfs_period(struct task_group *tg)
{
    u64 cfs_period_us;

    cfs_period_us = ktime_to_ns(tg->cfs_bandwidth.period);
    do_div(cfs_period_us, NSEC_PER_USEC);

    return cfs_period_us;
}

static s64 cpu_cfs_quota_read_s64(struct cgroup_subsys_state *css,
                                  struct cftype *cft)
{
    return tg_get_cfs_quota(css_tg(css));
}

static int cpu_cfs_quota_write_s64(struct cgroup_subsys_state *css,
                                   struct cftype *cftype, s64 cfs_quota_us)
{
    return tg_set_cfs_quota(css_tg(css), cfs_quota_us);
}

static u64 cpu_cfs_period_read_u64(struct cgroup_subsys_state *css,
                                   struct cftype *cft)
{
    return tg_get_cfs_period(css_tg(css));
}

static int cpu_cfs_period_write_u64(struct cgroup_subsys_state *css,
                                    struct cftype *cftype, u64 cfs_period_us)
{
    return tg_set_cfs_period(css_tg(css), cfs_period_us);
}

struct cfs_schedulable_data {
    struct task_group *tg;
    u64 period, quota;
};

/*
 * normalize group quota/period to be quota/max_period
 * note: units are usecs
 */
static u64 normalize_cfs_quota(struct task_group *tg,
                               struct cfs_schedulable_data *d)
{
    u64 quota, period, ratio;
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %p\n", tg);
    pr_view_on(stack_depth, "%20s : %p\n", d->tg);
    if (tg == d->tg) {
        period = d->period;
        quota = d->quota;
    } else {
        period = tg_get_cfs_period(tg);
        quota = tg_get_cfs_quota(tg);
    }

    pr_view_on(stack_depth, "%20s : %llu\n", period);
    pr_view_on(stack_depth, "%20s : %llu\n", quota);

    /* note: these should typically be equivalent */
    if (quota == RUNTIME_INF || quota == -1)
        return RUNTIME_INF;

    ratio = to_ratio(period, quota);

    pr_view_on(stack_depth, "%20s : %llu\n", ratio);
    pr_fn_end_on(stack_depth);
    return ratio;
}

static int tg_cfs_schedulable_down(struct task_group *tg, void *data)
{
    struct cfs_schedulable_data *d = data;
    struct cfs_bandwidth *cfs_b = &tg->cfs_bandwidth;
    s64 quota = 0, parent_quota = -1;

    pr_fn_start_on(stack_depth);

    if (!tg->parent) {
        quota = RUNTIME_INF;
    } else {
        struct cfs_bandwidth *parent_b = &tg->parent->cfs_bandwidth;

        quota = normalize_cfs_quota(tg, d);
        parent_quota = parent_b->hierarchical_quota;

        pr_view_on(stack_depth, "%20s : %lld\n", quota);
        pr_view_on(stack_depth, "%20s : %lld\n", parent_quota);

        /*
         * Ensure max(child_quota) <= parent_quota.  On cgroup2,
         * always take the min.  On cgroup1, only inherit when no
         * limit is set:
         */
        //Compile Error: undefined reference to `cpu_cgrp_subsys_on_dfl_key'
        //if (cgroup_subsys_on_dfl(cpu_cgrp_subsys)) {
        //    quota = min(quota, parent_quota);
        //} else {
            if (quota == RUNTIME_INF)
                quota = parent_quota;
            else if (parent_quota != RUNTIME_INF && quota > parent_quota)
                return -EINVAL;
        //}
    }
    cfs_b->hierarchical_quota = quota;

    pr_view_on(stack_depth, "%30s : %lld\n", quota);
    pr_view_on(stack_depth, "%30s : %lld\n", cfs_b->hierarchical_quota);
    pr_view_on(stack_depth, "%30s : %lld\n", parent_quota);

    pr_fn_end_on(stack_depth);
    return 0;
}

static int __cfs_schedulable(struct task_group *tg, u64 period, u64 quota)
{
    int ret;
    struct cfs_schedulable_data data = {
        .tg = tg,
        .period = period,
        .quota = quota,
    };

    if (quota != RUNTIME_INF) {
        do_div(data.period, NSEC_PER_USEC);
        do_div(data.quota, NSEC_PER_USEC);
    }

    rcu_read_lock();
    ret = walk_tg_tree(tg_cfs_schedulable_down, tg_nop, &data);
    rcu_read_unlock();

    return ret;
}

static int cpu_cfs_stat_show(struct seq_file *sf, void *v)
{
    struct task_group *tg = css_tg(seq_css(sf));
    struct cfs_bandwidth *cfs_b = &tg->cfs_bandwidth;

    seq_printf(sf, "nr_periods %d\n", cfs_b->nr_periods);
    seq_printf(sf, "nr_throttled %d\n", cfs_b->nr_throttled);
    seq_printf(sf, "throttled_time %llu\n", cfs_b->throttled_time);

    if (schedstat_enabled() && tg != &root_task_group) {
        u64 ws = 0;
        int i;

        for_each_possible_cpu(i)
                ws += schedstat_val(tg->se[i]->statistics.wait_sum);

        seq_printf(sf, "wait_sum %llu\n", ws);
    }

    return 0;
}
#endif /* CONFIG_CFS_BANDWIDTH */
#endif /* CONFIG_FAIR_GROUP_SCHED */
//7624 lines





//7703 lines
static int cpu_extra_stat_show(struct seq_file *sf,
                   struct cgroup_subsys_state *css)
{
#ifdef CONFIG_CFS_BANDWIDTH
    {
        struct task_group *tg = css_tg(css);
        struct cfs_bandwidth *cfs_b = &tg->cfs_bandwidth;
        u64 throttled_usec;

        throttled_usec = cfs_b->throttled_time;
        do_div(throttled_usec, NSEC_PER_USEC);

        seq_printf(sf, "nr_periods %d\n"
               "nr_throttled %d\n"
               "throttled_usec %llu\n",
               cfs_b->nr_periods, cfs_b->nr_throttled,
               throttled_usec);
    }
#endif
    return 0;
}
//7725 lines





//7884 lines
struct cgroup_subsys cpu_cgrp_subsys = {
    .css_alloc	= cpu_cgroup_css_alloc,
    .css_online	= cpu_cgroup_css_online,
    .css_released	= cpu_cgroup_css_released,
    .css_free	= cpu_cgroup_css_free,
    .css_extra_stat_show = cpu_extra_stat_show,
    .fork		= cpu_cgroup_fork,
    .can_attach	= cpu_cgroup_can_attach,
    .attach		= cpu_cgroup_attach,
#if 0
    .legacy_cftypes	= cpu_legacy_files,
    .dfl_cftypes	= cpu_files,
#endif //0
    .early_init	= true,
    .threaded	= true,
};

#endif	/* CONFIG_CGROUP_SCHED */
//7901 lines




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
