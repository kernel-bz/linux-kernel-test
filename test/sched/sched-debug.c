// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/sched/sched-test.c
 *  Linux Kernel Schedular Debug Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdio_ext.h>

#include <linux/sched.h>
#include <kernel/sched/pelt.h>
#include <linux/list.h>
#include "test/debug.h"

static void _pr_sched_rq(struct rq *rq)
{
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    pr_info_view_on(stack_depth, "%20s : %llu\n", rq->clock);
    pr_info_view_on(stack_depth, "%20s : %llu\n", rq->clock_task);
    pr_info_view_on(stack_depth, "%20s : %llu\n", rq->clock_pelt);
    pr_info_view_on(stack_depth, "%20s : %lu\n", rq->lost_idle_time);
    pr_info_view_on(stack_depth, "%20s : %u\n", rq->nr_running);

    pr_out_on(stack_depth, "\n");
}

static void _pr_sched_cfs_rq_pelt(struct cfs_rq *cfs_rq)
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

void pr_sched_avg_info(struct sched_avg *sa)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%30s : %llu\n", sa->last_update_time);
    pr_info_view_on(stack_depth, "%30s : %llu\n", sa->load_sum);
    pr_info_view_on(stack_depth, "%30s : %llu\n", sa->runnable_load_sum);
    pr_info_view_on(stack_depth, "%30s : %u\n", sa->util_sum);
    pr_info_view_on(stack_depth, "%30s : %u\n", sa->period_contrib);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa->load_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa->runnable_load_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa->util_avg);
    //pr_info_view_on(stack_depth, "%30s : %llu\n", sa->util_est);

    pr_fn_end_on(stack_depth);
}

static void _pr_sched_se_info(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
    if(!se) goto _end;

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->parent);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->cfs_rq);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q);
    pr_info_view_on(stack_depth, "%20s : %d\n", se->on_rq);
    pr_info_view_on(stack_depth, "%20s : %d\n", se->depth);

 _end:
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_cfs_rq_pelt_info(struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);
    u64 now = 0;

    if (cfs_rq) {
        struct rq *rq = rq_of(cfs_rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)&rq->cfs);

        _pr_sched_rq(rq);
        _pr_sched_cfs_rq_pelt(cfs_rq);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)&cfs_rq->avg);
        pr_sched_avg_info(&cfs_rq->avg);

        now = cfs_rq_clock_pelt(cfs_rq);	//rq->clock_pelt - rq->lost_idle_time;
    }
    pr_info_view_on(stack_depth, "%20s : %llu\n", now);

    pr_fn_end_on(stack_depth);
}

void pr_sched_pelt_info(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "=====================================================\n");

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->cfs_rq);
    _pr_sched_cfs_rq_pelt_info(se->cfs_rq);

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q);
    if (se->my_q) {
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q->curr);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q->tg);
        pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q->rq);
        _pr_sched_cfs_rq_pelt_info(se->my_q);
    }
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)se);
    pr_info_view_on(stack_depth, "%20s : %d\n", se->on_rq);
    pr_info_view_on(stack_depth, "%20s : %lu\n", se->load.weight);
    pr_info_view_on(stack_depth, "%20s : %u\n", se->load.inv_weight);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)&se->avg);
    pr_sched_avg_info(&se->avg);

    pr_out_on(stack_depth, "=====================================================\n");
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_cfs_rq_rblist(struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
    if(!cfs_rq) goto _end;

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->curr);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->next);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->tg);
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->rq);
    pr_info_view_on(stack_depth, "%20s : %u\n", cfs_rq->nr_running);
    pr_info_view_on(stack_depth, "%20s : %d\n", cfs_rq->on_list);
    if (cfs_rq->on_list > 0) {
        int i=0;
        struct sched_entity *se;
        struct rb_node *next = rb_first_cached(&cfs_rq->tasks_timeline);
        while (next) {
            se = rb_entry(next, struct sched_entity, run_node);
            pr_info_view_on(stack_depth, "%30s : %d\n", i++);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
            pr_info_view_on(stack_depth, "%30s : %llu\n", se->vruntime);
            pr_info_view_on(stack_depth, "%30s : %d\n", se->on_rq);
            next = rb_next(&se->run_node);
        }
    }
_end:
    pr_fn_end_on(stack_depth);
}

void pr_sched_tg_view_cpu(struct task_group *tg)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)tg);
    if (!tg) goto _end;
    pr_info_view_on(stack_depth, "%20s : %p\n", (void*)tg->parent);
    pr_out_on(stack_depth, "-----------------------------------------------\n");

    int cpu;
    for_each_possible_cpu(cpu) {
            struct rq *rq;
            rq = cpu_rq(cpu);
            pr_info_view_on(stack_depth, "%20s : %d\n", cpu);
            pr_out_on(stack_depth, "-----------------------------------------------\n");

            _pr_sched_rq(rq);

            struct cfs_rq *cfs_rq = tg->cfs_rq[cpu];
            _pr_sched_cfs_rq_rblist(cfs_rq);

            struct sched_entity *se;
            se = tg->se[cpu];
            _pr_sched_se_info(se);
   }

_end:
    pr_fn_end_on(stack_depth);
}

int pr_sched_tg_view_only(void)
{
    pr_fn_start_on(stack_depth);
    int index=-1;
    struct task_group *tg;

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        index++;
        pr_info_view_on(stack_depth, "%30s : %u\n", index);
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)tg);
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)tg->parent);
    }
    rcu_read_unlock();

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&root_task_group);

    pr_fn_end_on(stack_depth);

    return index;
}

