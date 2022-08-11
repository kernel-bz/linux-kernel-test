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
        pr_view_on(stack_depth, "%25s : %ld\n", cfs_rq->prop_runnable_sum);
        _pr_sched_avg_info(&cfs_rq->avg);
    }

    pr_fn_end_on(stack_depth);
}

static void _pr_sched_pelt_avg_rt(struct rq *rq)
{
    pr_fn_start_on(stack_depth);

    pr_out_on(stack_depth, "-------------- rq->avg_rt info --------------\n");
    _pr_sched_avg_info(&rq->avg_rt);
    pr_out_on(stack_depth, "-------------- rq->avg_dl info --------------\n");
    _pr_sched_avg_info(&rq->avg_dl);

    pr_fn_end_on(stack_depth);
}

static void _pr_sched_rq_info(struct rq *rq)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "=================== rq info ===================\n");

    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq);
    pr_view_on(stack_depth, "%20s : %d\n", rq->cpu);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->curr);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->idle);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->stop);
    //pr_view_on(stack_depth, "%20s : %p\n", (void*)&rq->cfs);
    //pr_view_on(stack_depth, "%20s : %p\n", (void*)&rq->rt);
    //pr_view_on(stack_depth, "%20s : %p\n", (void*)&rq->dl);
    pr_view_on(stack_depth, "%20s : %p\n", (void*)rq->cfs.curr);

    pr_view_on(stack_depth, "%20s : %u\n", rq->nr_running);
    pr_view_on(stack_depth, "%20s : %lu\n", rq->cpu_capacity);
    pr_view_on(stack_depth, "%20s : %lu\n", rq->calc_load_update);
    pr_view_on(stack_depth, "%20s : %l\n", rq->calc_load_active);
    pr_view_on(stack_depth, "%20s : %u\n", rq->sched_count);

    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock);
    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock_task);
    pr_view_on(stack_depth, "%20s : %llu\n", rq->clock_pelt);
    //pr_view_on(stack_depth, "%20s : %lu\n", rq->lost_idle_time);

    pr_sched_pelt_avg_info(NULL, &rq->cfs);
    _pr_sched_pelt_avg_rt(rq);

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
            pr_view_on(stack_depth, "%20s : %d\n", i++);
            pr_view_on(stack_depth, "%20s : %p\n", next);
            pr_view_on(stack_depth, "%20s : %p\n", next->rb_left);
            if (next->rb_left) {
                se = rb_entry(next->rb_left, struct sched_entity, run_node);
                pr_view_on(stack_depth, "%30s : left: %p\n", se);
            }
            pr_view_on(stack_depth, "%30s : %p\n", next->__rb_parent_color);
            pr_view_on(stack_depth, "%30s : %p\n", next->rb_right);
            if (next->rb_right) {
                se = rb_entry(next->rb_right, struct sched_entity, run_node);
                pr_view_on(stack_depth, "%30s : right: %p\n", se);
            }

            se = rb_entry(next, struct sched_entity, run_node);
            pr_view_on(stack_depth, "%20s : %p\n", se);
            pr_view_on(stack_depth, "%20s : %p\n", &se->run_node);

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

static void _pr_sched_cfs_rq_rblist_task(struct cfs_rq *cfs_rq)
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
        pr_view_on(stack_depth, "%30s : 0x%016llX\n", rt_rq->active.bitmap[i]);

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

static void _pr_sched_dl_rq_info(struct dl_rq *dl_rq)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%30s : %p\n", (void*)dl_rq);
    if(!dl_rq) goto _end;

    pr_view_on(stack_depth, "%30s : %llu\n", dl_rq->earliest_dl.curr);
    pr_view_on(stack_depth, "%30s : %llu\n", dl_rq->earliest_dl.next);
    pr_view_on(stack_depth, "%30s : %d\n", dl_rq->overloaded);
    pr_view_on(stack_depth, "%30s : %p\n", dl_rq->root);
    pr_view_on(stack_depth, "%30s : %p\n", dl_rq->pushable_dl_tasks_root);

