// SPDX-License-Identifier: GPL-2.0
/*
 * Scheduler topology setup/handling methods
 */
#include <urcu.h>

#include "sched.h"

DEFINE_MUTEX(sched_domains_mutex);

/* Protected by sched_domains_mutex: */
static cpumask_var_t sched_domains_tmpmask;
static cpumask_var_t sched_domains_tmpmask2;

#ifdef CONFIG_SCHED_DEBUG

static int __init sched_debug_setup(char *str)
{
    sched_debug_enabled = true;

    return 0;
}
early_param("sched_debug", sched_debug_setup);

static inline bool sched_debug(void)
{
    return sched_debug_enabled;
}

static int sched_domain_debug_one(struct sched_domain *sd, int cpu, int level,
                  struct cpumask *groupmask)
{
    struct sched_group *group = sd->groups;

    cpumask_clear(groupmask);

    printk(KERN_DEBUG "%*s domain-%d: ", level, "", level);

    if (!(sd->flags & SD_LOAD_BALANCE)) {
        printk("does not load-balance\n");
        if (sd->parent)
            printk(KERN_ERR "ERROR: !SD_LOAD_BALANCE domain has parent");
        return -1;
    }

    printk(KERN_CONT "span=%*pbl level=%s\n",
           cpumask_pr_args(sched_domain_span(sd)), sd->name);

    if (!cpumask_test_cpu(cpu, sched_domain_span(sd))) {
        printk(KERN_ERR "ERROR: domain->span does not contain CPU%d\n", cpu);
    }
    if (group && !cpumask_test_cpu(cpu, sched_group_span(group))) {
        printk(KERN_ERR "ERROR: domain->groups does not contain CPU%d\n", cpu);
    }

    printk(KERN_DEBUG "%*s groups:", level + 1, "");
    do {
        if (!group) {
            printk("\n");
            printk(KERN_ERR "ERROR: group is NULL\n");
            break;
        }

        if (!cpumask_weight(sched_group_span(group))) {
            printk(KERN_CONT "\n");
            printk(KERN_ERR "ERROR: empty group\n");
            break;
        }

        if (!(sd->flags & SD_OVERLAP) &&
            cpumask_intersects(groupmask, sched_group_span(group))) {
            printk(KERN_CONT "\n");
            printk(KERN_ERR "ERROR: repeated CPUs\n");
            break;
        }

        cpumask_or(groupmask, groupmask, sched_group_span(group));

        printk(KERN_CONT " %d:{ span=%*pbl",
                group->sgc->id,
                cpumask_pr_args(sched_group_span(group)));

        if ((sd->flags & SD_OVERLAP) &&
            !cpumask_equal(group_balance_mask(group), sched_group_span(group))) {
            printk(KERN_CONT " mask=%*pbl",
                cpumask_pr_args(group_balance_mask(group)));
        }

        if (group->sgc->capacity != SCHED_CAPACITY_SCALE)
            printk(KERN_CONT " cap=%lu", group->sgc->capacity);

        if (group == sd->groups && sd->child &&
            !cpumask_equal(sched_domain_span(sd->child),
                   sched_group_span(group))) {
            printk(KERN_ERR "ERROR: domain->groups does not match domain->child\n");
        }

        printk(KERN_CONT " }");

        group = group->next;

        if (group != sd->groups)
            printk(KERN_CONT ",");

    } while (group != sd->groups);
    printk(KERN_CONT "\n");

    if (!cpumask_equal(sched_domain_span(sd), groupmask))
        printk(KERN_ERR "ERROR: groups don't span domain->span\n");

    if (sd->parent &&
        !cpumask_subset(groupmask, sched_domain_span(sd->parent)))
        printk(KERN_ERR "ERROR: parent span is not a superset of domain->span\n");
    return 0;
}

static void sched_domain_debug(struct sched_domain *sd, int cpu)
{
    int level = 0;

    if (!sched_debug_enabled)
        return;

    if (!sd) {
        printk(KERN_DEBUG "CPU%d attaching NULL sched-domain.\n", cpu);
        return;
    }

    printk(KERN_DEBUG "CPU%d attaching sched-domain(s):\n", cpu);

    for (;;) {
        if (sched_domain_debug_one(sd, cpu, level, sched_domains_tmpmask))
            break;
        level++;
        sd = sd->parent;
        if (!sd)
            break;
    }
}
#else /* !CONFIG_SCHED_DEBUG */

