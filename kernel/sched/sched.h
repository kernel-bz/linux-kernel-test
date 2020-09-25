//kernel/sched/sched.h

#include <stdint.h>
#include "linux/types.h"
#include "linux/sched.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cfs_rq;
struct rt_rq;

extern struct list_head task_groups;

struct cfs_bandwidth {
#ifdef CONFIG_CFS_BANDWIDTH
        raw_spinlock_t          lock;
        ktime_t                 period;
        u64                     quota;
        u64                     runtime;
        s64                     hierarchical_quota;

        u8                      idle;
        u8                      period_active;
        u8                      distribute_running;
        u8                      slack_started;
        struct hrtimer          period_timer;
        struct hrtimer          slack_timer;
        struct list_head        throttled_cfs_rq;

        /* Statistics: */
        int                     nr_periods;
        int                     nr_throttled;
        u64                     throttled_time;
#endif
};

struct task_group {
        //struct cgroup_subsys_state css;

#ifdef CONFIG_FAIR_GROUP_SCHED
        /* schedulable entities of this group on each CPU */
        struct sched_entity     **se;
        /* runqueue "owned" by this group on each CPU */
        struct cfs_rq           **cfs_rq;
        unsigned long           shares;

#ifdef  CONFIG_SMP
        /*
         * load_avg can be heavily contended at clock tick time, so put
         * it in its own cacheline separated from the fields above which
         * will also be accessed at each tick.
         */
        //atomic_long_t           load_avg ____cacheline_aligned;
        atomic_t           		load_avg;
#endif
#endif

#ifdef CONFIG_RT_GROUP_SCHED
        struct sched_rt_entity  **rt_se;
        struct rt_rq            **rt_rq;

        struct rt_bandwidth     rt_bandwidth;
#endif

        struct rcu_head         rcu;
        struct list_head        list;

        struct task_group       *parent;
        struct list_head        siblings;
        struct list_head        children;

#ifdef CONFIG_SCHED_AUTOGROUP
        struct autogroup        *autogroup;
#endif

        struct cfs_bandwidth    cfs_bandwidth;

#ifdef CONFIG_UCLAMP_TASK_GROUP
        /* The two decimal precision [%] value requested from user-space */
        unsigned int            uclamp_pct[UCLAMP_CNT];
        /* Clamp values requested for a task group */
        struct uclamp_se        uclamp_req[UCLAMP_CNT];
        /* Effective clamp values used for a task group */
        struct uclamp_se        uclamp[UCLAMP_CNT];
#endif

};

struct cfs_bandwidth { };

/* CFS-related fields in a runqueue */
struct cfs_rq {
        struct load_weight      load;
        unsigned long           runnable_weight;
        unsigned int            nr_running;
        unsigned int            h_nr_running;      /* SCHED_{NORMAL,BATCH,IDLE} */
        unsigned int            idle_h_nr_running; /* SCHED_IDLE */

        u64                     exec_clock;
        u64                     min_vruntime;
#ifndef CONFIG_64BIT
        u64                     min_vruntime_copy;
#endif

        struct rb_root_cached   tasks_timeline;

        /*
         * 'curr' points to currently running entity on this cfs_rq.
         * It is set to NULL otherwise (i.e when none are currently running).
         */
        struct sched_entity     *curr;
        struct sched_entity     *next;
        struct sched_entity     *last;
        struct sched_entity     *skip;

#ifdef  CONFIG_SCHED_DEBUG
        unsigned int            nr_spread_over;
#endif

#ifdef CONFIG_SMP
        /*
         * CFS load tracking
         */
        struct sched_avg        avg;
#ifndef CONFIG_64BIT
        u64                     load_last_update_time_copy;
#endif
        struct {
                raw_spinlock_t  lock ____cacheline_aligned;
                int             nr;
                unsigned long   load_avg;
                unsigned long   util_avg;
                unsigned long   runnable_sum;
        } removed;

#ifdef CONFIG_FAIR_GROUP_SCHED
        unsigned long           tg_load_avg_contrib;
        long                    propagate;
        long                    prop_runnable_sum;