_end:
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_rt_prio_array_info(struct rt_rq *rt_rq)
{
    struct rt_prio_array *array = &rt_rq->active;
    //int prio = rt_rq->highest_prio.curr;
    //struct list_head *queue = array->queue + prio;
    struct list_head *queue;
    struct list_head *list;
    struct sched_rt_entity *rt_se;
    struct task_struct *p;
    unsigned int i = 0;
    int idx;
    unsigned long nbits = MAX_RT_PRIO;

    pr_fn_start_on(stack_depth);

    pr_out_on(stack_depth, "-----------------------------------------------\n");

    int asize = BITS_TO_LONGS(MAX_RT_PRIO+1);
    for (i=0; i<asize; i++)
        pr_out_on(stack_depth, "bitmap[%d] : 0x%016llX\n", i, rt_rq->active.bitmap[i]);

    i = 0;
    idx = find_first_bit(array->bitmap, nbits);
    while (idx < nbits) {
        pr_out_on(stack_depth, "prio idx : %d\n", idx);
        queue = array->queue + idx;
        list_for_each(list, queue) {
            rt_se = list_entry(list, struct sched_rt_entity, run_list);
            pr_out_on(stack_depth, "(%u) %p(%p)\n", i, rt_se, rt_se->my_q);
            if (!rt_se->my_q) {
                //p = rt_task_of(rt_se);
                p = container_of(rt_se, struct task_struct, rt);
                pr_out_on(stack_depth, "[%u] %p(%d, %u)\n", i, p, p->prio, p->rt_priority);
            }
            i++;
        }
        idx = find_next_bit(array->bitmap, nbits, idx + 1);
    }

    pr_out_on(stack_depth, "-------------- rt pushable_tasks -----------------\n");
    struct plist_head *head = &rt_rq->pushable_tasks;

    if (!plist_head_empty(&rt_rq->pushable_tasks)) {
        i = 0;
        plist_for_each_entry(p, head, pushable_tasks) {
            pr_out_on(stack_depth, "[%u] %p(%d, %u)\n", i, p, p->prio, p->rt_priority);
            i++;
        }
    }
    pr_out_on(stack_depth, "-----------------------------------------------\n");
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_tg_bandwidth_view(struct task_group *tg)
{
    struct cfs_bandwidth *cfs_b = &tg->cfs_bandwidth;
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%30s : %llu\n", cfs_b->quota);
    pr_view_on(stack_depth, "%30s : %llu\n", cfs_b->period);
    pr_view_on(stack_depth, "%30s : %llu\n", cfs_b->runtime);
    pr_view_on(stack_depth, "%30s : %lld\n", cfs_b->hierarchical_quota);
    pr_view_on(stack_depth, "%30s : %u\n", cfs_b->idle);
    pr_view_on(stack_depth, "%30s : %u\n", cfs_b->period_active);
    pr_view_on(stack_depth, "%30s : %u\n", cfs_b->distribute_running);
    pr_view_on(stack_depth, "%30s : %u\n", cfs_b->slack_started);
    pr_view_on(stack_depth, "%30s : %d\n", cfs_b->nr_throttled);
    pr_view_on(stack_depth, "%30s : %llu\n", cfs_b->throttled_time);

    pr_fn_end_on(stack_depth);
}

static void _pr_sched_tg_view_cpu_detail(struct task_group *tg, int cpu)
{
    pr_fn_start_on(stack_depth);

    struct sched_entity *se = tg->se[cpu];
    _pr_sched_se_info(se);
    pr_sched_pelt_avg_info(se, NULL);

    struct cfs_rq *cfs_rq = tg->cfs_rq[cpu];
    _pr_sched_cfs_rq_info(cfs_rq);
    __pr_sched_cfs_rq_info(cfs_rq);
    pr_sched_pelt_avg_info(NULL, cfs_rq);
    _pr_sched_cfs_rq_rblist(cfs_rq);

    struct rt_rq *rt_rq = tg->rt_rq[cpu];
    _pr_sched_rt_rq_info(rt_rq);
    _pr_sched_rt_prio_array_info(rt_rq);

    _pr_sched_tg_bandwidth_view(tg);

    pr_fn_end_on(stack_depth);
}

int pr_sched_tg_view_only(void)
{
    pr_fn_start_on(stack_depth);
    int index=0;
    struct task_group *tg;

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        pr_view_on(stack_depth, "%30s : %d\n", index++);
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
    pr_fn_start_on(stack_depth);

    struct cfs_rq *cfs_rq = tg->cfs_rq[cpu];
    _pr_sched_cfs_rq_info(cfs_rq);

    struct rt_rq *rt_rq = tg->rt_rq[cpu];
    _pr_sched_rt_rq_info(rt_rq);

    _pr_sched_tg_bandwidth_view(tg);

    pr_fn_end_on(stack_depth);
}

void pr_sched_tg_info(void)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "============== task group info ================\n");

    struct task_group *tg, *child;
    int cnt = 0;

    //if (pr_sched_tg_view_only() < 0) goto _end;

    int cpu;
    __fpurge(stdin);
    printf("Enter CPU Number[0,%d]: ", NR_CPUS - 1);
    scanf("%d", &cpu);

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        pr_view_on(stack_depth, "%10s : %d\n", cnt++);
        pr_view_on(stack_depth, "%10s : %p\n", (void*)tg);
        pr_view_on(stack_depth, "%20s : %lu\n", tg->shares);
        pr_view_on(stack_depth, "%20s : %p\n", tg->parent);
        pr_view_on(stack_depth, "%20s : %p\n", tg->se[cpu]);
        pr_view_on(stack_depth, "%20s : %p\n", tg->cfs_rq[cpu]);
        pr_out_on(stack_depth, "-------------------------------------------\n");

        list_for_each_entry_rcu(child, &tg->children, siblings)
            pr_view_on(stack_depth, "%20s : %p\n", child);
    } //tg

    cnt--;
    pr_out_on(stack_depth, "===============================================\n");
    list_for_each_entry_prev_rcu(tg, &task_groups, list) {
        pr_view_on(stack_depth, "%10s : %d\n", cnt--);
        pr_view_on(stack_depth, "%10s : %p\n", (void*)tg);
        pr_view_on(stack_depth, "%20s : %p\n", tg->parent);
        pr_out_on(stack_depth, "-------------------------------------------\n");

        list_for_each_entry_rcu(child, &tg->children, siblings)
            pr_view_on(stack_depth, "%20s : %p\n", child);
    } //tg

    pr_out_on(stack_depth, "===============================================\n");
    test_fair_walk_tg_tree_from(&root_task_group);
    test_fair_walk_tg_tree_from_new(&root_task_group);

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
    int cpu, cnt=0;

    //if (pr_sched_tg_view_only() < 0) goto _end;

    __fpurge(stdin);
    printf("Enter CPU Number[0,%d]: ", NR_CPUS - 1);
    scanf("%d", &cpu);

    rcu_read_lock();
    list_for_each_entry_rcu(tg, &task_groups, list) {
        pr_view_on(stack_depth, "%20s : %p\n", (void*)tg);
        pr_view_on(stack_depth, "%20s : %lu\n", tg->shares);
        pr_view_on(stack_depth, "%20s : %llu\n", tg->load_avg);
        pr_view_on(stack_depth, "%20s : %d\n", cnt++);
        pr_out_on(stack_depth, "============================================\n");
        _pr_sched_tg_view_cpu_detail(tg, cpu);
    }
    rcu_read_unlock();

    struct rq *rq = cpu_rq(cpu);
    _pr_sched_rq_info(rq);

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
        struct task_struct *p;

        pr_view_on(stack_depth, "%20s : %d\n", count++);
        pr_view_on(stack_depth, "%20s : %p\n", cfs_rq->tg);
        se = cfs_rq->tg->se[cpu];
        pr_view_on(stack_depth, "%20s : %p\n", se);
        pr_view_on(stack_depth, "%20s : %p\n", cfs_rq);
