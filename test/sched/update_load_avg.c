/*
 * 	kernel/sched/fair.c
 *  update_load_avg()
 */

#include <stdio.h>
#include "kernel/sched/sched-pelt.h"
/*
typedef unsigned long long u64;
typedef signed long long s64;
typedef unsigned int u32;
*/
#define __maybe_unused

//include/linux/sched.h
# define SCHED_FIXEDPOINT_SHIFT         10
# define SCHED_FIXEDPOINT_SCALE         (1L << SCHED_FIXEDPOINT_SHIFT)

/* Increase resolution of cpu_capacity calculations */
# define SCHED_CAPACITY_SHIFT           SCHED_FIXEDPOINT_SHIFT
# define SCHED_CAPACITY_SCALE           (1L << SCHED_CAPACITY_SHIFT)

#define CONFIG_64BIT

#ifdef CONFIG_64BIT
# define NICE_0_LOAD_SHIFT      (SCHED_FIXEDPOINT_SHIFT + SCHED_FIXEDPOINT_SHIFT)
# define scale_load(w)          ((w) << SCHED_FIXEDPOINT_SHIFT)
# define scale_load_down(w)     ((w) >> SCHED_FIXEDPOINT_SHIFT)
#else
# define NICE_0_LOAD_SHIFT      (SCHED_FIXEDPOINT_SHIFT)
# define scale_load(w)          (w)
# define scale_load_down(w)     (w)
#endif

struct util_est {
    unsigned int			enqueued;
    unsigned int			ewma;
#define UTIL_EST_WEIGHT_SHIFT		2
};
//__attribute__((__aligned__(sizeof(u64))));

struct sched_avg {
        u64                             last_update_time;
        u64                             load_sum;
        u64                             runnable_load_sum;
        u32                             util_sum;
        u32                             period_contrib;
        unsigned long                   load_avg;
        unsigned long                   runnable_load_avg;
        unsigned long                   util_avg;
        struct util_est                 util_est;
};

struct sched_avg avg = {
    .load_sum = 20000,
    .runnable_load_sum = 400000,
    .util_sum = 22907904
};

int Seq=0;

void pr_info(int i)
{
    printf("i=%d\n", i);
    printf("avg.last_update_time=%llu\n", avg.last_update_time);
    printf("avg.load_sum=%llu\n", avg.load_sum);
    printf("avg.runnable_load_sum=%llu\n", avg.runnable_load_sum);
    printf("avg.util_sum=%u\n", avg.util_sum);
    printf("avg.period_contrib=%u\n", avg.period_contrib);
    printf("avg.load_avg=%lu\n", avg.load_avg);
    printf("avg.runnable_load_avg=%lu\n", avg.runnable_load_avg);
    printf("avg.util_avg=%lu\n", avg.util_avg);
    //printf("avg.util_est=%llu\n", avg.util_est);
    printf("\n");
}

static inline u64 mul_u64_u32_shr(u64 a, u32 mul, unsigned int shift)
{
#ifdef __int128
        return (u64)(((unsigned __int128)a * mul) >> shift);
#else
        return (u64)((a * mul) >> shift);
#endif
}

static u64 decay_load(u64 val, u64 n)
{
        unsigned int local_n;

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
        return val;
}

static u32 __accumulate_pelt_segments(u64 periods, u32 d1, u32 d3)
{
        u32 c1, c2, c3 = d3; /* y^0 == 1 */

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

        printf("d1=%u, d3=%u, c1=%u, c2=%u, c3=%u\n", d1, d3, c1, c2, c3);

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
    u32 contrib = (u32)delta; /* p == 0 -> delta < 1024 */
    u64 periods;

    delta += sa->period_contrib;
    periods = delta / 1024; /* A period is 1024us (~1ms) */

    printf("accumulate_sum():\n");
    printf("sa->period_contrib=%u\n", sa->period_contrib);
    printf("delta+=%llu\n", delta);
    printf("periods=%llu\n", periods);

     /*
     * Step 1: decay old *_sum if we crossed period boundaries.
     */
    if (periods) {
        sa->load_sum = decay_load(sa->load_sum, periods);
        sa->runnable_load_sum =
            decay_load(sa->runnable_load_sum, periods);
        sa->util_sum = decay_load((u64)(sa->util_sum), periods);

        printf("accumulate_sum(): Step1:\n");
        pr_info(Seq);
        /*
         * Step 2
         */
        delta %= 1024;
        contrib = __accumulate_pelt_segments(periods,
                1024 - sa->period_contrib, delta);

        printf("contrib=%u\n", contrib);
    }
    sa->period_contrib = delta;

    if (load)
        sa->load_sum += load * contrib;
    if (runnable)
        sa->runnable_load_sum += runnable * contrib;
    if (running)
        sa->util_sum += contrib << SCHED_CAPACITY_SHIFT;

    return periods;
}

