// SPDX-License-Identifier: GPL-2.0-only
/*
 * Generic entry points for the idle threads and
 * implementation of the idle task scheduling class.
 *
 * (NOTE: these are not related to SCHED_IDLE batch scheduled
 *        tasks which are handled in sched/fair.c )
 */
#include "sched.h"



//451 lines
/*
 * Simple, special scheduling class for the per-CPU idle tasks:
 */
const struct sched_class idle_sched_class = {
    /* .next is NULL */
    /* no enqueue/yield_task for idle tasks */
#if 0
    /* dequeue is not valid, we print a debug message there: */
    .dequeue_task		= dequeue_task_idle,

    .check_preempt_curr	= check_preempt_curr_idle,

    .pick_next_task		= pick_next_task_idle,
    .put_prev_task		= put_prev_task_idle,
    .set_next_task          = set_next_task_idle,

#ifdef CONFIG_SMP
    .balance		= balance_idle,
    .select_task_rq		= select_task_rq_idle,
    .set_cpus_allowed	= set_cpus_allowed_common,
#endif

    .task_tick		= task_tick_idle,

    .get_rr_interval	= get_rr_interval_idle,

    .prio_changed		= prio_changed_idle,
    .switched_to		= switched_to_idle,
    .update_curr		= update_curr_idle,
#endif //0
};