        /*
         *   h_load = weight * f(tg)
         *
         * Where f(tg) is the recursive weight fraction assigned to
         * this group.
         */
        unsigned long           h_load;
        u64                     last_h_load_update;
        struct sched_entity     *h_load_next;
#endif /* CONFIG_FAIR_GROUP_SCHED */
#endif /* CONFIG_SMP */

#ifdef CONFIG_FAIR_GROUP_SCHED
        struct rq               *rq;    /* CPU runqueue to which this cfs_rq is attached */

        /*
         * leaf cfs_rqs are those that hold tasks (lowest schedulable entity in
         * a hierarchy). Non-leaf lrqs hold other higher schedulable entities
         * (like users, containers etc.)
         *
         * leaf_cfs_rq_list ties together list of leaf cfs_rq's in a CPU.
         * This list is used during load balance.
         */
        int                     on_list;
        struct list_head        leaf_cfs_rq_list;
        struct task_group       *tg;    /* group that "owns" this runqueue */

#ifdef CONFIG_CFS_BANDWIDTH
        int                     runtime_enabled;
        s64                     runtime_remaining;

        u64                     throttled_clock;
        u64                     throttled_clock_task;
        u64                     throttled_clock_task_time;
        int                     throttled;
        int                     throttle_count;
        struct list_head        throttled_list;
#endif /* CONFIG_CFS_BANDWIDTH */
#endif /* CONFIG_FAIR_GROUP_SCHED */
};


/* Real-Time classes' related field in a runqueue: */
struct rt_rq {
        struct rt_prio_array    active;
        unsigned int            rt_nr_running;
        unsigned int            rr_nr_running;
#if defined CONFIG_SMP || defined CONFIG_RT_GROUP_SCHED
        struct {
                int             curr; /* highest queued rt task prio */
#ifdef CONFIG_SMP
                int             next; /* next highest */
#endif
        } highest_prio;
#endif
#ifdef CONFIG_SMP
        unsigned long           rt_nr_migratory;
        unsigned long           rt_nr_total;
        int                     overloaded;
        struct plist_head       pushable_tasks;

#endif /* CONFIG_SMP */
        int                     rt_queued;

        int                     rt_throttled;
        u64                     rt_time;
        u64                     rt_runtime;
        /* Nests inside the rq lock: */
        raw_spinlock_t          rt_runtime_lock;

#ifdef CONFIG_RT_GROUP_SCHED
        unsigned long           rt_nr_boosted;

        struct rq               *rq;
        struct task_group       *tg;
#endif
};

/* Deadline class' related fields in a runqueue */
struct dl_rq {
        /* runqueue is an rbtree, ordered by deadline */
        struct rb_root_cached   root;

        unsigned long           dl_nr_running;

#ifdef CONFIG_SMP
        /*
         * Deadline values of the currently executing and the
         * earliest ready task on this rq. Caching these facilitates
         * the decision whether or not a ready but not running task
         * should migrate somewhere else.
         */
        struct {
                u64             curr;
                u64             next;
        } earliest_dl;

        unsigned long           dl_nr_migratory;
        int                     overloaded;

        /*
         * Tasks on this rq that can be pushed away. They are kept in
         * an rb-tree, ordered by tasks' deadlines, with caching
         * of the leftmost (earliest deadline) element.
         */
        struct rb_root_cached   pushable_dl_tasks_root;
#else
        struct dl_bw            dl_bw;
#endif
        /*
         * "Active utilization" for this runqueue: increased when a
         * task wakes up (becomes TASK_RUNNING) and decreased when a
         * task blocks
         */
        u64                     running_bw;
        /*
         * Utilization of the tasks "assigned" to this runqueue (including
         * the tasks that are in runqueue and the tasks that executed on this
         * CPU and blocked). Increased when a task moves to this runqueue, and
         * decreased when the task moves away (migrates, changes scheduling
         * policy, or terminates).
         * This is needed to compute the "inactive utilization" for the
         * runqueue (inactive utilization = this_bw - running_bw).
         */
        u64                     this_bw;
        u64                     extra_bw;

