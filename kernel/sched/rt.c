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


