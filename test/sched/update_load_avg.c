/*
 * 	kernel/sched/fair.c
 *  update_load_avg()
 */

#include <stdio.h>
#include <stdio_ext.h>

#include "test/user-types.h"
#include "test/config.h"
#include "test/debug.h"

#include <linux/sched.h>
#include <linux/math64.h>
#include <linux/limits.h>
#include "kernel/sched/sched-pelt.h"

static struct sched_avg se_avg = {
    .load_sum = 0,
    .runnable_load_sum = 0,
    .util_sum = 0
};

static struct sched_avg cfs_rq_avg = {
    .load_sum = 0,
    .runnable_load_sum = 0,
    .util_sum = 0
};

static unsigned long load, runnable;
static int running;
static unsigned long se_load_weight, se_runnable_weight;

static void _pr_sched_avg_info(struct sched_avg *sa)
{
    pr_fn_start_on(stack_depth);

    pr_view_on(stack_depth, "%25s : %llu\n", sa->last_update_time);
    pr_view_on(stack_depth, "%25s : %llu\n", sa->load_sum);
    pr_view_on(stack_depth, "%25s : %llu\n", sa->runnable_load_sum);
    pr_view_on(stack_depth, "%25s : %u\n", sa->util_sum);
    pr_view_on(stack_depth, "%25s : %u\n", sa->period_contrib);
    pr_view_on(stack_depth, "%25s : %lu\n", sa->load_avg);
    pr_view_on(stack_depth, "%25s : %lu\n", sa->runnable_load_avg);
    pr_view_on(stack_depth, "%25s : %lu\n", sa->util_avg);
    //pr_view_on(stack_depth, "%25s : %llu\n", sa->util_est.enqueued);
    //pr_view_on(stack_depth, "%25s : %llu\n", sa->util_est.ewma);

    pr_fn_end_on(stack_depth);
}

static u64 decay_load(u64 val, u64 n)
{
        unsigned int local_n;

        //pr_fn_start_on(stack_depth);

        //pr_view_on(stack_depth, "%10s : %llu\n", val);
        //pr_view_on(stack_depth, "%10s : %llu\n", n);

        if (n > LOAD_AVG_PERIOD * 63)
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
        if (local_n >= LOAD_AVG_PERIOD) {
                val >>= local_n / LOAD_AVG_PERIOD;
                local_n %= LOAD_AVG_PERIOD;
        }

        val = mul_u64_u32_shr(val, runnable_avg_yN_inv[local_n], 32);

        //pr_view_on(stack_depth, "%10s : %u\n", local_n);
        //pr_view_on(stack_depth, "%10s : %llu\n", val);

        //pr_fn_end_on(stack_depth);
        return val;
}

static u32 __accumulate_pelt_segments(u64 periods, u32 d1, u32 d3)
{
        u32 c1, c2, c3 = d3; /* y^0 == 1 */

        pr_fn_start_on(stack_depth);
        pr_view_on(stack_depth, "%10s : %u\n", d1);
        pr_view_on(stack_depth, "%10s : %llu\n", periods);
        pr_view_on(stack_depth, "%10s : %u\n", d3);

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
 *    = u y^p +                                 (Step 1)
 *
 *                     p-1
 *      d1 y^p + 1024 \Sum y^n + d3 y^0         (Step 2)
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

        pr_view_on(stack_depth, "%30s : %llu\n", sa->load_sum);
        pr_view_on(stack_depth, "%30s : %llu\n", sa->runnable_load_sum);
        pr_view_on(stack_depth, "%30s : %u\n", sa->util_sum);
        /*
         * Step 2
         */
        delta %= 1024;
        contrib = __accumulate_pelt_segments(periods,
                1024 - sa->period_contrib, delta);

        pr_view_on(stack_depth, "%24s(pelt) : %u\n", contrib);
    }
    sa->period_contrib = delta;

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
static int
___update_load_sum(u64 now, struct sched_avg *sa,
                  unsigned long load, unsigned long runnable, int running)
{
    u64 delta;
    int ret=1;

    pr_fn_start_on(stack_depth);
    pr_view_on(stack_depth, "%20s : %llu\n", now);

    delta = now - sa->last_update_time;
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

    if (!load)
        runnable = running = 0;

    if (!accumulate_sum(delta, sa, load, runnable, running))
        ret = 0;

    pr_fn_end_on(stack_depth);

    return ret;
}

