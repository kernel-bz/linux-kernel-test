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
#include <linux/sched/topology.h>
#include <kernel/sched/pelt.h>
#include <linux/list.h>
#include <linux/cpumask.h>
#include "test/debug.h"

static void _pr_sched_avg_info(struct sched_avg *sa)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "--------------- sched avg ----------------\n");

    pr_view_on(stack_depth, "%30s : %llu\n", sa->last_update_time);
    pr_view_on(stack_depth, "%30s : %llu\n", sa->load_sum);
    pr_view_on(stack_depth, "%30s : %llu\n", sa->runnable_load_sum);
    pr_view_on(stack_depth, "%30s : %u\n", sa->util_sum);
    pr_view_on(stack_depth, "%30s : %u\n", sa->period_contrib);
    pr_view_on(stack_depth, "%30s : %lu\n", sa->load_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", sa->runnable_load_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", sa->util_avg);
    pr_view_on(stack_depth, "%30s : %u\n", sa->util_est.enqueued);
    pr_view_on(stack_depth, "%30s : %u\n", sa->util_est.ewma);

    pr_fn_end_on(stack_depth);
}

void pr_sched_pelt_avg_info(struct sched_entity *se, struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    if (se) {
        pr_out_on(stack_depth, "-------------- se PELT info --------------\n");
        pr_view_on(stack_depth, "%25s : %p\n", se);
        pr_view_on(stack_depth, "%25s : %lu\n", se->load.weight);
        //pr_view_on(stack_depth, "%25s : %u\n", se->load.inv_weight);
        pr_view_on(stack_depth, "%25s : %lu\n", se->runnable_weight);
        _pr_sched_avg_info(&se->avg);
    }

    if (cfs_rq) {
        pr_out_on(stack_depth, "------------ cfs_rq PELT info ------------\n");
        pr_view_on(stack_depth, "%25s : %p\n", cfs_rq);
        pr_view_on(stack_depth, "%25s : %lu\n", cfs_rq->load.weight);
        //pr_view_on(stack_depth, "%25s : %u\n", cfs_rq->load.inv_weight);
        pr_view_on(stack_depth, "%25s : %lu\n", cfs_rq->runnable_weight);
        _pr_sched_avg_info(&cfs_rq->avg);
    }

    pr_fn_end_on(stack_depth);
}

static void _pr_sched_rq_info(struct rq *rq)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "--------------- rq info ------------------\n");

    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_view_on(stack_depth, "%20s : %d\n", rq->cpu);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->idle);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->stop);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)&rq->cfs);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)&rq->rt);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)&rq->dl);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->cfs.curr);

    pr_view_on(stack_depth, "%20s : %u\n", rq->nr_running);
    pr_view_on(stack_depth, "%20s : %lu\n", rq->cpu_capacity);
    pr_view_on(stack_depth, "%20s : %lu\n", rq->calc_load_update);
    pr_view_on(stack_depth, "%20s : %l\n", rq->calc_load_active);
    pr_view_on(stack_depth, "%20s : %u\n", rq->sched_count);

    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock);
    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock_task);
    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock_pelt);
    pr_view_on(stack_depth, "%20s : %lu\n", rq->lost_idle_time);

    pr_sched_pelt_avg_info(NULL, &rq->cfs);

    pr_fn_end_on(stack_depth);
}

static void __pr_sched_cfs_rq_info(struct cfs_rq *cfs_rq)
{
    pr_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);
    if (!cfs_rq) return;

    pr_view_on(stack_depth, "%30s : %llu\n", cfs_rq->min_vruntime);

    if (cfs_rq->tg) {
        pr_view_on(stack_depth, "%30s : %lu\n", cfs_rq->tg->shares);
        pr_view_on(stack_depth, "%30s : %ld\n", atomic_long_read(&cfs_rq->tg->load_avg));
    }
    pr_view_on(stack_depth, "%30s : %lu\n", cfs_rq->tg_load_avg_contrib);

    pr_view_on(stack_depth, "%30s : %ld\n", cfs_rq->propagate);
    pr_view_on(stack_depth, "%30s : %ld\n", cfs_rq->prop_runnable_sum);

    pr_view_on(stack_depth, "%30s : %u\n", cfs_rq->removed.nr);
    pr_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.load_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.util_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.runnable_sum);

    pr_view_on(stack_depth, "%30s : %d\n", cfs_rq->throttle_count);
    //pr_view_on(stack_depth, "%40s : %llu\n", cfs_rq->throttled_clock);
    //pr_view_on(stack_depth, "%40s : %llu\n", cfs_rq->throttled_clock_task);
    //pr_view_on(stack_depth, "%40s : %llu\n", cfs_rq->throttled_clock_task_time);

    pr_out_on(stack_depth, "\n");
}

