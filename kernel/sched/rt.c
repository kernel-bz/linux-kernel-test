// SPDX-License-Identifier: GPL-2.0
/*
 * Real-Time Scheduling Class (mapped to the SCHED_FIFO and SCHED_RR
 * policies)
 */
#include "sched.h"
#include "pelt.h"

int sched_rr_timeslice = RR_TIMESLICE;
int sysctl_sched_rr_timeslice = (MSEC_PER_SEC / HZ) * RR_TIMESLICE;

static int do_sched_rt_period_timer(struct rt_bandwidth *rt_b, int overrun);

struct rt_bandwidth def_rt_bandwidth;

static enum hrtimer_restart sched_rt_period_timer(struct hrtimer *timer)
{
    struct rt_bandwidth *rt_b =
        container_of(timer, struct rt_bandwidth, rt_period_timer);
    int idle = 0;
    int overrun;

    raw_spin_lock(&rt_b->rt_runtime_lock);
    for (;;) {
        overrun = hrtimer_forward_now(timer, rt_b->rt_period);
        if (!overrun)
            break;

        raw_spin_unlock(&rt_b->rt_runtime_lock);
        //idle = do_sched_rt_period_timer(rt_b, overrun);
        raw_spin_lock(&rt_b->rt_runtime_lock);
    }
    if (idle)
        rt_b->rt_period_active = 0;
    raw_spin_unlock(&rt_b->rt_runtime_lock);

    return idle ? HRTIMER_NORESTART : HRTIMER_RESTART;
}
//40 lines
void init_rt_bandwidth(struct rt_bandwidth *rt_b, u64 period, u64 runtime)
{
    rt_b->rt_period = ns_to_ktime(period);
    rt_b->rt_runtime = runtime;

    raw_spin_lock_init(&rt_b->rt_runtime_lock);

    hrtimer_init(&rt_b->rt_period_timer, CLOCK_MONOTONIC,
             HRTIMER_MODE_REL_HARD);
    rt_b->rt_period_timer.function = sched_rt_period_timer;
}


//75 lines
void init_rt_rq(struct rt_rq *rt_rq)
{
    struct rt_prio_array *array;
    int i;

    array = &rt_rq->active;
    for (i = 0; i < MAX_RT_PRIO; i++) {
        INIT_LIST_HEAD(array->queue + i);
        __clear_bit(i, array->bitmap);
    }
    /* delimiter for bitsearch: */
    __set_bit(MAX_RT_PRIO, array->bitmap);

#if defined CONFIG_SMP
    rt_rq->highest_prio.curr = MAX_RT_PRIO;
    rt_rq->highest_prio.next = MAX_RT_PRIO;
    rt_rq->rt_nr_migratory = 0;
    rt_rq->overloaded = 0;
    plist_head_init(&rt_rq->pushable_tasks);
#endif /* CONFIG_SMP */
    /* We start is dequeued state, because no RT tasks are queued */
    rt_rq->rt_queued = 0;

    rt_rq->rt_time = 0;
    rt_rq->rt_throttled = 0;
    rt_rq->rt_runtime = 0;
    raw_spin_lock_init(&rt_rq->rt_runtime_lock);
}





//104 lines
#ifdef CONFIG_RT_GROUP_SCHED
static void destroy_rt_bandwidth(struct rt_bandwidth *rt_b)
{
    hrtimer_cancel(&rt_b->rt_period_timer);
}

#define rt_entity_is_task(rt_se) (!(rt_se)->my_q)

static inline struct task_struct *rt_task_of(struct sched_rt_entity *rt_se)
{
#ifdef CONFIG_SCHED_DEBUG
    WARN_ON_ONCE(!rt_entity_is_task(rt_se));
#endif
    return container_of(rt_se, struct task_struct, rt);
}

static inline struct rq *rq_of_rt_rq(struct rt_rq *rt_rq)
{
    return rt_rq->rq;
}

static inline struct rt_rq *rt_rq_of_se(struct sched_rt_entity *rt_se)
{
    return rt_se->rt_rq;
}

static inline struct rq *rq_of_rt_se(struct sched_rt_entity *rt_se)
{
    struct rt_rq *rt_rq = rt_se->rt_rq;

    return rt_rq->rq;
}

void free_rt_sched_group(struct task_group *tg)
{
    int i;

    if (tg->rt_se)
        destroy_rt_bandwidth(&tg->rt_bandwidth);

    for_each_possible_cpu(i) {
        if (tg->rt_rq)
            kfree(tg->rt_rq[i]);
        if (tg->rt_se)
            kfree(tg->rt_se[i]);
    }

    kfree(tg->rt_rq);
    kfree(tg->rt_se);
}

