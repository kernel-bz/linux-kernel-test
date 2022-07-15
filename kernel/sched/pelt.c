// SPDX-License-Identifier: GPL-2.0
/*
 * Per Entity Load Tracking
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
 *
 *  Move PELT related code from fair.c into this pelt.c file
 *  Author: Vincent Guittot <vincent.guittot@linaro.org>
 */

#include "test/user-types.h"

#include <linux/sched.h>
#include "sched-pelt.h"
#include "sched.h"
#include "pelt.h"

#include <linux/math64.h>

//#include <trace/events/sched.h>

/*
 * Approximate:
 *   val * y^n,    where y^32 ~= 0.5 (~1 scheduling period)
 */
static u64 decay_load(u64 val, u64 n)
{
	unsigned int local_n;
    u64 oval = val;

    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%10s : %llu\n", val);
    //pr_view_on(stack_depth, "%10s : %llu\n", n);

	if (unlikely(n > LOAD_AVG_PERIOD * 63))
		return 0;

	/* after bounds checking we can collapse to 32-bit */
	local_n = n;

	/*
	 * As y^PERIOD = 1/2, we can combine
	 *    y^n = 1/2^(n/PERIOD) * y^(n%PERIOD)
	 * With a look-up table which covers y^n (n<PERIOD)
	 *
	 * To achieve constant time decay_load.
	 */
	if (unlikely(local_n >= LOAD_AVG_PERIOD)) {
		val >>= local_n / LOAD_AVG_PERIOD;
		local_n %= LOAD_AVG_PERIOD;
	}

	val = mul_u64_u32_shr(val, runnable_avg_yN_inv[local_n], 32);

    if (oval != val)
        pr_view_on(stack_depth, "%10s : %llu\n", val);

    pr_fn_end_on(stack_depth);
    return val;
}

static u32 __accumulate_pelt_segments(u64 periods, u32 d1, u32 d3)
{
	u32 c1, c2, c3 = d3; /* y^0 == 1 */

    pr_fn_start_on(stack_depth);
    /*
	 * c1 = d1 y^p
	 */
	c1 = decay_load((u64)d1, periods);

	/*
	 *            p-1
	 * c2 = 1024 \Sum y^n
	 *            n=1
	 *
	 *              inf        inf
	 *    = 1024 ( \Sum y^n - \Sum y^n - y^0 )
	 *              n=0        n=p
	 */
	c2 = LOAD_AVG_MAX - decay_load(LOAD_AVG_MAX, periods) - 1024;

    pr_view_on(stack_depth, "%10s : %u\n", c1);
    pr_view_on(stack_depth, "%10s : %u\n", c2);
    pr_view_on(stack_depth, "%10s : %u\n", c3);
    pr_view_on(stack_depth, "%10s : %u\n", c1+c2+c3);

    pr_fn_end_on(stack_depth);
    return c1 + c2 + c3;
}

#define cap_scale(v, s) ((v)*(s) >> SCHED_CAPACITY_SHIFT)

/*
 * Accumulate the three separate parts of the sum; d1 the remainder
 * of the last (incomplete) period, d2 the span of full periods and d3
 * the remainder of the (incomplete) current period.
 *
 *           d1          d2           d3
 *           ^           ^            ^
 *           |           |            |
 *         |<->|<----------------->|<--->|
 * ... |---x---|------| ... |------|-----x (now)
 *
 *                           p-1
 * u' = (u + d1) y^p + 1024 \Sum y^n + d3 y^0
 *                           n=1
 *
 *    = u y^p +					(Step 1)
 *
 *                     p-1
 *      d1 y^p + 1024 \Sum y^n + d3 y^0		(Step 2)
 *                     n=1
 */