void pr_sched_tg_info(void)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "=====================================================\n");

    struct task_group *tg;
    int cpu, cnt=0;

    if (pr_sched_tg_view_only() < 0) goto _end;

    pr_out_on(stack_depth, "-----------------------------------------------\n");

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)tg);
        pr_out_on(stack_depth, "cnt=%d ---------------------------------------------\n", cnt++);

        for_each_possible_cpu(cpu) {
            struct rq *rq;
            rq = cpu_rq(cpu);
            pr_info_view_on(stack_depth, "%30s : %d\n", cpu);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&rq->cfs);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->cfs.tg);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->cfs.rq);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->curr);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->nr_running);

            struct cfs_rq *cfs_rq = tg->cfs_rq[cpu];
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->tg);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->rq);
            pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->curr);
            pr_info_view_on(stack_depth, "%30s : %u\n", cfs_rq->nr_running);
            pr_info_view_on(stack_depth, "%30s : %d\n", cfs_rq->on_list);

            struct sched_entity *se;
            se = tg->se[cpu];
            _pr_sched_se_info(se);
        } //cpu
        printf("\n");
    } //tg
    rcu_read_unlock();

_end:
    pr_out_on(stack_depth, "=====================================================\n");
    pr_fn_end_on(stack_depth);
}

void pr_sched_tg_info_all(void)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "=====================================================\n");

    struct task_group *tg;

    if (pr_sched_tg_view_only() < 0) goto _end;

    pr_out_on(stack_depth, "-----------------------------------------------\n");

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        pr_sched_tg_view_cpu(tg);
    }
    rcu_read_unlock();

_end:
    pr_out_on(stack_depth, "=====================================================\n");
    pr_fn_end_on(stack_depth);
}

void pr_leaf_cfs_rq_info(void)
{
    struct rq *rq;
    struct cfs_rq *cfs_rq, *pos;
    int cpu, count=0;

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    pr_fn_start_on(stack_depth);

    rq = cpu_rq(cpu);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)&rq->leaf_cfs_rq_list);
    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)rq->tmp_alone_branch);

    //for_each_leaf_cfs_rq_safe(rq, cfs_rq, pos) {
    list_for_each_entry_safe(cfs_rq, pos, &rq->leaf_cfs_rq_list,
                 leaf_cfs_rq_list) {
        struct sched_entity *se;

        pr_info_view_on(stack_depth, "%30s : %d\n", count++);
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->tg);
        se = cfs_rq->tg->se[cpu];
        pr_info_view_on(stack_depth, "%30s : %p\n", (void*)se);
    }

    pr_fn_end_on(stack_depth);
}

void pr_cfs_rq_removed_info(struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%30s : %d\n", cfs_rq->removed.nr);
    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.load_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.util_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.runnable_sum);

    pr_fn_end_on(stack_depth);
}

void pr_sched_dl_entity_info(struct sched_dl_entity *dl_se)
{
    pr_fn_start_on(stack_depth);

    pr_info_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_runtime);
    pr_info_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_deadline);
    pr_info_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_period);
    pr_info_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_bw);
    pr_info_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_density);
    pr_info_view_on(stack_depth, "%30s : %llu\n", dl_se->runtime);
    pr_info_view_on(stack_depth, "%30s : %llu\n", dl_se->deadline);

    pr_info_view_on(stack_depth, "%30s : %u\n", dl_se->flags);

    pr_info_view_on(stack_depth, "%30s : %u\n", dl_se->dl_throttled);
    pr_info_view_on(stack_depth, "%30s : %u\n", dl_se->dl_boosted);
    pr_info_view_on(stack_depth, "%30s : %u\n", dl_se->dl_yielded);
    pr_info_view_on(stack_depth, "%30s : %u\n", dl_se->dl_non_contending);
    pr_info_view_on(stack_depth, "%30s : %u\n", dl_se->dl_overrun);

    pr_fn_end_on(stack_depth);
}

void pr_sched_curr_task_info(struct task_struct *p)
{
    pr_fn_start_on(stack_depth);

    //uapi/linux/sched.h
    char *spolicy[] = { "SCHED_NORMAL", "SCHED_FIFO", "SCHED_RR"
            , "SCHED_BATCH", "SCHED_ISO", "SCHED_IDLE", "SCHED_DEADLINE" };

    pr_info_view_on(stack_depth, "%30s : %p\n", (void*)p);
    if (!p) return;

    pr_info_view_on(stack_depth, "%30s : %d\n", p->prio);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->normal_prio);
    pr_info_view_on(stack_depth, "%30s : %d\n", p->static_prio);
    pr_info_view_on(stack_depth, "%30s : %u\n", p->rt_priority);
    pr_info_view_on(stack_depth, "%30s : %s\n", spolicy[p->policy]);

    if (p->sched_class == &stop_sched_class)
        pr_out_on(stack_depth, "%30s : %s\n", "p->sched_class", "stop_sched_class");
    else if (p->sched_class == &dl_sched_class)
        pr_out_on(stack_depth, "%30s : %s\n", "p->sched_class", "dl_sched_class");
    else if (p->sched_class == &rt_sched_class)
        pr_out_on(stack_depth, "%30s : %s\n", "p->sched_class", "rt_sched_class");
    else if (p->sched_class == &fair_sched_class)
        pr_out_on(stack_depth, "%30s : %s\n", "p->sched_class", "fair_sched_class");
    else if (p->sched_class == &idle_sched_class)
        pr_out_on(stack_depth, "%30s : %s\n", "p->sched_class", "idle_sched_class");

    pr_fn_end_on(stack_depth);
}