static void _pr_sched_cfs_rq_info(struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "---------------- cfs_rq info -------------\n");

    pr_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);
    if(!cfs_rq) goto _end;

    pr_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->tg);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->rq);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->curr);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->next);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->last);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq->skip);

    pr_view_on(stack_depth, "%30s : %d\n", cfs_rq->on_list);
    pr_view_on(stack_depth, "%30s : %u\n", cfs_rq->nr_running);
    pr_view_on(stack_depth, "%30s : %u\n", cfs_rq->h_nr_running);
    pr_view_on(stack_depth, "%30s : %u\n", cfs_rq->idle_h_nr_running);

 _end:
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_se_info(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "---------------- se info -----------------\n");

    pr_view_on(stack_depth, "%20s : %p\n", (void*)se);
    if(!se) goto _end;

    pr_view_on(stack_depth, "%20s : %p\n", (void*)se->parent);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)se->cfs_rq);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q);
    pr_view_on(stack_depth, "%20s : %d\n", se->on_rq);
    pr_view_on(stack_depth, "%20s : %d\n", se->depth);
    pr_view_on(stack_depth, "%20s : %llu\n", se->vruntime);

 _end:
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_cfs_rq_rblist(struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    pr_out_on(stack_depth, "--------------- cfs_rq rblist -------------\n");
    pr_view_on(stack_depth, "%10s : %p\n", (void*)cfs_rq);
    if(!cfs_rq) goto _end;

    if (cfs_rq->on_list > 0) {
        int i=0;
        struct sched_entity *se;
        struct task_struct *p;
        struct rb_node *next = rb_first_cached(&cfs_rq->tasks_timeline);
        while (next) {
            se = rb_entry(next, struct sched_entity, run_node);
            pr_view_on(stack_depth, "%10s : %d\n", i++);

            _pr_sched_se_info(se);
            pr_sched_pelt_avg_info(se, NULL);

            if (entity_is_task(se)) {
                p = container_of(se, struct task_struct, se);	//task_of
                if (p)
                    pr_sched_task_info(p);
            }
            next = rb_next(&se->run_node);
        }
    }
_end:
    pr_fn_end_on(stack_depth);
}

void pr_sched_pelt_info(struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "================= PELT Info ===================\n");

    pr_view_on(stack_depth, "%20s : %p\n", (void*)se);
    _pr_sched_se_info(se);
    pr_sched_pelt_avg_info(se, NULL);

    pr_view_on(stack_depth, "%20s : %p\n", (void*)se->cfs_rq);
    _pr_sched_cfs_rq_info(se->cfs_rq);
    __pr_sched_cfs_rq_info(se->cfs_rq);
    pr_sched_pelt_avg_info(NULL, se->cfs_rq);
    _pr_sched_cfs_rq_rblist(se->cfs_rq);

    pr_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q);
    if (se->my_q) {
        pr_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q->curr);
        pr_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q->tg);
        pr_view_on(stack_depth, "%20s : %p\n", (void*)se->my_q->rq);
        _pr_sched_cfs_rq_info(se->my_q);
        __pr_sched_cfs_rq_info(se->my_q);
        pr_sched_pelt_avg_info(NULL, se->my_q);

    }

    pr_out_on(stack_depth, "===============================================\n");
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_rt_rq_info(struct rt_rq *rt_rq)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%30s : %p\n", (void*)rt_rq);
    if(!rt_rq) goto _end;

    int i, asize = BITS_TO_LONGS(MAX_RT_PRIO+1);
    for (i=0; i<asize; i++)
        pr_view_on(stack_depth, "%30s : 0x%lX\n", rt_rq->active.bitmap[i]);

    pr_view_on(stack_depth, "%30s : %u\n", rt_rq->rt_nr_running);
    pr_view_on(stack_depth, "%30s : %u\n", rt_rq->rr_nr_running);
    pr_view_on(stack_depth, "%30s : %d\n", rt_rq->highest_prio.curr);
    pr_view_on(stack_depth, "%30s : %d\n", rt_rq->highest_prio.next);
    pr_view_on(stack_depth, "%30s : %d\n", rt_rq->rt_queued);
    pr_view_on(stack_depth, "%30s : %d\n", rt_rq->rt_throttled);
    pr_view_on(stack_depth, "%30s : %llu\n", rt_rq->rt_time);
    pr_view_on(stack_depth, "%30s : %llu\n", rt_rq->rt_runtime);

