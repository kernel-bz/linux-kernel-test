// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/cpu/cpus-mask-test.c
 *  CPU mask Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/debug.h"

#include <asm-generic/bitsperlong.h>
#include <asm-generic/bitops/atomic.h>
#include <linux/threads.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/bitmap.h>
#include <linux/cpumask.h>


static void _cpus_mask_info(void)
{
    pr_fn_start(stack_depth);

    pr_view(stack_depth, "%20s : %3d\n", __CHAR_BIT__);
    pr_view(stack_depth, "%20s : %3d\n", __SIZEOF_LONG__);
    pr_view(stack_depth, "%20s : %3d\n", BITS_PER_LONG);
    pr_view(stack_depth, "%20s : %3d\n", BITS_PER_LONG_LONG);
    pr_view(stack_depth, "%20s : %3d\n", nr_cpu_ids);
    pr_view(stack_depth, "%20s : %3d\n", nr_cpumask_bits);

    //reset_cpu_possible_mask();
    pr_view(stack_depth, "%30s : 0x%X\n", __cpu_possible_mask.bits[0]);
    pr_view(stack_depth, "%30s : 0x%X\n", __cpu_present_mask.bits[0]);
    pr_view(stack_depth, "%30s : 0x%X\n", __cpu_online_mask.bits[0]);
    pr_view(stack_depth, "%30s : 0x%X\n", __cpu_active_mask.bits[0]);

    pr_fn_end(stack_depth);
}

void cpus_mask_test(void)
{
    pr_fn_start(stack_depth);

    nr_cpu_ids = NR_CPUS;
    _cpus_mask_info();

    unsigned int i, nr = BITS_TO_LONGS(nr_cpumask_bits);
    cpumask_var_t cpus_mask;
    cpus_mask = calloc(1, cpumask_size());

    pr_view(stack_depth, "%30s : %u\n", NR_CPUS);
    pr_view(stack_depth, "%30s : %u\n", nr_cpu_ids);
    pr_view(stack_depth, "%30s : %u\n", BITS_TO_LONGS(nr_cpumask_bits));
    pr_view(stack_depth, "%30s : %u\n", nr);

    for (i=0; i<nr; i++)
        pr_view(stack_depth, "%30s : 0x%X\n\n", cpus_mask->bits[i]);

    unsigned int cpu;
    //for (cpu=0; cpu<nr_cpumask_bits; cpu++) {
    for_each_possible_cpu(cpu) {
        cpumask_set_cpu(cpu, cpus_mask);
        pr_view(stack_depth, "%30s : %u\n", cpu);
        pr_view(stack_depth, "%30s : 0x%X\n", cpus_mask->bits[0]);
        cpumask_clear_cpu(cpu, cpus_mask);
    }

    pr_fn_end(stack_depth);
}