static __always_inline u32
accumulate_sum(u64 delta, struct sched_avg *sa,
	       unsigned long load, unsigned long runnable, int running)
{
    pr_fn_start_on(stack_depth);

    u32 contrib = (u32)delta; /* p == 0 -> delta < 1024 */
	u64 periods;

    pr_view_on(stack_depth, "%23s(delta) : %u\n", contrib);

	delta += sa->period_contrib;
	periods = delta / 1024; /* A period is 1024us (~1ms) */

    pr_view_on(stack_depth, "%30s : %u\n", sa->period_contrib);
    pr_view_on(stack_depth, "%21s(+old_d3) : %llu\n", delta);
    pr_view_on(stack_depth, "%26s(ms) : %llu\n", periods);

	/*
	 * Step 1: decay old *_sum if we crossed period boundaries.
	 */
	if (periods) {
		sa->load_sum = decay_load(sa->load_sum, periods);
		sa->runnable_load_sum =
			decay_load(sa->runnable_load_sum, periods);
		sa->util_sum = decay_load((u64)(sa->util_sum), periods);

		/*
		 * Step 2
		 */
		delta %= 1024;
		contrib = __accumulate_pelt_segments(periods,
				1024 - sa->period_contrib, delta);

        pr_view_on(stack_depth, "%24s(pelt) : %u\n", contrib);
    }
	sa->period_contrib = delta;

    pr_view_on(stack_depth, "%30s : %u\n", sa->period_contrib);
    pr_view_on(stack_depth, "%30s : %lu\n", load);
    pr_view_on(stack_depth, "%30s : %lu\n", runnable);
    pr_view_on(stack_depth, "%30s : %d\n", running);

	if (load)
		sa->load_sum += load * contrib;
	if (runnable)
		sa->runnable_load_sum += runnable * contrib;
	if (running)
		sa->util_sum += contrib << SCHED_CAPACITY_SHIFT;

    pr_view_on(stack_depth, "%30s : %llu\n", sa->load_sum);
    pr_view_on(stack_depth, "%30s : %llu\n", sa->runnable_load_sum);
    pr_view_on(stack_depth, "%30s : %u\n", sa->util_sum);

    pr_fn_end_on(stack_depth);

    return periods;
}

/*
 * We can represent the historical contribution to runnable average as the
 * coefficients of a geometric series.  To do this we sub-divide our runnable
 * history into segments of approximately 1ms (1024us); label the segment that
 * occurred N-ms ago p_N, with p_0 corresponding to the current period, e.g.
 *
 * [<- 1024us ->|<- 1024us ->|<- 1024us ->| ...
 *      p0            p1           p2
 *     (now)       (~1ms ago)  (~2ms ago)
 *
 * Let u_i denote the fraction of p_i that the entity was runnable.
 *
 * We then designate the fractions u_i as our co-efficients, yielding the
 * following representation of historical load:
 *   u_0 + u_1*y + u_2*y^2 + u_3*y^3 + ...
 *
 * We choose y based on the with of a reasonably scheduling period, fixing:
 *   y^32 = 0.5
 *
 * This means that the contribution to load ~32ms ago (u_32) will be weighted
 * approximately half as much as the contribution to load within the last ms
 * (u_0).
 *
 * When a period "rolls over" and we have new u_0`, multiplying the previous
 * sum again by y is sufficient to update:
 *   load_avg = u_0` + y*(u_0 + u_1*y + u_2*y^2 + ... )
 *            = u_0 + u_1*y + u_2*y^2 + ... [re-labeling u_i --> u_{i+1}]
 */
