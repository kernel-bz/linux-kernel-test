// SPDX-License-Identifier: GPL-2.0-only
/*
 *  kernel/sched/core.c
 *
 *  Core kernel scheduler code and related syscalls
 *
 *  Copyright (C) 1991-2002  Linus Torvalds
 */
#include "test/config.h"

#include "sched.h"

//#include <linux/nospec.h>
//#include <linux/kcov.h>
//#include <asm/switch_to.h>
//#include <asm/tlb.h>

//#include "../workqueue_internal.h"
//#include "../smpboot.h"

#include "pelt.h"

#include <linux/gfp.h>
#include <linux/slab.h>
#include <kernel/sched/sched.h>
#include <linux/sched.h>

#define CREATE_TRACE_POINTS
#define const_debug

/*
 * Number of tasks to iterate in a single balance run.
 * Limited because this is done with IRQs disabled.
 */
const_debug unsigned int sysctl_sched_nr_migrate = 32;

/*
 * period over which we measure -rt task CPU usage in us.
 * default: 1s
 */
unsigned int sysctl_sched_rt_period = 1000000;

__read_mostly int scheduler_running;

/*
 * part of the period that we allow rt tasks to run in us.
 * default: 0.95s
 */
int sysctl_sched_rt_runtime = 950000;


#ifdef CONFIG_CGROUP_SCHED
/*
 * Default task group.
 * Every task in system belongs to this group at bootup.
 */
struct task_group root_task_group;
LIST_HEAD(task_groups);

/* Cacheline aligned slab cache for task_group */
static struct kmem_cache *task_group_cache __read_mostly;
#endif

DECLARE_PER_CPU(cpumask_var_t, load_balance_mask);
DECLARE_PER_CPU(cpumask_var_t, select_idle_mask);


void __init sched_init(void)
{
        unsigned long ptr = 0;
        int i;

        //wait_bit_init();

#ifdef CONFIG_FAIR_GROUP_SCHED
        ptr += 2 * nr_cpu_ids * sizeof(void **);
#endif
#ifdef CONFIG_RT_GROUP_SCHED
        ptr += 2 * nr_cpu_ids * sizeof(void **);
#endif
        if (ptr) {
                ptr = (unsigned long)kzalloc(ptr, GFP_NOWAIT);

#ifdef CONFIG_FAIR_GROUP_SCHED
                root_task_group.se = (struct sched_entity **)ptr;
                ptr += nr_cpu_ids * sizeof(void **);

                root_task_group.cfs_rq = (struct cfs_rq **)ptr;
                ptr += nr_cpu_ids * sizeof(void **);

#endif /* CONFIG_FAIR_GROUP_SCHED */
#ifdef CONFIG_RT_GROUP_SCHED
                root_task_group.rt_se = (struct sched_rt_entity **)ptr;
                ptr += nr_cpu_ids * sizeof(void **);

                root_task_group.rt_rq = (struct rt_rq **)ptr;
                ptr += nr_cpu_ids * sizeof(void **);

#endif /* CONFIG_RT_GROUP_SCHED */
        }
#ifdef CONFIG_CPUMASK_OFFSTACK
        for_each_possible_cpu(i) {
                per_cpu(load_balance_mask, i) = (cpumask_var_t)kzalloc_node(
                        cpumask_size(), GFP_KERNEL, cpu_to_node(i));
                per_cpu(select_idle_mask, i) = (cpumask_var_t)kzalloc_node(
                        cpumask_size(), GFP_KERNEL, cpu_to_node(i));
        }
#endif /* CONFIG_CPUMASK_OFFSTACK */

        //init_rt_bandwidth(&def_rt_bandwidth, global_rt_period(), global_rt_runtime());
        //init_dl_bandwidth(&def_dl_bandwidth, global_rt_period(), global_rt_runtime());
#ifdef CONFIG_SMP
        //init_defrootdomain();
#endif

#ifdef CONFIG_RT_GROUP_SCHED
        init_rt_bandwidth(&root_task_group.rt_bandwidth,
                        global_rt_period(), global_rt_runtime());
#endif /* CONFIG_RT_GROUP_SCHED */

#ifdef CONFIG_CGROUP_SCHED
        task_group_cache = KMEM_CACHE(task_group, 0);

        list_add(&root_task_group.list, &task_groups);
        INIT_LIST_HEAD(&root_task_group.children);
        INIT_LIST_HEAD(&root_task_group.siblings);
        //autogroup_init(&init_task);
#endif /* CONFIG_CGROUP_SCHED */

        for_each_possible_cpu(i) {
                struct rq *rq;

                //rq = cpu_rq(i);
                raw_spin_lock_init(&rq->lock);
                rq->nr_running = 0;
                rq->calc_load_active = 0;
                //rq->calc_load_update = jiffies + LOAD_FREQ;
                init_cfs_rq(&rq->cfs);
                init_rt_rq(&rq->rt);
                init_dl_rq(&rq->dl);
#ifdef CONFIG_FAIR_GROUP_SCHED
                root_task_group.shares = ROOT_TASK_GROUP_LOAD;
                INIT_LIST_HEAD(&rq->leaf_cfs_rq_list);
                rq->tmp_alone_branch = &rq->leaf_cfs_rq_list;

                init_cfs_bandwidth(&root_task_group.cfs_bandwidth);
                init_tg_cfs_entry(&root_task_group, &rq->cfs, NULL, i, NULL);
#endif /* CONFIG_FAIR_GROUP_SCHED */

                //rq->rt.rt_runtime = def_rt_bandwidth.rt_runtime;
#ifdef CONFIG_RT_GROUP_SCHED
                init_tg_rt_entry(&root_task_group, &rq->rt, NULL, i, NULL);
#endif
#ifdef CONFIG_SMP
                rq->sd = NULL;
                rq->rd = NULL;
                rq->cpu_capacity = rq->cpu_capacity_orig = SCHED_CAPACITY_SCALE;
                rq->balance_callback = NULL;
                rq->active_balance = 0;
                //rq->next_balance = jiffies;
                rq->push_cpu = 0;
                rq->cpu = i;
                rq->online = 0;
                rq->idle_stamp = 0;
                //rq->avg_idle = 2*sysctl_sched_migration_cost;
                //rq->max_idle_balance_cost = sysctl_sched_migration_cost;

                INIT_LIST_HEAD(&rq->cfs_tasks);

                rq_attach_root(rq, &def_root_domain);
#ifdef CONFIG_NO_HZ_COMMON
                rq->last_load_update_tick = jiffies;
                rq->last_blocked_load_update_tick = jiffies;
                atomic_set(&rq->nohz_flags, 0);
#endif
#endif /* CONFIG_SMP */
                hrtick_rq_init(rq);
                atomic_set(&rq->nr_iowait, 0);
        }

        //set_load_weight(&init_task, false);
        /*
         * The boot idle thread does lazy MMU switching as well:
         */
        //mmgrab(&init_mm);
        //enter_lazy_tlb(&init_mm, current);

        /*
         * Make us the idle thread. Technically, schedule() should not be
         * called from this thread, however somewhere below it might be,
         * but because we are the idle thread, we just pick up running again
         * when this runqueue becomes "idle".
         */
        //init_idle(current, smp_processor_id());

        //calc_load_update = jiffies + LOAD_FREQ;

#ifdef CONFIG_SMP
        //idle_thread_set_boot_cpu();
#endif
        //init_sched_fair_class();

        //init_schedstats();

        //psi_init();

        //init_uclamp();

        scheduler_running = 1;
}



