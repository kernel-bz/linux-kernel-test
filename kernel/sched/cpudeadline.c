// SPDX-License-Identifier: GPL-2.0-only
/*
 *  kernel/sched/cpudl.c
 *
 *  Global CPU deadline management
 *
 *  Author: Juri Lelli <j.lelli@sssup.it>
 */
#include "sched.h"

static inline int parent(int i)
{
	return (i - 1) >> 1;
}

static inline int left_child(int i)
{
	return (i << 1) + 1;
}

static inline int right_child(int i)
{
	return (i << 1) + 2;
}

static void cpudl_heapify_down(struct cpudl *cp, int idx)
{
    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %d\n", idx);

	int l, r, largest;

	int orig_cpu = cp->elements[idx].cpu;
	u64 orig_dl = cp->elements[idx].dl;

	if (left_child(idx) >= cp->size)
		return;

	/* adapted from lib/prio_heap.c */
	while (1) {
		u64 largest_dl;

		l = left_child(idx);
		r = right_child(idx);
		largest = idx;
		largest_dl = orig_dl;

		if ((l < cp->size) && dl_time_before(orig_dl,
						cp->elements[l].dl)) {
			largest = l;
			largest_dl = cp->elements[l].dl;
		}
		if ((r < cp->size) && dl_time_before(largest_dl,
						cp->elements[r].dl))
			largest = r;

		if (largest == idx)
			break;

		/* pull largest child onto idx */
		cp->elements[idx].cpu = cp->elements[largest].cpu;
		cp->elements[idx].dl = cp->elements[largest].dl;
		cp->elements[cp->elements[idx].cpu].idx = idx;
		idx = largest;
        pr_view_on(stack_depth, "%20s : %d\n", idx);
    }
	/* actual push down of saved original values orig_* */
	cp->elements[idx].cpu = orig_cpu;
	cp->elements[idx].dl = orig_dl;
	cp->elements[cp->elements[idx].cpu].idx = idx;

    pr_view_on(stack_depth, "%20s : %d\n", idx);
    pr_fn_end_on(stack_depth);
}

static void cpudl_heapify_up(struct cpudl *cp, int idx)
{
    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %d\n", idx);

    int p;

	int orig_cpu = cp->elements[idx].cpu;
	u64 orig_dl = cp->elements[idx].dl;

	if (idx == 0)
		return;

	do {
		p = parent(idx);
		if (dl_time_before(orig_dl, cp->elements[p].dl))
			break;
		/* pull parent onto idx */
		cp->elements[idx].cpu = cp->elements[p].cpu;
		cp->elements[idx].dl = cp->elements[p].dl;
		cp->elements[cp->elements[idx].cpu].idx = idx;
		idx = p;
        pr_view_on(stack_depth, "%20s : %d\n", idx);
    } while (idx != 0);
	/* actual push up of saved original values orig_* */
	cp->elements[idx].cpu = orig_cpu;
	cp->elements[idx].dl = orig_dl;
	cp->elements[cp->elements[idx].cpu].idx = idx;

    pr_view_on(stack_depth, "%20s : %d\n", idx);
    pr_fn_end_on(stack_depth);
}

static void cpudl_heapify(struct cpudl *cp, int idx)
{
	if (idx > 0 && dl_time_before(cp->elements[parent(idx)].dl,
				cp->elements[idx].dl))
		cpudl_heapify_up(cp, idx);
	else
		cpudl_heapify_down(cp, idx);
}

static inline int cpudl_maximum(struct cpudl *cp)
{
	return cp->elements[0].cpu;
}

/*
 * cpudl_find - find the best (later-dl) CPU in the system
 * @cp: the cpudl max-heap context
 * @p: the task
 * @later_mask: a mask to fill in with the selected CPUs (or NULL)
 *
 * Returns: int - CPUs were found
 */
int cpudl_find(struct cpudl *cp, struct task_struct *p,
	       struct cpumask *later_mask)
{
	const struct sched_dl_entity *dl_se = &p->dl;

	if (later_mask &&
	    cpumask_and(later_mask, cp->free_cpus, p->cpus_ptr)) {
		return 1;
	} else {
		int best_cpu = cpudl_maximum(cp);

		WARN_ON(best_cpu != -1 && !cpu_present(best_cpu));

		if (cpumask_test_cpu(best_cpu, p->cpus_ptr) &&
		    dl_time_before(dl_se->deadline, cp->elements[0].dl)) {
			if (later_mask)
				cpumask_set_cpu(best_cpu, later_mask);

			return 1;
		}
	}
	return 0;
}

/*
 * cpudl_clear - remove a CPU from the cpudl max-heap
 * @cp: the cpudl max-heap context
 * @cpu: the target CPU
 *
 * Notes: assumes cpu_rq(cpu)->lock is locked
 *
 * Returns: (void)
 */