# define sched_debug_enabled 0
# define sched_domain_debug(sd, cpu) do { } while (0)
static inline bool sched_debug(void)
{
    return false;
}
#endif /* CONFIG_SCHED_DEBUG */
//147 lines





//439 lines
void rq_attach_root(struct rq *rq, struct root_domain *rd)
{
    struct root_domain *old_rd = NULL;
    unsigned long flags;

    raw_spin_lock_irqsave(&rq->lock, flags);

    if (rq->rd) {
        old_rd = rq->rd;

        if (cpumask_test_cpu(rq->cpu, old_rd->online))
            set_rq_offline(rq);

        cpumask_clear_cpu(rq->cpu, old_rd->span);

        /*
         * If we dont want to free the old_rd yet then
         * set old_rd to NULL to skip the freeing later
         * in this function:
         */
        if (!atomic_dec_and_test(&old_rd->refcount))
            old_rd = NULL;
    }

    atomic_inc(&rd->refcount);
    rq->rd = rd;

    cpumask_set_cpu(rq->cpu, rd->span);
    if (cpumask_test_cpu(rq->cpu, cpu_active_mask))
        set_rq_online(rq);

    raw_spin_unlock_irqrestore(&rq->lock, flags);

    //if (old_rd)
    //    call_rcu(&old_rd->rcu, free_rootdomain);
}

void sched_get_rd(struct root_domain *rd)
{
    atomic_inc(&rd->refcount);
}

void sched_put_rd(struct root_domain *rd)
{
    if (!atomic_dec_and_test(&rd->refcount))
        return;

    //call_rcu(&rd->rcu, free_rootdomain);
}
//489
static int init_rootdomain(struct root_domain *rd)
{
    if (!zalloc_cpumask_var(&rd->span, GFP_KERNEL))
        goto out;
    if (!zalloc_cpumask_var(&rd->online, GFP_KERNEL))
        goto free_span;
    if (!zalloc_cpumask_var(&rd->dlo_mask, GFP_KERNEL))
        goto free_online;
    if (!zalloc_cpumask_var(&rd->rto_mask, GFP_KERNEL))
        goto free_dlo_mask;

#ifdef HAVE_RT_PUSH_IPI
    rd->rto_cpu = -1;
    raw_spin_lock_init(&rd->rto_lock);
    init_irq_work(&rd->rto_push_work, rto_push_irq_work_func);
#endif

    init_dl_bw(&rd->dl_bw);
    if (cpudl_init(&rd->cpudl) != 0)
        goto free_rto_mask;

    if (cpupri_init(&rd->cpupri) != 0)
        goto free_cpudl;
    return 0;

free_cpudl:
    cpudl_cleanup(&rd->cpudl);
free_rto_mask:
    free_cpumask_var(rd->rto_mask);
free_dlo_mask:
    free_cpumask_var(rd->dlo_mask);
free_online:
    free_cpumask_var(rd->online);
free_span:
    free_cpumask_var(rd->span);
out:
    return -ENOMEM;
}

/*
 * By default the system creates a single root-domain with all CPUs as
 * members (mimicking the global state we have today).
 */
struct root_domain def_root_domain;

void init_defrootdomain(void)
{
    pr_fn_start_on(stack_depth);

    init_rootdomain(&def_root_domain);

    atomic_set(&def_root_domain.refcount, 1);

    pr_fn_end_on(stack_depth);
}
//541 lines






//608 lines
/*
 * Keep a special pointer to the highest sched_domain that has
 * SD_SHARE_PKG_RESOURCE set (Last Level Cache Domain) for this
 * allows us to avoid some pointer chasing select_idle_sibling().
 *
 * Also keep a unique ID per domain (we use the first CPU number in
 * the cpumask of the domain), this allows us to quickly tell if
 * two CPUs are in the same cache domain, see cpus_share_cache().
 */
DEFINE_PER_CPU(struct sched_domain __rcu *, sd_llc);
DEFINE_PER_CPU(int, sd_llc_size);
DEFINE_PER_CPU(int, sd_llc_id);
DEFINE_PER_CPU(struct sched_domain_shared __rcu *, sd_llc_shared);
DEFINE_PER_CPU(struct sched_domain __rcu *, sd_numa);
DEFINE_PER_CPU(struct sched_domain __rcu *, sd_asym_packing);
DEFINE_PER_CPU(struct sched_domain __rcu *, sd_asym_cpucapacity);
DEFINE_STATIC_KEY_FALSE(sched_asym_cpucapacity);
//626 lines

