// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/basic/basic-test.c
 *  Basic Test Module
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */
#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/debug.h"

#if (CONFIG_RUN_ARCH == ARCH_X86 || CONFIG_RUN_ARCH == ARCH_X86_64)
//arch/x86/include/
#include <asm/tsc.h>

static void _test_busy_loop(u64 iters)
{
    volatile int sink;
    do {
        sink = 0;
    } while (--iters > 0);
    (void)sink;
}

static void _run_time_test(u64 loop, u64 cnt)
{
    pr_fn_start(stack_depth);

    u64 time1, time2, time3, cycles=0;
    u64 i;

    for (i=0; i<cnt; i++) {
        time1 = get_cycles();
        _test_busy_loop(loop);
        time2 = get_cycles();
        time3 = time2-time1;
        cycles += time3;
        pr_out(stack_depth, "rdtsc %d: run_cycles=%llu, one call:%llu cycles\n",
            i+1, time3, div_u64(time3, loop));
    }

    time1 = div_u64(cycles, loop * cnt);
    pr_out(stack_depth, "*Total Average: one call=%llu cycles\n", time1);

    pr_fn_end(stack_depth);
}

void basic_run_time_test(void)
{
    int loop, cnt;

    __fpurge(stdin);
    printf("Enter Loop Iteration Number: ");
    scanf("%d", &loop);
    printf("Enter Run Counter: ");
    scanf("%d", &cnt);

    _run_time_test(loop, cnt);
}

#else

void basic_run_time_test(void)
{
   pr_warn("This function can run on ARCH_X86\n");
}

#endif
