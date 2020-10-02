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