        /*
         * Inverse of the fraction of CPU utilization that can be reclaimed
         * by the GRUB algorithm.
         */
        u64                     bw_ratio;
};


struct rq {
        /* runqueue lock: */
        raw_spinlock_t          lock;

        /*
         * nr_running and cpu_load should be in the same cacheline because
         * remote CPUs use both these fields when doing load calculation.
         */
        unsigned int            nr_running;
#ifdef CONFIG_NUMA_BALANCING
        unsigned int            nr_numa_running;
        unsigned int            nr_preferred_running;
        unsigned int            numa_migrate_on;
#endif
#ifdef CONFIG_NO_HZ_COMMON
#ifdef CONFIG_SMP
        unsigned long           last_load_update_tick;
        unsigned long           last_blocked_load_update_tick;
        unsigned int            has_blocked_load;
#endif /* CONFIG_SMP */
        unsigned int            nohz_tick_stopped;
        atomic_t nohz_flags;
#endif /* CONFIG_NO_HZ_COMMON */

        unsigned long           nr_load_updates;
        u64                     nr_switches;

#ifdef CONFIG_UCLAMP_TASK
        /* Utilization clamp values based on CPU's RUNNABLE tasks */
        struct uclamp_rq        uclamp[UCLAMP_CNT] ____cacheline_aligned;
        unsigned int            uclamp_flags;
#define UCLAMP_FLAG_IDLE 0x01
#endif

        struct cfs_rq           cfs;
        struct rt_rq            rt;
        struct dl_rq            dl;

#ifdef CONFIG_FAIR_GROUP_SCHED
        /* list of leaf cfs_rq on this CPU: */
        struct list_head        leaf_cfs_rq_list;
        struct list_head        *tmp_alone_branch;
#endif /* CONFIG_FAIR_GROUP_SCHED */

        /*
         * This is part of a global counter where only the total sum
         * over all CPUs matters. A task can increase this counter on
         * one CPU and if it got migrated afterwards it may decrease
         * it on another CPU. Always updated under the runqueue lock:
         */
        unsigned long           nr_uninterruptible;

        struct task_struct      *curr;
        struct task_struct      *idle;
        struct task_struct      *stop;
        unsigned long           next_balance;
        struct mm_struct        *prev_mm;

        unsigned int            clock_update_flags;
        u64                     clock;
        /* Ensure that all clocks are in the same cache line */
        u64                     clock_task ____cacheline_aligned;
        u64                     clock_pelt;
        unsigned long           lost_idle_time;

        atomic_t                nr_iowait;

#ifdef CONFIG_MEMBARRIER
        int membarrier_state;
#endif

#ifdef CONFIG_SMP
        struct root_domain              *rd;
        struct sched_domain __rcu       *sd;

        unsigned long           cpu_capacity;
        unsigned long           cpu_capacity_orig;

        struct callback_head    *balance_callback;

        unsigned char           idle_balance;

        unsigned long           misfit_task_load;

        /* For active balancing */
        int                     active_balance;
        int                     push_cpu;
        struct cpu_stop_work    active_balance_work;

        /* CPU of this runqueue: */
        int                     cpu;
        int                     online;

        struct list_head cfs_tasks;

        struct sched_avg        avg_rt;
        struct sched_avg        avg_dl;
#ifdef CONFIG_HAVE_SCHED_AVG_IRQ
        struct sched_avg        avg_irq;
#endif
        u64                     idle_stamp;
        u64                     avg_idle;

        /* This is used to determine avg_idle's max value */
        u64                     max_idle_balance_cost;
#endif

#ifdef CONFIG_IRQ_TIME_ACCOUNTING
        u64                     prev_irq_time;
#endif
#ifdef CONFIG_PARAVIRT
        u64                     prev_steal_time;
#endif
#ifdef CONFIG_PARAVIRT_TIME_ACCOUNTING
        u64                     prev_steal_time_rq;
#endif

