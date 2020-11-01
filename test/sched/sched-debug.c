// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/sched/sched-test.c
 *  Linux Kernel Schedular Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdio_ext.h>

#include <linux/sched.h>
#include <kernel/sched/pelt.h>
#include "test/debug.h"

static void _pr_sched_rq(struct rq *rq)
{
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);

    pr_info_view_on(stack_depth, "%30s : %llu\n", rq->clock);
    pr_info_view_on(stack_depth, "%30s : %llu\n", rq->clock_task);
    pr_info_view_on(stack_depth, "%30s : %llu\n", rq->clock_pelt);
    pr_info_view_on(stack_depth, "%30s : %lu\n", rq->lost_idle_time);
    pr_info_view_on(stack_depth, "%30s : %u\n", rq->nr_running);

    pr_out_on(stack_depth, "\n");
}

static void _pr_sched_cfs_rq(struct cfs_rq *cfs_rq)
{
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);

    pr_info_view_on(stack_depth, "%30s : %d\n", cfs_rq->throttle_count);
    //pr_info_view_on(stack_depth, "%40s : %llu\n", cfs_rq->throttled_clock);
    //pr_info_view_on(stack_depth, "%40s : %llu\n", cfs_rq->throttled_clock_task);
    //pr_info_view_on(stack_depth, "%40s : %llu\n", cfs_rq->throttled_clock_task_time);

    if (cfs_rq->tg) {
        pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->tg->shares);
        pr_info_view_on(stack_depth, "%30s : %ld\n", atomic_long_read(&cfs_rq->tg->load_avg));
    }

    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->tg_load_avg_contrib);
    pr_info_view_on(stack_depth, "%30s : %ld\n", cfs_rq->propagate);
    pr_info_view_on(stack_depth, "%30s : %ld\n", cfs_rq->prop_runnable_sum);

    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->runnable_weight);
    pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->nr_running);
    pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->h_nr_running);
    pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->idle_h_nr_running);

    pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->removed.nr);
    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.load_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.util_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.runnable_sum);

    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->load.weight);
    pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->load.inv_weight);

    pr_out_on(stack_depth, "\n");
}

static void _pr_sched_avg(struct sched_avg *sa)
{
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)sa);

    pr_info_view_on(stack_depth, "%30s : %llu\n", sa->last_update_time);
    pr_info_view_on(stack_depth, "%30s : %llu\n", sa->load_sum);
    pr_info_view_on(stack_depth, "%30s : %llu\n", sa->runnable_load_sum);
    pr_info_view_on(stack_depth, "%30s : %u\n", sa->util_sum);
    pr_info_view_on(stack_depth, "%30s : %u\n", sa->period_contrib);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa->load_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa->runnable_load_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa->util_avg);
    //pr_info_view_on(stack_depth, "%30s : %llu\n", sa->util_est);

    pr_out_on(stack_depth, "\n");
}

void pr_sched_info(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "=====================================================\n");
    u64 now = 0;

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%20s : %d\n", se->on_rq);

    struct cfs_rq *cfs_rq;
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->cfs_rq);
    cfs_rq = (se->on_rq) ? se->cfs_rq : se->my_q;
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);

    if (cfs_rq) {
        struct rq *rq = rq_of(cfs_rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)&rq->cfs);

        _pr_sched_rq(rq);
        _pr_sched_cfs_rq(cfs_rq);
        _pr_sched_avg(&cfs_rq->avg);

        now = cfs_rq_clock_pelt(cfs_rq);	//rq->clock_pelt - rq->lost_idle_time;
    }

    pr_info_view_on(stack_depth, "%20s : %llu\n", now);
    pr_info_view_on(stack_depth, "%20s : %lu\n", se->load.weight);
    pr_info_view_on(stack_depth, "%20s : %u\n", se->load.inv_weight);
    _pr_sched_avg(&se->avg);

    pr_out_on(stack_depth, "=====================================================\n");
    pr_fn_end_on(stack_depth);
}