_end:
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_tg_view_cpu_detail(struct task_group *tg, int cpu)
{
    struct rq *rq;
    rq = cpu_rq(cpu);
    pr_view_on(stack_depth, "%10s : %d\n", cpu);
    pr_out_on(stack_depth, "------------------------------------------\n");

    _pr_sched_rq_info(rq);

    struct sched_entity *se = tg->se[cpu];
    _pr_sched_se_info(se);
    pr_sched_pelt_avg_info(se, NULL);

    struct cfs_rq *cfs_rq = tg->cfs_rq[cpu];
    _pr_sched_cfs_rq_info(cfs_rq);
    __pr_sched_cfs_rq_info(cfs_rq);
    pr_sched_pelt_avg_info(NULL, cfs_rq);
    _pr_sched_cfs_rq_rblist(cfs_rq);

    //struct rt_rq *rt_rq = tg->rt_rq[cpu];
    //_pr_sched_rt_rq_info(rt_rq);
}

int pr_sched_tg_view_only(void)
{
    pr_fn_start_on(stack_depth);
    int index=0;
    struct task_group *tg;

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        pr_view_on(stack_depth, "%30s : %u\n", index++);
        pr_view_on(stack_depth, "%30s : %p\n", (void*)tg);
        pr_view_on(stack_depth, "%30s : %p\n", (void*)tg->parent);
    }
    rcu_read_unlock();

    pr_view_on(stack_depth, "%30s : %p\n", (void*)&root_task_group);

    pr_fn_end_on(stack_depth);

    return index;
}

static void _pr_sched_tg_view_cpu(struct task_group *tg, int cpu)
{
    pr_view_on(stack_depth, "%10s : %d\n", cpu);
    pr_out_on(stack_depth, "------------------------------------------\n");

    struct rq *rq = cpu_rq(cpu);
    _pr_sched_rq_info(rq);

    struct sched_entity *se = tg->se[cpu];
    _pr_sched_se_info(se);

    struct cfs_rq *cfs_rq = tg->cfs_rq[cpu];
    _pr_sched_cfs_rq_info(cfs_rq);
}

void pr_sched_tg_info(void)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "============== task group info ================\n");

    struct task_group *tg;
    int cpu, mycpu=NR_CPUS, cnt=0;

    if (pr_sched_tg_view_only() < 0) goto _end;

    __fpurge(stdin);
    printf("Enter CPU Number[0,%d]: ", NR_CPUS);
    scanf("%d", &mycpu);

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        pr_view_on(stack_depth, "%10s : %p\n", (void*)tg);
        pr_view_on(stack_depth, "%10s : %d\n", cnt++);
        pr_out_on(stack_depth, "--------------------------------------\n");

        for_each_possible_cpu(cpu) {
            if (mycpu < NR_CPUS && mycpu != cpu) continue;
            _pr_sched_tg_view_cpu(tg, cpu);
        } //cpu
        printf("\n");
    } //tg
    rcu_read_unlock();

_end:
    pr_out_on(stack_depth, "===============================================\n");
    pr_fn_end_on(stack_depth);
}

void pr_sched_tg_info_all(void)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "============ task group info all ==============\n");

    struct task_group *tg;
    int cpu, mycpu=NR_CPUS, cnt=0;

    if (pr_sched_tg_view_only() < 0) goto _end;

    __fpurge(stdin);
    printf("Enter CPU Number[0,%d]: ", NR_CPUS);
    scanf("%d", &mycpu);

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        pr_view_on(stack_depth, "%20s : %p\n", (void*)tg);
        pr_view_on(stack_depth, "%20s : %lu\n", tg->shares);
        pr_view_on(stack_depth, "%20s : %llu\n", tg->load_avg);
        pr_view_on(stack_depth, "%20s : %d\n", cnt++);

        for_each_possible_cpu(cpu) {
            if (mycpu < NR_CPUS && mycpu != cpu) continue;
            _pr_sched_tg_view_cpu_detail(tg, cpu);
        }
    }
    rcu_read_unlock();

_end:
    pr_out_on(stack_depth, "===============================================\n");
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
    pr_view_on(stack_depth, "%10s : %p\n", (void*)rq);

    //for_each_leaf_cfs_rq_safe(rq, cfs_rq, pos) {
    list_for_each_entry_safe(cfs_rq, pos, &rq->leaf_cfs_rq_list,
                 leaf_cfs_rq_list) {
        struct sched_entity *se;

        pr_view_on(stack_depth, "%20s : %d\n", count++);
        pr_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq->tg);
        se = cfs_rq->tg->se[cpu];
        pr_view_on(stack_depth, "%30s : %p\n", (void*)se);
        pr_view_on(stack_depth, "%30s : %p\n", (void*)cfs_rq);

        _pr_sched_cfs_rq_info(cfs_rq);
        __pr_sched_cfs_rq_info(cfs_rq);
        pr_sched_pelt_avg_info(NULL, cfs_rq);

        if (cfs_rq->curr) {
            _pr_sched_se_info(cfs_rq->curr);
            pr_sched_pelt_avg_info(cfs_rq->curr, NULL);
        }

        _pr_sched_cfs_rq_rblist(cfs_rq);
    }

    pr_fn_end_on(stack_depth);
}