void cpudl_clear(struct cpudl *cp, int cpu)
{
	int old_idx, new_cpu;
	unsigned long flags;

	WARN_ON(!cpu_present(cpu));

	raw_spin_lock_irqsave(&cp->lock, flags);

	old_idx = cp->elements[cpu].idx;
	if (old_idx == IDX_INVALID) {
		/*
		 * Nothing to remove if old_idx was invalid.
		 * This could happen if a rq_offline_dl is
		 * called for a CPU without -dl tasks running.
		 */
	} else {
		new_cpu = cp->elements[cp->size - 1].cpu;
		cp->elements[old_idx].dl = cp->elements[cp->size - 1].dl;
		cp->elements[old_idx].cpu = new_cpu;
		cp->size--;
		cp->elements[new_cpu].idx = old_idx;
		cp->elements[cpu].idx = IDX_INVALID;
		cpudl_heapify(cp, old_idx);

		cpumask_set_cpu(cpu, cp->free_cpus);
	}
	raw_spin_unlock_irqrestore(&cp->lock, flags);
}

/*
 * cpudl_set - update the cpudl max-heap
 * @cp: the cpudl max-heap context
 * @cpu: the target CPU
 * @dl: the new earliest deadline for this CPU
 *
 * Notes: assumes cpu_rq(cpu)->lock is locked
 *
 * Returns: (void)
 */
void cpudl_set(struct cpudl *cp, int cpu, u64 dl)
{
    pr_fn_start_on(stack_depth);

	int old_idx;
	unsigned long flags;

    //WARN_ON(!cpu_present(cpu));

    pr_view_on(stack_depth, "%20s : %d\n", cpu);
    pr_view_on(stack_depth, "%20s : %llu\n", dl);

    raw_spin_lock_irqsave(&cp->lock, flags);

	old_idx = cp->elements[cpu].idx;
    pr_view_on(stack_depth, "%20s : %d\n", old_idx);
    if (old_idx == IDX_INVALID) {
		int new_idx = cp->size++;
        pr_view_on(stack_depth, "%20s : %d\n", new_idx);

        cp->elements[new_idx].dl = dl;
		cp->elements[new_idx].cpu = cpu;
		cp->elements[cpu].idx = new_idx;
		cpudl_heapify_up(cp, new_idx);
		cpumask_clear_cpu(cpu, cp->free_cpus);
	} else {
		cp->elements[old_idx].dl = dl;
		cpudl_heapify(cp, old_idx);
	}

    int i;
    pr_view_on(stack_depth, "%20s : %d\n", cp->size);
    for (i=0; i < cp->size; i++) {
        pr_view_on(stack_depth, "%30s : %d\n", i);
        pr_view_on(stack_depth, "%30s : %d\n", cp->elements[i].idx);
        pr_view_on(stack_depth, "%30s : %d\n", cp->elements[i].cpu);
        pr_view_on(stack_depth, "%30s : %llu\n", cp->elements[i].dl);
        //pr_view_on(stack_depth, "%30s : 0x%X\n", cp->free_cpus);
    }

    raw_spin_unlock_irqrestore(&cp->lock, flags);

    pr_fn_end_on(stack_depth);
}

/*
 * cpudl_set_freecpu - Set the cpudl.free_cpus
 * @cp: the cpudl max-heap context
 * @cpu: rd attached CPU
 */
void cpudl_set_freecpu(struct cpudl *cp, int cpu)
{
	cpumask_set_cpu(cpu, cp->free_cpus);
}

/*
 * cpudl_clear_freecpu - Clear the cpudl.free_cpus
 * @cp: the cpudl max-heap context
 * @cpu: rd attached CPU
 */
void cpudl_clear_freecpu(struct cpudl *cp, int cpu)
{
	cpumask_clear_cpu(cpu, cp->free_cpus);
}

/*
 * cpudl_init - initialize the cpudl structure
 * @cp: the cpudl max-heap context
 */
int cpudl_init(struct cpudl *cp)
{
	int i;
    pr_fn_start_on(stack_depth);

	raw_spin_lock_init(&cp->lock);
	cp->size = 0;

	cp->elements = kcalloc(nr_cpu_ids,
			       sizeof(struct cpudl_item),
			       GFP_KERNEL);
	if (!cp->elements)
		return -ENOMEM;

	if (!zalloc_cpumask_var(&cp->free_cpus, GFP_KERNEL)) {
		kfree(cp->elements);
		return -ENOMEM;
	}

    for_each_possible_cpu(i)
		cp->elements[i].idx = IDX_INVALID;

    pr_fn_end_on(stack_depth);

	return 0;
}

/*
 * cpudl_cleanup - clean up the cpudl structure
 * @cp: the cpudl max-heap context
 */
void cpudl_cleanup(struct cpudl *cp)
{
	free_cpumask_var(cp->free_cpus);
	kfree(cp->elements);
}