#if 0
        _pr_sched_cfs_rq_info(cfs_rq);
        __pr_sched_cfs_rq_info(cfs_rq);
        pr_sched_pelt_avg_info(NULL, cfs_rq);

#endif
        if (cfs_rq->curr) {
            pr_view_on(stack_depth, "%30s : %p\n", cfs_rq->curr);
            se = cfs_rq->curr;
            if (!se->my_q) {
                p = container_of(se, struct task_struct, se);	//task_of
                pr_sched_task_info(p);
            }
        }
        if (cfs_rq->next) {
            pr_view_on(stack_depth, "%30s : %p\n", cfs_rq->next);
            se = cfs_rq->next;
            if (!se->my_q) {
                p = container_of(se, struct task_struct, se);	//task_of
                pr_sched_task_info(p);
            }
        }
        _pr_sched_cfs_rq_rblist_task(cfs_rq);
    }

    pr_fn_end_on(stack_depth);
}

void pr_sched_cfs_tasks_info(void)
{
    int cpu;

    pr_fn_start_on(stack_depth);

_retry:
     __fpurge(stdin);
    printf("Input CPU Number[0,%d]: ", NR_CPUS-1);
    scanf("%u", &cpu);
    if (cpu >= NR_CPUS) goto _retry;

    struct rq *rq = cpu_rq(cpu);
    struct list_head *tasks = &rq->cfs_tasks, *cur, *n;
    struct task_struct *p;

    list_for_each_prev_safe(cur, n, tasks) {
        //p = list_last_entry(cur, struct task_struct, se.group_node);
        p = list_entry(cur, struct task_struct, se.group_node);
        pr_sched_task_info(p);
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
    pr_view_on(stack_depth, "%30s : 0x%X\n", p->cpus_ptr->bits[0]);
    pr_view_on(stack_depth, "%30s : %p\n", (void*)p->sched_task_group);
    pr_view_on(stack_depth, "%30s : %d\n", p->prio);
    pr_view_on(stack_depth, "%30s : %d\n", p->normal_prio);
    pr_view_on(stack_depth, "%30s : %d\n", p->static_prio);
    pr_view_on(stack_depth, "%30s : %u\n", p->rt_priority);
    pr_view_on(stack_depth, "%30s : %s\n", spolicy[p->policy]);
    pr_view_on(stack_depth, "%30s : 0x%X\n", p->state);

    if (p->sched_class == &stop_sched_class) {
        pr_view_on(stack_depth, "%30s : %s\n", "stop_sched_class");
    } else if (p->sched_class == &dl_sched_class) {
        pr_view_on(stack_depth, "%30s : %s\n", "dl_sched_class");
        pr_sched_dl_entity_info(&p->dl);
    } else if (p->sched_class == &rt_sched_class) {
        pr_view_on(stack_depth, "%30s : %s\n", "rt_sched_class");
    } else if (p->sched_class == &fair_sched_class) {
        pr_view_on(stack_depth, "%30s : %s\n", "fair_sched_class");
        _pr_sched_se_info(&p->se);
    } else if (p->sched_class == &idle_sched_class) {
        pr_view_on(stack_depth, "%30s : %s\n", "idle_sched_class");
    }

_end:
    pr_out_on(stack_depth, "===============================================\n");
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_cfs_tasks_info(struct rq *rq)
{
    unsigned int i = 0;
    struct list_head *tasks = &rq->cfs_tasks, *cur, *n;
    struct task_struct *p;

    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "--------------------------------------\n");
    list_for_each_prev_safe(cur, n, tasks) {
        //p = list_last_entry(cur, struct task_struct, se.group_node);
        p = list_entry(cur, struct task_struct, se.group_node);
        pr_out_on(stack_depth, "[%u] %p(%d, %d)\n", i, p, p->on_rq, p->prio);
        i++;
    }
    pr_out_on(stack_depth, "--------------------------------------\n");
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_rbtree_pushable_dl(struct rb_root *rb_root)
{
    unsigned int i = 0;
    struct rb_node *node;
    struct task_struct *p;

    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "--------------------------------------\n");
    for (node = rb_first(rb_root); node; node = rb_next(node)) {
        p = rb_entry(node, struct task_struct, pushable_dl_tasks);
        pr_out_on(stack_depth, "[%u] %p(%llu)\n", i, p, p->dl.deadline);
        i++;
    }
    pr_out_on(stack_depth, "--------------------------------------\n");
    pr_fn_end_on(stack_depth);
}

static void _pr_sched_rbtree_root_dl(struct rb_root *rb_root)
{
    unsigned int i = 0;
    struct rb_node *node;
    struct task_struct *p;
    struct sched_dl_entity *dl_se;

    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "--------------------------------------\n");
    for (node = rb_first(rb_root); node; node = rb_next(node)) {
        dl_se = rb_entry(node, struct sched_dl_entity, rb_node);
        //p = dl_task_of(dl_se);
        p = container_of(dl_se, struct task_struct, dl);
        pr_out_on(stack_depth, "[%u] %p(%llu) %p\n", i, p, p->dl.deadline, dl_se);
        i++;
    }
    pr_out_on(stack_depth, "--------------------------------------\n");
    pr_fn_end_on(stack_depth);
}



