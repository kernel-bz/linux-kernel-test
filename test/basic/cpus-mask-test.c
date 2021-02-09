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
    pr_out_view(stack_depth, "%20s : %3d\n", __CHAR_BIT__);
    pr_out_view(stack_depth, "%20s : %3d\n", __SIZEOF_LONG__);
    pr_out_view(stack_depth, "%20s : %3d\n", BITS_PER_LONG);
    pr_out_view(stack_depth, "%20s : %3d\n", BITS_PER_LONG_LONG);
    pr_out_view(stack_depth, "%20s : %3d\n", nr_cpu_ids);
    pr_out_view(stack_depth, "%20s : %3d\n", nr_cpumask_bits);

    //reset_cpu_possible_mask();
    pr_out_view(stack_depth, "%30s : 0x%X\n", __cpu_possible_mask.bits[0]);
    pr_out_view(stack_depth, "%30s : 0x%X\n", __cpu_present_mask.bits[0]);
    pr_out_view(stack_depth, "%30s : 0x%X\n", __cpu_online_mask.bits[0]);
    pr_out_view(stack_depth, "%30s : 0x%X\n", __cpu_active_mask.bits[0]);
    printf("\n");
}

void cpus_mask_test(void)
{
    unsigned int bits;

    printf("[#] cpus mask testing...\n");

    pr_fn_start(stack_depth);

    _cpus_mask_info();

    for (bits=0; bits<=128; bits+=8)
        printf("BITS_TO_LONGS bits=%u, nr=%u\n", bits, BITS_TO_LONGS(bits));

    unsigned int i, nr = BITS_TO_LONGS(nr_cpumask_bits);
    cpumask_var_t cpus_mask;
    cpus_mask = calloc(1, cpumask_size());

    for (i=0; i<nr; i++)
        printf("cpus_mask->bits = %lu\n", cpus_mask->bits[i]);

    unsigned int cpu;
    //for (cpu=0; cpu<nr_cpumask_bits; cpu++) {
    for_each_possible_cpu(cpu) {
        cpumask_set_cpu(cpu, cpus_mask);
        for (i=0; i<nr; i++)
            printf("cpu=%d, cpus_mask->bits=%lu\n", cpu, cpus_mask->bits[i]);
        cpumask_clear_cpu(cpu, cpus_mask);
    }
    printf("<--End of %s().\n", __func__);

    pr_fn_end(stack_depth);
}