void pr_cfs_rq_removed_info(struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%30s : %d\n", cfs_rq->removed.nr);
    pr_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.load_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.util_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", cfs_rq->removed.runnable_sum);

    pr_fn_end_on(stack_depth);
}

void pr_sched_dl_entity_info(struct sched_dl_entity *dl_se)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_runtime);
    pr_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_deadline);
    pr_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_period);
    pr_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_bw);
    pr_view_on(stack_depth, "%30s : %llu\n", dl_se->dl_density);
    pr_view_on(stack_depth, "%30s : %llu\n", dl_se->runtime);
    pr_view_on(stack_depth, "%30s : %llu\n", dl_se->deadline);

    pr_view_on(stack_depth, "%30s : %u\n", dl_se->flags);

    pr_view_on(stack_depth, "%30s : %u\n", dl_se->dl_throttled);
    pr_view_on(stack_depth, "%30s : %u\n", dl_se->dl_boosted);
    pr_view_on(stack_depth, "%30s : %u\n", dl_se->dl_yielded);
    pr_view_on(stack_depth, "%30s : %u\n", dl_se->dl_non_contending);
    pr_view_on(stack_depth, "%30s : %u\n", dl_se->dl_overrun);

    pr_fn_end_on(stack_depth);
}

void pr_sched_task_info(struct task_struct *p)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "================== task info ==================\n");

    //uapi/linux/sched.h
    char *spolicy[] = { "SCHED_NORMAL", "SCHED_FIFO", "SCHED_RR"
            , "SCHED_BATCH", "SCHED_ISO", "SCHED_IDLE", "SCHED_DEADLINE" };

    pr_view_on(stack_depth, "%30s : %p\n", (void*)p);
    if (!p) goto _end;

    pr_view_on(stack_depth, "%30s : %d\n", p->cpu);
    pr_view_on(stack_depth, "%30s : %d\n", p->on_cpu);
    pr_view_on(stack_depth, "%30s : %d\n", p->wake_cpu);
    pr_view_on(stack_depth, "%30s : %d\n", p->recent_used_cpu);
    pr_view_on(stack_depth, "%30s : %p\n", (void*)p->sched_task_group);
    pr_view_on(stack_depth, "%30s : %d\n", p->prio);
    pr_view_on(stack_depth, "%30s : %d\n", p->normal_prio);
    pr_view_on(stack_depth, "%30s : %d\n", p->static_prio);
    pr_view_on(stack_depth, "%30s : %u\n", p->rt_priority);
    pr_view_on(stack_depth, "%30s : %s\n", spolicy[p->policy]);
    pr_view_on(stack_depth, "%30s : 0x%X\n", p->state);

    if (p->sched_class == &stop_sched_class)
        pr_view_on(stack_depth, "%30s : %s\n", "stop_sched_class");
    else if (p->sched_class == &dl_sched_class)
        pr_view_on(stack_depth, "%30s : %s\n", "dl_sched_class");
    else if (p->sched_class == &rt_sched_class)
        pr_view_on(stack_depth, "%30s : %s\n", "rt_sched_class");
    else if (p->sched_class == &fair_sched_class)
        pr_view_on(stack_depth, "%30s : %s\n", "fair_sched_class");
    else if (p->sched_class == &idle_sched_class)
        pr_view_on(stack_depth, "%30s : %s\n", "idle_sched_class");

    pr_view_on(stack_depth, "%30s : %p\n", &p->se);
    _pr_sched_se_info(&p->se);

_end:
    pr_out_on(stack_depth, "===============================================\n");
    pr_fn_end_on(stack_depth);
}

void pr_sched_cpumask_bits_info(unsigned int nr_cpu)
{
    pr_view_on(stack_depth, "%30s : %u\n", nr_cpu);

    pr_view_on(stack_depth, "%30s : 0x%X\n", cpu_possible_mask->bits[0]);
    pr_view_on(stack_depth, "%30s : 0x%X\n", cpu_online_mask->bits[0]);
    pr_view_on(stack_depth, "%30s : 0x%X\n", cpu_present_mask->bits[0]);
    pr_view_on(stack_depth, "%30s : 0x%X\n", cpu_active_mask->bits[0]);
}