void pr_sched_rq_info(struct rq *rq)
{
    pr_fn_start_on(stack_depth);
    pr_out_on(stack_depth, "=================== rq info ===================\n");

    pr_view_on(stack_depth, "%10s : %p\n", (void*)rq);
    if (!rq) goto _end;

    pr_view_on(stack_depth, "%10s : %d\n", rq->cpu);
    pr_view_on(stack_depth, "%10s : %d\n", rq->online);
    pr_view_on(stack_depth, "%10s : %p\n", rq->curr);
    pr_view_on(stack_depth, "%10s : %p\n", rq->idle);
    pr_view_on(stack_depth, "%10s : %p\n\n", rq->stop);

    pr_view_on(stack_depth, "%20s : %u\n", rq->nr_running);
    //pr_view_on(stack_depth, "%20s : %u\n", rq->nr_numa_running);
    pr_view_on(stack_depth, "%20s : %llu\n", rq->nr_switches);
    pr_view_on(stack_depth, "%20s : %lu\n", rq->nr_load_updates);
    pr_view_on(stack_depth, "%20s : %u\n\n", rq->sched_count);

    pr_view_on(stack_depth, "%20s : %lu\n", rq->cpu_capacity);
    pr_view_on(stack_depth, "%20s : %lu\n", rq->calc_load_update);
    pr_view_on(stack_depth, "%20s : %l\n\n", rq->calc_load_active);

    pr_view_on(stack_depth, "%30s : %p\n", rq->cfs.curr);
    pr_view_on(stack_depth, "%30s : %u\n", rq->cfs.nr_running);
    pr_view_on(stack_depth, "%30s : %u\n", rq->cfs.h_nr_running);
    pr_view_on(stack_depth, "%30s : %u\n\n", rq->cfs.idle_h_nr_running);

    pr_view_on(stack_depth, "%30s : %u\n", rq->rt.rt_nr_running);
    pr_view_on(stack_depth, "%30s : %u\n", rq->rt.rr_nr_running);
    pr_view_on(stack_depth, "%30s : %lu\n", rq->rt.rt_nr_total);
    pr_view_on(stack_depth, "%30s : %d\n", rq->rt.highest_prio.curr);
    pr_view_on(stack_depth, "%30s : %d\n\n", rq->rt.highest_prio.next);

    pr_view_on(stack_depth, "%30s : %lu\n", rq->dl.dl_nr_running);
    pr_view_on(stack_depth, "%30s : %llu\n", rq->dl.earliest_dl.curr);
    pr_view_on(stack_depth, "%30s : %llu\n\n", rq->dl.earliest_dl.next);

    _pr_sched_cfs_tasks_info(rq);

    _pr_sched_rt_prio_array_info(&rq->rt);

    _pr_sched_rbtree_pushable_dl(&rq->dl.pushable_dl_tasks_root);
    _pr_sched_rbtree_root_dl(&rq->dl.root);

    pr_sched_pelt_avg_info(NULL, &rq->cfs);
    _pr_sched_pelt_avg_rt(rq);

_end:
    pr_out_on(stack_depth, "===============================================\n");
    pr_fn_end_on(stack_depth);
}

void pr_sched_cpumask_bits_info(unsigned int nr_cpu)
{
    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%30s : %u\n", nr_cpu);
    pr_view_on(stack_depth, "%30s : 0x%X\n", cpu_possible_mask->bits[0]);
    pr_view_on(stack_depth, "%30s : 0x%X\n", cpu_online_mask->bits[0]);
    pr_view_on(stack_depth, "%30s : 0x%X\n", cpu_present_mask->bits[0]);
    pr_view_on(stack_depth, "%30s : 0x%X\n", cpu_active_mask->bits[0]);
    pr_fn_end_on(stack_depth);
}