void init_tg_rt_entry(struct task_group *tg, struct rt_rq *rt_rq,
        struct sched_rt_entity *rt_se, int cpu,
        struct sched_rt_entity *parent)
{
    struct rq *rq = cpu_rq(cpu);

    rt_rq->highest_prio.curr = MAX_RT_PRIO;
    rt_rq->rt_nr_boosted = 0;
    rt_rq->rq = rq;
    rt_rq->tg = tg;

    tg->rt_rq[cpu] = rt_rq;
    tg->rt_se[cpu] = rt_se;

    if (!rt_se)
        return;

    if (!parent)
        rt_se->rt_rq = &rq->rt;
    else
        rt_se->rt_rq = parent->my_q;

    rt_se->my_q = rt_rq;
    rt_se->parent = parent;
    INIT_LIST_HEAD(&rt_se->run_list);
}

int alloc_rt_sched_group(struct task_group *tg, struct task_group *parent)
{
    struct rt_rq *rt_rq;
    struct sched_rt_entity *rt_se;
    int i;

    tg->rt_rq = kcalloc(nr_cpu_ids, sizeof(rt_rq), GFP_KERNEL);
    if (!tg->rt_rq)
        goto err;
    tg->rt_se = kcalloc(nr_cpu_ids, sizeof(rt_se), GFP_KERNEL);
    if (!tg->rt_se)
        goto err;

    init_rt_bandwidth(&tg->rt_bandwidth,
            ktime_to_ns(def_rt_bandwidth.rt_period), 0);

    for_each_possible_cpu(i) {
        rt_rq = kzalloc_node(sizeof(struct rt_rq),
                     GFP_KERNEL, cpu_to_node(i));
        if (!rt_rq)
            goto err;

        rt_se = kzalloc_node(sizeof(struct sched_rt_entity),
                     GFP_KERNEL, cpu_to_node(i));
        if (!rt_se)
            goto err_free_rq;

        init_rt_rq(rt_rq);
        rt_rq->rt_runtime = tg->rt_bandwidth.rt_runtime;
        init_tg_rt_entry(tg, rt_rq, rt_se, i, parent->rt_se[i]);
    }

    return 1;

err_free_rq:
    kfree(rt_rq);
err:
    return 0;
}
//222
#else /* CONFIG_RT_GROUP_SCHED */

#define rt_entity_is_task(rt_se) (1)

static inline struct task_struct *rt_task_of(struct sched_rt_entity *rt_se)
{
    return container_of(rt_se, struct task_struct, rt);
}

static inline struct rq *rq_of_rt_rq(struct rt_rq *rt_rq)
{
    return container_of(rt_rq, struct rq, rt);
}

static inline struct rq *rq_of_rt_se(struct sched_rt_entity *rt_se)
{
    struct task_struct *p = rt_task_of(rt_se);

    return task_rq(p);
}

static inline struct rt_rq *rt_rq_of_se(struct sched_rt_entity *rt_se)
{
    struct rq *rq = rq_of_rt_se(rt_se);

    return &rq->rt;
}

void free_rt_sched_group(struct task_group *tg) { }

int alloc_rt_sched_group(struct task_group *tg, struct task_group *parent)
{
    return 1;
}
#endif /* CONFIG_RT_GROUP_SCHED */
//258 lines







//2356 lines
const struct sched_class rt_sched_class = {
    .next			= &fair_sched_class,
#if 0
    .enqueue_task		= enqueue_task_rt,
    .dequeue_task		= dequeue_task_rt,
    .yield_task		= yield_task_rt,

    .check_preempt_curr	= check_preempt_curr_rt,

    .pick_next_task		= pick_next_task_rt,
    .put_prev_task		= put_prev_task_rt,
    .set_next_task          = set_next_task_rt,

#ifdef CONFIG_SMP
    .balance		= balance_rt,
    .select_task_rq		= select_task_rq_rt,
    .set_cpus_allowed       = set_cpus_allowed_common,
    .rq_online              = rq_online_rt,
    .rq_offline             = rq_offline_rt,
    .task_woken		= task_woken_rt,
    .switched_from		= switched_from_rt,
#endif

    .task_tick		= task_tick_rt,

    .get_rr_interval	= get_rr_interval_rt,

    .prio_changed		= prio_changed_rt,
    .switched_to		= switched_to_rt,

    .update_curr		= update_curr_rt,

#ifdef CONFIG_UCLAMP_TASK
    .uclamp_enabled		= 1,
#endif

#endif //0
};


