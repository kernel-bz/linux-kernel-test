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
//1288 lines





//1336 lines
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
    if (prev_class != p->sched_class) {
        if (prev_class->switched_from)
            prev_class->switched_from(rq, p);

        p->sched_class->switched_to(rq, p);
    } else if (oldprio != p->prio || dl_task(p))
        p->sched_class->prio_changed(rq, p, oldprio);
}

void check_preempt_curr(struct rq *rq, struct task_struct *p, int flags)
{
    const struct sched_class *class;

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
}
//1440 lines





//2675 lines
/*
 * Perform scheduler related setup for a newly forked process p.
 * p is forked by current.
 *
 * __sched_fork() is basic setup used by init_idle() too:
 */
static void __sched_fork(unsigned long clone_flags, struct task_struct *p)
{
    p->on_rq			= 0;

    p->se.on_rq			= 0;
    p->se.exec_start		= 0;
    p->se.sum_exec_runtime		= 0;
    p->se.prev_sum_exec_runtime	= 0;
    p->se.nr_migrations		= 0;
    p->se.vruntime			= 0;
    INIT_LIST_HEAD(&p->se.group_node);

#ifdef CONFIG_FAIR_GROUP_SCHED
    //p->se.cfs_rq			= NULL;
#endif

#ifdef CONFIG_SCHEDSTATS
    /* Even if schedstat is disabled, there should not be garbage */
    memset(&p->se.statistics, 0, sizeof(p->se.statistics));
#endif

    RB_CLEAR_NODE(&p->dl.rb_node);
    //init_dl_task_timer(&p->dl);
    //init_dl_inactive_task_timer(&p->dl);
    //__dl_clear_params(p);

    INIT_LIST_HEAD(&p->rt.run_list);
    p->rt.timeout		= 0;
    p->rt.time_slice	= sched_rr_timeslice;
    p->rt.on_rq		= 0;
    p->rt.on_list		= 0;

#ifdef CONFIG_PREEMPT_NOTIFIERS
    INIT_HLIST_HEAD(&p->preempt_notifiers);
#endif

#ifdef CONFIG_COMPACTION
    p->capture_control = NULL;
#endif
    init_numa_balancing(clone_flags, p);
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
    //p->prio = current->normal_prio;

    uclamp_fork(p);

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

    pr_info_view("%30s : %d\n", p->prio);

    if (dl_prio(p->prio))
        return -EAGAIN;
    else if (rt_prio(p->prio))
        p->sched_class = &rt_sched_class;
    else
        p->sched_class = &fair_sched_class;

    init_entity_runnable_average(&p->se);

    pr_info_view("%30s : %p\n", &rt_sched_class);
    pr_info_view("%30s : %p\n", &fair_sched_class);
    pr_info_view("%30s : %p\n", p->sched_class);

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
    //__set_task_cpu(p, smp_processor_id());	//error

    pr_info_view("%30s : %p\n", p->sched_class->task_fork);
    pr_info_view("%30s : %p\n", p->se.cfs_rq);

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



//6078 lines
#ifdef CONFIG_SMP





//6496 lines
//init/main.c:
//  start_kernel()
//    rest_init()
//      kernel_thread(kernel_init, NULL, CLONE_FS)
//        kernel_init()
//          kernel_init_freeable()
#if 0
void __init sched_init_smp(void)
{
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
    if (set_cpus_allowed_ptr(current, housekeeping_cpumask(HK_FLAG_DOMAIN)) < 0)
        BUG();
    sched_init_granularity();

    init_sched_rt_class();
    init_sched_dl_class();

    sched_smp_initialized = true;
}

static int __init migration_init(void)
{
    sched_cpu_starting(smp_processor_id());
    return 0;
}
early_initcall(migration_init);
#endif //0

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

DECLARE_PER_CPU(cpumask_var_t, load_balance_mask);
DECLARE_PER_CPU(cpumask_var_t, select_idle_mask);

//init/main.c:
//  start_kernel()
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
                pr_info_view("%30s : %p\n", root_task_group.se);

                root_task_group.cfs_rq = (struct cfs_rq **)ptr;
                ptr += nr_cpu_ids * sizeof(void **);
                pr_info_view("%30s : %p\n", root_task_group.cfs_rq);
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

        pr_info_view("%30s : %p\n", &def_rt_bandwidth);
        pr_info_view("%30s : %p\n", &def_dl_bandwidth);

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
        pr_info_view("%30s : %lu\n", init_task.se.load.weight);
        pr_info_view("%30s : %u\n", init_task.se.load.inv_weight);

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
        pr_info_view("%20s : %lu\n", jiffies);
        pr_info_view("%20s : %lu\n", calc_load_update);

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
//6720 lines





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