        /* calc_load related fields */
        unsigned long           calc_load_update;
        long                    calc_load_active;

#ifdef CONFIG_SCHED_HRTICK
#ifdef CONFIG_SMP
        int                     hrtick_csd_pending;
        call_single_data_t      hrtick_csd;
#endif
        struct hrtimer          hrtick_timer;
#endif

#ifdef CONFIG_SCHEDSTATS
        /* latency stats */
        struct sched_info       rq_sched_info;
        unsigned long long      rq_cpu_time;
        /* could above be rq->cfs_rq.exec_clock + rq->rt_rq.rt_runtime ? */

        /* sys_sched_yield() stats */
        unsigned int            yld_count;

        /* schedule() stats */
        unsigned int            sched_count;
        unsigned int            sched_goidle;

        /* try_to_wake_up() stats */
        unsigned int            ttwu_count;
        unsigned int            ttwu_local;
#endif

#ifdef CONFIG_SMP
        struct llist_head       wake_list;
#endif

#ifdef CONFIG_CPU_IDLE
        /* Must be inspected within a rcu lock section */
        struct cpuidle_state    *idle_state;
#endif
};


struct sched_class {
        const struct sched_class *next;

#ifdef CONFIG_UCLAMP_TASK
        int uclamp_enabled;
#endif

        void (*enqueue_task) (struct rq *rq, struct task_struct *p, int flags);
        void (*dequeue_task) (struct rq *rq, struct task_struct *p, int flags);
        void (*yield_task)   (struct rq *rq);
        bool (*yield_to_task)(struct rq *rq, struct task_struct *p, bool preempt);

        void (*check_preempt_curr)(struct rq *rq, struct task_struct *p, int flags);

        /*
         * Both @prev and @rf are optional and may be NULL, in which case the
         * caller must already have invoked put_prev_task(rq, prev, rf).
         *
         * Otherwise it is the responsibility of the pick_next_task() to call
         * put_prev_task() on the @prev task or something equivalent, IFF it
         * returns a next task.
         *
         * In that case (@rf != NULL) it may return RETRY_TASK when it finds a
         * higher prio class has runnable tasks.
         */
        struct task_struct * (*pick_next_task)(struct rq *rq,
                                               struct task_struct *prev,
                                               struct rq_flags *rf);
        void (*put_prev_task)(struct rq *rq, struct task_struct *p);
        void (*set_next_task)(struct rq *rq, struct task_struct *p);

#ifdef CONFIG_SMP
        int (*balance)(struct rq *rq, struct task_struct *prev, struct rq_flags *rf);
        int  (*select_task_rq)(struct task_struct *p, int task_cpu, int sd_flag, int flags);
        void (*migrate_task_rq)(struct task_struct *p, int new_cpu);

        void (*task_woken)(struct rq *this_rq, struct task_struct *task);

        void (*set_cpus_allowed)(struct task_struct *p,
                                 const struct cpumask *newmask);

        void (*rq_online)(struct rq *rq);
        void (*rq_offline)(struct rq *rq);
#endif

        void (*task_tick)(struct rq *rq, struct task_struct *p, int queued);
        void (*task_fork)(struct task_struct *p);
        void (*task_dead)(struct task_struct *p);

        /*
         * The switched_from() call is allowed to drop rq->lock, therefore we
         * cannot assume the switched_from/switched_to pair is serliazed by
         * rq->lock. They are however serialized by p->pi_lock.
         */
        void (*switched_from)(struct rq *this_rq, struct task_struct *task);
        void (*switched_to)  (struct rq *this_rq, struct task_struct *task);
        void (*prio_changed) (struct rq *this_rq, struct task_struct *task,
                              int oldprio);

        unsigned int (*get_rr_interval)(struct rq *rq,
                                        struct task_struct *task);

        void (*update_curr)(struct rq *rq);

#define TASK_SET_GROUP          0
#define TASK_MOVE_GROUP         1

#ifdef CONFIG_FAIR_GROUP_SCHED
        void (*task_change_group)(struct task_struct *p, int type);
#endif
};



#ifdef __cplusplus
}
#endif
