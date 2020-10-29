/*
 * 	kernel/sched/fair.c
 *  update_load_avg()
 */

#include <stdio.h>
#include <stdio_ext.h>

#include "test/config.h"
#include "test/debug.h"

#include <linux/types-user.h>
#include <linux/sched.h>
#include <linux/math64.h>
#include <linux/limits.h>
#include "kernel/sched/sched-pelt.h"

static struct sched_avg sa = {
    .load_sum = 840000,
    .runnable_load_sum = 8500000,
    .util_sum = 86000000
};

static void _pr_sched_avg_info(int i)
{
    pr_info_view_on(stack_depth, "%30s : %d\n", i);
    pr_info_view_on(stack_depth, "%30s : %llu\n", sa.last_update_time);
    pr_info_view_on(stack_depth, "%30s : %llu\n", sa.load_sum);
    pr_info_view_on(stack_depth, "%30s : %llu\n", sa.runnable_load_sum);
    pr_info_view_on(stack_depth, "%30s : %u\n", sa.util_sum);
    pr_info_view_on(stack_depth, "%30s : %u\n", sa.period_contrib);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa.load_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa.runnable_load_avg);
    pr_info_view_on(stack_depth, "%30s : %lu\n", sa.util_avg);
    //pr_info_view_on(stack_depth, "%30s : %llu\n", sa.util_est);
    pr_out_on(stack_depth, "\n");
}

static u64 decay_load(u64 val, u64 n)
{
        unsigned int local_n;

        pr_fn_start_on(stack_depth);

        pr_info_view_on(stack_depth, "%10s : %llu\n", val);
        pr_info_view_on(stack_depth, "%10s : %llu\n", n);

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

        pr_info_view_on(stack_depth, "%10s : %u\n", local_n);
        pr_info_view_on(stack_depth, "%10s : %llu\n", val);

        pr_fn_end_on(stack_depth);
        return val;
}

static u32 __accumulate_pelt_segments(u64 periods, u32 d1, u32 d3)
{
        u32 c1, c2, c3 = d3; /* y^0 == 1 */

        pr_fn_start_on(stack_depth);
        pr_info_view_on(stack_depth, "%10s : %u\n", d1);
        pr_info_view_on(stack_depth, "%10s : %llu\n", periods);
        pr_info_view_on(stack_depth, "%10s : %u\n", d3);

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

        pr_info_view_on(stack_depth, "%10s : %u\n", c1);
        pr_info_view_on(stack_depth, "%10s : %u\n", c2);
        pr_info_view_on(stack_depth, "%10s : %u\n", c3);
        pr_info_view_on(stack_depth, "%10s : %u\n", c1+c2+c3);

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

    delta += sa->period_contrib;
    periods = delta / 1024; /* A period is 1024us (~1ms) */

    pr_info_view_on(stack_depth, "%20s : %u\n", sa->period_contrib);
    pr_info_view_on(stack_depth, "%20s : %llu\n", delta);
    pr_info_view_on(stack_depth, "%20s(ms) : %llu\n", periods);

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

        pr_info_view_on(stack_depth, "%20s : %u\n", contrib);
    }
    sa->period_contrib = delta;

    if (load)
        sa->load_sum += load * contrib;
    if (runnable)
        sa->runnable_load_sum += runnable * contrib;
    if (running)
        sa->util_sum += contrib << SCHED_CAPACITY_SHIFT;

    pr_fn_end_on(stack_depth);

    return periods;
}

static int
___update_load_sum(u64 now, struct sched_avg *sa,
                  unsigned long load, unsigned long runnable, int running)
{
    u64 delta;
    int ret=1;

    pr_fn_start_on(stack_depth);
    pr_info_view_on(stack_depth, "%20s : %lu\n", load);
    pr_info_view_on(stack_depth, "%20s : %lu\n", runnable);
    pr_info_view_on(stack_depth, "%20s : %d\n", running);

    delta = now - sa->last_update_time;
    pr_info_view_on(stack_depth, "%20s : %llu\n", now);
    pr_info_view_on(stack_depth, "%20s : %llu\n", sa->last_update_time);
    pr_info_view_on(stack_depth, "%20s : %llu\n", delta);

    if ((s64)delta < 0) {
        sa->last_update_time = now;
        return 0;
    }

    delta >>= 10;	//us
    if (!delta)
        return 0;

    pr_info_view_on(stack_depth, "%20s(us) : %llu\n", delta);

    sa->last_update_time += delta << 10;
    pr_info_view_on(stack_depth, "%20s : %llu\n", sa->last_update_time);

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
        pr_info_view_on(stack_depth, "%20s : %u\n", sa->period_contrib);
        pr_info_view_on(stack_depth, "%20s : %u\n", divider);

        /*
         * Step 2: update *_avg.
         */
        //sa->load_avg = div_u64(load * sa->load_sum, divider);
        sa->load_avg = (load * sa->load_sum) / divider;
        //sa->runnable_load_avg = div_u64(runnable * sa->runnable_load_sum, divider);
        sa->runnable_load_avg = (runnable * sa->runnable_load_sum) / divider;
        //WRITE_ONCE(sa->util_avg, sa->util_sum / divider);
        sa->util_avg = sa->util_sum / divider;

        pr_fn_end_on(stack_depth);
}

#if 0
kernel/sched/fair.c
update_load_avg()
  kernel/sched/pelt.c
  __update_load_avg_se(now, *cfs_rq, *se)
    ___update_load_sum(now, *sa, load, runnable, running)
        ___update_load_avg(*sa, load, runnable)
#endif //0

void update_load_avg_test(void)
{
    unsigned long load, runnable, load_avg, runnable_avg;
    int running;
    u64 now = 0, hz, ns;
    int i, loop_cnt;

_retry:
    __fpurge(stdin);
    printf("Input Load Weight Value[0,%d]: ", LOAD_AVG_MAX);
    scanf("%lu", &load);
    printf("Input Runnable Weight Value[0,%d]: ", LOAD_AVG_MAX);
    scanf("%lu", &runnable);
    printf("Input Running Value[0,%d]: ", LOAD_AVG_MAX);
    scanf("%d", &running);

    printf("Input Load_Avg Weight Value[0,%d]: ", LOAD_AVG_MAX);
    scanf("%lu", &load_avg);
    printf("Input Runnable_Avg Weight Value[0,%d]: ", LOAD_AVG_MAX);
    scanf("%lu", &runnable_avg);

    printf("Input HZ Value[%d,%d]: ", HZ, HZ*100);
    scanf("%llu", &hz);
    printf("Input Loop Counter[0,%d]: ", S32_MAX);
    scanf("%d", &loop_cnt);

    pr_fn_start_on(stack_depth);

    ns = 1000000000UL / hz;	//ns

    _pr_sched_avg_info(0);
    for (i=0; i<loop_cnt; i++) {
        now += ns;
        unsigned long load, runnable;
        int running;
        if (___update_load_sum(now, &sa, load, runnable, running)) {
            _pr_sched_avg_info(i);
            ___update_load_avg(&sa, load_avg, runnable_avg);
            _pr_sched_avg_info(i);
        }
        printf("++%d---------------------------------------------------\n", i+1);
    }

    pr_fn_end_on(stack_depth);
}
