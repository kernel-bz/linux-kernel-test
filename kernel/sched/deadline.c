// SPDX-License-Identifier: GPL-2.0
/*
 * Deadline Scheduling Class (SCHED_DEADLINE)
 *
 * Earliest Deadline First (EDF) + Constant Bandwidth Server (CBS).
 *
 * Tasks that periodically executes their instances for less than their
 * runtime won't miss any of their deadlines.
 * Tasks that are not periodic or sporadic or that tries to execute more
 * than their reserved bandwidth will be slowed down (and may potentially
 * miss some of their deadlines), and won't affect any other task.
 *
 * Copyright (C) 2012 Dario Faggioli <raistlin@linux.it>,
 *                    Juri Lelli <juri.lelli@gmail.com>,
 *                    Michael Trimarchi <michael@amarulasolutions.com>,
 *                    Fabio Checconi <fchecconi@gmail.com>
 */
#include "sched.h"
#include "pelt.h"

struct dl_bandwidth def_dl_bandwidth;




const struct sched_class dl_sched_class = {
    .next			= &rt_sched_class,
#if 0
    .enqueue_task		= enqueue_task_dl,
    .dequeue_task		= dequeue_task_dl,
    .yield_task		= yield_task_dl,

    .check_preempt_curr	= check_preempt_curr_dl,

    .pick_next_task		= pick_next_task_dl,
    .put_prev_task		= put_prev_task_dl,
    .set_next_task		= set_next_task_dl,

#ifdef CONFIG_SMP
    .balance		= balance_dl,
    .select_task_rq		= select_task_rq_dl,
    .migrate_task_rq	= migrate_task_rq_dl,
    .set_cpus_allowed       = set_cpus_allowed_dl,
    .rq_online              = rq_online_dl,
    .rq_offline             = rq_offline_dl,
    .task_woken		= task_woken_dl,
#endif

    .task_tick		= task_tick_dl,
    .task_fork              = task_fork_dl,

    .prio_changed           = prio_changed_dl,
    .switched_from		= switched_from_dl,
    .switched_to		= switched_to_dl,

    .update_curr		= update_curr_dl,
#endif //0
};
