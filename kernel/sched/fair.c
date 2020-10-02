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
#include "sched.h"





/*
 * All the scheduling class methods:
 */
const struct sched_class fair_sched_class = {
    .next			= &idle_sched_class,
#if 0
    .enqueue_task		= enqueue_task_fair,
    .dequeue_task		= dequeue_task_fair,
    .yield_task		= yield_task_fair,
    .yield_to_task		= yield_to_task_fair,

    .check_preempt_curr	= check_preempt_wakeup,

    .pick_next_task		= pick_next_task_fair,
    .put_prev_task		= put_prev_task_fair,
    .set_next_task          = set_next_task_fair,

#ifdef CONFIG_SMP
    .balance		= balance_fair,
    .select_task_rq		= select_task_rq_fair,
    .migrate_task_rq	= migrate_task_rq_fair,

    .rq_online		= rq_online_fair,
    .rq_offline		= rq_offline_fair,

    .task_dead		= task_dead_fair,
    .set_cpus_allowed	= set_cpus_allowed_common,
#endif

    .task_tick		= task_tick_fair,
    .task_fork		= task_fork_fair,

    .prio_changed		= prio_changed_fair,
    .switched_from		= switched_from_fair,
    .switched_to		= switched_to_fair,

    .get_rr_interval	= get_rr_interval_fair,

    .update_curr		= update_curr_fair,

#ifdef CONFIG_FAIR_GROUP_SCHED
    .task_change_group	= task_change_group_fair,
#endif

#ifdef CONFIG_UCLAMP_TASK
    .uclamp_enabled		= 1,
#endif

#endif //0
};
