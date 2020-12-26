// SPDX-License-Identifier: GPL-2.0-only
/*
 *  test/init/init.c
 *  init routine
 *
 *  Copyright(C) Jung-JaeJoon <rgbi3307@naver.com> on the www.kernel.bz
 */

#include <stdio.h>
#include <stdlib.h>

#include "test/config.h"
#include "test/debug.h"
#include "test/test.h"
#include <linux/sched/init.h>

//init/main.c:
//  start_kernel()
//    rest_init()
//      kernel_thread(kernel_init, NULL, CLONE_FS)
//        kernel_init()
//          kernel_init_freeable()
static void _init_start_kernel(void)
{
    pr_fn_start_on(stack_depth);

    //setup_nr_cpu_ids();	//kernel/smp.c
    nr_cpu_ids = find_last_bit(cpumask_bits(cpu_possible_mask),NR_CPUS) + 1;

    //arch/arm64/mm/numa.c
    numa_usr_set_node(NR_CPUS / nr_node_ids);
#ifdef CONFIG_ARM64
    arm64_numa_init();
#endif

    boot_cpu_init();		//kernel/cpu.c
    cpumask_setall(cpu_active_mask);
    cpumask_setall(cpu_online_mask);
    //cpumask_set_cpu(0, cpu_online_mask);
    //cpumask_clear_cpu(0, cpu_online_mask);
    cpumask_setall(cpu_present_mask);

    sched_init();
    sched_clock_init();

    pr_sched_cpumask_bits_info(nr_cpu_ids);
    numa_distance_info();

    pr_fn_end_on(stack_depth);
}

static void _init_menu_help(void)
{
    //help messages...
    printf("\n");
    printf("Tis is the Linux kernel source testing program\n");
    printf("that can be run at the user level.\n");
    printf("written by www.kernel.bz\n");
    printf("Kernel Version: v%d.%d-td%d\n",
           CONFIG_VERSION_1, CONFIG_VERSION_2, CONFIG_VERSION_3);
    printf("\n");
    return;
}

static int _init_menu(int asize)
{
    int idx;
    __fpurge(stdin);

    printf("\n");
    printf("[#]--> Start Kernel Test Menu\n");
    printf("0: help.\n");
    printf("1: sched_init test.\n");
    printf("2: sched_init_smp test.\n");
    printf("3: exit.\n");
    printf("\n");

    printf("Enter Menu Number[0,%d]: ", asize);
    scanf("%d", &idx);
    return (idx >= 0 && idx < asize) ? idx : -1;
}

void init_start_kernel(void)
{
    void (*fn[])(void) = { _init_menu_help
        , _init_start_kernel
        , sched_init_smp	//kernel/sched/core.c
    };
    int idx;
    int asize = sizeof (fn) / sizeof (fn[0]);

_retry:
    idx = _init_menu(asize);
    if (idx >= 0) {
        fn[idx]();
        goto _retry;
    }
}