static __always_inline int
___update_load_sum(u64 now, struct sched_avg *sa,
		  unsigned long load, unsigned long runnable, int running)
{
    u64 delta;
    int ret=1;

    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %lu\n", load);
    pr_view_on(stack_depth, "%20s : %lu\n", runnable);
    pr_view_on(stack_depth, "%20s : %d\n", running);

	delta = now - sa->last_update_time;
    pr_view_on(stack_depth, "%20s : %llu\n", now);
    pr_view_on(stack_depth, "%20s : %llu\n", sa->last_update_time);
    pr_view_on(stack_depth, "%16s(ns) : %lld\n", (s64)delta);
    /*
	 * This should only happen when time goes backwards, which it
	 * unfortunately does during sched clock init when we swap over to TSC.
	 */
	if ((s64)delta < 0) {
		sa->last_update_time = now;
		return 0;
	}

    /*
	 * Use 1024ns as the unit of measurement since it's a reasonable
	 * approximation of 1us and fast to compute.
	 */
	delta >>= 10;
	if (!delta)
		return 0;

    pr_view_on(stack_depth, "%16s(us) : %llu\n", delta);

	sa->last_update_time += delta << 10;

    pr_view_on(stack_depth, "%20s : %llu\n", sa->last_update_time);

	/*
	 * running is a subset of runnable (weight) so running can't be set if
	 * runnable is clear. But there are some corner cases where the current
	 * se has been already dequeued but cfs_rq->curr still points to it.
	 * This means that weight will be 0 but not running for a sched_entity
	 * but also for a cfs_rq if the latter becomes idle. As an example,
	 * this happens during idle_balance() which calls
	 * update_blocked_averages()
	 */
	if (!load)
		runnable = running = 0;

	/*
	 * Now we know we crossed measurement unit boundaries. The *_avg
	 * accrues by two steps:
	 *
	 * Step 1: accumulate *_sum since last_update_time. If we haven't
	 * crossed period boundaries, finish.
	 */
    if (!accumulate_sum(delta, sa, load, runnable, running))
        ret = 0;

    pr_fn_end_on(stack_depth);

    return ret;
}

static __always_inline void
___update_load_avg(struct sched_avg *sa, unsigned long load, unsigned long runnable)
{
    pr_fn_start_on(stack_depth);

    u32 divider = LOAD_AVG_MAX - 1024 + sa->period_contrib;

    pr_view_on(stack_depth, "%20s : %lu\n", load);
    pr_view_on(stack_depth, "%20s : %lu\n", runnable);
    pr_view_on(stack_depth, "%20s : %u\n", sa->period_contrib);
    pr_view_on(stack_depth, "%20s : %u\n", divider);
    /*
	 * Step 2: update *_avg.
	 */
	sa->load_avg = div_u64(load * sa->load_sum, divider);
	sa->runnable_load_avg =	div_u64(runnable * sa->runnable_load_sum, divider);
	WRITE_ONCE(sa->util_avg, sa->util_sum / divider);

    pr_view_on(stack_depth, "%30s : %lu\n", sa->load_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", sa->runnable_load_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", sa->util_avg);
    pr_fn_end_on(stack_depth);
}

/*
 * sched_entity:
 *
 *   task:
 *     se_runnable() == se_weight()
 *
 *   group: [ see update_cfs_group() ]
 *     se_weight()   = tg->weight * grq->load_avg / tg->load_avg
 *     se_runnable() = se_weight(se) * grq->runnable_load_avg / grq->load_avg
 *
 *   load_sum := runnable_sum
 *   load_avg = se_weight(se) * runnable_avg
 *
 *   runnable_load_sum := runnable_sum
 *   runnable_load_avg = se_runnable(se) * runnable_avg
 *
 * XXX collapse load_sum and runnable_load_sum
 *
 * cfq_rq:
 *
 *   load_sum = \Sum se_weight(se) * se->avg.load_sum
 *   load_avg = \Sum se->avg.load_avg
 *
 *   runnable_load_sum = \Sum se_runnable(se) * se->avg.runnable_load_sum
 *   runnable_load_avg = \Sum se->avg.runable_load_avg
 */

int __update_load_avg_blocked_se(u64 now, struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %p\n", (void*)se);

	if (___update_load_sum(now, &se->avg, 0, 0, 0)) {
		___update_load_avg(&se->avg, se_weight(se), se_runnable(se));
        //trace_pelt_se_tp(se);
        pr_fn_end_on(stack_depth);
        return 1;
	}

    pr_fn_end_on(stack_depth);
	return 0;
}