static void
___update_load_avg(struct sched_avg *sa, unsigned long load, unsigned long runnable)
{
    pr_fn_start_on(stack_depth);

    u32 divider = LOAD_AVG_MAX - 1024 + sa->period_contrib;

    pr_view_on(stack_depth, "%30s : %u\n", sa->period_contrib);
    pr_view_on(stack_depth, "%30s : %u\n", divider);
    pr_view_on(stack_depth, "%30s : %lu\n", load);
    pr_view_on(stack_depth, "%30s : %lu\n", runnable);
    /*
     * Step 2: update *_avg.
     */
    //sa->load_avg = div_u64(load * sa->load_sum, divider);
    sa->load_avg = (load * sa->load_sum) / divider;
    //sa->runnable_load_avg = div_u64(runnable * sa->runnable_load_sum, divider);
    sa->runnable_load_avg = (runnable * sa->runnable_load_sum) / divider;
    //WRITE_ONCE(sa->util_avg, sa->util_sum / divider);
    sa->util_avg = sa->util_sum / divider;

    pr_view_on(stack_depth, "%30s : %lu\n", sa->load_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", sa->runnable_load_avg);
    pr_view_on(stack_depth, "%30s : %lu\n", sa->util_avg);
    pr_fn_end_on(stack_depth);
}

#if 0
kernel/sched/fair.c
update_load_avg()
  kernel/sched/pelt.c
  __update_load_avg_se(now, *cfs_rq, *se)
    //kernel/sched/pelt.c
    ___update_load_sum(now, *sa, load, runnable, running)
        ___update_load_avg(*sa, load, runnable)

int __update_load_avg_blocked_se(u64 now, struct sched_entity *se)
{
        if (___update_load_sum(now, &se->avg, 0, 0, 0)) {
                ___update_load_avg(&se->avg, se_weight(se), se_runnable(se));
                trace_pelt_se_tp(se);
                return 1;
        }

        return 0;
}

int __update_load_avg_se(u64 now, struct cfs_rq *cfs_rq, struct sched_entity *se)
{
        if (___update_load_sum(now, &se->avg, !!se->on_rq, !!se->on_rq,
                                cfs_rq->curr == se)) {

                ___update_load_avg(&se->avg, se_weight(se), se_runnable(se));
                cfs_se_util_change(&se->avg);
                trace_pelt_se_tp(se);
                return 1;
        }

        return 0;
}

int __update_load_avg_cfs_rq(u64 now, struct cfs_rq *cfs_rq)
{
        if (___update_load_sum(now, &cfs_rq->avg,
                                scale_load_down(cfs_rq->load.weight),
                                scale_load_down(cfs_rq->runnable_weight),
                                cfs_rq->curr != NULL)) {

                ___update_load_avg(&cfs_rq->avg, 1, 1);
                trace_pelt_cfs_tp(cfs_rq);
                return 1;
        }

        return 0;
}
#endif //0

static int __update_load_avg_se(u64 now)
{
    int decayed;

    pr_fn_start_on(stack_depth);

    if (___update_load_sum(now, &se_avg, load, runnable, running)) {
            ___update_load_avg(&se_avg, se_load_weight, se_runnable_weight);
            //cfs_se_util_change(&se_avg);
            decayed = 1;
    } else
        decayed = 0;

    pr_view_on(stack_depth, "%20s : %d\n", decayed);
    _pr_sched_avg_info(&se_avg);

    pr_fn_end_on(stack_depth);
}

static int __update_load_avg_cfs_rq(u64 now)
{
    int decayed;
    pr_fn_start_on(stack_depth);

    if (___update_load_sum(now, &cfs_rq_avg,
                           se_load_weight, se_runnable_weight, running)) {
            ___update_load_avg(&cfs_rq_avg, 1, 1);
            decayed = 1;
    } else
        decayed = 0;

    pr_view_on(stack_depth, "%20s : %d\n", decayed);
    _pr_sched_avg_info(&cfs_rq_avg);

    pr_fn_end_on(stack_depth);
}

void test_update_load_avg(void)
{
    u64 now = 0, hz, ns;
    int i, loop_cnt, on_rq, run, weight, which;

    pr_fn_start_on(stack_depth);

    //se_load_weight = 1024;
    //se_runnable_weight = 1024;

    _pr_sched_avg_info(&se_avg);
    _pr_sched_avg_info(&cfs_rq_avg);

    __fpurge(stdin);
    printf("Input on_rq value[0,1]: ");
    scanf("%lu", &on_rq);
    printf("Input running value[0,1]: ");
    scanf("%d", &run);
    printf("Input weight value[0,%d]: ", LOAD_AVG_MAX);
    scanf("%d", &weight);
    printf("Select which one[0,1]: ");
    scanf("%d", &which);

    load = on_rq;
    runnable = on_rq;
    running = run;
    se_load_weight = weight;
    se_runnable_weight = weight;

    printf("Input HZ Value[%d,%d]: ", HZ, HZ*100);
    scanf("%llu", &hz);
    printf("Input Loop Counter[0,%d]: ", S32_MAX);
    scanf("%d", &loop_cnt);

    ns = 1000000000UL / hz;	//ns

    for (i=0; i<loop_cnt; i++) {
        now += ns;
        printf("----- loop %d ------------------------------------------\n", i);

        if (which == 0 || which >= 2)
            __update_load_avg_se(now);
        if (which == 1 || which >= 2)
            __update_load_avg_cfs_rq(now);
    }

    _pr_sched_avg_info(&se_avg);
    _pr_sched_avg_info(&cfs_rq_avg);
    pr_fn_end_on(stack_depth);
}