static int
___update_load_sum(u64 now, struct sched_avg *sa,
                  unsigned long load, unsigned long runnable, int running)
{
    u64 delta;

    delta = now - sa->last_update_time;
    printf("now=%llu\n", now);
    printf("sa->last_update_time=%llu\n", sa->last_update_time);
    printf("delta=%llu\n", delta);

    if ((s64)delta < 0) {
        sa->last_update_time = now;
        return 0;
    }

    delta >>= 10;	//us
    if (!delta)
        return 0;

    printf("delta=%llu\n", delta);

    sa->last_update_time += delta << 10;
    printf("sa->last_update_time=%llu\n", sa->last_update_time);

    if (!load)
        runnable = running = 0;

    if (!accumulate_sum(delta, sa, load, runnable, running))
        return 0;

    return 1;
}

static void
___update_load_avg(struct sched_avg *sa, unsigned long load, unsigned long runnable)
{
        u32 divider = LOAD_AVG_MAX - 1024 + sa->period_contrib;

        /*
         * Step 2: update *_avg.
         */
        //sa->load_avg = div_u64(load * sa->load_sum, divider);
        sa->load_avg = (load * sa->load_sum) / divider;
        //sa->runnable_load_avg = div_u64(runnable * sa->runnable_load_sum, divider);
        sa->runnable_load_avg = (runnable * sa->runnable_load_sum) / divider;
        //WRITE_ONCE(sa->util_avg, sa->util_sum / divider);
        sa->util_avg = sa->util_sum / divider;
}

/**
int __update_load_avg_cfs_rq(u64 now, struct cfs_rq *cfs_rq)
{
        if (___update_load_sum(now, &cfs_rq->avg,
                                scale_load_down(cfs_rq->load.weight),
                                scale_load_down(cfs_rq->runnable_weight),
                                cfs_rq->curr != NULL)) {

                ___update_load_avg(&cfs_rq->avg, 1, 1);
                //trace_pelt_cfs_tp(cfs_rq);
                return 1;
        }

        return 0;
}
*/

/*
int __update_load_avg_se(u64 now, struct cfs_rq *cfs_rq, struct sched_entity *se)
{
        if (___update_load_sum(now, &se->avg, !!se->on_rq, !!se->on_rq,
                                cfs_rq->curr == se)) {

                ___update_load_avg(&se->avg, se_weight(se), se_runnable(se));
                cfs_se_util_change(&se->avg);
                //trace_pelt_se_tp(se);
                return 1;
        }

        return 0;
}
*/


//update_load_avg()
int update_load_avg_test(void)
{
    int runnable[] = {1, 1, 1, 0, 0, 0, 1, 1, 1, 1};
    int running[] = {1, 1, 0, 0, 0, 0, 0, 0, 1, 1};
    u64 now = 0;	//ns
    //unsigned long	weight;
    //unsigned long	runnable_weight;

    int i;
    pr_info(0);
    for (i=0; i<10; i++) {
        Seq = i;
        //now += (i+1) * 10000000;	//+10ms
        now += 10000000;	//+10ms
        if (___update_load_sum(now, &avg, runnable[i], runnable[i], running[i])) {
            pr_info(i);
            ___update_load_avg(&avg, 1024, 1024);
            pr_info(i);
        }
    }

    return 0;
}