int __update_load_avg_se(u64 now, struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %p\n", (void*)se);

	if (___update_load_sum(now, &se->avg, !!se->on_rq, !!se->on_rq,
				cfs_rq->curr == se)) {

		___update_load_avg(&se->avg, se_weight(se), se_runnable(se));
		cfs_se_util_change(&se->avg);
        //trace_pelt_se_tp(se);
        pr_fn_end_on(stack_depth);
        return 1;
	}

    pr_fn_end_on(stack_depth);
	return 0;
}

int __update_load_avg_cfs_rq(u64 now, struct cfs_rq *cfs_rq)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%20s : %p\n", (void*)cfs_rq);

	if (___update_load_sum(now, &cfs_rq->avg,
				scale_load_down(cfs_rq->load.weight),
				scale_load_down(cfs_rq->runnable_weight),
				cfs_rq->curr != NULL)) {

		___update_load_avg(&cfs_rq->avg, 1, 1);
        //trace_pelt_cfs_tp(cfs_rq);
        pr_fn_end_on(stack_depth);
		return 1;
	}

    pr_fn_end_on(stack_depth);
	return 0;
}

/*
 * rt_rq:
 *
 *   util_sum = \Sum se->avg.util_sum but se->avg.util_sum is not tracked
 *   util_sum = cpu_scale * load_sum
 *   runnable_load_sum = load_sum
 *
 *   load_avg and runnable_load_avg are not supported and meaningless.
 *
 */

int update_rt_rq_load_avg(u64 now, struct rq *rq, int running)
{
    pr_fn_start_on(stack_depth);

    if (___update_load_sum(now, &rq->avg_rt,
				running,
				running,
				running)) {

		___update_load_avg(&rq->avg_rt, 1, 1);
        //trace_pelt_rt_tp(rq);
        pr_fn_end_on(stack_depth);
        return 1;
	}

    pr_fn_end_on(stack_depth);
    return 0;
}

/*
 * dl_rq:
 *
 *   util_sum = \Sum se->avg.util_sum but se->avg.util_sum is not tracked
 *   util_sum = cpu_scale * load_sum
 *   runnable_load_sum = load_sum
 *
 */

int update_dl_rq_load_avg(u64 now, struct rq *rq, int running)
{
    pr_fn_start_on(stack_depth);

    if (___update_load_sum(now, &rq->avg_dl,
				running,
				running,
				running)) {

		___update_load_avg(&rq->avg_dl, 1, 1);
        //trace_pelt_dl_tp(rq);
        pr_fn_end_on(stack_depth);
        return 1;
	}

    pr_fn_end_on(stack_depth);
	return 0;
}

#ifdef CONFIG_HAVE_SCHED_AVG_IRQ
/*
 * irq:
 *
 *   util_sum = \Sum se->avg.util_sum but se->avg.util_sum is not tracked
 *   util_sum = cpu_scale * load_sum
 *   runnable_load_sum = load_sum
 *
 */

int update_irq_load_avg(struct rq *rq, u64 running)
{
	int ret = 0;

	/*
	 * We can't use clock_pelt because irq time is not accounted in
	 * clock_task. Instead we directly scale the running time to
	 * reflect the real amount of computation
	 */
	running = cap_scale(running, arch_scale_freq_capacity(cpu_of(rq)));
	running = cap_scale(running, arch_scale_cpu_capacity(cpu_of(rq)));

	/*
	 * We know the time that has been used by interrupt since last update
	 * but we don't when. Let be pessimistic and assume that interrupt has
	 * happened just before the update. This is not so far from reality
	 * because interrupt will most probably wake up task and trig an update
	 * of rq clock during which the metric is updated.
	 * We start to decay with normal context time and then we add the
	 * interrupt context time.
	 * We can safely remove running from rq->clock because
	 * rq->clock += delta with delta >= running
	 */
	ret = ___update_load_sum(rq->clock - running, &rq->avg_irq,
				0,
				0,
				0);
	ret += ___update_load_sum(rq->clock, &rq->avg_irq,
				1,
				1,
				1);

	if (ret) {
		___update_load_avg(&rq->avg_irq, 1, 1);
        //trace_pelt_irq_tp(rq);
	}

	return ret;
}
#endif
