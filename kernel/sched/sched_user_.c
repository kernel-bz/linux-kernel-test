// SPDX-License-Identifier: GPL-2.0
/*
 *  test/sched/sched_user_.c
 *  Linux Kernel Schedular Temp Source
 *
 *  Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include "sched.h"

//kernel/sched/cpufreq_schedutil.c: 636 lines
struct cpufreq_governor schedutil_gov;

/*
 * This function computes an effective utilization for the given CPU, to be
 * used for frequency selection given the linear relation: f = u * f_max.
 *
 * The scheduler tracks the following metrics:
 *
 *   cpu_util_{cfs,rt,dl,irq}()
 *   cpu_bw_dl()
 *
 * Where the cfs,rt and dl util numbers are tracked with the same metric and
 * synchronized windows and are thus directly comparable.
 *
 * The cfs,rt,dl utilization are the running times measured with rq->clock_task
 * which excludes things like IRQ and steal-time. These latter are then accrued
 * in the irq utilization.
 *
 * The DL bandwidth number otoh is not a measured metric but a value computed
 * based on the task model parameters and gives the minimal utilization
 * required to meet deadlines.
 */
//kernel/sched/cpufreq_schedutil.c: 208 lines
unsigned long schedutil_cpu_util(int cpu, unsigned long util_cfs,
                 unsigned long max, enum schedutil_type type,
                 struct task_struct *p)
{
    unsigned long dl_util, util, irq;
    struct rq *rq = cpu_rq(cpu);

    if (!IS_BUILTIN(CONFIG_UCLAMP_TASK) &&
        type == FREQUENCY_UTIL && rt_rq_is_runnable(&rq->rt)) {
        return max;
    }

    /*
     * Early check to see if IRQ/steal time saturates the CPU, can be
     * because of inaccuracies in how we track these -- see
     * update_irq_load_avg().
     */
    irq = cpu_util_irq(rq);
    if (unlikely(irq >= max))
        return max;

    /*
     * Because the time spend on RT/DL tasks is visible as 'lost' time to
     * CFS tasks and we use the same metric to track the effective
     * utilization (PELT windows are synchronized) we can directly add them
     * to obtain the CPU's actual utilization.
     *
     * CFS and RT utilization can be boosted or capped, depending on
     * utilization clamp constraints requested by currently RUNNABLE
     * tasks.
     * When there are no CFS RUNNABLE tasks, clamps are released and
     * frequency will be gracefully reduced with the utilization decay.
     */
    util = util_cfs + cpu_util_rt(rq);
    if (type == FREQUENCY_UTIL)
        util = uclamp_util_with(rq, util, p);

    dl_util = cpu_util_dl(rq);

    /*
     * For frequency selection we do not make cpu_util_dl() a permanent part
     * of this sum because we want to use cpu_bw_dl() later on, but we need
     * to check if the CFS+RT+DL sum is saturated (ie. no idle time) such
     * that we select f_max when there is no idle time.
     *
     * NOTE: numerical errors or stop class might cause us to not quite hit
     * saturation when we should -- something for later.
     */
    if (util + dl_util >= max)
        return max;

    /*
     * OTOH, for energy computation we need the estimated running time, so
     * include util_dl and ignore dl_bw.
     */
    if (type == ENERGY_UTIL)
        util += dl_util;

    /*
     * There is still idle time; further improve the number by using the
     * irq metric. Because IRQ/steal time is hidden from the task clock we
     * need to scale the task numbers:
     *
     *              max - irq
     *   U' = irq + --------- * U
     *                 max
     */
    util = scale_irq_capacity(util, irq, max);
    util += irq;

    /*
     * Bandwidth required by DEADLINE must always be granted while, for
     * FAIR and RT, we use blocked utilization of IDLE CPUs as a mechanism
     * to gracefully reduce the frequency when no tasks show up for longer
     * periods of time.
     *
     * Ideally we would like to set bw_dl as min/guaranteed freq and util +
     * bw_dl as requested freq. However, cpufreq is not yet ready for such
     * an interface. So, we only do the latter for now.
     */
    if (type == FREQUENCY_UTIL)
        util += cpu_bw_dl(rq);

    return min(max, util);
}
